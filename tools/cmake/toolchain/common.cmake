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

#Stop built in CMakeDetermine<lang>.cmake scripts to run.
set (CMAKE_C_COMPILER_ID_RUN 1)
#Stop cmake run compiler tests.
set (CMAKE_C_COMPILER_FORCED true)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set(CMAKE_SHARED_LIBRARY_SUFFIX "")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")


if(NOT DEFINED CROSS_COMPILE)
    message(FATAL_ERROR "CROSS_COMPILE is undefined.")
else()
    set(CMAKE_C_COMPILER  "${CROSS_COMPILE}gcc")
    set(CMAKE_C_COMPILER_ID "gnuarm" CACHE INTERNAL "CROSS_COMPILE ID" FORCE)
    set(COMPILER_FILE "${ROOT_DIR}/tools/cmake/toolchain/gnuarm.cmake")
endif ()

# Overwrite COMPILER_FILE and CMAKE_C_COMPILER if CC is passed as command-line argument
if(DEFINED CC)
    set(CMAKE_C_COMPILER  ${CC})
    if (CC MATCHES "^.*armclang(\\.exe)?$")
        set(CMAKE_C_COMPILER_ID "ARMCLANG" CACHE INTERNAL "C compiler ID" FORCE)
        set(COMPILER_FILE "${ROOT_DIR}/tools/cmake/toolchain/armclang.cmake")
    elseif (CC MATCHES "^.*clang(\\.exe)?$")
        set(CMAKE_C_COMPILER_ID "CLANG" CACHE INTERNAL "C compiler ID" FORCE)
        set(COMPILER_FILE "${ROOT_DIR}/tools/cmake/toolchain/clang.cmake")
    else()
        message(FATAL_ERROR "CC is unknown.")
    endif()
endif()

## Always use ld for linking
set(LINKER_FILE "${ROOT_DIR}/tools/cmake/toolchain/linker.cmake")

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE INTERNAL "CROSS_COMPILE is set to ${CROSS_COMPILE}" FORCE)
set(CMAKE_C_COMPILER ${CMAKE_C_COMPILER} CACHE INTERNAL "CMAKE_C_COMPILER is set to ${CMAKE_C_COMPILER}" FORCE)

include(${COMPILER_FILE})
include(${LINKER_FILE})
