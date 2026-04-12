.PHONY: all BootLoader Kernel32 Kernel64 ImageMaker NetworkTransfer clean re run run-on-mac qemu create_hdd

all: Disk.img NetworkTransfer

BootLoader:
	make -C 00.BootLoader

Kernel32:
	make -C 01.Kernel32

Kernel64:
	make -C 02.Kernel64

ImageMaker:
	make -C 04.Utility/00.ImageMaker

NetworkTransfer:
	make -C 04.Utility/01.NetworkTransfer

Disk.img: BootLoader Kernel32 Kernel64 ImageMaker
	04.Utility/00.ImageMaker/ImageMaker 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility/00.ImageMaker clean
	make -C 04.Utility/01.NetworkTransfer clean
	rm -f Disk.img

re:
	make clean
	make all

qemu:
	qemu-system-x86_64 \
		-L . -m 64 -M pc \
		-drive format=raw,file=Disk.img,if=floppy \
		-hda HDD.img \
		-serial tcp::4444,server,nowait \
		-smp 4 \
		-display curses	# Press ALT + 2 or ESC + 2, then enter quit

run: all qemu

run-on-mac:
	docker exec -t builder bash -i -c "cd /root/mintos && make"
	make qemu

create_hdd:
	qemu-img create HDD.img 20M
