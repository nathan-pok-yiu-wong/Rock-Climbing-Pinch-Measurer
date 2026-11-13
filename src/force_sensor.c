#include <zephyr/kernel.h>
#include "force_sensor.h"

static int32_t offset_g;

// Generating a triangle reading
static int32_t raw_g(void)
{
	uint32_t phase = k_uptime_get_32() % 4000;
	int32_t tri = (phase < 2000) ? phase : (4000 - phase);
	return tri * 20;
}

int32_t force_sensor_read_g(void)
{
	return raw_g() - offset_g;
}

void force_sensor_tare(void)
{
	offset_g = raw_g();
}