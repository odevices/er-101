# ER-101 Indexed Quad Sequencer

This repo contains the source code for the ER-101 firmware.

## Toolchain Installation (64-bit linux host)

`scripts/install-toolchain.sh` will install the toolchain to `~/microchip`. 

If you decide to install the toolchain in another location then you will need to edit `scripts/avr32.mk` to reflect your chosen location.

## Compilation

```bash
cd {project-root}
make firmware
```

The flash-able HEX file will be written to `release/avr32/er-101-firmware-{version}.hex`.

## Helpful AVR32 References

* https://hofmeyr.de/avr32%20hello%20world/
