// variables.c
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "default_values.h"

uint32_t alarm_time = 0;

//"live" variables
bool alarm = false;
bool enabled = false;
bool ptt = false;
bool last_ptt = false;
bool getfreq_flag = false;
bool force_ptt = false;
uint8_t pwm_pump = 0;
uint8_t pwm_cooler = 0;
float water_temp = 0.0f;
float plate_temp = 0.0f;
bool auto_pwm_pump = false;
bool auto_pwm_fan = false;
char current_band[5] = {0};
float voltage = 0.0f;
float current = 0.0f;
bool protection_enabled = true;
float psu_power = 0.0f;
float coeff = 0.0f;

float forward_power = 0.0f;
float reverse_power = 0.0f;
float input_power = 0.0f;
float swr = 1.0f;
char alert_reason[20] = "";

uint16_t adc_dma_buffer[6] = {0};
volatile uint32_t overflow_count = 0;

uint8_t max_swr = DEFAULT_MAX_SWR;
uint8_t max_current = DEFAULT_MAX_CURRENT;
uint8_t max_voltage = DEFAULT_MAX_VOLTAGE;
uint8_t max_water_temp = DEFAULT_MAX_WATER_TEMP;
uint8_t max_plate_temp = DEFAULT_MAX_PLATE_TEMP;
uint8_t max_pump_speed_temp = DEFAULT_MAX_PUMP_SPEED_TEMP;
uint8_t min_pump_speed_temp = DEFAULT_MIN_PUMP_SPEED_TEMP;
uint8_t max_fan_speed_temp = DEFAULT_MAX_FAN_SPEED_TEMP;
uint8_t min_fan_speed_temp = DEFAULT_MIN_FAN_SPEED_TEMP;
uint8_t max_input_power = DEFAULT_MAX_INPUT_POWER;
uint8_t min_coeff = DEFAULT_MIN_COEFF;
bool autoband = DEFAULT_AUTOBAND;
char default_band[5] = DEFAULT_BAND_VALUE;

//calibration
float low_fwd_coeff = 173.0000;
float low_rev_coeff = 173.0000;
float low_ifwd_coeff = 14.2850;
float mid_fwd_coeff = 173.0000;
float mid_rev_coeff = 173.0000;
float mid_ifwd_coeff = 14.2850;
float high_fwd_coeff = 173.0000;
float high_rev_coeff = 173.0000;
float high_ifwd_coeff = 14.2850;

float voltage_coeff = 18.4000;
float current_coeff = 1.0000;
float rsrv_coeff = 1.0000;
