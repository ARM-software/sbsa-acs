/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include "SbsaAvs.h"

IMAGE_BASE = PLATFORM_HOST_IMAGE_BASE;
OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(acs_host_entry)

MEMORY {
    RAM (rwx): ORIGIN = IMAGE_BASE, LENGTH = PLATFORM_HOST_IMAGE_SIZE
}

SECTIONS
{
    . = IMAGE_BASE;

    ASSERT(. == ALIGN(PAGE_SIZE),
           "TEXT_START address is not aligned to PAGE_SIZE.")
    .text : {
        __TEXT_START__ = .;
        *AvsBootEntry.S.o(.text*)
        *(.text*)
        . = NEXT(PAGE_SIZE);
        __TEXT_END__ = .;
    }

    .rodata : {
        . = ALIGN(PAGE_SIZE);
        __RODATA_START__ = .;
        *(.rodata*)

        . = NEXT(PAGE_SIZE);
        __RODATA_END__ = .;

    }

    .data : {
        . = ALIGN(PAGE_SIZE);
        __DATA_START__ = .;
        *(.data*)

        . = ALIGN(8);
        __GOT_START__ = .;
        *(.got)
        __GOT_END__ = .;

        . = NEXT(PAGE_SIZE);
        __DATA_END__ = .;
    }

    .bss (NOLOAD) : {
        . = ALIGN(PAGE_SIZE);
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = NEXT(PAGE_SIZE);
        __BSS_END__ = .;
    }
}
