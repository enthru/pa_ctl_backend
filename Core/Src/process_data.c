#include "process_data.h"
#include "variables.h"
#include "frequency_measurement.h"
#include "main.h"
#include "set_functions.h"
#include <math.h>
#include <string.h>
#include "adc.h"
#include <stdio.h>

typedef enum {
    BAND_GROUP_LOW,    // 160m, 80m, 60m, 40m
    BAND_GROUP_MID,    // 30m, 20m, 17m, 15m
    BAND_GROUP_HIGH,   // 12m, 10m, 6m
    BAND_GROUP_UNK
} band_group_t;

static band_group_t band_group = BAND_GROUP_UNK;

void update_band_group(void) {
    if (strcmp(current_band, "160m") == 0 || strcmp(current_band, "80m") == 0 ||
        strcmp(current_band, "60m")  == 0 || strcmp(current_band, "40m") == 0) {
        band_group = BAND_GROUP_LOW;
    } else
    if (strcmp(current_band, "30m") == 0 || strcmp(current_band, "20m") == 0 ||
        strcmp(current_band, "17m") == 0 || strcmp(current_band, "15m") == 0) {
        band_group = BAND_GROUP_MID;
    } else
    if (strcmp(current_band, "12m") == 0 || strcmp(current_band, "10m") == 0 ||
        strcmp(current_band, "6m")  == 0) {
        band_group = BAND_GROUP_HIGH;
    } else {
        band_group = BAND_GROUP_UNK;
    }
}

float voltage_to_current(float voltage) {
    return (acs_zero - voltage) / acs_sens;
}

void disable_ptt(void) {
    last_ptt = false;
    ptt = false;
    force_ptt = false;
    HAL_GPIO_WritePin(PTT_OUT_GPIO_Port, PTT_OUT_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_RESET);
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

#define FILT_ALPHA_SLOW  0.02f

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

static ema_filter_t f_fwd_slow = {0};
static ema_filter_t f_rev_slow = {0};
static ema_filter_t f_inp_slow = {0};
static ema_filter_t f_cur_slow = {0};
static ema_filter_t f_volt_slow = {0};

static uint8_t cnt_swr   = 0;
static uint8_t cnt_cur   = 0;
static uint8_t cnt_inp   = 0;
static uint8_t cnt_volt  = 0;
static uint8_t cnt_coeff = 0;

static bool delay_active = false;
static uint32_t delay_start = 0;

static char prev_band[8] = {0};

static inline void compute_powers(float v_fwd, float v_rev, float v_inp,
                                  band_group_t group,
                                  float *out_fwd, float *out_rev, float *out_inp)
{
    float fwd_c, rev_c, inp_c;

    switch (group) {
    case BAND_GROUP_LOW:
        fwd_c = low_fwd_coeff;
        rev_c = low_rev_coeff;
        inp_c = low_ifwd_coeff;
        break;
    case BAND_GROUP_MID:
        fwd_c = mid_fwd_coeff;
        rev_c = mid_rev_coeff;
        inp_c = mid_ifwd_coeff;
        break;
    case BAND_GROUP_HIGH:
        fwd_c = high_fwd_coeff;
        rev_c = high_rev_coeff;
        inp_c = high_ifwd_coeff;
        break;
    default:
        *out_fwd = 0.0f;
        *out_rev = 0.0f;
        *out_inp = 0.0f;
        return;
    }

    *out_fwd = v_fwd * v_fwd * fwd_c;
    *out_rev = v_rev * v_rev * rev_c;
    *out_inp = v_inp * v_inp * inp_c;
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

void process_data(void) {

    if (strcmp(current_band, prev_band) != 0) {
        strncpy(prev_band, current_band, sizeof(prev_band) - 1);
        prev_band[sizeof(prev_band) - 1] = '\0';
        update_band_group();
    }

    float raw_v0 = adc_data.voltage0;   // FWD detector
    float raw_v1 = adc_data.voltage1;   // REV detector
    float raw_v2 = adc_data.voltage2;   // Input FWD detector
    float raw_v3 = adc_data.voltage3;   // Voltage sense
    float raw_v4 = adc_data.voltage4;   // Current sense (ACS)

    float raw_fwd = 0.0f, raw_rev = 0.0f, raw_inp = 0.0f;
    compute_powers(raw_v0, raw_v1, raw_v2, band_group,
                   &raw_fwd, &raw_rev, &raw_inp);

    float raw_cur     = voltage_to_current(raw_v4 * current_coeff);
    float raw_voltage = raw_v3 * voltage_coeff;

    float raw_fwd_nz = (raw_fwd < 0.1f) ? 0.0f : raw_fwd;
    float raw_rev_nz = (raw_rev < 0.1f) ? 0.0f : raw_rev;
    float raw_swr    = clamp_swr(compute_swr(raw_fwd_nz, raw_rev_nz));

    forward_power = ema_update(&f_fwd_slow, raw_fwd, FILT_ALPHA_SLOW);
    reverse_power = ema_update(&f_rev_slow, raw_rev, FILT_ALPHA_SLOW);
    input_power   = ema_update(&f_inp_slow, raw_inp, FILT_ALPHA_SLOW);
    current       = ema_update(&f_cur_slow, raw_cur, FILT_ALPHA_SLOW);
    voltage       = ema_update(&f_volt_slow, raw_voltage, FILT_ALPHA_SLOW);

    if (forward_power < 0.1f) forward_power = 0.0f;
    if (reverse_power < 0.1f) reverse_power = 0.0f;
    if (input_power   < 0.1f) input_power   = 0.0f;
    if (input_power > 100.0f) input_power   = 100.0f;

    swr = clamp_swr(compute_swr(forward_power, reverse_power));

    psu_power = voltage * current;
    if (psu_power > 1.0f) {
        coeff = ((forward_power - reverse_power) / psu_power) * 100.0f;
    } else {
        coeff = 0.0f;
    }

    if (protection_enabled && !alarm && startup_complete) {

        if (band_group == BAND_GROUP_UNK) {
            trigger_alarm();
            strcpy(alert_reason, "band");
        }

        CHECK_PROT(raw_swr,     max_swr,         cnt_swr,   "swr")
        CHECK_PROT(raw_voltage,  max_voltage,      cnt_volt,  "voltage")
        CHECK_PROT(raw_cur,      max_current,      cnt_cur,   "current")
        CHECK_PROT(raw_inp,      max_input_power,  cnt_inp,   "ipower")

        if (water_temp > (float)max_water_temp) {
            trigger_alarm();
            strcpy(alert_reason, "water_temp");
        }
        if (plate_temp > (float)max_plate_temp) {
            trigger_alarm();
            strcpy(alert_reason, "plate_temp");
        }

        if (ptt && psu_power > 1.0f && raw_fwd_nz > 100.0f) {
            float raw_coeff = ((raw_fwd - raw_rev) / (raw_voltage * raw_cur)) * 100.0f;
            if (raw_coeff < (float)min_coeff) {
                if (++cnt_coeff >= PROT_TRIGGER_COUNT) {
                    cnt_coeff = 0;
                    trigger_alarm();
                    strcpy(alert_reason, "coeff");
                }
            } else {
                cnt_coeff = 0;
            }
        } else {
            cnt_coeff = 0;
        }
    }

    if (alarm) {
        disable_ptt();
        enabled = false;
    }

    if (HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin) || force_ptt) {
        if (enabled && !alarm) {
            ptt = true;
            if (!last_ptt) {
                delay_active = true;
                delay_start = HAL_GetTick();
                last_ptt = true;
            }
            if (delay_active && !getfreq_flag) {
                if (HAL_GetTick() - delay_start >= 30) {
                    delay_active = false;
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

    if (enabled && startup_complete) {
        HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(PWR_CTL_GPIO_Port, PWR_CTL_Pin, GPIO_PIN_RESET);
    }
}
