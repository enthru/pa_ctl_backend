#include "frequency_measurement.h"
#include <stdbool.h>

#define MEDIAN_FILTER_SIZE     5
#define HYSTERESIS_THRESHOLD   10000

static uint32_t last_stable_frequency = 0;

//#define FREQ_PPM_CORRECTION  -28506
#define FREQ_PPM_CORRECTION     1

static inline uint32_t ApplyPPMCorrection(uint32_t freq)
{
    int64_t corrected = (int64_t)freq * (1000000LL + FREQ_PPM_CORRECTION);
    return (uint32_t)(corrected / 1000000LL);
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

static uint32_t MedianOfArray(uint32_t *arr, uint8_t n)
{
    for (uint8_t i = 0; i < n - 1; i++) {
        for (uint8_t j = i + 1; j < n; j++) {
            if (arr[i] > arr[j]) {
                uint32_t tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
    return arr[n / 2];
}

uint32_t FrequencyCounter_GetRobustFrequency(void)
{
    __disable_irq();

    uint32_t measurements[MEDIAN_FILTER_SIZE];
    uint8_t valid_count = 0;

    for (int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        uint32_t m = FrequencyCounter_MeasureRaw();

        if (m > 1000000 && m < 52000000) {
            measurements[valid_count++] = m;
        }
    }

    if (valid_count == 0) {
        uint32_t res = ApplyPPMCorrection(0);
        __enable_irq();
        return res;
    }

    uint32_t median = MedianOfArray(measurements, valid_count);

    if (last_stable_frequency == 0) {
        last_stable_frequency = median;
    } else {
        uint32_t deviation = (median > last_stable_frequency) ?
                             (median - last_stable_frequency) :
                             (last_stable_frequency - median);

        if (deviation > HYSTERESIS_THRESHOLD) {
            last_stable_frequency = median;
        }
    }

    uint32_t result = ApplyPPMCorrection(last_stable_frequency);
    __enable_irq();
    return result;
}
