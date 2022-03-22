# ER-101 Indexed Quad Sequencer

This repo contains the source code for the ER-101 firmware.

## Toolchain Installation

These instructions assume that you developing on a 64-bit linux host OS.

```bash
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
```

If you decide to install the toolchain in another location then you will need to edit `avr32.mk` to reflect your chosen location.

## Compilation

```bash
cd {project-root}
make firmware
```

The flash-able HEX file will be written to `release/avr32/firmware`.

## Helpful AVR32 References

* https://hofmeyr.de/avr32%20hello%20world/