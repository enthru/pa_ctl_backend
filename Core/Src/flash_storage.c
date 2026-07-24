#include "flash_storage.h"
#include "variables.h"
#include "default_values.h"
#include "stm32f4xx.h"
#include "main.h"          // PTT_OUT / PTT_RELAY pin macros
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
#define ACS_ZERO	   			 (FLASH_SECTOR_ADDR + 108)
#define ACS_SENS				 (FLASH_SECTOR_ADDR + 112)
#define FLASH_SIGNATURE          0x55AA1234

// CRC32 over the whole config image (offsets 0..CONFIG_CRC_LEN, which includes the
// signature). Stored just past the last value (ACS_SENS ends at offset 115).
#define CONFIG_CRC_LEN           116
#define CONFIG_CRC_ADDR          (FLASH_SECTOR_ADDR + CONFIG_CRC_LEN)

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// IWDG reload values (prescaler 128, ~4 ms/tick — must match MX_IWDG_Init in main.c):
//   250  -> ~1.0 s  (normal operation)
//   4095 -> ~16 s   (spans a 2-4 s sector erase that freezes the CPU)
#define IWDG_RELOAD_NORMAL   250
#define IWDG_RELOAD_FLASH    0x0FFF

// Bitwise CRC32 (poly 0xEDB88320) over a flash byte range — used to detect config
// corruption / bit-rot. Small and only run at boot / on save, so no table needed.
static uint32_t crc32_region(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
        }
    }
    return ~crc;
}

// Reprogram the IWDG down-counter reload and apply it immediately. Harmless if the
// IWDG has not been started yet (e.g. the first-boot save from Flash_LoadAll).
static void iwdg_set_reload(uint16_t rlr) {
    IWDG->KR  = 0x5555;   // enable write access to RLR
    IWDG->RLR = rlr;
    IWDG->KR  = 0xAAAA;   // reload counter with the new value now
}

#if FLASH_SECTOR_ADDR < 0x08000000 || FLASH_SECTOR_ADDR >= 0x08100000
#error "Invalid flash address for STM32F405RGTx"
#endif

// ==== Универсальные функции чтения/записи ====
__attribute__((section(".RamFunc")))
void Flash_WriteU32(uint32_t address, uint32_t value) {
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value) != HAL_OK) {
	    //Error_Handler();
	}
}

__attribute__((section(".RamFunc")))
void Flash_WriteBool(uint32_t address, bool value) {
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value ? 1 : 0) != HAL_OK) {
        //Error_Handler();
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
        //Error_Handler();
    }
}


__attribute__((section(".RamFunc")))
uint8_t Flash_ReadU8(uint32_t address, uint8_t default_val) {
    uint32_t raw = *(__IO uint32_t*)address;
    if (raw == 0xFFFFFFFF || raw > 255) return default_val;
    return (uint8_t)raw;
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
    max_swr             = DEFAULT_MAX_SWR;
    max_current         = DEFAULT_MAX_CURRENT;
    max_voltage         = DEFAULT_MAX_VOLTAGE;
    max_water_temp      = DEFAULT_MAX_WATER_TEMP;
    max_plate_temp      = DEFAULT_MAX_PLATE_TEMP;
    max_pump_speed_temp = DEFAULT_MAX_PUMP_SPEED_TEMP;
    min_pump_speed_temp = DEFAULT_MIN_PUMP_SPEED_TEMP;
    max_fan_speed_temp  = DEFAULT_MAX_FAN_SPEED_TEMP;
    min_fan_speed_temp  = DEFAULT_MIN_FAN_SPEED_TEMP;
    max_input_power     = DEFAULT_MAX_INPUT_POWER;
    min_coeff           = DEFAULT_MIN_COEFF;
    autoband            = DEFAULT_AUTOBAND;

    low_fwd_coeff   = DEFAULT_LOW_FWD_COEFF;
    low_rev_coeff   = DEFAULT_LOW_REV_COEFF;
    low_ifwd_coeff  = DEFAULT_LOW_IFWD_COEFF;
    mid_fwd_coeff   = DEFAULT_MID_FWD_COEFF;
    mid_rev_coeff   = DEFAULT_MID_REV_COEFF;
    mid_ifwd_coeff  = DEFAULT_MID_IFWD_COEFF;
    high_fwd_coeff  = DEFAULT_HIGH_FWD_COEFF;
    high_rev_coeff  = DEFAULT_HIGH_REV_COEFF;
    high_ifwd_coeff = DEFAULT_HIGH_IFWD_COEFF;

    voltage_coeff   = DEFAULT_VOLTAGE_COEFF;
    current_coeff   = DEFAULT_CURRENT_COEFF;
    rsrv_coeff      = DEFAULT_RSRV_COEFF;
    acs_zero        = DEFAULT_ACS_ZERO;
    acs_sens        = DEFAULT_ACS_SENS;

    snprintf(default_band, sizeof(default_band), "%s", DEFAULT_BAND_VALUE);
}


bool Flash_IsDataSaved(void) {
    uint32_t signature = *(__IO uint32_t*)FLASH_SIGNATURE_ADDR;
    return (signature == FLASH_SIGNATURE);
}

void Flash_SaveAll(void) {
    // Hard TX interlock: forbid transmission for the whole persistent-memory operation.
    // Erasing sector 11 stalls code fetch on single-bank F4 (~1-4 s) and freezes the
    // protection ISR, so drive PTT/relay off and hold flash_busy (process_data() refuses
    // to re-key while it is set) until the write completes.
    flash_busy = true;
    HAL_GPIO_WritePin(PTT_OUT_GPIO_Port,   PTT_OUT_Pin,   GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PTT_RELAY_GPIO_Port, PTT_RELAY_Pin, GPIO_PIN_RESET);

    // The main loop cannot refresh the watchdog while the erase stalls the CPU, so widen
    // the IWDG window to span the whole operation, then restore the ~1 s window at the end.
    iwdg_set_reload(IWDG_RELOAD_FLASH);

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
    Flash_WriteU32(MIN_COEFF, min_coeff);           // <-- было WriteFloat

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
    Flash_WriteFloat(ACS_ZERO, acs_zero);
    Flash_WriteFloat(ACS_SENS, acs_sens);

    Flash_WriteString4(DEFAULT_BAND_ADDR, default_band);

    Flash_WriteU32(FLASH_SIGNATURE_ADDR, FLASH_SIGNATURE);

    // CRC over everything just written (signature included), stored last.
    uint32_t crc = crc32_region((const uint8_t *)FLASH_SECTOR_ADDR, CONFIG_CRC_LEN);
    Flash_WriteU32(CONFIG_CRC_ADDR, crc);

    HAL_FLASH_Lock();

    iwdg_set_reload(IWDG_RELOAD_NORMAL);
    flash_busy = false;
}

void Flash_LoadAll(void) {
    if (!Flash_IsDataSaved()) {
        Flash_SetDefaultValues();
        Flash_SaveAll();
        return;
    }

    // Integrity check. A stored CRC of 0xFFFFFFFF means an old (pre-CRC) image — trust the
    // signature and keep the values (they get a CRC on the next save). Otherwise a mismatch
    // means corruption/bit-rot: fall back to safe defaults rather than run on bad
    // calibration (which would mis-scale power/SWR).
    uint32_t stored_crc = *(__IO uint32_t *)CONFIG_CRC_ADDR;
    if (stored_crc != 0xFFFFFFFFu) {
        uint32_t calc_crc = crc32_region((const uint8_t *)FLASH_SECTOR_ADDR, CONFIG_CRC_LEN);
        if (calc_crc != stored_crc) {
            Flash_SetDefaultValues();
            Flash_SaveAll();
            return;
        }
    }

    max_swr              = Flash_ReadU8(MAX_SWR_ADDR, DEFAULT_MAX_SWR);
    max_current          = Flash_ReadU8(MAX_CURRENT_ADDR, DEFAULT_MAX_CURRENT);
    max_voltage          = Flash_ReadU8(MAX_VOLTAGE_ADDR, DEFAULT_MAX_VOLTAGE);
    max_water_temp       = Flash_ReadU8(MAX_WATER_TEMP_ADDR, DEFAULT_MAX_WATER_TEMP);
    max_plate_temp       = Flash_ReadU8(MAX_PLATE_TEMP_ADDR, DEFAULT_MAX_PLATE_TEMP);
    max_pump_speed_temp  = Flash_ReadU8(MAX_PUMP_SPEED_TEMP_ADDR, DEFAULT_MAX_PUMP_SPEED_TEMP);
    min_pump_speed_temp  = Flash_ReadU8(MIN_PUMP_SPEED_TEMP_ADDR, DEFAULT_MIN_PUMP_SPEED_TEMP);
    max_fan_speed_temp   = Flash_ReadU8(MAX_FAN_SPEED_TEMP_ADDR, DEFAULT_MAX_FAN_SPEED_TEMP);
    min_fan_speed_temp   = Flash_ReadU8(MIN_FAN_SPEED_TEMP_ADDR, DEFAULT_MIN_FAN_SPEED_TEMP);
    max_input_power      = Flash_ReadU8(MAX_INPUT_POWER_ADDR, DEFAULT_MAX_INPUT_POWER);
    min_coeff            = Flash_ReadU8(MIN_COEFF, DEFAULT_MIN_COEFF);  // <-- было ReadFloat

    autoband             = Flash_ReadBool(AUTOBAND_ADDR, DEFAULT_AUTOBAND);

    low_fwd_coeff        = Flash_ReadFloat(LOW_FWD_COEFF, DEFAULT_LOW_FWD_COEFF);
    low_rev_coeff        = Flash_ReadFloat(LOW_REV_COEFF, DEFAULT_LOW_REV_COEFF);
    low_ifwd_coeff       = Flash_ReadFloat(LOW_IFWD_COEFF, DEFAULT_LOW_IFWD_COEFF);

    mid_fwd_coeff        = Flash_ReadFloat(MID_FWD_COEFF, DEFAULT_MID_FWD_COEFF);
    mid_rev_coeff        = Flash_ReadFloat(MID_REV_COEFF, DEFAULT_MID_REV_COEFF);
    mid_ifwd_coeff       = Flash_ReadFloat(MID_IFWD_COEFF, DEFAULT_MID_IFWD_COEFF);

    high_fwd_coeff       = Flash_ReadFloat(HIGH_FWD_COEFF, DEFAULT_HIGH_FWD_COEFF);
    high_rev_coeff       = Flash_ReadFloat(HIGH_REV_COEFF, DEFAULT_HIGH_REV_COEFF);
    high_ifwd_coeff      = Flash_ReadFloat(HIGH_IFWD_COEFF, DEFAULT_HIGH_IFWD_COEFF);

    voltage_coeff        = Flash_ReadFloat(VOLTAGE_COEFF, DEFAULT_VOLTAGE_COEFF);
    current_coeff        = Flash_ReadFloat(CURRENT_COEFF, DEFAULT_CURRENT_COEFF);
    rsrv_coeff           = Flash_ReadFloat(RSRV_COEFF, DEFAULT_RSRV_COEFF);
    acs_zero             = Flash_ReadFloat(ACS_ZERO, DEFAULT_ACS_ZERO);
    acs_sens             = Flash_ReadFloat(ACS_SENS, DEFAULT_ACS_SENS);

    Flash_ReadString4(DEFAULT_BAND_ADDR, default_band);
}

