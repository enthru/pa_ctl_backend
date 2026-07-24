#ifndef SET_FUNCTIONS_H
#define SET_FUNCTIONS_H

#include "main.h"
#include "variables.h"
#include <stdbool.h>

void set_band_gpio(const char *band);
void set_band_from_frequency(uint32_t frequency);
const char *get_band_from_frequency(uint32_t frequency);
Band_t band_from_string(const char *band);

#endif
