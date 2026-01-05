#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"

#include <stdio.h>

float voltage_to_current(float voltage) {
	float current = (acs_zero - voltage) / acs_zero;
    return current;
}

void disable_ptt(void) {
	last_ptt = false;
	ptt = false;
	force_ptt = false;
	HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_RESET);
}

void trigger_alarm(void) {
	disable_ptt();
	enabled = false;
	alarm = true;
	alarm_time = HAL_GetTick();
}

static bool delay_active = false;
static uint32_t delay_start = 0;

typedef struct {
	float y;
	uint8_t initialized;
} ema_filter_t;

static inline float ema_update(ema_filter_t *f, float x, float alpha) {
	if (!f->initialized) {
		f->y = x;
		f->initialized = 1;
		return x;
	}
	f->y = f->y + alpha * (x - f->y);
	return f->y;
}

#define ADC_FILT_ALPHA 0.20f

static ema_filter_t f_v0 = {0};
static ema_filter_t f_v1 = {0};
static ema_filter_t f_v2 = {0};
static ema_filter_t f_v3 = {0};
static ema_filter_t f_v4 = {0};

void process_data(void) {

	float v0 = ema_update(&f_v0, adc_data.voltage0, ADC_FILT_ALPHA);
	float v1 = ema_update(&f_v1, adc_data.voltage1, ADC_FILT_ALPHA);
	float v2 = ema_update(&f_v2, adc_data.voltage2, ADC_FILT_ALPHA);
	float v3 = ema_update(&f_v3, adc_data.voltage3, ADC_FILT_ALPHA);
	float v4 = ema_update(&f_v4, adc_data.voltage4, ADC_FILT_ALPHA);

	if (strcmp(current_band, "160m") == 0 || strcmp(current_band, "80m") == 0 || strcmp(current_band, "60m") == 0 || strcmp(current_band, "40m") == 0) {
		forward_power = v0 * v0 * low_fwd_coeff;
		reverse_power = v1 * v1 * low_rev_coeff;
	    input_power = v2 * v2 * low_ifwd_coeff;
	} else
	if (strcmp(current_band, "30m") == 0 || strcmp(current_band, "20m") == 0 || strcmp(current_band, "17m") == 0 || strcmp(current_band, "15m") == 0) {
		forward_power = v0 * v0 * mid_fwd_coeff;
		reverse_power = v1 * v1 * mid_rev_coeff;
	    input_power = v2 * v2 * mid_ifwd_coeff;
	} else
	if (strcmp(current_band, "12m") == 0 || strcmp(current_band, "10m") == 0 || strcmp(current_band, "6m") == 0) {
		forward_power = v0 * v0 * high_fwd_coeff;
		reverse_power = v1 * v1 * high_rev_coeff;
	    input_power = v2 * v2 * high_ifwd_coeff;
	}

    if (forward_power < 0.1f) forward_power = 0.0f;
    if (reverse_power < 0.1f) reverse_power = 0.0f;

    if (forward_power > 0.1f && reverse_power > 0.1f) {
        float reflection_coef = sqrtf(reverse_power / forward_power);
        if (reflection_coef >= 1.0f) {
            swr = 99.99f;
        } else {
            swr = (1.0f + reflection_coef) / (1.0f - reflection_coef);
        }
    } else {
        swr = 1.0f;
    }

    if (swr < 1.0f) swr = 1.0f;
    if (swr > 99.99f) swr = 99.99f;

    if (input_power < 0.1f) input_power = 0.0f;
    if (input_power > 100.0f) input_power = 100.0f;

    voltage = v3 * voltage_coeff;

    current = voltage_to_current(v4 * current_coeff);

    psu_power = voltage * current;
    coeff = (forward_power + reverse_power / psu_power) * 100.0;

    if (protection_enabled && !alarm) {
    	if (strcmp(current_band, "unk") == 0) {
    		trigger_alarm();
    		strcpy(alert_reason, "band");
    	}

    	if (swr > (float)max_swr) {
    		trigger_alarm();
    		strcpy(alert_reason, "swr");
    	}

    	if (voltage > (float)max_voltage) {
    		trigger_alarm();
    		strcpy(alert_reason, "voltage");
    	}

    	if (current > (float)max_current) {
    		trigger_alarm();
    		strcpy(alert_reason, "current");
    	}
    	if (water_temp > (float)max_water_temp) {
    		trigger_alarm();
    		strcpy(alert_reason, "water_temp");
    	}
    	if (plate_temp > (float)max_plate_temp) {
    		trigger_alarm();
    		strcpy(alert_reason, "plate_temp");
    	}
    	if (input_power > max_input_power) {
    		trigger_alarm();
    		strcpy(alert_reason, "ipower");
    	}
    	if (ptt == true && coeff < min_coeff && forward_power > 100) {
    		trigger_alarm();
    		strcpy(alert_reason, "coeff");
    	}
   }

   if (alarm) {
		disable_ptt();
		enabled = false;
   }

    if (HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin) || force_ptt) {
        if (enabled && !alarm) {
            ptt = true;
            if (!last_ptt) {
                //getfreq_flag = true;
                delay_active = true;
                delay_start = HAL_GetTick();
                last_ptt = true;
            }

            if (delay_active && !getfreq_flag) {
                if (HAL_GetTick() - delay_start >= 30) {
                    delay_active = false;
                    HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_SET);
                }
            }

        } else {
            disable_ptt();
        }
    } else {
        disable_ptt();
    }

    if (enabled) {
    	HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_SET);
    } else HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_RESET);
}
