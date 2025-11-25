#include "flash_storage.h"
#include "variables.h"
#include "default_values.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

// STM32F405RGTx
// Sector 11: 0x080E0000 - 0x080FFFFF (128 KB)
//#define FLASH_SECTOR_ADDR        ((uint32_t)0x080E0000)
#define FLASH_SECTOR_ADDR         0x080E0000
#define MAX_SWR_ADDR             (FLASH_SECTOR_ADDR + 0)
#define MAX_CURRENT_ADDR         (FLASH_SECTOR_ADDR + 4)
#define MAX_VOLTAGE_ADDR         (FLASH_SECTOR_ADDR + 8)
#define MAX_WATER_TEMP_ADDR      (FLASH_SECTOR_ADDR + 12)
#define MAX_PLATE_TEMP_ADDR      (FLASH_SECTOR_ADDR + 16)
#define MAX_PUMP_SPEED_TEMP_ADDR (FLASH_SECTOR_ADDR + 20)
#define MIN_PUMP_SPEED_TEMP_ADDR (FLASH_SECTOR_ADDR + 24)
#define MAX_FAN_SPEED_TEMP_ADDR  (FLASH_SECTOR_ADDR + 28)
#define MIN_FAN_SPEED_TEMP_ADDR  (FLASH_SECTOR_ADDR + 32)
#define AUTOBAND_ADDR            (FLASH_SECTOR_ADDR + 36)
#define DEFAULT_BAND_ADDR        (FLASH_SECTOR_ADDR + 40)
#define FLASH_SIGNATURE_ADDR     (FLASH_SECTOR_ADDR + 48)
#define MAX_INPUT_POWER_ADDR     (FLASH_SECTOR_ADDR + 52)
#define FLASH_SIGNATURE          0x55AA1234

#if FLASH_SECTOR_ADDR < 0x08000000 || FLASH_SECTOR_ADDR >= 0x08100000
#error "Invalid flash address for STM32F405RGTx"
#endif

void Flash_EraseSector(void) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_11;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t sector_error;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);

    if (status != HAL_OK) {
    }

    HAL_FLASH_Lock();
}

static void Flash_SetDefaultValues(void) {
    max_swr = DEFAULT_MAX_SWR;
    max_current = DEFAULT_MAX_CURRENT;
    max_voltage = DEFAULT_MAX_VOLTAGE;
    max_water_temp = DEFAULT_MAX_WATER_TEMP;
    max_plate_temp = DEFAULT_MAX_PLATE_TEMP;
    max_pump_speed_temp = DEFAULT_MAX_PUMP_SPEED_TEMP;
    min_pump_speed_temp = DEFAULT_MIN_PUMP_SPEED_TEMP;
    max_fan_speed_temp = DEFAULT_MAX_FAN_SPEED_TEMP;
    min_fan_speed_temp = DEFAULT_MIN_FAN_SPEED_TEMP;
    max_input_power = DEFAULT_MAX_INPUT_POWER;
    autoband = DEFAULT_AUTOBAND;
    const char default_band_str[] = DEFAULT_BAND_VALUE;
    snprintf(default_band, sizeof(default_band), "%s", default_band_str);
}

bool Flash_IsDataSaved(void) {
    uint32_t signature = *(__IO uint32_t*)FLASH_SIGNATURE_ADDR;
    return (signature == FLASH_SIGNATURE);
}

void Flash_SaveAll(void) {
    Flash_EraseSector();
    HAL_FLASH_Unlock();

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_SWR_ADDR, max_swr);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_CURRENT_ADDR, max_current);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_VOLTAGE_ADDR, max_voltage);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_WATER_TEMP_ADDR, max_water_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_PLATE_TEMP_ADDR, max_plate_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_PUMP_SPEED_TEMP_ADDR, max_pump_speed_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MIN_PUMP_SPEED_TEMP_ADDR, min_pump_speed_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_FAN_SPEED_TEMP_ADDR, max_fan_speed_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MIN_FAN_SPEED_TEMP_ADDR, min_fan_speed_temp);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAX_INPUT_POWER_ADDR, max_input_power);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, AUTOBAND_ADDR, (uint32_t)autoband);


    uint32_t band_packed = 0;
    memcpy(&band_packed, default_band, sizeof(uint32_t));
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, DEFAULT_BAND_ADDR, band_packed);

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SIGNATURE_ADDR, FLASH_SIGNATURE);

    HAL_FLASH_Lock();
}

void Flash_LoadAll(void) {
    if (!Flash_IsDataSaved()) {
        Flash_SetDefaultValues();
        Flash_SaveAll();
        return;
    }

    uint32_t temp_val;

    temp_val = *(__IO uint32_t*)MAX_SWR_ADDR;
    if (temp_val != 0xFFFFFFFF) max_swr = temp_val;

    temp_val = *(__IO uint32_t*)MAX_CURRENT_ADDR;
    if (temp_val != 0xFFFFFFFF) max_current = temp_val;

    temp_val = *(__IO uint32_t*)MAX_VOLTAGE_ADDR;
    if (temp_val != 0xFFFFFFFF) max_voltage = temp_val;

    temp_val = *(__IO uint32_t*)MAX_WATER_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) max_water_temp = temp_val;

    temp_val = *(__IO uint32_t*)MAX_PLATE_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) max_plate_temp = temp_val;

    temp_val = *(__IO uint32_t*)MAX_PUMP_SPEED_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) max_pump_speed_temp = temp_val;

    temp_val = *(__IO uint32_t*)MIN_PUMP_SPEED_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) min_pump_speed_temp = temp_val;

    temp_val = *(__IO uint32_t*)MAX_FAN_SPEED_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) max_fan_speed_temp = temp_val;

    temp_val = *(__IO uint32_t*)MIN_FAN_SPEED_TEMP_ADDR;
    if (temp_val != 0xFFFFFFFF) min_fan_speed_temp = temp_val;

    temp_val = *(__IO uint32_t*)MAX_INPUT_POWER_ADDR;
    if (temp_val != 0xFFFFFFFF) max_input_power = temp_val;

    temp_val = *(__IO uint32_t*)AUTOBAND_ADDR;
    if (temp_val != 0xFFFFFFFF) autoband = (bool)temp_val;

    temp_val = *(__IO uint32_t*)DEFAULT_BAND_ADDR;
    if (temp_val != 0xFFFFFFFF) {
        memcpy(default_band, &temp_val, sizeof(uint32_t));
    }
}

bool Flash_VerifySave(void) {
    return (*(__IO uint32_t*)MAX_SWR_ADDR == max_swr) &&
           (*(__IO uint32_t*)MAX_CURRENT_ADDR == max_current);
}
