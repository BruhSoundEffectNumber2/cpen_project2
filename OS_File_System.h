#ifndef OS_FILE_SYSTEM_H
#define OS_FILE_SYSTEM_H

#include <stdint.h>
#include "tm4c123gh6pm_def.h"
#include "FlashProgram.h"

void OS_FS_Init(void);
uint8_t OS_File_New(void);
uint8_t OS_File_Size(uint8_t);
uint8_t find_free_sector(void);
uint8_t last_sector(uint8_t);
void append_fat(uint8_t, uint8_t);
uint8_t OS_File_Read(uint8_t, uint8_t, uint8_t *);
uint8_t eDisk_WriteSector(uint8_t *, uint8_t);
uint8_t OS_File_Flush(void);
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512]);
int Flash_Erase(uint32_t);
uint8_t OS_File_Format(void);

#endif // OS_FILE_SYSTEM_H