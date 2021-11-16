SBSA_ROOT:= $(SBSA_PATH)
SBSA_DIR := $(SBSA_ROOT)/platform/pal_baremetal/src/
SBSA_A64_DIR := $(SBSA_ROOT)/platform/pal_baremetal/src/AArch64/
FVP_DIR  := $(SBSA_ROOT)/platform/pal_baremetal/FVP/src/

CFLAGS    += -I$(SBSA_ROOT)/
CFLAGS    += -I$(SBSA_ROOT)/val/include
CFLAGS    += -I$(SBSA_ROOT)/platform/pal_baremetal/
CFLAGS    += -I$(SBSA_ROOT)/platform/pal_baremetal/FVP/
CFLAGS    += -I$(SBSA_ROOT)/platform/pal_baremetal/FVP/include/
ASFLAGS   += -I$(SBSA_ROOT)/platform/pal_baremetal/src/AArch64/

OUT_DIR = $(SBSA_ROOT)/build/
OBJ_DIR := $(SBSA_ROOT)/build/obj
LIB_DIR := $(SBSA_ROOT)/build/lib

CC = $(GCC49_AARCH64_PREFIX)gcc -march=armv8.2-a
AR = $(GCC49_AARCH64_PREFIX)ar

FILES   += $(foreach files,$(SBSA_DIR)/,$(wildcard $(files)/*.S))
FILES   += $(foreach files,$(SBSA_DIR)/,$(wildcard $(files)/*.c))
FILES   += $(foreach files,$(SBSA_A64_DIR),$(wildcard $(files)/*.S))
FILES   += $(foreach files,$(FVP_DIR)/,$(wildcard $(files)/*.S))
FILES   += $(foreach files,$(FVP_DIR)/,$(wildcard $(files)/*.c))

FILE    = `find $(FILES) -type f -exec sh -c 'echo {} $$(basename {})' \; | sort -u --stable -k2,2 | awk '{print $$1}'`
FILE_1  := $(shell echo $(FILE))
PAL_OBJS +=$(addprefix $(OBJ_DIR)/,$(addsuffix .o, $(basename $(notdir $(foreach dirz,$(FILE_1),$(dirz))))))

all:    PAL_LIB
	echo $(PAL_OBJS)

create_dirs:
	rm -rf ${OBJ_DIR}
	rm -rf ${LIB_DIR}
	rm -rf ${OUT_DIR}
	@mkdir ${OUT_DIR}
	@mkdir ${OBJ_DIR}
	@mkdir ${LIB_DIR}

$(OBJ_DIR)/%.o: $(SBSA_A64_DIR)/%.S
	$(CC) $(CFLAGS) $(ASFLAGS) -c -o $@ $< >> $(OUT_DIR)/compile.log 2>&1

$(OBJ_DIR)/%.o: $(SBSA_DIR)/%.c
	$(CC) $(CFLAGS) -g -c -o $@ $< >> $(OUT_DIR)/compile.log 2>&1

$(OBJ_DIR)/%.o: $(SBSA_DIR)/%.S
	$(CC) $(CFLAGS) $(ASFLAGS) -c -o $@ $< >> $(OUT_DIR)/compile.log 2>&1

$(OBJ_DIR)/%.o: $(FVP_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $< >> $(OUT_DIR)/compile.log 2>&1

$(OBJ_DIR)/%.o: $(SBSA_DIR)/AArch64/%.S
	$(CC) $(CFLAGS) $(ASFLAGS) -c -o $@ $< >> $(OUT_DIR)/compile.log 2>&1

$(LIB_DIR)/lib_pal.a: $(PAL_OBJS)
	$(AR) $(ARFLAGS) $@ $^ >> $(OUT_DIR)/link.log 2>&1

PAL_LIB: $(LIB_DIR)/lib_pal.a
clean:
	rm -rf ${OBJ_DIR}
	rm -rf ${LIB_DIR}
	rm -rf ${OUT_DIR}

.PHONY: all PAL_LIB

