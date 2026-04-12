NASM32 = nasm
GCC32 = x86_64-linux-gnu-gcc -c -m32 -ffreestanding -MMD -MP
LD32 = x86_64-linux-gnu-ld -melf_i386 -T elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = x86_64-linux-gnu-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

CSOURCEFILES = $(wildcard *.c)
ASMSOURCEFILES = $(wildcard *.asm)
COBJECTFILES = $(patsubst %.c, %.o, $(CSOURCEFILES))
ASSEMBLYOBJECTFILES = $(patsubst %.asm, %.o, $(ASMSOURCEFILES))

all: Kernel32.bin

EntryPoint.bin: EntryPoint.s
	$(NASM32) -o $@ $<

Kernel32.elf.bin: Kernel32.elf
	$(OBJCOPY32) $< $@

Kernel32.bin: EntryPoint.bin Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin *.elf *.o *.d

%.o: %.c
	$(GCC32) -o $@ $<

%.o: %.asm
	$(NASM32) -f elf32 -o $@ $<

Kernel32.elf: Main.o $(filter-out Main.o, $(COBJECTFILES)) $(ASSEMBLYOBJECTFILES)
	$(LD32) -o $@ $^

-include $(COBJECTFILES:.o=.d)
