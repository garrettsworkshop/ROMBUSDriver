PREFIX=m68k-apple-macos
AS=$(PREFIX)-as
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OBJCOPY=$(PREFIX)-objcopy
OBJDUMP=$(PREFIX)-objdump

all: bin/ROMBUS_8M.bin obj/rombus.s obj/driver.s obj/driver_abs.sym

obj:
	mkdir -p $@
bin:
	mkdir -p $@


obj/entry.o: entry.s obj
	$(AS) $< -o $@

obj/entry_rel.sym: obj obj/entry.o
	$(OBJDUMP) -t obj/entry.o > $@


obj/spi.o: spi.c obj
	$(CC) -Wall -march=68020 -c -Os $< -o $@
obj/spi_hal.o: spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_rx8.o: spi_rx8.s spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_rx16.o: spi_rx16.s spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_tx8.o: spi_tx8.s spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_tx16.o: spi_tx16.s spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_rxtx8.o: spi_rxtx8.s spi_hal.s spi_hal_common.s obj
	$(AS) $< -o $@
obj/spi_delay.o: spi_delay.s obj
	$(AS) $< -o $@


obj/rombus.o: rombus.c obj
	$(CC) -Wall -march=68020 -c -Os $< -o $@

obj/rombus.s: obj obj/rombus.o
	$(OBJDUMP) -d obj/rombus.o > $@

obj/driver.o: obj obj/entry.o obj/rombus.o obj/spi.o obj/spi_hal.o \
			  obj/spi_rx8.o obj/spi_rx16.o \
			  obj/spi_tx8.o obj/spi_tx16.o \
			  obj/spi_rxtx8.o
	$(LD) -Ttext=40851D70 -o $@ obj/entry.o obj/rombus.o obj/spi.o obj/spi_hal.o \
								obj/spi_rx8.o obj/spi_rx16.o \
								obj/spi_tx8.o obj/spi_tx16.o \
								obj/spi_rxtx8.o

obj/driver.s: obj obj/driver.o
	$(OBJDUMP) -d obj/driver.o > $@

obj/driver_abs.sym: obj obj/driver.o
	$(OBJDUMP) -t obj/driver.o > $@

bin/driver.bin: bin obj/driver.o
	$(OBJCOPY) -O binary obj/driver.o $@


bin/baserom_rombus_ramtest.bin: bin roms/baserom.bin bin/driver.bin obj/driver_abs.sym obj/entry_rel.sym 
	cp roms/baserom.bin $@ # Copy base rom
	# Patch driver
	dd if=bin/driver.bin of=$@ bs=1 seek=335248 skip=32 conv=notrunc # Copy driver code
	printf '\x78' | dd of=$@ bs=1 seek=335168 count=1 conv=notrunc # Set resource flags
	printf '\x4F' | dd of=$@ bs=1 seek=335216 count=1 conv=notrunc # Set driver flags
	cat obj/entry_rel.sym | grep "[0-9]\s*DOpen" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335224 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "[0-9]\s*DPrime" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335226 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "[0-9]\s*DControl" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335228 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "[0-9]\s*DStatus" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335230 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "[0-9]\s*DClose" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335232 count=2 conv=notrunc

bin/baserom_rombus_noramtest.bin: bin bin/baserom_rombus_ramtest.bin
	cp bin/baserom_rombus_ramtest.bin $@ # Copy base rom
	# Disable RAM test
	printf '\x4E\xD6' | dd of=$@ bs=1 seek=288736 count=2 conv=notrunc
	printf '\x4E\xD6' | dd of=$@ bs=1 seek=289016 count=2 conv=notrunc


bin/ROMBUS_8M.bin: bin bin/baserom_rombus_noramtest.bin disks/RDisk.dsk
	# Copy base rom with ROM disk driver
	cp bin/baserom_rombus_noramtest.bin $@
	printf '\x00\x78\x00\x00' | dd of=$@ bs=1 seek=335276 count=4 conv=notrunc # Patch ROM disk size
	# Copy ROM disk image
	dd if=disks/RDisk.dsk of=$@ bs=1024 seek=512 conv=notrunc

.PHONY: clean
clean:
	rm -fr bin obj
