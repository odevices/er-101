firmware: 
	+$(MAKE) -f scripts/firmware.mk

clean:
	+$(MAKE) -f scripts/firmware.mk clean

dist-clean:
	rm -rf release debug testing