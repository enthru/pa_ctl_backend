#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"

#define REFERENCE_POWER 1500.0f
#define REFERENCE_VOLTAGE 8.0f
#define VOLTAGE_DIVIDER_RATIO 3.0f

#define INPUT_REFERENCE_POWER 100.0f
#define INPUT_REFERENCE_VOLTAGE 7.0f
#define INPUT_VOLTAGE_DIVIDER_RATIO 2.0f

#define VOLTAGE_DIVIDER_RATIO_MAIN 20.0f
#define ACS722_SENSITIVITY 0.04f  // ACS722-40AU
#define ACS722_VCC 3.3f
#define ACS722_ZERO_CURRENT_VOLTAGE (ACS722_VCC / 2.0f)

float voltage_to_power(float voltage, float reference_power, float reference_voltage, float divider_ratio) {
    float actual_voltage = voltage * divider_ratio;
    float power = reference_power * (actual_voltage * actual_voltage) / (reference_voltage * reference_voltage);
    return power;
}

float voltage_to_current(float voltage) {
    float current = (voltage - ACS722_ZERO_CURRENT_VOLTAGE) / ACS722_SENSITIVITY;
    return current;
    //return voltage;
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
    forward_power = voltage_to_power(adc_data.voltage0, REFERENCE_POWER, REFERENCE_VOLTAGE, VOLTAGE_DIVIDER_RATIO);
    reverse_power = voltage_to_power(adc_data.voltage1, REFERENCE_POWER, REFERENCE_VOLTAGE, VOLTAGE_DIVIDER_RATIO);

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

    input_power = voltage_to_power(adc_data.voltage2, INPUT_REFERENCE_POWER, INPUT_REFERENCE_VOLTAGE, INPUT_VOLTAGE_DIVIDER_RATIO);
    if (input_power < 0.1f) input_power = 0.0f;
    if (input_power > 100.0f) input_power = 100.0f;

    voltage = adc_data.voltage3 * VOLTAGE_DIVIDER_RATIO_MAIN;

    current = voltage_to_current(adc_data.voltage4);

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
