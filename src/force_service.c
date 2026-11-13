#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>
#include "force_service.h"

// UUID objects for different commands
static const struct bt_uuid_128 svc_uuid =
	BT_UUID_INIT_128(FORCE_SVC_UUID_VAL);
static const struct bt_uuid_128 force_uuid =
	BT_UUID_INIT_128(FORCE_UUID_VAL);
static const struct bt_uuid_128 cmd_uuid =
	BT_UUID_INIT_128(CMD_UUID_VAL);

static bool notify_on;	// Has the user subscribed to notifications
static force_cmd_cb_t cmd_cb; // Callback created by force_service_init

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_on = (value == BT_GATT_CCC_NOTIFY); //Generic Attribute Profile
	printk("Force notifications %s\n", notify_on ? "enabled" : "disabled");
}

// conn = which connection wrote
// attr = which attribute was written
static ssize_t write_cmd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	// The length of the message has to be exactlyy 1 byte
	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (cmd_cb) { // Ensures that cmd_cb has a value or else the pointer would call NULL and crash
		cmd_cb(((const uint8_t *)buf)[0]);
	}
	return len;
}

// CCC = Client Characteristic Configuration
// BT_GATT_PRIMARY_SERVICE = Service with this UUID begins here
// BT_GATT_CHARACTERISTIC = Force Data
// 		Arguments: UUID, notify properties, permission
// Second characteristic is the command chanell
BT_GATT_SERVICE_DEFINE(force_svc,
	BT_GATT_PRIMARY_SERVICE(&svc_uuid),
	BT_GATT_CHARACTERISTIC(&force_uuid.uuid, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(&cmd_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_cmd, NULL),
);

// Stores callback pointer
void force_service_init(force_cmd_cb_t cb)
{
	cmd_cb = cb;
}

// Returns notify_on flag
bool force_service_notify_enabled(void)
{
	return notify_on;
}

// Sends sample packet
int force_service_notify(const struct force_sample *s)
{
	return bt_gatt_notify_uuid(NULL, &force_uuid.uuid, force_svc.attrs,
				   s, sizeof(*s));
}