#ifndef PWM_H
#define PWM_H

#include "main.h"

void PWM_SetPumpDuty(uint8_t duty_percent);
void PWM_SetCoolerDuty(uint8_t duty_percent);
uint8_t calculate_pwm_percentage(int16_t current_temp, int16_t lower_temp, int16_t upper_temp);

#endif
