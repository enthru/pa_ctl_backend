#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"

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



/*
float voltage0; //fwd
float voltage1; //ref
float voltage2; //ifwd
float voltage3; //voltage
float voltage4; //currrent
float voltage5; //reserved
*/

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

    if (protection_enabled) {
    	if (swr > (float)max_swr) {
    		disable_ptt();
    		alarm = true;
    		enabled = false;
    		strcpy(alert_reason, "swr");
    	}

    	if (voltage > (float)max_voltage) {
    		disable_ptt();
    		alarm = true;
    		enabled = false;
    		strcpy(alert_reason, "voltage");
    	}

    	if (current > (float)max_current) {
    		disable_ptt();
    		alarm = true;
    		enabled = false;
    		strcpy(alert_reason, "current");
    	}
    	if (water_temp > (float)max_water_temp || plate_temp > (float)max_plate_temp) {
    		disable_ptt();
    		alarm = true;
    		enabled = false;
    		strcpy(alert_reason, "temp");
    	}
    	if (input_power > max_input_power) {
    		disable_ptt();
    		alarm = true;
    		enabled = false;
    		strcpy(alert_reason, "ipower");
    	}
   }

    if (HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin) || force_ptt) {
    	if (enabled && !alarm) {
    		ptt = true;
    		if (!last_ptt) {
    			last_ptt = true;
    			if (autoband) {
    				uint32_t freq = FrequencyCounter_GetRobustFrequency();
    				set_band_from_frequency(freq/1000);
    			}
    		//HAL_Delay(10);
    		HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_SET);
    		HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_SET);
    		}
    	}
    	else
    		//just to make sure :)
    		disable_ptt();
    }
    else {
    	disable_ptt();
    }

    if (enabled) {
    	HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_SET);
    } else HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_RESET);


}
