# Designed for AVR32 processors

# Tools install root
toolchain_dir ?= $(HOME)/microchip
gcc_dir := $(toolchain_dir)/avr32-gnu-toolchain-linux_x86_64
headers_dir := $(toolchain_dir)/avr-headers

# Set compiler tools
CC := export LC_ALL=C && $(gcc_dir)/bin/avr32-gcc -mpart=uc3c1512c -I$(headers_dir)
CPP := export LC_ALL=C && $(gcc_dir)/bin/avr32-g++ -mpart=uc3c1512c -I$(headers_dir)
OBJCOPY := export LC_ALL=C && $(gcc_dir)/bin/avr32-objcopy
OBJDUMP := export LC_ALL=C && $(gcc_dir)/bin/avr32-objdump
ADDR2LINE := export LC_ALL=C && $(gcc_dir)/bin/avr32-addr2line
LD := export LC_ALL=C && $(gcc_dir)/bin/avr32-gcc -mpart=uc3c1512c
AR := export LC_ALL=C && $(gcc_dir)/bin/avr32-gcc-ar
SIZE := export LC_ALL=C && $(gcc_dir)/bin/avr32-size
STRIP := export LC_ALL=C && $(gcc_dir)/bin/avr32-strip
READELF := export LC_ALL=C && $(gcc_dir)/bin/avr32-readelf
NM := $(gcc_dir)/bin/avr32-nm
PYTHON := python3
ZIP := zip