// Only define header once
#ifndef FORCE_SERVICE_H_
#define FORCE_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/uuid.h>

// Generate a UUID for the device
#define FORCE_SVC_UUID_VAL \
	BT_UUID_128_ENCODE(0x7c3a0001, 0x5b1e, 0x4d2a, 0x9f60, 0x3e8a1c0d4b52)

#define FORCE_UUID_VAL \
    BT_UUID_128_ENCODE(0x7c3a0002, 0x5b1e, 0x4d2a, 0x9f60, 0x3e8a1c0d4b52)

#define CMD_UUID_VAL \
    BT_UUID_128_ENCODE(0x7c3a0003, 0x5b1e, 0x4d2a, 0x9f60, 0x3e8a1c0d4b52)

// 1-byte commands
enum force_cmd {
    FORCE_CMD_TARE = 0x01, // Temporary Zero
    FORCE_CMD_START = 0x02,
    FORCE_CMD_STOP = 0x03,
};

struct force_sample
{
    uint32_t t_ms; // Timestamp for Packet
    int32_t force_g; // Reading in grams
} __packed;

typedef void (*force_cmd_cb_t)(uint8_t cmd); // Create a callback alias

void force_service_init(force_cmd_cb_t cb);
bool force_service_notify_enabled(void); // Enable notification sending
int force_service_notify(const struct force_sample *s); // Send one sample as a notification


#endif