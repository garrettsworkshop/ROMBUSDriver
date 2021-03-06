# path to RETRO68
RETRO68=~/Retro68-build/toolchain

PREFIX=$(RETRO68)/bin/m68k-apple-macos
AS=$(PREFIX)-as
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OBJCOPY=$(PREFIX)-objcopy
OBJDUMP=$(PREFIX)-objdump
CFLAGS=-march=68030 -c -Os

all: bin/rom16M_swap.bin obj/rdisk7M5.s obj/driver7M5.s obj/entry_rel.sym obj/driver_abs.sym

obj:
	mkdir obj

bin:
	mkdir bin


obj/entry.o: entry.s obj
	$(AS) $< -o $@

obj/entry_rel.sym: obj obj/entry.o
	$(OBJDUMP) -t obj/entry.o > $@


obj/rdisk7M5.o: rdisk.c obj
	$(CC) -Wall -DRDiskSize=7864320 $(CFLAGS) $< -o $@

obj/rdisk7M5.s: obj obj/rdisk7M5.o
	$(OBJDUMP) -d obj/rdisk7M5.o > $@


obj/spi.o: spi.c obj
	$(CC) -Wall $(CFLAGS) $< -o $@

obj/spi.s: obj obj/spi.o
	$(OBJDUMP) -d obj/spi.o > $@


obj/sdmmc.o: sdmmc.c obj
	$(CC) -Wall $(CFLAGS) -Os $< -o $@

obj/sdmmc.s: obj obj/sdmmc.o
	$(OBJDUMP) -d obj/sdmmc.o > $@



obj/driver7M5.o: obj obj/entry.o obj/rdisk7M5.o
	$(LD) -Ttext=40851D70 -o $@ obj/entry.o obj/rdisk7M5.o obj/spi.o obj/sdmmc.o

obj/driver7M5.s: obj obj/driver7M5.o
	$(OBJDUMP) -d obj/driver7M5.o > $@

obj/driver_abs.sym: obj obj/driver7M5.o
	$(OBJDUMP) -t obj/driver7M5.o > $@


bin/driver7M5.bin: bin obj/driver7M5.o
	$(OBJCOPY) -O binary obj/driver7M5.o $@

bin/rom8M.bin: bin baserom.bin RDisk7M5.dsk bin bin/driver7M5.bin obj/driver_abs.sym obj/entry_rel.sym 
	cp baserom.bin $@ # Copy base rom
	# Patch driver
	dd if=bin/driver7M5.bin of=$@ bs=1 seek=335248 skip=32 conv=notrunc # Copy driver code
	printf '\x78' | dd of=$@ bs=1 seek=335168 count=1 conv=notrunc # Set resource flags
	printf '\x4F' | dd of=$@ bs=1 seek=335216 count=1 conv=notrunc # Set driver flags
	cat obj/entry_rel.sym | grep "DOpen" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335224 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "DPrime" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335226 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "DControl" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335228 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "DStatus" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335230 count=2 conv=notrunc
	cat obj/entry_rel.sym | grep "DClose" | cut -c5-8 | xxd -r -p - | dd of=$@ bs=1 seek=335232 count=2 conv=notrunc
	dd if=RDisk7M5.dsk of=$@ bs=1024 seek=512 count=7680 conv=notrunc # copy disk image

bin/rom8M_swap.bin: bin bin/rom8M.bin
	dd if=bin/rom8M.bin of=$@ conv=swab # swap bytes

bin/iisi_swap.bin: bin iisi.bin
	dd if=iisi.bin of=$@ conv=swab # swap bytes

bin/rom16M_swap.bin: bin/iisi_swap.bin bin/rom8M_swap.bin
	cat bin/rom8M_swap.bin > $@
	cat bin/iisi_swap.bin >> $@


.PHONY: clean
clean:
	rm -fr bin obj
