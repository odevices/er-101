firmware: 
	+$(MAKE) -f scripts/firmware.mk

dist-clean:
	rm -rf release debug testing