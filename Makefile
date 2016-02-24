TOOLPATH = ./z_tools/
NASK     = $(TOOLPATH)nask
EDIMG    = $(TOOLPATH)edimg
CC       = $(TOOLPATH)gocc1 -I ./z_tools/haribote/ -Os -Wall

default: img

hankaku.obj: hankaku.txt
	$(TOOLPATH)makefont $< hankaku.bin
	$(TOOLPATH)bin2obj hankaku.bin $@ _hankaku

bootpack.bim: bootpack.obj graphic.obj dsctbl.obj naskfunc.obj hankaku.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj
	$(TOOLPATH)obj2bim @$(TOOLPATH)haribote/haribote.rul out:bootpack.bim stack:3136k map:bootpack.map $^

haribote.img: haribote.sys ipl.bin
	$(EDIMG)   imgin:./z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0\
		copy from:$< to:@: \
		imgout:$@

haribote.sys:	asmhead.bin bootpack.hrb
	cat $^ > $@

%.bin: %.nas
	$(NASK) $< $@ $*.lst

%.gas: %.c bootpack.h
	$(CC) $< -o $@

%.nas: %.gas
	$(TOOLPATH)gas2nask $< $@

%.obj: %.nas
	$(NASK) $< $@ $*.lst

%.hrb: %.bim
	$(TOOLPATH)bim2hrb $< $@ 0

img: haribote.img

run: haribote.img
	qemu-system-i386 -fda $<
