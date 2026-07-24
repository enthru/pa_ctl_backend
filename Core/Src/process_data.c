#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"
#include "default_values.h"   // ADC_SCALE_FACTOR
#include <stdio.h>

// Cached band power coefficients — set by process_data_on_band_change(), never in ISR
static float c_fwd = 0.0f, c_rev = 0.0f, c_inp = 0.0f;

// Precomputed reciprocal of acs_sens — replaces FP divide in hot path
static float c_inv_acs_sens = 1.0f;

// SWR > N  ⟺  rev/fwd > ((N-1)/(N+1))²  — avoids sqrtf in ISR
static float swr_thresh_ratio_sq = 0.0f;

void process_data_on_band_change(void) {
    // Compute the new coefficient set into locals first, then publish it to the
    // ISR-visible cache atomically — otherwise the 50 kHz process_data() could read
    // a half-updated {fwd,rev,inp} triple and mis-scale power/SWR/coeff for one sample.
    float f, r, i;
    switch (current_band_id) {
    case BAND_160M: case BAND_80M: case BAND_60M: case BAND_40M:
        f = low_fwd_coeff;  r = low_rev_coeff;  i = low_ifwd_coeff;  break;
    case BAND_30M: case BAND_20M: case BAND_17M: case BAND_15M:
        f = mid_fwd_coeff;  r = mid_rev_coeff;  i = mid_ifwd_coeff;  break;
    case BAND_12M: case BAND_10M: case BAND_6M:
        f = high_fwd_coeff; r = high_rev_coeff; i = high_ifwd_coeff; break;
    default:
        f = 0.0f; r = 0.0f; i = 0.0f; break;
    }

    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    c_fwd = f; c_rev = r; c_inp = i;
    __set_PRIMASK(pm);
}

void process_data_on_config_change(void) {
    float inv = (acs_sens != 0.0f) ? (1.0f / acs_sens) : 1.0f;
    float r = ((float)max_swr - 1.0f) / ((float)max_swr + 1.0f);
    float thr = r * r;

    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    c_inv_acs_sens = inv;
    swr_thresh_ratio_sq = thr;
    __set_PRIMASK(pm);

    process_data_on_band_change();
}

float voltage_to_current(float voltage) {
    return (acs_zero - voltage) * c_inv_acs_sens;
}

static bool ptt_gpio_active = false;

static inline void disable_ptt(void) {
    last_ptt = false;
    ptt = false;
    force_ptt = false;
    if (ptt_gpio_active) {
        ptt_gpio_active = false;
        HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_RESET);
    }
}

void trigger_alarm(void) {
    disable_ptt();
    enabled = false;
    alarm = true;
    alarm_time = HAL_GetTick();
}

typedef struct {
    float y;
    uint8_t initialized;
} ema_filter_t;

static inline float ema_update(ema_filter_t *f, float x, float alpha) {
    if (!f->initialized) {
        f->y = x;
        f->initialized = 1;
        return x;
    }
    f->y += alpha * (x - f->y);
    return f->y;
}

// tau = 1/(alpha * 50000) = 200 us — fast enough for PA protection
#define FILT_ALPHA_SLOW  0.10f

#define PROT_TRIGGER_COUNT  3

#define CHECK_PROT(val, threshold, counter, reason)  \
    if ((val) > (float)(threshold)) {                \
        if (++(counter) >= PROT_TRIGGER_COUNT) {     \
            (counter) = 0;                           \
            trigger_alarm();                         \
            strcpy(alert_reason, (reason));          \
        }                                            \
    } else {                                         \
        (counter) = 0;                               \
    }

static ema_filter_t f_fwd_slow  = {0};
static ema_filter_t f_rev_slow  = {0};
static ema_filter_t f_inp_slow  = {0};
static ema_filter_t f_cur_slow  = {0};
static ema_filter_t f_volt_slow = {0};

static uint8_t cnt_swr   = 0;
static uint8_t cnt_cur   = 0;
static uint8_t cnt_inp   = 0;
static uint8_t cnt_volt  = 0;
static uint8_t cnt_coeff = 0;

// Blanking window for coeff after relay close: 2500 * 20us = 50ms
#define COEFF_BLANK_COUNT 2500
static uint32_t coeff_blank = 0;
static bool fwd_above = false;  // edge detection for forward_power > 100W

static bool delay_active = false;
static uint32_t delay_start = 0;
static bool pwr_ctl_active = false;

static inline void compute_powers(float v_fwd, float v_rev, float v_inp,
                                  float *out_fwd, float *out_rev, float *out_inp)
{
    if (c_fwd == 0.0f) {
        *out_fwd = 0.0f;
        *out_rev = 0.0f;
        *out_inp = 0.0f;
        return;
    }
    *out_fwd = v_fwd * v_fwd * c_fwd;
    *out_rev = v_rev * v_rev * c_rev;
    *out_inp = v_inp * v_inp * c_inp;
}

static inline float compute_swr(float fwd, float rev) {
    if (fwd > 0.1f && rev > 0.1f) {
        float rc = sqrtf(rev / fwd);
        if (rc >= 1.0f) return 99.99f;
        return (1.0f + rc) / (1.0f - rc);
    }
    return 1.0f;
}

static inline float clamp_swr(float s) {
    if (s < 1.0f)   return 1.0f;
    if (s > 99.99f)  return 99.99f;
    return s;
}

float process_data_get_swr(void) {
    return clamp_swr(compute_swr(forward_power, reverse_power));
}

void process_data(void) {

    // Read the DMA buffer directly (circular DMA keeps it fresh). This removes the
    // ~130 kHz ADC transfer-complete ISR that previously refreshed adc_data on every
    // scan; process_data() is the only consumer and runs at 50 kHz, so scaling here
    // is both cheaper and more deterministic for the protection loop.
    float raw_v0 = adc_dma_buffer[0] * ADC_SCALE_FACTOR;   // FWD detector
    float raw_v1 = adc_dma_buffer[1] * ADC_SCALE_FACTOR;   // REV detector
    float raw_v2 = adc_dma_buffer[2] * ADC_SCALE_FACTOR;   // Input FWD detector
    float raw_v3 = adc_dma_buffer[3] * ADC_SCALE_FACTOR;   // Voltage sense
    float raw_v4 = adc_dma_buffer[4] * ADC_SCALE_FACTOR;   // Current sense (ACS)

    float raw_fwd = 0.0f, raw_rev = 0.0f, raw_inp = 0.0f;
    compute_powers(raw_v0, raw_v1, raw_v2, &raw_fwd, &raw_rev, &raw_inp);

    float raw_cur     = voltage_to_current(raw_v4 * current_coeff);
    float raw_voltage = raw_v3 * voltage_coeff;

    forward_power = ema_update(&f_fwd_slow,  raw_fwd,     FILT_ALPHA_SLOW);
    reverse_power = ema_update(&f_rev_slow,  raw_rev,     FILT_ALPHA_SLOW);
    input_power   = ema_update(&f_inp_slow,  raw_inp,     FILT_ALPHA_SLOW);
    current       = ema_update(&f_cur_slow,  raw_cur,     FILT_ALPHA_SLOW);
    voltage       = ema_update(&f_volt_slow, raw_voltage, FILT_ALPHA_SLOW);

    if (forward_power < 0.1f) forward_power = 0.0f;
    if (reverse_power < 0.1f) reverse_power = 0.0f;
    if (input_power   < 0.1f) input_power   = 0.0f;
    if (input_power > 100.0f) input_power   = 100.0f;

    psu_power = voltage * current;
    if (psu_power > 1.0f) {
        coeff = ((forward_power - reverse_power) / psu_power) * 100.0f;
    } else {
        coeff = 0.0f;
    }

    if (protection_enabled && !alarm && startup_complete) {

        if (current_band_id == BAND_UNKNOWN) {
            trigger_alarm();
            strcpy(alert_reason, "band");
        }

        if (forward_power > 0.0f) {
            CHECK_PROT(reverse_power / forward_power, swr_thresh_ratio_sq, cnt_swr, "swr")
        } else {
            cnt_swr = 0;
        }
        CHECK_PROT(voltage,     max_voltage,      cnt_volt, "voltage")
        CHECK_PROT(current,     max_current,      cnt_cur,  "current")
        CHECK_PROT(input_power, max_input_power,  cnt_inp,  "ipower")

        if (water_temp > (float)max_water_temp) {
            trigger_alarm();
            strcpy(alert_reason, "water_temp");
        }
        if (plate_temp > (float)max_plate_temp) {
            trigger_alarm();
            strcpy(alert_reason, "plate_temp");
        }

        if (ptt && !delay_active && psu_power > 1.0f && forward_power > 100.0f) {
            if (!fwd_above) {
                fwd_above = true;
                coeff_blank = COEFF_BLANK_COUNT;
            }
            if (coeff_blank > 0) {
                coeff_blank--;
                cnt_coeff = 0;
            } else if (coeff < (float)min_coeff) {
                if (++cnt_coeff >= PROT_TRIGGER_COUNT) {
                    cnt_coeff = 0;
                    trigger_alarm();
                    strcpy(alert_reason, "coeff");
                }
            } else {
                cnt_coeff = 0;
            }
        } else {
            fwd_above = false;
            cnt_coeff = 0;
        }
    }

    if (alarm) {
        disable_ptt();
        enabled = false;
    }

    if ((PTT_IN_GPIO_Port->IDR & PTT_IN_Pin) || force_ptt) {
        if (enabled && !alarm && !flash_busy) {
            ptt = true;
            if (!last_ptt) {
                delay_active = true;
                delay_start = HAL_GetTick();
                last_ptt = true;
            }
            if (delay_active && !getfreq_flag) {
                if (HAL_GetTick() - delay_start >= 30) {
                    delay_active = false;
                    coeff_blank = COEFF_BLANK_COUNT;
                    ptt_gpio_active = true;
                    HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_SET);
                }
            }
        } else {
            disable_ptt();
        }
    } else {
        disable_ptt();
    }

    bool want_pwr = (enabled && startup_complete);
    if (want_pwr != pwr_ctl_active) {
        pwr_ctl_active = want_pwr;
        HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin,
                          want_pwr ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
