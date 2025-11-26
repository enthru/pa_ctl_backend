#ifndef DEFAULT_VALUES_H
#define DEFAULT_VALUES_H

#include <stdint.h>
#include <stdbool.h>


#define DEFAULT_MAX_SWR                 2
#define DEFAULT_MAX_CURRENT             40
#define DEFAULT_MAX_VOLTAGE             55
#define DEFAULT_MAX_WATER_TEMP          60
#define DEFAULT_MAX_PLATE_TEMP          80
#define DEFAULT_MAX_PUMP_SPEED_TEMP     55
#define DEFAULT_MIN_PUMP_SPEED_TEMP     20
#define DEFAULT_MAX_FAN_SPEED_TEMP      70
#define DEFAULT_MIN_FAN_SPEED_TEMP      20
#define DEFAULT_AUTOBAND                false
#define DEFAULT_BAND_VALUE              {'2', '0', 'm', '\0'}
#define DEFAULT_MAX_INPUT_POWER			3

#define ADC_REF_VOLTAGE      			3.3f
#define ADC_MAX_VALUE        			4095.0f
#define ADC_CALIBRATION_FACTOR 			1.03f
#define ADC_SCALE_FACTOR     			(ADC_REF_VOLTAGE * ADC_CALIBRATION_FACTOR / ADC_MAX_VALUE)

// for limit checks
#define MAX_SWR_MIN          			1
#define MAX_SWR_MAX          			10
#define MAX_CURRENT_MIN      			1
#define MAX_CURRENT_MAX      			70
#define MAX_VOLTAGE_MIN      			1
#define MAX_VOLTAGE_MAX      			70
#define MAX_WATER_TEMP_MIN   			10
#define MAX_WATER_TEMP_MAX   			100
#define MAX_PLATE_TEMP_MIN   			10
#define MAX_PLATE_TEMP_MAX   			200
#define MAX_PUMP_SPEED_TEMP_MIN 		10
#define MAX_PUMP_SPEED_TEMP_MAX 		90
#define MIN_PUMP_SPEED_TEMP_MIN 		10
#define MIN_PUMP_SPEED_TEMP_MAX 		90
#define MAX_FAN_SPEED_TEMP_MIN  		10
#define MAX_FAN_SPEED_TEMP_MAX  		200
#define MIN_FAN_SPEED_TEMP_MIN  		10
#define MIN_FAN_SPEED_TEMP_MAX  		200
#define PWM_PUMP_MIN         			1
#define PWM_PUMP_MAX         			100
#define PWM_COOLER_MIN       			1
#define PWM_COOLER_MAX       			100
#define BAND_MAX_LENGTH      			4
#define MIN_MAX_INPUT_POWER				1
#define MAX_MAX_INPUT_POWER             10

#define LOW_FWD_COEFF_MIN 1.0
#define LOW_FWD_COEFF_MAX 1000.0
#define LOW_REV_COEFF_MIN 1.0
#define LOW_REV_COEFF_MAX 1000.0
#define LOW_IFWD_COEFF_MIN 1.0
#define LOW_IFWD_COEFF_MAX 1000.0

#define MID_FWD_COEFF_MIN 1.0
#define MID_FWD_COEFF_MAX 1000.0
#define MID_REV_COEFF_MIN 1.0
#define MID_REV_COEFF_MAX 1000.0
#define MID_IFWD_COEFF_MIN 1.0
#define MID_IFWD_COEFF_MAX 1000.0

#define HIGH_FWD_COEFF_MIN 1.0
#define HIGH_FWD_COEFF_MAX 1000.0
#define HIGH_REV_COEFF_MIN 1.0
#define HIGH_REV_COEFF_MAX 1000.0
#define HIGH_IFWD_COEFF_MIN 1.0
#define HIGH_IFWD_COEFF_MAX 1000.0

#define VOLTAGE_COEFF_MIN 1.0
#define VOLTAGE_COEFF_MAX 50.0
#define CURRENT_COEFF_MIN 1.0
#define CURRENT_COEFF_MAX 50.0
#define RSRV_COEFF_MIN 1.0
#define RSRV_COEFF_MAX 1000.0

#endif
