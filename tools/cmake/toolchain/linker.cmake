## @file
 # Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

if(_LINKER_CMAKE_LOADED)
  return()
endif()
set(_LINKER_CMAKE_LOADED TRUE)

set(GNUARM_LINKER "${CROSS_COMPILE}ld" CACHE FILEPATH "The GNUARM linker" FORCE)
set(GNUARM_OBJCOPY "${CROSS_COMPILE}objcopy" CACHE FILEPATH "The GNUARM objcopy" FORCE)
set(GNUARM_OBJDUMP "${CROSS_COMPILE}objdump" CACHE FILEPATH "The GNUARM objdump" FORCE)

if(${ENABLE_PIE})
    set(LINKER_PIE_SWITCH "-pie --no-dynamic-linker")
else()
    set(LINKER_PIE_SWITCH "")
endif()

set(LINKER_DEBUG_OPTIONS "-g")
set(GNUARM_LINKER_FLAGS "--fatal-warnings  ${LINKER_PIE_SWITCH} ${LINKER_DEBUG_OPTIONS} -O1 --gc-sections --build-id=none")
set(GNUARM_OBJDUMP_FLAGS    "-dSx")
set(GNUARM_OBJCOPY_FLAGS    "-Obinary")

function (create_executable EXE_NAME OUTPUT_DIR TEST)
    set(SCATTER_INPUT_FILE "${SBSA_DIR}/tools/cmake/sbsa/image.ld.S")
    set(SCATTER_OUTPUT_FILE "${OUTPUT_DIR}/${EXE_NAME}_image.ld")

    # Preprocess the scatter file for image layout symbols
    add_custom_command(OUTPUT CPP-LD--${EXE_NAME}${TEST}
                    COMMAND ${CROSS_COMPILE}gcc -E -P -I${SBSA_DIR}/baremetal_app/ -I${ROOT_DIR}/pal/baremetal/target/${TARGET}/common/include -I${ROOT_DIR}/pal/baremetal/target/${TARGET}/sbsa/include -I${ROOT_DIR} ${SCATTER_INPUT_FILE} -o ${SCATTER_OUTPUT_FILE} -DCMAKE_BUILD={CMAKE_BUILD}
                     DEPENDS ${VAL_LIB} ${PAL_LIB} ${TEST_LIB})
    add_custom_target(CPP-LD-${EXE_NAME}${TEST} ALL DEPENDS CPP-LD--${EXE_NAME}${TEST})

    # Link the objects
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.elf
                    COMMAND ${GNUARM_LINKER} ${CMAKE_LINKER_FLAGS} -T ${SCATTER_OUTPUT_FILE} -o ${OUTPUT_DIR}/${EXE_NAME}.elf ${VAL_LIB}.a ${PAL_LIB}.a ${TEST_LIB}.a ${VAL_LIB}.a ${PAL_LIB}.a ${PAL_OBJ_LIST} -Map=${OUTPUT_DIR}/${EXE_NAME}.map
                    DEPENDS CPP-LD-${EXE_NAME}${TEST})
    add_custom_target(${EXE_NAME}${TEST}_elf ALL DEPENDS ${EXE_NAME}${TEST}.elf)

    # Create the dump info
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.dump
                    COMMAND ${GNUARM_OBJDUMP} ${GNUARM_OBJDUMP_FLAGS} ${OUTPUT_DIR}/${EXE_NAME}.elf > ${OUTPUT_DIR}/${EXE_NAME}.dump
                    DEPENDS ${EXE_NAME}${TEST}_elf)
    add_custom_target(${EXE_NAME}${TEST}_dump ALL DEPENDS ${EXE_NAME}${TEST}.dump)

    # Create the binary
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.bin
                    COMMAND ${GNUARM_OBJCOPY} ${GNUARM_OBJCOPY_FLAGS} ${OUTPUT_DIR}/${EXE_NAME}.elf ${OUTPUT_DIR}/${EXE_NAME}.bin
                    DEPENDS ${EXE_NAME}${TEST}_dump)
    add_custom_target(${EXE_NAME}${TEST}_bin ALL DEPENDS ${EXE_NAME}${TEST}.bin)

    # Create the image
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.img
                    COMMAND ${GNUARM_OBJCOPY} ${GNUARM_OBJCOPY_FLAGS} ${OUTPUT_DIR}/${EXE_NAME}.elf ${OUTPUT_DIR}/${EXE_NAME}.img
                    DEPENDS ${EXE_NAME}${TEST}_dump)
    add_custom_target(${EXE_NAME}${TEST}_img ALL DEPENDS ${EXE_NAME}${TEST}.img)

endfunction()
