include scripts/env.mk
include scripts/utils.mk

program_name := firmware
out_dir := $(build_dir)/$(program_name)
src_dirs := src $(asf_dir) 

includes += src \
	$(asf_dir)/avr32/drivers/intc \
	$(asf_dir)/avr32/utils \
	$(asf_dir)/avr32/utils/preprocessor \
	$(asf_dir)/common/boards \
	$(asf_dir)/common/boards/user_board \
	$(asf_dir)/common/utils \
	$(arch_dir)/$(ARCH)/config \
	$(asf_dir)/avr32/drivers/cpu/cycle_counter \
	$(asf_dir)/avr32/drivers/gpio \
	$(asf_dir)/avr32/drivers/qdec \
	$(asf_dir)/avr32/drivers/tc \
	$(asf_dir)/avr32/drivers/usart \
	$(asf_dir)/avr32/drivers/flashc \
	$(asf_dir)/common/services/clock \
	$(asf_dir)/common/services/fifo \
	$(asf_dir)/common/drivers/nvm \
	$(asf_dir)/avr32/drivers/pm \
	$(asf_dir)/avr32/drivers/scif \
	$(asf_dir)/avr32/utils/libs/dsplib/include \
	$(asf_dir)/avr32/drivers/spi \
	$(asf_dir)/avr32/drivers/pdca \
	$(asf_dir)/avr32/drivers/wdt \
	$(asf_dir)/avr32/drivers/ast

libraries :=

# Recursive search for source files
cpp_sources := $(foreach D,$(src_dirs),$(call rwildcard,$D,*.cpp)) 
c_sources := $(foreach D,$(src_dirs),$(call rwildcard,$D,*.c))
S_sources := $(foreach D,$(src_dirs),$(call rwildcard,$D,*.S)) 

objects := $(addprefix $(out_dir)/,$(c_sources:%.c=%.o) $(cpp_sources:%.cpp=%.o) $(S_sources:%.S=%.o)) 

CFLAGS += -DFIRMWARE_VERSION=\"$(FIRMWARE_VERSION)\"
CFLAGS += -DBUILD_PROFILE=\"$(PROFILE)\"
LFLAGS = $(LFLAGS.$(ARCH))

all: $(out_dir)/$(program_name).hex

$(objects): scripts/env.mk

# Final linking of the ELF executable.
$(out_dir)/$(program_name).elf: $(objects) $(libraries)
	@mkdir -p $(@D)	
	@echo $(describe_env) LINK $(describe_target)
	@$(LD) $(CFLAGS) -o $@ $(objects) $(libraries) $(LFLAGS)
	
# Strip the executable down to a memory-loadable binary.
$(out_dir)/$(program_name).hex: $(out_dir)/$(program_name).elf
	@echo $(describe_env) OBJCOPY $(describe_target)
	@mkdir -p $(@D)
	@$(SIZE) $<
	@$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

clean:
	rm -rf $(out_dir)

# Look up the source file and line that corresponds to an address in the binary exe.
addr2line: $(out_dir)/$(program_name).elf
	@echo $(describe_env) Find ${ADDRESS} in $(out_dir)/$(program_name).elf
	@$(ADDR2LINE) -p -f -i -C -e $(out_dir)/$(program_name).elf -a $(ADDRESS)

include scripts/rules.mk
