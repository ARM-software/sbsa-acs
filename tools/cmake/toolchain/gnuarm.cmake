## @file
 # Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

if(_TOOLCHAIN_CMAKE_LOADED)
  return()
endif()
set(_TOOLCHAIN_CMAKE_LOADED TRUE)

get_filename_component(_CMAKE_C_TOOLCHAIN_LOCATION "${CMAKE_C_COMPILER}" PATH)

set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "The GNUARM asm" FORCE)
set(CMAKE_AR "${CROSS_COMPILE}ar" CACHE FILEPATH "The GNUARM archiver" FORCE)

# Set the compiler's target architecture profile based on ARM_ARCH_MINOR option
if(${ARM_ARCH_MINOR} STREQUAL 0)
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}-a+profile+sve")
else()
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}.${ARM_ARCH_MINOR}-a+profile+sve")
endif()

if(${ENABLE_PIE})
    set(COMPILE_PIE_SWITCH "-fpie")
    add_definitions(-DENABLE_PIE)
else()
    set(COMPILE_PIE_SWITCH "")
endif()

set(C_COMPILE_DEBUG_OPTIONS "-g -Wa,-gdwarf-4")
set(ASM_COMPILE_DEBUG_OPTIONS "-g -Wa,--gdwarf-4")

add_definitions(-DTARGET_EMULATION)
add_definitions(-DTARGET_BM_BOOT)

set(CMAKE_C_FLAGS          "${TARGET_SWITCH}  ${COMPILE_PIE_SWITCH} ${C_COMPILE_DEBUG_OPTIONS} -ffunction-sections -fdata-sections -mstrict-align -O0 -ffreestanding -Wall -Werror -std=gnu99 -Wextra -Wstrict-overflow -DCMAKE_GNUARM_COMPILE -Wno-packed-bitfield-compat -Wno-missing-field-initializers -mcmodel=small")  #   -Wcast-align -Wmissing-prototypes -Wmissing-declarations
set(CMAKE_ASM_FLAGS        "${TARGET_SWITCH} ${ASM_COMPILE_DEBUG_OPTIONS} -c -x assembler-with-cpp -Wall -Werror -ffunction-sections -fdata-sections -Wstrict-prototypes -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wstrict-overflow -DCMAKE_GNUARM_COMPILE")
