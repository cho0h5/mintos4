all:
	make BootLoader
	make Disk.img

BootLoader:
	make -C 00.BootLoader

Disk.img:
	cp 00.BootLoader/BootLoader.bin Disk.img

clean:
	make -C 00.BootLoader clean
	rm -f Disk.img

re:
	make clean
	make all

run: all
	qemu-system-x86_64 \
		-L . -m 64 -M pc \
		-drive format=raw,file=Disk.img,if=floppy \
		-nographic
