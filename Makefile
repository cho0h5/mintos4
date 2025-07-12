all: BootLoader Kernel32 Kernel64 ImageMaker
	make Disk.img

BootLoader:
	make -C 00.BootLoader

Kernel32:
	make -C 01.Kernel32

Kernel64:
	make -C 02.Kernel64

ImageMaker:
	make -C 04.Utility/00.ImageMaker
	cp 04.Utility/00.ImageMaker/ImageMaker .

Disk.img: BootLoader Kernel32
	./ImageMaker 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility/00.ImageMaker clean
	rm -f Disk.img
	rm -f ImageMaker

re:
	make clean
	make all

run: re
	qemu-system-x86_64 \
		-L . -m 64 -M pc \
		-drive format=raw,file=Disk.img,if=floppy \
		-hda HDD.img \
		-serial tcp::4444,server,nowait

create_hdd:
	qemu-img create HDD.img 20M
