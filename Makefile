# make jkpatch

TARGET = payload.bin
KTARGET = kpayload.elf

all: clean $(TARGET) $(KTARGET)

$(TARGET):
	cd payload && $(MAKE) -s
	cp payload/$(TARGET) $(TARGET)
	cd tool && $(MAKE) -s
	tool/bin2js $(TARGET) > exploit/payload.js

$(KTARGET):
	cd kpayload && $(MAKE) -s
	cp kpayload/$(KTARGET) $(KTARGET)
	
.PHONY: clean
clean:
	rm -f $(TARGET) $(KTARGET)
	cd payload && $(MAKE) -s clean
	cd kpayload && $(MAKE) -s clean
