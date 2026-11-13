#ifndef FORCE_SENSOR_H_
#define FORCE_SENSOR_H_
#include <stdint.h>

int32_t force_sensor_read_g(void);
void force_sensor_tare(void);

#endif