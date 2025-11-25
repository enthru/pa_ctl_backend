#ifndef FREQUENCY_MEASUREMENT_H
#define FREQUENCY_MEASUREMENT_H

#include "stm32f4xx.h"
#include "main.h"

void FrequencyCounter_Init(void);
uint32_t FrequencyCounter_MeasureRaw(void);
uint32_t FrequencyCounter_GetLastFrequency(void);
uint32_t FrequencyCounter_GetRobustFrequency(void);

#endif
