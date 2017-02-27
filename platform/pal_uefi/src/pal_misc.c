/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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
      Print(L"\n  Error-Input address is not aligned. Masking the last 2 bits \n");
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }
  //Print(L"Address = %8x  ", addr);
  data = (*(volatile UINT32 *)addr);

  //Print(L" data = %8x \n", data);

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
  //Print(L"Address = %8x  Data = %8x \n", addr, data);
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
      Print(L"Error in writing to log file\n");
  } else
      AsciiPrint(string, data);
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

  //Print(L"Shared memory is %llx \n", gSharedMemory);

  if (EFI_ERROR(Status)) {
    Print(L"Allocate Pool shared memory failed %x \n", Status);
  }
  pal_pe_data_cache_ops_by_va((UINT64)&gSharedMemory, CLEAN_AND_INVALIDATE);

  return;

}

/**
  @brief  Return the base address of the shared memory region to the VAL layer
**/
UINT64
pal_mem_get_shared_addr()
{

  //Shared memory is always less than 4GB for now
  return (UINT64) (gSharedMemory);
}

/**
  @brief  Free the shared memory region allocated above
**/
VOID
pal_mem_free_shared()
{
  gBS->FreePool ((VOID *)gSharedMemory);
}
