avrdude -i 1000 -p m32u4 -c usbtiny -P USB -U lfuse:w:0xfd:m -U hfuse:w:0xd8:m -U efuse:w:0xce:m
avrdude -p m32u4 -c usbtiny -P USB -U flash:w:caterina-WIOT.hex