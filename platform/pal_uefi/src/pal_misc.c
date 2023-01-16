/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

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
#include  <Library/BaseMemoryLib.h>
#include <Protocol/Cpu.h>


#include "include/pal_uefi.h"

UINT8   *gSharedMemory;

/**
 @brief This API provides a single point of abstraction to write 8-bit
        data to all memory-mapped I/O addresses.

 @param addr 64-bit address
 @param data 8-bit data write to address

 @return None
**/
VOID
pal_mmio_write8(UINT64 addr, UINT8 data)
{

  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_write8 Address = %llx  Data = %lx \n", addr, data);

  *(volatile UINT8 *)addr = data;
}

/**
  @brief This API provides a single point of abstraction to write 16-bit
         data to all memory-mapped I/O addresses.

  @param addr 64-bit address
  @param data 16-bit data write to address

  @return None
**/
VOID
pal_mmio_write16(UINT64 addr, UINT16 data)
{

  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_write16 Address = %llx  Data = %lx \n", addr, data);

  *(volatile UINT16 *)addr = data;
}

/**
   @brief This API provides a single point of abstraction to write 64-bit
          data to all memory-mapped I/O addresses.

   @param addr 64-bit address
   @param data 64-bit data write to address

   @return None
**/
VOID
pal_mmio_write64(UINT64 addr, UINT64 data)
{

  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_write64 Address = %llx  Data = %lx \n", addr, data);

  *(volatile UINT64 *)addr = data;
}

/**
  @brief This API provides a single point of abstraction to read 8-bit data
         from all memory-mapped I/O addresses.

  @param addr 64-bit input address

  @return 8-bit data read from the input address
**/
UINT8
pal_mmio_read8(UINT64 addr)
{
  UINT8 data;

  data = (*(volatile UINT8 *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_read8 Address = %llx  Data = %lx \n", addr, data);

  return data;
}

/**
  @brief This API provides a single point of abstraction to read 16-bit data
         from all memory-mapped I/O addresses.

  @param addr 64-bit input address

  @return 16-bit data read from the input address
**/
UINT16
pal_mmio_read16(UINT64 addr)
{
  UINT16 data;

  data = (*(volatile UINT16 *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_read16 Address = %llx  Data = %lx \n", addr, data);

  return data;
}

/**
  @brief This API provides a single point of abstraction to read 64-bit data
         from all memory-mapped I/O addresses.

  @param addr 64-bit input address

  @return 64-bit data read from the input address
**/
UINT64
pal_mmio_read64(UINT64 addr)
{
  UINT64 data;

  data = (*(volatile UINT64 *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_read64 Address = %llx  Data = %lx \n", addr, data);

  return data;
}

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

  if (g_print_mmio || (g_curr_module & g_enable_module))
      sbsa_print(AVS_PRINT_INFO, L" pal_mmio_read Address = %llx  Data = %x \n", addr, data);

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
  if (g_print_mmio || (g_curr_module & g_enable_module))
    sbsa_print(AVS_PRINT_INFO, L" pal_mmio_write Address = %llx  Data = %x \n", addr, data);

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
      sbsa_print(AVS_PRINT_ERR, L" Error in writing to log file\n");
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
  @brief  Compare the contents of the src and dest buffers
  @param  Src   - source buffer to be compared
  @param  Dest  - destination buffer to be compared
  @param  Len   - Length of the comparison to be performed

  @return Zero if the buffer contecnts are same, else Nonzero
**/
UINT32
pal_mem_compare (
  VOID *Src,
  VOID *Dest,
  UINT32 Len
  )
{
  return CompareMem (Src, Dest, Len);
}

/**
  @brie a buffer with a known specified input value
  @param  Buf   - Pointer to the buffer to fill
  @param  Size  - Number of bytes in buffer to fill
  @param  Value - Value to fill buffer with

  @return None
**/
VOID
pal_mem_set (
  VOID *Buf,
  UINT32 Size,
  UINT8 Value
  )
{
  SetMem(Buf, Size, Value);
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

  sbsa_print(AVS_PRINT_INFO, L" Shared memory is %llx \n", gSharedMemory);

  if (EFI_ERROR(Status)) {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pool shared memory failed %x \n", Status);
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

/**
 * @brief  Allocates requested buffer size in bytes in a contiguous memory
 *         and returns the base address of the range.
 *
 * @param  Size         allocation size in bytes
 * @retval if SUCCESS   pointer to allocated memory
 * @retval if FAILURE   NULL
 */
VOID *
pal_mem_alloc (
  UINT32 Size
  )
{

  EFI_STATUS            Status;
  VOID                  *Buffer;

  Buffer = NULL;
  Status = gBS->AllocatePool (EfiBootServicesData,
                              Size,
                              (VOID **) &Buffer);
  if (EFI_ERROR(Status))
  {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pool failed %x \n", Status);
    return NULL;
  }

  return Buffer;

}

/**
 * @brief  Allocates requested buffer size in bytes with zeros in a contiguous memory
 *         and returns the base address of the range.
 *
 * @param  Size         allocation size in bytes
 * @retval if SUCCESS   pointer to allocated memory
 * @retval if FAILURE   NULL
 */
VOID *
pal_mem_calloc (
  UINT32 num,
  UINT32 Size
  )
{
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Buffer = NULL;
  Status = gBS->AllocatePool (EfiBootServicesData,
                              num * Size,
                              (VOID **) &Buffer);
  if (EFI_ERROR(Status))
  {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pool failed %x \n", Status);
    return NULL;
  }

  gBS->SetMem (Buffer, num * Size, 0);

  return Buffer;
}

/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   mem_size    - Size of the memory range of interest
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
VOID *
pal_mem_alloc_at_address (
  UINT64 mem_base,
  UINT64 Size
  )
{
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS PageBase;

  PageBase = mem_base;
  Status = gBS->AllocatePages (AllocateAddress,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES(Size),
                               &PageBase);
  if (EFI_ERROR(Status))
  {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pages failed %x \n", Status);
    return NULL;
  }

  return (VOID*)(UINTN)PageBase;
}

/**
  @brief Free number of pages in the memory as requested.

  @param PageBase Address from where we need to free
  @param NumPages Number of memory pages needed

  @return None
**/
VOID
pal_mem_free_at_address(
  UINT64 mem_base,
  UINT64 Size
  )
{
  gBS->FreePages(mem_base, EFI_SIZE_TO_PAGES(Size));
}
/**
 * @brief  Allocates requested buffer size in bytes in a contiguous cacheable
 *         memory and returns the base address of the range.
 *
 * @param  Size         allocation size in bytes
 * @param  Pa           Pointer to Physical Addr
 * @retval if SUCCESS   Pointer to Virtual Addr
 * @retval if FAILURE   NULL
 */
VOID *
pal_mem_alloc_cacheable (
  UINT32 Bdf,
  UINT32 Size,
  VOID **Pa
  )
{
  EFI_PHYSICAL_ADDRESS      Address;
  EFI_CPU_ARCH_PROTOCOL     *Cpu;
  EFI_STATUS                Status;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES(Size),
                               &Address);
  if (EFI_ERROR(Status)) {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pool failed %x \n", Status);
    return NULL;
  }

  /* Check Whether Cpu architectural protocol is installed */
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR(Status)) {
    sbsa_print(AVS_PRINT_ERR, L" Could not get Cpu Arch Protocol %x \n", Status);
    return NULL;
  }

  /* Set Memory Attributes */
  Status = Cpu->SetMemoryAttributes (Cpu,
                                     Address,
                                     Size,
                                     EFI_MEMORY_WB);
  if (EFI_ERROR (Status)) {
    sbsa_print(AVS_PRINT_ERR, L" Could not Set Memory Attribute %x \n", Status);
    return NULL;
  }

  *Pa = (VOID *)Address;
  return (VOID *)Address;
}

/**
 * @brief  Free the cacheable memory region allocated above
 *
 * @param  Size         allocation size in bytes
 * @param  Va           Pointer to Virtual Addr
 * @param  Pa           Pointer to Physical Addr
 * @retval None
 */
VOID
pal_mem_free_cacheable (
  UINT32 Bdf,
  UINT32 Size,
  VOID *Va,
  VOID *Pa
  )
{
  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)Va, EFI_SIZE_TO_PAGES(Size));
}

/* Place holder function. Need to be
 * implemented if needed in later releases
 */
VOID *
pal_mem_virt_to_phys (
  VOID *Va
  )
{
  return Va;
}

VOID *
pal_mem_phys_to_virt (
  UINT64 Pa
  )
{
  return (VOID*)Pa;
}

/**
  @brief  Compares two strings

  @param  FirstString   The pointer to a Null-terminated ASCII string.
  @param  SecondString  The pointer to a Null-terminated ASCII string.
  @param  Length        The maximum number of ASCII characters for compare.

  @return Zero if strings are identical, else non-zero value
**/
UINT32
pal_strncmp (
  CHAR8 *FirstString,
  CHAR8 *SecondString,
  UINT32 Length
  )
{
  return AsciiStrnCmp(FirstString, SecondString, Length);
}

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
VOID *
pal_memcpy (
  VOID *DestinationBuffer,
  VOID *SourceBuffer,
  UINT32 Length
  )
{
  return CopyMem (DestinationBuffer, SourceBuffer, Length);
}

/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return 0 - Success

**/
UINT64
pal_time_delay_ms (
  UINT64 MicroSeconds
  )
{
  return gBS->Stall(MicroSeconds);
}

/**
 @brief Returns the memory page size (in bytes) used by the platform.

 @param None

 @return Size of memory page
**/
UINT32
pal_mem_page_size()
{
    return EFI_PAGE_SIZE;
}

/**
  @brief Allocates the requested number of memory pages

  @param NumPages Number of memory pages needed

  @return Address of the allocated space
**/
VOID *
pal_mem_alloc_pages (
  UINT32 NumPages
  )
{
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS PageBase;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               NumPages,
                               &PageBase);
  if (EFI_ERROR(Status))
  {
    sbsa_print(AVS_PRINT_ERR, L" Allocate Pages failed %x \n", Status);
    return NULL;
  }

  return (VOID*)(UINTN)PageBase;
}

/**
  @brief  Allocates memory with the given alignement.

  @param  Alignment   Specifies the alignment.
  @param  Size        Requested memory allocation size.

  @return Pointer to the allocated memory with requested alignment.
**/
VOID *
pal_aligned_alloc( UINT32 alignment, UINT32 size)
{
  VOID *Mem = NULL;
  VOID *Aligned_Ptr = NULL;

  /* Generate mask for the Alignment parameter*/
  UINT64 Mask = ~(UINT64)(alignment - 1);

  /* Allocate memory with extra bytes, so we can return an aligned address*/
  Mem = (VOID *)pal_mem_alloc(size + alignment);

  if( Mem == NULL)
    return 0;

  /* Add the alignment to allocated memory address and align it to target alignment*/
  Aligned_Ptr = (VOID *)(((UINT64) Mem + alignment-1) & Mask);

  return Aligned_Ptr;
}

/**
  @brief Free number of pages in the memory as requested.

  @param PageBase Address from where we need to free
  @param NumPages Number of memory pages needed

  @return None
**/
VOID
pal_mem_free_pages(
  VOID *PageBase,
  UINT32 NumPages
  )
{
  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)PageBase, NumPages);
}

