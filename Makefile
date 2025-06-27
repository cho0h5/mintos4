all:
	make BootLoader
	make Kernel32
	make Disk.img

BootLoader:
	make -C 00.BootLoader

Kernel32:
	make -C 01.Kernel32

Disk.img: BootLoader Kernel32
	cat 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin > Disk.img

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	rm -f Disk.img

re:
	make clean
	make all

run: re
	qemu-system-x86_64 \
		-L . -m 64 -M pc \
		-drive format=raw,file=Disk.img,if=floppy \
		-serial mon:stdio \
		-display curses
