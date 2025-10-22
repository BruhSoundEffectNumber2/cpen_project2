// OS functions pertaining to simple write-once file system
// John Tadrous
// August 9, 2020

#include "OS_File_System.h"

uint32_t Disk_Start_Address = 0x20000; // First address in the ROM

uint8_t RAM_Directory[256]; // Directory loaded in RAM
uint8_t RAM_FAT[256];       // FAT in RAM
uint8_t Access_FB;          // Access Feedback

// OS_FS_Init()  Temporarily initialize RAM_Directory and RAM_FAT
void OS_FS_Init(void)
{
  int i;

  for (i = 0; i < 256; i++)
  {
    RAM_Directory[i] = 255;
    RAM_FAT[i] = 255;
  }
}

//******** OS_File_New*************
// Returns a file number of a new file for writing
// Inputs: none
// Outputs: number of a new file
// Errors: return 255 on failure or disk full
uint8_t OS_File_New(void)
{
  // Is there any disk space free?
  if (find_free_sector() == 255)
  {
    return 255;
  }

  // Find the first file we can use
  for (uint8_t i = 0; i < 255; i++)
  {
    if (RAM_Directory[i] == 255)
    {
      return i;
    }
  }

  // If we got here, we cannot add more files
  return 255;
}

//******** OS_File_Size*************
// Check the size of this file
// Inputs: num, 8-bit file number, 0 to 254
// Outputs: 0 if empty, otherwise the number of sectors
// Errors: none
uint8_t OS_File_Size(uint8_t num)
{
  uint8_t current = RAM_Directory[num];

  // The file could be empty
  if (current == 255)
  {
    return 0;
  }

  uint8_t size = 1;

  while (RAM_FAT[current] != 0xff)
  {
    current = RAM_FAT[current];
    size++;
  }

  return size;
}

//******** OS_File_Append*************
// Save 512 bytes into the file
// Inputs: num, 8-bit file number, 0 to 254
// buf, pointer to 512 bytes of data
// Outputs: 0 if successful
// Errors: 255 on failure or disk full
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512])
{
}

// Helper function find_free_sector returns the logical
// address of the first free sector
uint8_t find_free_sector(void)
{
  uint8_t fs = -1;
  uint8_t ls;

  // Look through all files, finding the last sector used by any of them
  for (uint8_t i = 0;; i++)
  {
    ls = last_sector(RAM_Directory[i]);

    if (ls == 255)
    {
      // We have looked through all files
      return fs + 1;
    }

    if (ls > fs)
    {
      fs = ls;
    }
  }
}

// Helper function last_sector returns the logical address
// of the last sector assigned to the file whose number is 'start'
uint8_t last_sector(uint8_t start)
{
  // Last sector is reserved
  if (start == 0xff)
  {
    return 0xff;
  }

  if (RAM_Directory[start] == 0xff)
  {
    // File does not exist
    return 0xff;
  }

  uint8_t current = RAM_Directory[start];

  while (RAM_FAT[current] != 0xff)
  {
    current = RAM_FAT[current];
  }

  return current;
}

// Helper function append_fat() modifies the FAT to append
// the sector with logical address n to the sectors of file
// num
void append_fat(uint8_t num, uint8_t n)
{
  uint8_t i = RAM_Directory[num];

  // If the file is new, the append is easy
  if (i == 255)
  {
    RAM_Directory[num] = n;
    return;
  }

  // Go through the FAT to find the last used sector
  uint8_t m = RAM_FAT[i];

  while (m != 255)
  {
    i = m;
    m = RAM_FAT[i];
  }

  RAM_FAT[i] = n;
}

// eDisk_WriteSector
// input: pointer to a 512-byte data buffer in RAM buf[512],
//        sector logical address n
// output: 0 if no error, 1 if error
// use the Flash_Write function
uint8_t eDisk_WriteSector(uint8_t buf[512], uint8_t n)
{
  uint32_t phys_address = Disk_Start_Address + n * 512;

  for (uint32_t i = 0; i < 512; i += 4)
  {
    // Push the 4 bytes into 1 word
    uint32_t word = 0;

    for (uint8_t j = 0; j < 4; j++)
    {
      word |= (buf[i + j] << j * 8);
    }

    if (Flash_Write(phys_address + i, word))
    {
      return 1;
    }
  }

  return 0;
}

//******** OS_File_Read*************
// Read 512 bytes from the file
// Inputs: num, 8-bit file number, 0 to 254
//         location, order of the sector in the file, 0 to 254
//         buf, pointer to 512 empty spaces in RAM
// Outputs: 0 if successful
// Errors: 255 on failure because no data
uint8_t OS_File_Read(uint8_t num, uint8_t location, uint8_t buf[512])
{
  // Find the logical sector
  uint8_t current = RAM_Directory[num];

  if (current == 255)
  {
    // File does not exist
    return 255;
  }

  for (uint8_t i = 0; i < location; i++)
  {
    current = RAM_FAT[current];

    if (current == 255)
    {
      // File has less sectors than expected
      return 255;
    }
  }

  uint32_t *phys_address = (uint32_t *)(Disk_Start_Address + current * 512);

  for (uint32_t i = 0; i < 512; i += 4)
  {
    uint32_t word = phys_address[i];

    // Split the word into 4 bytes
    for (uint8_t j = 0; j < 4; j++)
    {
      buf[i + j] = word >> j * 8;
    }
  }

  return 0;
}

//******** OS_File_Format*************
// Erase all files and all data
// Inputs: none
// Outputs: 0 if success
// Errors: 255 on disk write failure
uint8_t OS_File_Format(void)
{
  uint32_t address = 0x00020000; // start of disk

  while (address <= 0x00040000)
  {
    Flash_Erase(address); // erase 1k block
    address += 1024;
  }
}

//******** OS_File_Flush*************
// Update working buffers onto the disk
// Power can be removed after calling flush
// Inputs: none
// Outputs: 0 if success
// Errors: 255 on disk write failure
uint8_t OS_File_Flush(void)
{
  uint32_t phys_address = Disk_Start_Address + 255 * 512;

  for (uint32_t i = 0; i < 512; i += 4)
  {
    // Push the 4 bytes into 1 word
    uint32_t word = 0;

    for (uint8_t j = 0; j < 4; j++)
    {
      // Get from directory or FAT
      if (i < 256)
      {
        word |= (RAM_Directory[i + j] << j * 8);
      }
      else
      {
        word |= (RAM_FAT[i + j] << j * 8);
      }
    }

    if (Flash_Write(phys_address + i, word))
    {
      return 1;
    }
  }
}
