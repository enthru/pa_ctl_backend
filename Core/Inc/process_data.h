#ifndef PROCESS_DATA_H
#define PROCESS_DATA_H

#include <stdbool.h>

void process_data(void);
float voltage_to_power(float voltage, float reference_power, float reference_voltage, float divider_ratio);
float voltage_to_current(float voltage);
void trigger_alarm(void);
void disable_ptt(void);

#endif
