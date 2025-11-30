#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdbool.h>
#include <stdint.h>

extern uint32_t alarm_time;

//"live" variables
extern bool alarm;
extern bool enabled;
extern bool ptt;
extern bool last_ptt;
extern bool force_ptt;
extern bool getfreq_flag;
extern uint8_t pwm_pump;
extern uint8_t pwm_cooler;
extern float water_temp;
extern float plate_temp;
extern bool auto_pwm_pump;
extern bool auto_pwm_fan;
extern char current_band[5];
extern float voltage;
extern float current;
extern bool protection_enabled;

extern float forward_power;
extern float reverse_power;
extern float input_power;
extern float swr;
extern char alert_reason[20];

extern volatile uint32_t overflow_count;
extern uint16_t adc_dma_buffer[6];

//saved variables
extern uint8_t max_swr;
extern uint8_t max_current;
extern uint8_t max_voltage;
extern uint8_t max_water_temp;
extern uint8_t max_plate_temp;
extern uint8_t max_pump_speed_temp;
extern uint8_t min_pump_speed_temp;
extern uint8_t max_fan_speed_temp;
extern uint8_t min_fan_speed_temp;
extern uint8_t max_input_power;
extern bool autoband;
extern char default_band[5];

//calibration
extern float low_fwd_coeff;
extern float low_rev_coeff;
extern float low_ifwd_coeff;
extern float mid_fwd_coeff;
extern float mid_rev_coeff;
extern float mid_ifwd_coeff;
extern float high_fwd_coeff;
extern float high_rev_coeff;
extern float high_ifwd_coeff;

extern float voltage_coeff;
extern float current_coeff;
extern float rsrv_coeff;


#endif
