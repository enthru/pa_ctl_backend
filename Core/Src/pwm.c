#include "pwm.h"


extern uint8_t pwm_pump;
extern uint8_t pwm_cooler;

extern TIM_HandleTypeDef htim3;

void PWM_SetPumpDuty(uint8_t duty_percent) {
    if (duty_percent > 100) {
        duty_percent = 100;
    }

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    uint32_t compare_value = (period * duty_percent) / 100;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_value);
}

void PWM_SetCoolerDuty(uint8_t duty_percent) {
    if (duty_percent > 100) {
        duty_percent = 100;
    }

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    uint32_t compare_value = (period * duty_percent) / 100;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compare_value);
}

uint8_t calculate_pwm_percentage(int16_t current_temp, int16_t lower_temp, int16_t upper_temp)
{

    if (lower_temp >= upper_temp) {
        return 10;
    }
    if (current_temp <= lower_temp) {
        return 10;
    }
    if (current_temp >= upper_temp) {
        return 100;
    }

    int32_t temp_range = upper_temp - lower_temp;
    int32_t temp_position = current_temp - lower_temp;

    int32_t pwm = 10 + (90 * temp_position) / temp_range;

    return (uint8_t)pwm;
}
