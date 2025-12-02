#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"

#include <stdio.h>

#define ACS_SENSITIVITY 0.040f        // 40mV/A ACS758-050B
#define ACS_VCC 3.3f
#define ACS_ZERO_CURRENT_VOLTAGE (ACS_VCC / 2.0f)

float voltage_to_current(float voltage) {
    float current = (voltage - ACS_ZERO_CURRENT_VOLTAGE) / ACS_SENSITIVITY;
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

/*
float voltage0; //fwd
float voltage1; //ref
float voltage2; //ifwd
float voltage3; //voltage
float voltage4; //currrent
float voltage5; //reserved
*/

static bool delay_active = false;
static uint32_t delay_start = 0;

void process_data(void) {

	if (strcmp(current_band, "160m") == 0 || strcmp(current_band, "80m") == 0 || strcmp(current_band, "60m") == 0 || strcmp(current_band, "40m") == 0) {
		forward_power = adc_data.voltage0 * adc_data.voltage0 * low_fwd_coeff;
		reverse_power = adc_data.voltage1 * adc_data.voltage1 * low_rev_coeff;
	    input_power = adc_data.voltage2 * adc_data.voltage2 * low_ifwd_coeff;
	} else
	if (strcmp(current_band, "30m") == 0 || strcmp(current_band, "20m") == 0 || strcmp(current_band, "17m") == 0 || strcmp(current_band, "15m") == 0) {
		forward_power = adc_data.voltage0 * adc_data.voltage0 * mid_fwd_coeff;
		reverse_power = adc_data.voltage1 * adc_data.voltage1 * mid_rev_coeff;
	    input_power = adc_data.voltage2 * adc_data.voltage2 * mid_ifwd_coeff;
	} else
	if (strcmp(current_band, "12m") == 0 || strcmp(current_band, "10m") == 0 || strcmp(current_band, "6m") == 0) {
		forward_power = adc_data.voltage0 * adc_data.voltage0 * high_fwd_coeff;
		reverse_power = adc_data.voltage1 * adc_data.voltage1 * high_rev_coeff;
	    input_power = adc_data.voltage2 * adc_data.voltage2 * high_ifwd_coeff;
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

    voltage = adc_data.voltage3 * voltage_coeff;

    current = voltage_to_current(adc_data.voltage4 * current_coeff);

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
    	if (ptt == true && coeff < min_coeff && forward_power > 100) { // need to think about that and review again, on low power coeff will be low
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
                if (autoband) {
                	getfreq_flag = true;
                }

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
