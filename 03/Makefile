TOOLPATH = ../z_tools/
NASK     = $(TOOLPATH)nask
EDIMG    = $(TOOLPATH)edimg

default: img

ipl.bin: ipl.nas
	$(NASK) ipl.nas ipl.bin ipl.lst

haribote.img: haribote.sys ipl.bin
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0\
		copy from:haribote.sys to:@: \
		imgout:haribote.img

haribote.sys:	haribote.nas
	$(NASK) haribote.nas haribote.sys haribote.lst

asm: ipl.bin

img: haribote.img

run: img
	qemu-system-i386 -fda haribote.img
