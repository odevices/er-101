#!/bin/bash
set -e -x
mkdir ~/microchip
cd ~/microchip
# downloads
wget https://ww1.microchip.com/downloads/archive/avr32-gnu-toolchain-3.4.3.820-linux.any.x86_64.tar.gz
wget https://ww1.microchip.com/downloads/archive/avr-headers.zip
# unpack
tar xvf avr32-gnu-toolchain-3.4.3.820-linux.any.x86_64.tar.gz
unzip avr-headers
# test
cd avr32-gnu-toolchain-linux_x86_64/bin
./avr32-gcc --version
