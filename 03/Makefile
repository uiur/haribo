TOOLPATH = ../z_tools/
NASK     = $(TOOLPATH)nask
EDIMG    = $(TOOLPATH)edimg
CC       = $(TOOLPATH)gocc1

default: img

asmhead.bin: asmhead.nas
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

bootpack.gas: bootpack.c
	$(CC) bootpack.c -o bootpack.gas

bootpack.nas: bootpack.gas
	$(TOOLPATH)gas2nask bootpack.gas bootpack.nas

bootpack.obj: bootpack.nas
	$(NASK) bootpack.nas bootpack.obj bootpack.lst

bootpack.bim: bootpack.obj
	$(TOOLPATH)obj2bim @$(TOOLPATH)haribote/haribote.rul out:bootpack.bim stack:3136k map:bootpack.map bootpack.obj

bootpack.hrb: bootpack.bim
	$(TOOLPATH)bim2hrb bootpack.bim bootpack.hrb 0

ipl.bin: ipl.nas
	$(NASK) ipl.nas ipl.bin ipl.lst

haribote.img: haribote.sys ipl.bin
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0\
		copy from:haribote.sys to:@: \
		imgout:haribote.img

haribote.sys:	asmhead.bin bootpack.hrb
	cat asmhead.bin bootpack.hrb > haribote.sys

asm: ipl.bin

img: haribote.img

run: img
	qemu-system-i386 -fda haribote.img

clean:
	rm bootpack.{map,nas,bim,hrb,gas}
