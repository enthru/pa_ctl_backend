#include "frequency_measurement.h"
#include <stdbool.h>
#include <string.h>
#include "set_functions.h"


#define MEDIAN_FILTER_SIZE     5
#define HYSTERESIS_THRESHOLD   10000

// Valid raw-measurement window. Upper bound must be above 6 m (50-54 MHz) + margin —
// the old 52 MHz cutoff silently rejected the top half of 6 m so it could never lock.
#define FREQ_VALID_MIN_HZ      1000000u
#define FREQ_VALID_MAX_HZ      55000000u

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

    return end_count * 2000; // GATE_TIME_MULTIPLIER    2000

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
    uint32_t measurements[MEDIAN_FILTER_SIZE];
    uint8_t valid_count = 0;

    // Each raw measurement is a 500 us hardware gate that must not be interrupted
    // between gate-expiry and stopping TIM8 (an ISR there over-counts). Disable IRQs
    // only for the single measurement, not the whole batch — this caps the protection
    // blind-spot at ~500 us instead of ~2.5 ms. Measurement runs while the PA relay is
    // still open (pre-RF window), so brief blind time here is acceptable; the median +
    // hysteresis + consecutive-band voting below reject any measurement that still slips.
    for (int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        uint32_t pm = __get_PRIMASK();
        __disable_irq();
        uint32_t m = FrequencyCounter_MeasureRaw();
        __set_PRIMASK(pm);

        if (m > FREQ_VALID_MIN_HZ && m < FREQ_VALID_MAX_HZ) {
            measurements[valid_count++] = m;
        }
    }

    if (valid_count == 0) {
        return 0;
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

    return ApplyPPMCorrection(last_stable_frequency);
}

// Coarse "is there any RF on HF_IN right now?" probe. IRQs stay ENABLED: an ISR can
// only ADD counts to a gate, so it can cause a false positive (harmless — the full
// measurement below then reports NO_SIGNAL) but never a false negative. This lets us
// poll cheaply during an SSB pause without ever blinding the protection loop.
#define RF_PRESENT_MIN_HZ   1000000u   // below 160 m (1.8 MHz); anything above => RF present

static bool rf_present(void)
{
    uint32_t m = FrequencyCounter_MeasureRaw();
    return (m > RF_PRESENT_MIN_HZ);
}

#define BAND_LOCK_COUNT   7   // consecutive identical classifications required

freq_status_t getFrequencyBand(uint32_t *freq_out)
{
    // Fail fast on silence (SSB pause / not yet keyed). No IRQ-off here.
    if (!rf_present()) {
        return FREQ_NO_SIGNAL;
    }

    // RF is present — run the accurate measurement and vote. Track a stable lock on a
    // known band AND a stable "outside all bands" condition separately (decision: RF
    // present but stably out of band => alarm).
    char last_band[5] = {0};
    int in_band_row = 0;
    int oob_row = 0;
    uint32_t last_freq = 0;

    for (int i = 0; i < 10; i++) {
        uint32_t freq = FrequencyCounter_GetRobustFrequency();

        if (freq == 0) {
            // Signal vanished mid-attempt (SSB null between syllables) — let the caller
            // retry next loop iteration rather than treating it as an out-of-band fault.
            return FREQ_NO_SIGNAL;
        }

        const char *band = get_band_from_frequency(freq);

        if (strcmp(band, "unk") == 0) {
            oob_row++;
            in_band_row = 0;
            if (oob_row >= BAND_LOCK_COUNT) {
                return FREQ_OUT_OF_BAND;
            }
        } else {
            oob_row = 0;
            if (in_band_row == 0 || strcmp(band, last_band) != 0) {
                strncpy(last_band, band, sizeof(last_band) - 1);
                last_band[sizeof(last_band) - 1] = '\0';
                in_band_row = 1;
            } else {
                in_band_row++;
            }
            last_freq = freq;
            if (in_band_row >= BAND_LOCK_COUNT) {
                *freq_out = last_freq;
                return FREQ_LOCKED;
            }
        }
    }

    // RF present but neither a band nor "out of band" reached the lock count — unstable.
    return FREQ_UNSTABLE;
}
