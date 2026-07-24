#ifndef FREQUENCY_MEASUREMENT_H
#define FREQUENCY_MEASUREMENT_H

#include "stm32f4xx.h"
#include "main.h"

// Result of one band-detection attempt. SSB has no carrier during pauses, so
// "no RF yet" must be distinguished from "RF present but not a valid band".
typedef enum {
    FREQ_NO_SIGNAL = 0,   // no RF on HF_IN (SSB pause / not keyed up yet) — keep waiting
    FREQ_UNSTABLE,        // RF present but no stable band lock yet — keep waiting / retry
    FREQ_LOCKED,          // stable in-band frequency; *freq_out is valid
    FREQ_OUT_OF_BAND      // RF present and stably outside every ham band — alarm
} freq_status_t;

void FrequencyCounter_Init(void);
uint32_t FrequencyCounter_MeasureRaw(void);
uint32_t FrequencyCounter_GetLastFrequency(void);
uint32_t FrequencyCounter_GetRobustFrequency(void);

// One non-blocking-ish band-detection attempt. Returns immediately (no IRQ-off) when
// there is no RF, so it can be polled every main-loop iteration during an SSB pause.
freq_status_t getFrequencyBand(uint32_t *freq_out);

#endif
