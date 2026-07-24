#ifndef PROCESS_DATA_H
#define PROCESS_DATA_H

#include <stdbool.h>

void process_data(void);
float voltage_to_power(float voltage, float reference_power, float reference_voltage, float divider_ratio);
float voltage_to_current(float voltage);
void trigger_alarm(void);

// Call after current_band_id changes (band switch, autoband, alarm reset)
void process_data_on_band_change(void);

// Call after calibration changes (acs_sens, band coefficients); also calls on_band_change
void process_data_on_config_change(void);

// Compute SWR from current power values — call at telemetry rate, not in ISR
float process_data_get_swr(void);

#endif
