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
volatile uint8_t getfreq_flag;
bool force_ptt = false;
uint8_t pwm_pump = 0;
uint8_t pwm_cooler = 0;
float water_temp = 0.0f;
float plate_temp = 0.0f;
bool auto_pwm_pump = true;
bool auto_pwm_fan = true;
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
float low_fwd_coeff  = DEFAULT_LOW_FWD_COEFF;
float low_rev_coeff  = DEFAULT_LOW_REV_COEFF;
float low_ifwd_coeff = DEFAULT_LOW_IFWD_COEFF;
float mid_fwd_coeff  = DEFAULT_MID_FWD_COEFF;
float mid_rev_coeff  = DEFAULT_MID_REV_COEFF;
float mid_ifwd_coeff = DEFAULT_MID_IFWD_COEFF;
float high_fwd_coeff  = DEFAULT_HIGH_FWD_COEFF;
float high_rev_coeff  = DEFAULT_HIGH_REV_COEFF;
float high_ifwd_coeff = DEFAULT_HIGH_IFWD_COEFF;

float voltage_coeff = DEFAULT_VOLTAGE_COEFF;
float current_coeff = DEFAULT_CURRENT_COEFF;
float rsrv_coeff    = DEFAULT_RSRV_COEFF;
float acs_zero      = DEFAULT_ACS_ZERO;
float acs_sens      = DEFAULT_ACS_SENS;

volatile bool startup_complete = false;
