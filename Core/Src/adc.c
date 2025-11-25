#include "adc.h"
#include "main.h"
#include "uart_handler.h"
#include <string.h>
#include "default_values.h"
#include "variables.h"

ADC_Values_t adc_data;

void processADCValues(void) {
    adc_data.voltage0 = adc_dma_buffer[0] * ADC_SCALE_FACTOR;
    adc_data.voltage1 = adc_dma_buffer[1] * ADC_SCALE_FACTOR;
    adc_data.voltage2 = adc_dma_buffer[2] * ADC_SCALE_FACTOR;
    adc_data.voltage3 = adc_dma_buffer[3] * ADC_SCALE_FACTOR;
    adc_data.voltage4 = adc_dma_buffer[4] * ADC_SCALE_FACTOR;
    adc_data.voltage5 = adc_dma_buffer[5] * ADC_SCALE_FACTOR;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        processADCValues();
    }
}

