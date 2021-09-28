/** @file
 * Copyright (c) 2020-2021, Arm Limited or its affiliates. All rights reserved.
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


#include "include/pal_pcie_enum.h"
#include "include/pal_common_support.h"

#ifdef ENABLE_OOB
/* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

#include  <Library/ShellCEntryLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellLib.h>
#include  <Library/PrintLib.h>
#include  <Library/BaseMemoryLib.h>
#include <Protocol/Cpu.h>

#endif

extern VOID* g_sbsa_log_file_handle;

uint8_t   *gSharedMemory;
/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 8-bit data read from the input address
**/
uint8_t
pal_mmio_read8(uint64_t addr)
{
  uint8_t data;

  data = (*(volatile uint8_t *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_read8 Address = %llx  Data = %lx \n", addr, data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 16-bit data read from the input address
**/
uint16_t
pal_mmio_read16(uint64_t addr)
{
  uint16_t data;

  data = (*(volatile uint16_t *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_read16 Address = %llx  Data = %lx \n", addr, data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 64-bit data read from the input address
**/
uint64_t
pal_mmio_read64(uint64_t addr)
{
  uint64_t data;

  data = (*(volatile uint64_t *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_read64 Address = %llx  Data = %llx \n", addr, data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 32-bit data read from the input address
**/
uint32_t
pal_mmio_read(uint64_t addr)
{

  uint32_t data;

  if (addr & 0x3) {
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }

  data = (*(volatile uint32_t *)addr);
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_read Address = %8x  Data = %x \n", addr, data);

  return data;

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  8-bit data to write to address

  @return None
**/
void
pal_mmio_write8(uint64_t addr, uint8_t data)
{
  *(volatile uint8_t *)addr = data;
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write8 Address = %llx  Data = %lx \n", addr, data);

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  16-bit data to write to address

  @return None
**/
void
pal_mmio_write16(uint64_t addr, uint16_t data)
{
  *(volatile uint16_t *)addr = data;
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write16 Address = %llx  Data = %lx \n", addr, data);

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  64-bit data to write to address

  @return None
**/
void
pal_mmio_write64(uint64_t addr, uint64_t data)
{
  *(volatile uint64_t *)addr = data;
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write64 Address = %llx  Data = %llx \n", addr, data);

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  32-bit data to write to address

  @return None
**/
void
pal_mmio_write(uint64_t addr, uint32_t data)
{

  if (addr & 0x3) {
      print(AVS_PRINT_WARN, "\n  Error-Input address is not aligned. Masking the last 2 bits \n");
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }

    *(volatile uint32_t *)addr = data;
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write Address = %8x  Data = %x \n", addr, data);

}

/**
  @brief  Sends a formatted string to the output console

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print(char *string, uint64_t data)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
 */

 if(g_sbsa_log_file_handle)
  {
    CHAR8 Buffer[1024];
    UINTN BufferSize = 1;
    EFI_STATUS Status = 0;
    BufferSize = AsciiSPrint(Buffer, 1024, string, data);
    AsciiPrint(Buffer);
    Status = ShellWriteFile(g_sbsa_log_file_handle, &BufferSize, (VOID*)Buffer);
    if(EFI_ERROR(Status))
      print(AVS_PRINT_ERR, "Error in writing to log file\n", 0);
  } else
      AsciiPrint(string, data);
#endif

}

/**
  @brief  Sends a string to the output console without using Baremetal print function
          This function will get COMM port address and directly writes to the addr char-by-char

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print_raw(uint64_t addr, char *string, uint64_t data)
{
    uint8_t j, buffer[16];
    uint8_t  i=0;
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
                    *(volatile uint8_t *)addr = buffer[--i];
            } else
                *(volatile uint8_t *)addr = 48;

        } else
            *(volatile uint8_t *)addr = *string;
    }
}

/**
  @brief  Compares two strings

  @param  FirstString   The pointer to a Null-terminated ASCII string.
  @param  SecondString  The pointer to a Null-terminated ASCII string.
  @param  Length        The maximum number of ASCII characters for compare.

  @return Zero if strings are identical, else non-zero value
**/
uint32_t
pal_strncmp(char *FirstString, char *SecondString, uint32_t Length)
{

  return strncmp(FirstString, SecondString, Length);
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
void
pal_mem_free(void *Buffer)
{
  free(Buffer);
}

/**
  @brief  Compare the contents of the src and dest buffers
  @param  Src   - source buffer to be compared
  @param  Dest  - destination buffer to be compared
  @param  Len   - Length of the comparison to be performed

  @return Zero if the buffer contecnts are same, else Nonzero
**/
uint32_t
pal_mem_compare(void *Src, void *Dest, uint32_t Len)
{

  return memcmp(Src, Dest, Len);
}

/**
  @brief a buffer with a known specified input value
  @param  Buf   - Pointer to the buffer to fill
  @param  Size  - Number of bytes in buffer to fill
  @param  Value - Value to fill buffer with

  @return None
**/
void
pal_mem_set(void *Buf, uint32_t Size, uint8_t Value)
{
  memset(Buf, Value, Size);
}

uint64_t
pal_mem_get_shared_addr()
{
  return (uint64_t)(gSharedMemory);
}

/**
  @brief  Free the shared memory region allocated above

  @param  None

  @return  None
**/
void
pal_mem_free_shared()
{
  free ((void *)gSharedMemory);
}

/**
  @brief  Allocates requested buffer size in bytes in a contiguous memory
          and returns the base address of the range.

  @param  Size         allocation size in bytes
  @retval if SUCCESS   pointer to allocated memory
  @retval if FAILURE   NULL
**/
void *
pal_mem_alloc(uint32_t Size)
{

  return malloc(Size);
}

/**
  @brief  Allocate memory which is to be used to share data across PEs

  @param  num_pe      - Number of PEs in the system
  @param  sizeofentry - Size of memory region allocated to each PE

  @return None
**/
void
pal_mem_allocate_shared(uint32_t num_pe, uint32_t sizeofentry)
{
   gSharedMemory = 0;
   gSharedMemory = pal_mem_alloc(num_pe * sizeofentry);
   pal_pe_data_cache_ops_by_va((uint64_t)&gSharedMemory, CLEAN_AND_INVALIDATE);
}

/**
  @brief  Allocates memory of the requested size.

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be allocated
  @param  Pa:   physical address of the allocated memory
**/
void *
pal_mem_alloc_cacheable(uint32_t Bdf, uint32_t Size, void **Pa)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  EFI_PHYSICAL_ADDRESS      Address;
  EFI_CPU_ARCH_PROTOCOL     *Cpu;
  EFI_STATUS                Status;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES(Size),
                               &Address);
  if (EFI_ERROR(Status)) {
    print(AVS_PRINT_ERR, "Allocate Pool failed %x \n", Status);
    return NULL;
  }

  /* Check Whether Cpu architectural protocol is installed */
  Status = gBS->LocateProtocol ( &gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR(Status)) {
    print(AVS_PRINT_ERR, "Could not get Cpu Arch Protocol %x \n", Status);
    return NULL;
  }

  /* Set Memory Attributes */
  Status = Cpu->SetMemoryAttributes (Cpu,
                                     Address,
                                     Size,
                                     EFI_MEMORY_WB);
  if (EFI_ERROR (Status)) {
    print(AVS_PRINT_ERR, "Could not Set Memory Attribute %x \n", Status);
    return NULL;
  }

  *Pa = (VOID *)Address;
  return (VOID *)Address;
#endif

  return 0;
}

/**
  @brief  Frees the memory allocated

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be freed
  @param  Va:   virtual address of the memory to be freed
  @param  Pa:   physical address of the memory to be freed
**/
void
pal_mem_free_cacheable(uint32_t Bdf, uint32_t Size, void *Va, void *Pa)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)Va, EFI_SIZE_TO_PAGES(Size));
#endif

}

/**
  @brief  Returns the physical address of the input virtual address.

  @param Va virtual address of the memory to be converted

  Returns the physical address.
**/
void *
pal_mem_virt_to_phys(void *Va)
{
  /* Place holder function. Need to be
   * implemented if needed in later releases
   */
  return Va;
}

/**
  @brief  Returns the virtual address of the input physical address.

  @param Pa physical address of the memory to be converted

  Returns the virtual address.
**/
void *
pal_mem_phys_to_virt (
  uint64_t Pa
  )
{
  /* Place holder function*/
  return (void*)Pa;
}

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
void *
pal_memcpy(void *DestinationBuffer, void *SourceBuffer, uint32_t Length)
{

  return memcpy(DestinationBuffer, SourceBuffer, Length);
}

/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return 1 - Success, 0 -Failure

**/
uint64_t
pal_time_delay_ms(uint64_t MicroSeconds)
{
  /**Need to implement**/
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  gBS->Stall(MicroSeconds);
  return 1;
#endif
  return 0;
}

/**
  @brief  page size being used in current translation regime.

  @return page size being used
**/
uint32_t
pal_mem_page_size()
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

    return EFI_PAGE_SIZE;
#endif
    return 0;
}

/**
  @brief  allocates contiguous numpages of size
          returned by pal_mem_page_size()

  @return Start address of base page
**/
void *
pal_mem_alloc_pages (uint32_t NumPages)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS PageBase;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               NumPages,
                               &PageBase);
  if (EFI_ERROR(Status))
  {
    print(AVS_PRINT_ERR, "Allocate Pages failed %x \n", Status);
    return NULL;
  }

  return (VOID*)(UINTN)PageBase;
#endif
  return 0;

}

/**
  @brief  frees continguous numpages starting from page
          at address PageBase

**/
void
pal_mem_free_pages(void *PageBase, uint32_t NumPages)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)PageBase, NumPages);
#endif
}

/**
  @brief   Checks if System information is passed using Baremetal (BM)
           This api is also used to check if GIC/Interrupt Init ACS Code
           is used or not. In case of BM, ACS Code is used for INIT

  @param  None

  @return True/False
*/
UINT32
pal_target_is_bm()
{
  return 1;
}

