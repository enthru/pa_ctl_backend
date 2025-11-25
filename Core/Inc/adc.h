#ifndef ADC_H
#define ADC_H

#include "stm32f4xx_hal.h"

typedef struct {
    float voltage0; //current
    float voltage1; //ifwd
    float voltage2; //fwd
    float voltage3; //reserv
    float voltage4; //voltage
    float voltage5; //rev
} ADC_Values_t;

extern ADC_HandleTypeDef hadc1;
extern ADC_Values_t adc_data;

void processADCValues(void);

#endif
