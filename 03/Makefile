img: haribo.img

run: haribo.img
	qemu-system-i386 -fda haribo.img

haribo.img: ipl.nas
	../z_tools/nask ipl.nas haribo.img
