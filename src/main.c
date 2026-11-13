#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>

#include "force_service.h"
#include "force_sensor.h"

#define DEVICE_NAME      CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN  (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED   DK_LED1
#define CON_STATUS_LED   DK_LED2
#define TARE_BUTTON      DK_BTN1_MSK
#define SAMPLE_PERIOD_MS 100

static struct k_work adv_work;
static volatile bool streaming;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, FORCE_SVC_UUID_VAL),
};

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}
	printk("Connected\n");
	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
	dk_set_led_off(CON_STATUS_LED);
	streaming = false;
}

static void recycled_cb(void)
{
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
	.recycled     = recycled_cb,
};

static void on_command(uint8_t cmd)
{
	switch (cmd) {
	case FORCE_CMD_TARE:  force_sensor_tare(); break;
	case FORCE_CMD_START: streaming = true;    break;
	case FORCE_CMD_STOP:  streaming = false;   break;
	default: printk("Unknown command 0x%02x\n", cmd);
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & TARE_BUTTON) && (button_state & TARE_BUTTON)) {
		force_sensor_tare();
	}
}

static void sample_thread_fn(void *a, void *b, void *c)
{
	for (;;) {
		k_sleep(K_MSEC(SAMPLE_PERIOD_MS));
		if (!streaming || !force_service_notify_enabled()) {
			continue;
		}
		struct force_sample s = {
			.t_ms    = k_uptime_get_32(),
			.force_g = force_sensor_read_g(),
		};
		int err = force_service_notify(&s);
		if (err && err != -ENOTCONN) {
			printk("Notify failed (err %d)\n", err);
		}
	}
}
K_THREAD_DEFINE(sample_tid, 1024, sample_thread_fn, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	int blink = 0;
	int err;

	printk("Starting PincherZ\n");

	err = dk_leds_init();
	if (err) { printk("LEDs init failed (err %d)\n", err); return 0; }

	err = dk_buttons_init(button_changed);
	if (err) { printk("Buttons init failed (err %d)\n", err); return 0; }

	err = bt_enable(NULL);
	if (err) { printk("Bluetooth init failed (err %d)\n", err); return 0; }

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	force_service_init(on_command);

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink) % 2);
		k_sleep(K_MSEC(1000));
	}
}