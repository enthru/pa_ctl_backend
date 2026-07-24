#include "adc.h"
#include "main.h"
#include "uart_handler.h"
#include <string.h>
#include "default_values.h"
#include "variables.h"

volatile ADC_Values_t adc_data;

void processADCValues(void) {
    adc_data.voltage0 = adc_dma_buffer[0] * ADC_SCALE_FACTOR;
    adc_data.voltage1 = adc_dma_buffer[1] * ADC_SCALE_FACTOR;
    adc_data.voltage2 = adc_dma_buffer[2] * ADC_SCALE_FACTOR;
    adc_data.voltage3 = adc_dma_buffer[3] * ADC_SCALE_FACTOR;
    adc_data.voltage4 = adc_dma_buffer[4] * ADC_SCALE_FACTOR;
    adc_data.voltage5 = adc_dma_buffer[5] * ADC_SCALE_FACTOR;
}

// NOTE: the ADC DMA transfer-complete interrupt is disabled in main() (see
// __HAL_DMA_DISABLE_IT after HAL_ADC_Start_DMA), so this callback no longer fires.
// process_data() reads adc_dma_buffer directly at 50 kHz. Kept for reference / in
// case the per-scan callback is ever needed again.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        processADCValues();
    }
}

