/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#include <stdint.h>
#include <stdarg.h>
#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "pal_pl011_uart.h"

extern uint32_t  g_print_level;
extern void* g_sbsa_log_file_handle;
uint8_t   *gSharedMemory;

#define get_num_va_args(_args, _lcount)             \
    (((_lcount) > 1)  ? va_arg(_args, long long int) :  \
    (((_lcount) == 1) ? va_arg(_args, long int) :       \
                va_arg(_args, int)))

#define get_unum_va_args(_args, _lcount)                \
    (((_lcount) > 1)  ? va_arg(_args, unsigned long long int) : \
    (((_lcount) == 1) ? va_arg(_args, unsigned long int) :      \
                va_arg(_args, unsigned int)))

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
  uint64_t mask = 0x3;

  if (addr & 0x3) {
      addr = addr & ~mask;  //make sure addr is aligned to 4 bytes
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
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write8 Address = %llx  Data = %lx \n", addr, data);

  *(volatile uint8_t *)addr = data;
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
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write16 Address = %llx  Data = %lx \n", addr, data);

  *(volatile uint16_t *)addr = data;
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
  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write64 Address = %llx  Data = %llx \n", addr, data);

  *(volatile uint64_t *)addr = data;
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

  if (g_print_mmio || (g_curr_module & g_enable_module))
      print(AVS_PRINT_INFO, " pal_mmio_write Address = %8x  Data = %x \n", addr, data);

    *(volatile uint32_t *)addr = data;
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
                while(i!=0)
                    *(volatile uint8_t *)addr = buffer[--i];
            } else
                *(volatile uint8_t *)addr = 48;

        } else
            *(volatile uint8_t *)addr = *string;
    }
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
void
pal_mem_free(void *Buffer)
{
#ifndef TARGET_BM_BOOT
  free(Buffer);
#else
  pal_mem_free_aligned(Buffer);
#endif
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
#ifndef TARGET_BM_BOOT
  free ((void *)gSharedMemory);
#else
  pal_mem_free_aligned((void *)gSharedMemory);
#endif
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
#ifndef TARGET_BM_BOOT
  return malloc(Size);
#else
  uint32_t alignment = 0x08;
  return (void *)mem_alloc(alignment, Size);
#endif

}

/**
  @brief  Allocates requested buffer size in bytes with zeros in a contiguous memory
          and returns the base address of the range.

  @param  Size         allocation size in bytes
  @retval if SUCCESS   pointer to allocated memory
  @retval if FAILURE   NULL
**/
void *
pal_mem_calloc(uint32_t num, uint32_t Size)
{
#ifndef TARGET_BM_BOOT
  return calloc(num, Size);
#else
  void* ptr;
  uint32_t alignment = 0x08;

  ptr = mem_alloc(alignment, num * Size);

  if (ptr != NULL)
  {
    pal_mem_set(ptr, num * Size, 0);
  }
  return ptr;
#endif

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
  @brief   Checks if System information is passed using Baremetal (BM)
           This api is also used to check if GIC/Interrupt Init ACS Code
           is used or not. In case of BM, ACS Code is used for INIT

  @param  None

  @return True/False
*/
uint32_t
pal_target_is_bm()
{
  return 1;
}

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
void *
pal_memcpy(void *DestinationBuffer, const void *SourceBuffer, uint32_t Length)
{

    uint32_t i;
    const char *s = (char *)SourceBuffer;
    char *d = (char *) DestinationBuffer;

    for(i = 0; i < Length; i++)
    {
        d[i] = s[i];
    }

    return d;
}

uint32_t pal_strncmp(const char8_t *str1, const char8_t *str2, uint32_t len)
{
    while ( len && *str1 && ( *str1 == *str2 ) )
    {
        ++str1;
        ++str2;
        --len;
    }
    if ( len == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)str1 - *(unsigned char *)str2 );
    }
}

void *pal_strncpy(void *DestinationStr, const void *SourceStr, uint32_t Length)
{
  const char *s = SourceStr;
  char *d = DestinationStr;

  if (d == NULL) {
      return NULL;
  }

  char* ptr = d;

  while (*s && Length--)
  {
      *d = *s;
      d++;
      s++;
  }
  *d = '\0';

  return ptr;
}

int
pal_mem_compare(void *Src, void *Dest, uint32_t Len)
{
    if (Len != 0) {
    register const unsigned char *p1 = Dest, *p2 = Src;

    do {
      if (*p1++ != *p2++)
        return (*--p1 - *--p2);
    } while (--Len != 0);
  }
  return (0);
}

void
pal_mem_set(void *buf, uint32_t size, uint8_t value)
{
    unsigned char *ptr = buf;

    while (size--)
    {
        *ptr++ = (unsigned char)value;
    }

    return (void) buf;
}

/* The functions implemented below are to enable console prints via UART driver */

static int string_print(const char *str)
{
    int count = 0;

    for ( ; *str != '\0'; str++) {
        (void)pal_uart_putc(*str);
        count++;
    }

    return count;
}

static int unsigned_num_print(unsigned long long int unum, unsigned int radix,
                  char padc, int padn)
{
    /* Just need enough space to store 64 bit decimal integer */
    char num_buf[20];
    int i = 0, count = 0;
    unsigned int rem;

    /* num_buf is only large enough for radix >= 10 */
    if (radix < 10) {
        return 0;
    }

    do {
        rem = unum % radix;
        if (rem < 0xa)
            num_buf[i] = '0' + rem;
        else
            num_buf[i] = 'a' + (rem - 0xa);
        i++;
        unum /= radix;
    } while (unum > 0U);

    if (padn > 0) {
        while (i < padn) {
            (void)pal_uart_putc(padc);
            count++;
            padn--;
        }
    }

    while (--i >= 0) {
        (void)pal_uart_putc(num_buf[i]);
        count++;
    }

    return count;
}

int vprintf(const char *fmt, va_list args)
{
    int l_count;
    long long int num;
    unsigned long long int unum;
    char *str;
    char padc = '\0'; /* Padding character */
    int padn;         /* Number of characters to pad */
    int count = 0;    /* Number of printed characters */

    while (*fmt != '\0') {
        l_count = 0;
        padn = 0;

        if (*fmt == '%') {
            fmt++;
            /* Check the format specifier */
loop:
            switch (*fmt) {
            case '%':
                (void)pal_uart_putc('%');
                break;
            case 'i': /* Fall through to next one */
            case 'd':
                num = get_num_va_args(args, l_count);
                if (num < 0) {
                    (void)pal_uart_putc('-');
                    unum = (unsigned long long int)-num;
                    padn--;
                } else
                    unum = (unsigned long long int)num;

                count += unsigned_num_print(unum, 10,
                                padc, padn);
                break;
            case 's':
                str = va_arg(args, char *);
                count += string_print(str);
                break;
            case 'p':
                unum = (uintptr_t)va_arg(args, void *);
                if (unum > 0U) {
                    count += string_print("0x");
                    padn -= 2;
                }

                count += unsigned_num_print(unum, 16,
                                padc, padn);
                break;
            case 'x':
                unum = get_unum_va_args(args, l_count);
                count += unsigned_num_print(unum, 16,
                                padc, padn);
                break;
            case 'z':
                if (sizeof(size_t) == 8U)
                    l_count = 2;

                fmt++;
                goto loop;
            case 'l':
                l_count++;
                fmt++;
                goto loop;
            case 'u':
                unum = get_unum_va_args(args, l_count);
                count += unsigned_num_print(unum, 10,
                                padc, padn);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                padc = '0';
                padn = 0;
                fmt++;

                for (;;) {
                    char ch = *fmt;
                    if ((ch < '0') || (ch > '9')) {
                        goto loop;
                    }
                    padn = (padn * 10) + (ch - '0');
                    fmt++;
                }

            default:
                /* Exit on any other format specifier */
                return -1;
            }

            fmt++;
            continue;
        }
        else
        {
            (void)pal_uart_putc(*fmt);
            if (*fmt == '\n')
            {
                (void)pal_uart_putc('\r');
            }
        }

        fmt++;
        count++;
    }

    return count;
}

static const char *prefix_str[] = {
        "", "", "", "", ""};

const char *log_get_prefix(int log_level)
{
        int level;

        if (log_level > AVS_PRINT_ERR) {
                level = AVS_PRINT_ERR;
        } else if (log_level < AVS_PRINT_INFO) {
                level = AVS_PRINT_TEST;
        } else {
                level = log_level;
        }

        return prefix_str[level - 1];
}

void pal_uart_print(int log, const char *fmt, ...)
{
    va_list args;
    const char *prefix_str;

    prefix_str = log_get_prefix(log);

    while (*prefix_str != '\0') {
            pal_uart_putc(*prefix_str);
            prefix_str++;
    }

    va_start(args, fmt);
    (void)vprintf(fmt, args);
    va_end(args);
    (void) log;
}