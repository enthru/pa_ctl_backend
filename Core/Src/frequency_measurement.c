#include "frequency_measurement.h"
#include <stdbool.h>

#define MEDIAN_FILTER_SIZE 5
#define AVG_SAMPLES 3
#define HYSTERESIS_THRESHOLD 10000

static uint32_t last_stable_frequency = 0;
static uint32_t frequency_buffer[MEDIAN_FILTER_SIZE] = {0};
static uint8_t buffer_index = 0;

#define FREQ_PPM_CORRECTION  -28506

static inline uint32_t ApplyPPMCorrection(uint32_t freq)
{
    int64_t corrected = (int64_t)freq * (1000000LL + FREQ_PPM_CORRECTION);
    return (uint32_t)(corrected / 1000000LL);
}

uint32_t FrequencyCounter_MedianFilter(uint32_t new_value)
{
    frequency_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % MEDIAN_FILTER_SIZE;

    uint32_t temp[MEDIAN_FILTER_SIZE];
    for(int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        temp[i] = frequency_buffer[i];
    }

    for(int i = 0; i < MEDIAN_FILTER_SIZE/2 + 1; i++) {
        for(int j = i+1; j < MEDIAN_FILTER_SIZE; j++) {
            if(temp[i] > temp[j]) {
                uint32_t swap = temp[i];
                temp[i] = temp[j];
                temp[j] = swap;
            }
        }
    }

    return temp[MEDIAN_FILTER_SIZE/2];
}

uint32_t FrequencyCounter_SmartAverage(void)
{
    uint32_t measurements[AVG_SAMPLES];
    uint32_t sum = 0;
    uint8_t valid_count = 0;

    for(int i = 0; i < AVG_SAMPLES; i++) {

        measurements[i] = FrequencyCounter_MeasureRaw();

        if(measurements[i] > 1000000 && measurements[i] < 30000000) {
            sum += measurements[i];
            valid_count++;
        }
    }

    return (valid_count > 0) ? (sum / valid_count) : last_stable_frequency;
}

uint32_t FrequencyCounter_GetRobustFrequency(void)
{
    uint32_t raw_avg = FrequencyCounter_SmartAverage();
    uint32_t median_filtered = FrequencyCounter_MedianFilter(raw_avg);

    if(last_stable_frequency == 0) {
        last_stable_frequency = median_filtered;
    } else {
        uint32_t deviation = (median_filtered > last_stable_frequency) ?
                           (median_filtered - last_stable_frequency) :
                           (last_stable_frequency - median_filtered);

        if(deviation > HYSTERESIS_THRESHOLD) {
            last_stable_frequency = median_filtered;
        }
    }

    return ApplyPPMCorrection(last_stable_frequency);
}

uint32_t FrequencyCounter_MeasureRaw(void)
{
    uint32_t end_count;

    TIM8->CNT = 0;
    TIM3->CNT = 0;

    TIM8->CR1 = TIM_CR1_CEN;
    TIM3->CR1 = TIM_CR1_CEN;

    while (!(TIM3->SR & TIM_SR_UIF));

    TIM3->SR = 0;
    TIM8->CR1 = 0;
    TIM3->CR1 = 0;

    end_count = TIM8->CNT;

    return end_count * 2000;
}
