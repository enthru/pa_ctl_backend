#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void Flash_Init(void);
void Flash_SaveAll(void);
void Flash_LoadAll(void);
void Flash_ErasePage(void);
bool Flash_VerifySave(void);
bool Flash_IsDataSaved(void);

#endif
