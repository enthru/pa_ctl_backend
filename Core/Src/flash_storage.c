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
#define LOW_FWD_COEFF			 (FLASH_SECTOR_ADDR + 56)
#define LOW_REV_COEFF     		 (FLASH_SECTOR_ADDR + 60)
#define LOW_IFWD_COEFF     		 (FLASH_SECTOR_ADDR + 64)
#define MID_FWD_COEFF			 (FLASH_SECTOR_ADDR + 68)
#define MID_REV_COEFF     	     (FLASH_SECTOR_ADDR + 72)
#define MID_IFWD_COEFF     	     (FLASH_SECTOR_ADDR + 76)
#define HIGH_FWD_COEFF			 (FLASH_SECTOR_ADDR + 80)
#define HIGH_REV_COEFF     	     (FLASH_SECTOR_ADDR + 84)
#define HIGH_IFWD_COEFF     	 (FLASH_SECTOR_ADDR + 88)
#define VOLTAGE_COEFF     		 (FLASH_SECTOR_ADDR + 92)
#define CURRENT_COEFF     		 (FLASH_SECTOR_ADDR + 96)
#define RSRV_COEFF     			 (FLASH_SECTOR_ADDR + 100)
#define MIN_COEFF     			 (FLASH_SECTOR_ADDR + 104)
#define FLASH_SIGNATURE          0x55AA1234

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#if FLASH_SECTOR_ADDR < 0x08000000 || FLASH_SECTOR_ADDR >= 0x08100000
#error "Invalid flash address for STM32F405RGTx"
#endif

// ==== Универсальные функции чтения/записи ====
__attribute__((section(".RamFunc")))
void Flash_WriteU32(uint32_t address, uint32_t value) {
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value) != HAL_OK) {
	    Error_Handler();
	}
}

__attribute__((section(".RamFunc")))
void Flash_WriteBool(uint32_t address, bool value) {
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value ? 1 : 0) != HAL_OK) {
        Error_Handler();
    }
}

__attribute__((section(".RamFunc")))
void Flash_WriteFloat(uint32_t address, float value) {
    union {
        float f;
        uint32_t u;
    } conv;
    conv.f = value;

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, conv.u) != HAL_OK) {
        Error_Handler();
    }
}

__attribute__((section(".RamFunc")))
uint32_t Flash_ReadU32(uint32_t address, uint32_t default_val) {
    uint32_t raw = *(__IO uint32_t*)address;
    return (raw == 0xFFFFFFFF) ? default_val : raw;
}

__attribute__((section(".RamFunc")))
bool Flash_ReadBool(uint32_t address, bool default_val) {
    uint32_t raw = *(__IO uint32_t*)address;
    if (raw == 0xFFFFFFFF) return default_val;
    return (raw != 0);
}

__attribute__((section(".RamFunc")))
float Flash_ReadFloat(uint32_t address, float default_val) {
    uint32_t raw = *(__IO uint32_t*)address;
    if (raw == 0xFFFFFFFF) return default_val;

    union {
        float f;
        uint32_t u;
    } conv;

    conv.u = raw;
    return conv.f;
}

__attribute__((section(".RamFunc")))
void Flash_WriteString4(uint32_t address, const char *str) {
	uint32_t packed = 0;
	memcpy(&packed, str, 4);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, packed);
}

__attribute__((section(".RamFunc")))
void Flash_ReadString4(uint32_t address, char *dest) {
	uint32_t packed = *(__IO uint32_t*)address;
	if (packed == 0xFFFFFFFF) {
		dest[0] = 0;
	}
	else {
		memcpy(dest, &packed, 4); dest[4] = 0;
	} }


__attribute__((section(".RamFunc")))
void Flash_EraseSector(void) {
    //HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_11;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t sector_error;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);

    if (status != HAL_OK) {
    }

    //HAL_FLASH_Lock();
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
    voltage_coeff = 1;
    current_coeff = 1;
    rsrv_coeff = 1;
    min_coeff = DEFAULT_MIN_COEFF;
    const char default_band_str[] = DEFAULT_BAND_VALUE;
    snprintf(default_band, sizeof(default_band), "%s", default_band_str);
}

bool Flash_IsDataSaved(void) {
    uint32_t signature = *(__IO uint32_t*)FLASH_SIGNATURE_ADDR;
    return (signature == FLASH_SIGNATURE);
}

void Flash_SaveAll(void) {
    HAL_FLASH_Unlock();
    Flash_EraseSector();

    Flash_WriteU32(MAX_SWR_ADDR, max_swr);
    Flash_WriteU32(MAX_CURRENT_ADDR, max_current);
    Flash_WriteU32(MAX_VOLTAGE_ADDR, max_voltage);
    Flash_WriteU32(MAX_WATER_TEMP_ADDR, max_water_temp);
    Flash_WriteU32(MAX_PLATE_TEMP_ADDR, max_plate_temp);
    Flash_WriteU32(MAX_PUMP_SPEED_TEMP_ADDR, max_pump_speed_temp);
    Flash_WriteU32(MIN_PUMP_SPEED_TEMP_ADDR, min_pump_speed_temp);
    Flash_WriteU32(MAX_FAN_SPEED_TEMP_ADDR, max_fan_speed_temp);
    Flash_WriteU32(MIN_FAN_SPEED_TEMP_ADDR, min_fan_speed_temp);
    Flash_WriteU32(MAX_INPUT_POWER_ADDR, max_input_power);

    Flash_WriteBool(AUTOBAND_ADDR, autoband);

    Flash_WriteFloat(LOW_FWD_COEFF, low_fwd_coeff);
    Flash_WriteFloat(LOW_REV_COEFF, low_rev_coeff);
    Flash_WriteFloat(LOW_IFWD_COEFF, low_ifwd_coeff);

    Flash_WriteFloat(MID_FWD_COEFF, mid_fwd_coeff);
    Flash_WriteFloat(MID_REV_COEFF, mid_rev_coeff);
    Flash_WriteFloat(MID_IFWD_COEFF, mid_ifwd_coeff);

    Flash_WriteFloat(HIGH_FWD_COEFF, high_fwd_coeff);
    Flash_WriteFloat(HIGH_REV_COEFF, high_rev_coeff);
    Flash_WriteFloat(HIGH_IFWD_COEFF, high_ifwd_coeff);

    Flash_WriteFloat(VOLTAGE_COEFF, voltage_coeff);
    Flash_WriteFloat(CURRENT_COEFF, current_coeff);
    Flash_WriteFloat(RSRV_COEFF, rsrv_coeff);
    Flash_WriteFloat(MIN_COEFF, min_coeff);

    Flash_WriteString4(DEFAULT_BAND_ADDR, default_band);

    Flash_WriteU32(FLASH_SIGNATURE_ADDR, FLASH_SIGNATURE);

    HAL_FLASH_Lock();
}


void Flash_LoadAll(void) {
    if (!Flash_IsDataSaved()) {
        Flash_SetDefaultValues();
        Flash_SaveAll();
        return;
    }

    max_swr              = Flash_ReadU32(MAX_SWR_ADDR, DEFAULT_MAX_SWR);
    max_current          = Flash_ReadU32(MAX_CURRENT_ADDR, DEFAULT_MAX_CURRENT);
    max_voltage          = Flash_ReadU32(MAX_VOLTAGE_ADDR, DEFAULT_MAX_VOLTAGE);
    max_water_temp       = Flash_ReadU32(MAX_WATER_TEMP_ADDR, DEFAULT_MAX_WATER_TEMP);
    max_plate_temp       = Flash_ReadU32(MAX_PLATE_TEMP_ADDR, DEFAULT_MAX_PLATE_TEMP);
    max_pump_speed_temp  = Flash_ReadU32(MAX_PUMP_SPEED_TEMP_ADDR, DEFAULT_MAX_PUMP_SPEED_TEMP);
    min_pump_speed_temp  = Flash_ReadU32(MIN_PUMP_SPEED_TEMP_ADDR, DEFAULT_MIN_PUMP_SPEED_TEMP);
    max_fan_speed_temp   = Flash_ReadU32(MAX_FAN_SPEED_TEMP_ADDR, DEFAULT_MAX_FAN_SPEED_TEMP);
    min_fan_speed_temp   = Flash_ReadU32(MIN_FAN_SPEED_TEMP_ADDR, DEFAULT_MIN_FAN_SPEED_TEMP);
    max_input_power      = Flash_ReadU32(MAX_INPUT_POWER_ADDR, DEFAULT_MAX_INPUT_POWER);

    autoband             = Flash_ReadBool(AUTOBAND_ADDR, DEFAULT_AUTOBAND);

    low_fwd_coeff        = Flash_ReadFloat(LOW_FWD_COEFF, 1.0f);
    low_rev_coeff        = Flash_ReadFloat(LOW_REV_COEFF, 1.0f);
    low_ifwd_coeff       = Flash_ReadFloat(LOW_IFWD_COEFF, 1.0f);

    mid_fwd_coeff        = Flash_ReadFloat(MID_FWD_COEFF, 1.0f);
    mid_rev_coeff        = Flash_ReadFloat(MID_REV_COEFF, 1.0f);
    mid_ifwd_coeff       = Flash_ReadFloat(MID_IFWD_COEFF, 1.0f);

    high_fwd_coeff       = Flash_ReadFloat(HIGH_FWD_COEFF, 1.0f);
    high_rev_coeff       = Flash_ReadFloat(HIGH_REV_COEFF, 1.0f);
    high_ifwd_coeff      = Flash_ReadFloat(HIGH_IFWD_COEFF, 1.0f);

    voltage_coeff        = Flash_ReadFloat(VOLTAGE_COEFF, 1.0f);
    current_coeff        = Flash_ReadFloat(CURRENT_COEFF, 1.0f);
    rsrv_coeff           = Flash_ReadFloat(RSRV_COEFF, 1.0f);
    min_coeff           = Flash_ReadFloat(MIN_COEFF, 1.0f);

    Flash_ReadString4(DEFAULT_BAND_ADDR, default_band);
}


bool Flash_VerifySave(void) {
    return (*(__IO uint32_t*)MAX_SWR_ADDR == max_swr) &&
           (*(__IO uint32_t*)MAX_CURRENT_ADDR == max_current);
}
