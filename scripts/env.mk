# Determine PROFILE if it's not provided...
# testing | release | debug
PROFILE ?= release
ARCH = avr32
FIRMWARE_VERSION = 2.10

scriptname := $(word 1, $(MAKEFILE_LIST))
scriptname := $(scriptname:scripts/%=%)

# colorized labels
yellowON = "\033[33m"
yellowOFF = "\033[0m"
blueON = "\033[34m"
blueOFF = "\033[0m"
describe_input = $(yellowON)$<$(yellowOFF)
describe_target = $(yellowON)$@$(yellowOFF)
describe_env = $(blueON)[$(scriptname) $(ARCH) $(PROFILE)]$(blueOFF)

# Frequently used paths
build_dir = $(PROFILE)/$(ARCH)
arch_dir = arch

CFLAGS.common = -Wall -ffunction-sections -fdata-sections
CFLAGS.only = -std=gnu99 -fno-strict-aliasing -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Werror=implicit-function-declaration

CFLAGS.speed = -O2
CFLAGS.size = -Os

symbols = 
includes = 

###########################

#### avr32-specific
ifeq ($(ARCH),avr32)

CFLAGS.release ?= $(CFLAGS.speed) -Wno-unused
CFLAGS.testing ?= $(CFLAGS.speed) -DBUILDOPT_TESTING
CFLAGS.debug ?= -g -DBUILDOPT_TESTING

asf_dir = $(arch_dir)/$(ARCH)/ASF

include scripts/avr32.mk

symbols += BOARD=USER_BOARD

CFLAGS.avr32 = -mrelax -mno-cond-exec-before-reload
LFLAGS.avr32 = -Wl,--relax -Wl,-e,_trampoline \
	-nostartfiles --section-start=.ref_tables=0x80017360 \
	-L$(asf_dir)/avr32/utils/libs/dsplib/at32ucr3fp/gcc \
	-ldsp-at32ucr3fp-dspspeed_opt

endif

###########################

ifndef CFLAGS.$(PROFILE)
$(error Error: '$(PROFILE)' is not a valid build profile.)
endif

CFLAGS += $(CFLAGS.common) $(CFLAGS.$(ARCH)) $(CFLAGS.$(PROFILE))
CFLAGS += $(addprefix -I,$(includes)) 
CFLAGS += $(addprefix -D,$(symbols))

