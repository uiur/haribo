TOOLPATH = ./z_tools/
NASK     = $(TOOLPATH)nask
EDIMG    = $(TOOLPATH)edimg
CC       = $(TOOLPATH)gocc1 -I ./z_tools/haribote/ -Os -Wall

default: img

%.bin: %.nas
	$(NASK) $< $@ $*.lst

%.gas: %.c
	$(CC) $< -o $@

%.nas: %.gas
	$(TOOLPATH)gas2nask $< $@

hankaku.obj: hankaku.txt
	$(TOOLPATH)makefont $< hankaku.bin
	$(TOOLPATH)bin2obj hankaku.bin $@ _hankaku

%.obj: %.nas
	$(NASK) $< $@ $*.lst

bootpack.bim: bootpack.obj graphic.obj naskfunc.obj hankaku.obj
	$(TOOLPATH)obj2bim @$(TOOLPATH)haribote/haribote.rul out:bootpack.bim stack:3136k map:bootpack.map $^

%.hrb: %.bim
	$(TOOLPATH)bim2hrb $< $@ 0

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
