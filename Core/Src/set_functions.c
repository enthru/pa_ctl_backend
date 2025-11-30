#include "set_functions.h"
#include "main.h"
#include "variables.h"
#include <string.h>

#define BAND_GPIO_PIN_1 GPIO_PIN_9   // PA9
#define BAND_GPIO_PIN_2 GPIO_PIN_10  // PA10
#define BAND_GPIO_PIN_3 GPIO_PIN_11  // PA11
#define BAND_GPIO_PIN_4 GPIO_PIN_12  // PA12

int MAX_ERROR = 200;

#define BAND_GPIO_PORT GPIOA

void set_band_from_frequency(uint32_t frequency) {
    if (frequency >= 1800 - MAX_ERROR && frequency <= 2000 + MAX_ERROR) {
        strcpy(current_band, "160m");
    }
    else if (frequency >= 3500 - MAX_ERROR && frequency <= 4000 + MAX_ERROR) {
        strcpy(current_band, "80m");
    }
    else if (frequency >= 5300 - MAX_ERROR && frequency <= 5500 + MAX_ERROR) {
        strcpy(current_band, "60m");
    }
    else if (frequency >= 7000 - MAX_ERROR && frequency <= 7300 + MAX_ERROR) {
        strcpy(current_band, "40m");
    }
    else if (frequency >= 10100 - MAX_ERROR && frequency <= 10150 + MAX_ERROR) {
        strcpy(current_band, "30m");
    }
    else if (frequency >= 14000 - MAX_ERROR && frequency <= 14350 + MAX_ERROR) {
        strcpy(current_band, "20m");
    }
    else if (frequency >= 18068 - MAX_ERROR && frequency <= 18168 + MAX_ERROR) {
        strcpy(current_band, "17m");
    }
    else if (frequency >= 21000 - MAX_ERROR && frequency <= 21450 + MAX_ERROR) {
        strcpy(current_band, "15m");
    }
    else if (frequency >= 24890 - MAX_ERROR && frequency <= 24990 + MAX_ERROR) {
        strcpy(current_band, "12m");
    }
    else if (frequency >= 28000 - MAX_ERROR && frequency <= 29700 + MAX_ERROR) {
        strcpy(current_band, "10m");
    }
    else if (frequency >= 50000 - MAX_ERROR && frequency <= 54000 + MAX_ERROR) {
        strcpy(current_band, "6m");
    }
    else {
        strcpy(current_band, "unk");
    }
}

void set_band_gpio(const char *band)
{
    uint8_t bcd_code[4] = {0, 0, 0, 0}; // BCD: [PIN1, PIN2, PIN3, PIN4]

    if (strcmp(band, "160m") == 0) {
        bcd_code[0] = 0; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 0;
    }
    else if (strcmp(band, "80m") == 0) {
        bcd_code[0] = 1; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 0;
    }
    else if (strcmp(band, "60m") == 0) {
        bcd_code[0] = 0; bcd_code[1] = 1; bcd_code[2] = 0; bcd_code[3] = 0;
    }
    else if (strcmp(band, "40m") == 0) {
        bcd_code[0] = 1; bcd_code[1] = 1; bcd_code[2] = 0; bcd_code[3] = 0;
    }
    else if (strcmp(band, "30m") == 0) {
        bcd_code[0] = 0; bcd_code[1] = 0; bcd_code[2] = 1; bcd_code[3] = 0;
    }
    else if (strcmp(band, "20m") == 0) {
        bcd_code[0] = 1; bcd_code[1] = 0; bcd_code[2] = 1; bcd_code[3] = 0;
    }
    else if (strcmp(band, "17m") == 0) {
        bcd_code[0] = 0; bcd_code[1] = 1; bcd_code[2] = 1; bcd_code[3] = 0;
    }
    else if (strcmp(band, "15m") == 0) {
        bcd_code[0] = 1; bcd_code[1] = 1; bcd_code[2] = 1; bcd_code[3] = 0;
    }
    else if (strcmp(band, "12m") == 0) {
        bcd_code[0] = 0; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 1;
    }
    else if (strcmp(band, "10m") == 0) {
        //bcd_code[0] = 1; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 1;
    	bcd_code[0] = 0; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 1;
    }
    else if (strcmp(band, "6m") == 0) {
        //bcd_code[0] = 0; bcd_code[1] = 1; bcd_code[2] = 0; bcd_code[3] = 1;
    	bcd_code[0] = 1; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 1;
    }
    else {
        bcd_code[0] = 0; bcd_code[1] = 0; bcd_code[2] = 0; bcd_code[3] = 0;
    }

    HAL_GPIO_WritePin(BAND_GPIO_PORT, BAND_GPIO_PIN_1, bcd_code[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BAND_GPIO_PORT, BAND_GPIO_PIN_2, bcd_code[1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BAND_GPIO_PORT, BAND_GPIO_PIN_3, bcd_code[2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BAND_GPIO_PORT, BAND_GPIO_PIN_4, bcd_code[3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
