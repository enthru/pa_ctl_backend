#include "set_functions.h"
#include "main.h"
#include "variables.h"
#include <string.h>

#define BAND_GPIO_PIN_1 GPIO_PIN_9   // PA9
#define BAND_GPIO_PIN_2 GPIO_PIN_10  // PA10
#define BAND_GPIO_PIN_3 GPIO_PIN_11  // PA11
#define BAND_GPIO_PIN_4 GPIO_PIN_12  // PA12

static const uint32_t MAX_ERROR = 200;

#define BAND_GPIO_PORT GPIOA

typedef enum {
    BAND_160M = 0,
    BAND_80M,
    BAND_60M,
    BAND_40M,
    BAND_30M,
    BAND_20M,
    BAND_17M,
    BAND_15M,
    BAND_12M,
    BAND_10M,
    BAND_6M,
    BAND_UNKNOWN,
    BAND_COUNT
} Band_t;

const char *get_band_from_frequency(uint32_t frequency) {
    if (frequency >= 1800000 - MAX_ERROR && frequency <= 2000000 + MAX_ERROR) {
        return "160m";
    }
    else if (frequency >= 3500000 - MAX_ERROR && frequency <= 4000000 + MAX_ERROR) {
        return "80m";
    }
    else if (frequency >= 5300000 - MAX_ERROR && frequency <= 5500000 + MAX_ERROR) {
        return "60m";
    }
    else if (frequency >= 7000000 - MAX_ERROR && frequency <= 7300000 + MAX_ERROR) {
        return "40m";
    }
    else if (frequency >= 10100000 - MAX_ERROR && frequency <= 10150000 + MAX_ERROR) {
        return "30m";
    }
    else if (frequency >= 14000000 - MAX_ERROR && frequency <= 14350000 + MAX_ERROR) {
        return "20m";
    }
    else if (frequency >= 18068000 - MAX_ERROR && frequency <= 18168000 + MAX_ERROR) {
        return "17m";
    }
    else if (frequency >= 21000000 - MAX_ERROR && frequency <= 21450000 + MAX_ERROR) {
        return "15m";
    }
    else if (frequency >= 24890000 - MAX_ERROR && frequency <= 24990000 + MAX_ERROR) {
        return "12m";
    }
    else if (frequency >= 28000000 - MAX_ERROR && frequency <= 29700000 + MAX_ERROR) {
        return "10m";
    }
    else if (frequency >= 50000000 - MAX_ERROR && frequency <= 54000000 + MAX_ERROR) {
        return "6m";
    }
    else {
        return "unk";
    }
}

void set_band_from_frequency(uint32_t frequency) {
    const char *band = get_band_from_frequency(frequency);
    strncpy(current_band, band, sizeof(current_band) - 1);
    current_band[sizeof(current_band) - 1] = '\0';
}

static Band_t band_from_string(const char *band)
{
    typedef struct {
        const char *name;
        Band_t      id;
    } BandEntry_t;

    static const BandEntry_t table[] = {
        { "160m", BAND_160M },
        { "80m",  BAND_80M  },
        { "60m",  BAND_60M  },
        { "40m",  BAND_40M  },
        { "30m",  BAND_30M  },
        { "20m",  BAND_20M  },
        { "17m",  BAND_17M  },
        { "15m",  BAND_15M  },
        { "12m",  BAND_12M  },
        { "10m",  BAND_10M  },
        { "6m",   BAND_6M   },
    };

    for (uint8_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(band, table[i].name) == 0) {
            return table[i].id;
        }
    }

    return BAND_UNKNOWN;
}

void set_band_gpio(const char *band)
{
	static const uint8_t bcd_table[BAND_COUNT] = {
	        [BAND_160M]   = 0x0,   /* 0,0,0,0 */
	        [BAND_80M]    = 0x1,   /* 1,0,0,0 */
	        [BAND_60M]    = 0x2,   /* 0,1,0,0 */
	        [BAND_40M]    = 0x3,   /* 1,1,0,0 */
	        [BAND_30M]    = 0x4,   /* 0,0,1,0 */
	        [BAND_20M]    = 0x5,   /* 1,0,1,0 */
	        [BAND_17M]    = 0x6,   /* 0,1,1,0 */
	        [BAND_15M]    = 0x7,   /* 1,1,1,0 */
	        [BAND_12M]    = 0x8,   /* 0,0,0,1 */
	        [BAND_10M]    = 0x8,   /* 0,0,0,1 */
	        [BAND_6M]     = 0x9,   /* 1,0,0,1 */
	        [BAND_UNKNOWN]= 0x0,   /* 0,0,0,0 */
	    };

    static const uint16_t gpio_pins[4] = {
        BAND_GPIO_PIN_1,
        BAND_GPIO_PIN_2,
        BAND_GPIO_PIN_3,
        BAND_GPIO_PIN_4,
    };

    Band_t band_id = band_from_string(band);
    uint8_t code = bcd_table[band_id];

    for (uint8_t i = 0; i < 4; i++) {
        GPIO_PinState state = (code & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(BAND_GPIO_PORT, gpio_pins[i], state);
    }
}
