/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/
#include  <Library/ShellCEntryLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellLib.h>
#include  <Library/PrintLib.h>

#include "include/pal_uefi.h"

UINT8   *gSharedMemory;

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 32-bit data read from the input address
**/
UINT32
pal_mmio_read(UINT64 addr)
{
  UINT32 data;

  if (addr & 0x3) {
      sbsa_print(AVS_PRINT_WARN, L"\n  Error-Input address is not aligned. Masking the last 2 bits \n");
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }
  data = (*(volatile UINT32 *)addr);

  sbsa_print(AVS_PRINT_INFO, L" pal_mmio_read Address = %8x  Data = %x \n", addr, data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  32-bit data to write to address

  @return None
**/
VOID
pal_mmio_write(UINT64 addr, UINT32 data)
{
  sbsa_print(AVS_PRINT_INFO, L" pal_mmio_write Address = %8x  Data = %x \n", addr, data);
  *(volatile UINT32 *)addr = data;

}

/**
  @brief  Sends a formatted string to the output console

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
VOID
pal_print(CHAR8 *string, UINT64 data)
{
  if(g_sbsa_log_file_handle)
  {
    CHAR8 Buffer[1024];
    UINTN BufferSize = 1;
    EFI_STATUS Status = 0;
    BufferSize = AsciiSPrint(Buffer, 1024, string, data);
    AsciiPrint(Buffer);
    Status = ShellWriteFile(g_sbsa_log_file_handle, &BufferSize, (VOID*)Buffer);
    if(EFI_ERROR(Status))
      sbsa_print(AVS_PRINT_ERR, L"Error in writing to log file\n");
  } else
      AsciiPrint(string, data);
}

/**
  @brief  Sends a string to the output console without using UEFI print function
          This function will get COMM port address and directly writes to the addr char-by-char

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
VOID
pal_print_raw(UINT64 addr, CHAR8 *string, UINT64 data)
{
    UINT8 j, buffer[16];
    INT8  i=0;
    for(;*string!='\0';++string){
        if(*string == '%'){
            ++string;
            if(*string == 'd'){
                while(data != 0){
                    j = data%10;
                    data = data/10;
                    buffer[i]= j + 48 ;
                    i = i+1;
                }
            } else if(*string == 'x' || *string == 'X'){
                while(data != 0){
                    j = data & 0xf;
                    data = data >> 4;
                    buffer[i]= j + ((j > 9) ? 55 : 48) ;
                    i = i+1;
                }
            }
            if(i>0) {
                while(i>=0)
                    *(volatile UINT8 *)addr = buffer[--i];
            } else
                *(volatile UINT8 *)addr = 48;

        } else
            *(volatile UINT8 *)addr = *string;
    }
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
VOID
pal_mem_free(VOID *Buffer)
{

  gBS->FreePool (Buffer);
}

/**
  @brief  Allocate memory which is to be used to share data across PEs

  @param  num_pe      - Number of PEs in the system
  @param  sizeofentry - Size of memory region allocated to each PE

  @return None
**/
VOID
pal_mem_allocate_shared(UINT32 num_pe, UINT32 sizeofentry)
{
  EFI_STATUS Status;
  gSharedMemory = 0;

  Status = gBS->AllocatePool ( EfiBootServicesData,
                               (num_pe * sizeofentry),
                               (VOID **) &gSharedMemory );

  sbsa_print(AVS_PRINT_INFO, L"Shared memory is %llx \n", gSharedMemory);

  if (EFI_ERROR(Status)) {
    sbsa_print(AVS_PRINT_ERR, L"Allocate Pool shared memory failed %x \n", Status);
  }
  pal_pe_data_cache_ops_by_va((UINT64)&gSharedMemory, CLEAN_AND_INVALIDATE);

  return;

}

/**
  @brief  Return the base address of the shared memory region to the VAL layer

  @param  None

  @return  shared memory region address
**/
UINT64
pal_mem_get_shared_addr()
{

  //Shared memory is always less than 4GB for now
  return (UINT64) (gSharedMemory);
}

/**
  @brief  Free the shared memory region allocated above

  @param  None

  @return  None
**/
VOID
pal_mem_free_shared()
{
  gBS->FreePool ((VOID *)gSharedMemory);
}
