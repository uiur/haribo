TOOLPATH = ./z_tools/
NASK     = $(TOOLPATH)nask
EDIMG    = $(TOOLPATH)edimg
CC       = $(TOOLPATH)gocc1 -I ./z_tools/haribote/ -Os -Wall

default: img

asmhead.bin: asmhead.nas
	$(NASK) $< $@ asmhead.lst

bootpack.gas: bootpack.c bootpack.h
	$(CC) $< -o $@

bootpack.nas: bootpack.gas
	$(TOOLPATH)gas2nask $< $@

bootpack.obj: bootpack.nas
	$(NASK) $< $@ bootpack.lst

naskfunc.obj: naskfunc.nas
	$(NASK) $< $@ naskfunc.lst

hankaku.obj: hankaku.txt
	$(TOOLPATH)makefont $< hankaku.bin
	$(TOOLPATH)bin2obj hankaku.bin $@ _hankaku

bootpack.bim: bootpack.obj naskfunc.obj hankaku.obj
	$(TOOLPATH)obj2bim @$(TOOLPATH)haribote/haribote.rul out:bootpack.bim stack:3136k map:bootpack.map $^

bootpack.hrb: bootpack.bim
	$(TOOLPATH)bim2hrb bootpack.bim bootpack.hrb 0

ipl.bin: ipl.nas
	$(NASK) ipl.nas ipl.bin ipl.lst

haribote.img: haribote.sys ipl.bin
	$(EDIMG)   imgin:./z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0\
		copy from:haribote.sys to:@: \
		imgout:haribote.img

haribote.sys:	asmhead.bin bootpack.hrb
	cat $^ > $@

asm: ipl.bin

img: haribote.img

run: img
	qemu-system-i386 -fda haribote.img
