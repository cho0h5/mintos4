NASM64 = nasm -f elf64
GCC64 = x86_64-linux-gnu-gcc -c -m64 -ffreestanding -fno-stack-protector -MMD -MP
LD64 = x86_64-linux-gnu-ld -melf_x86_64 -T elf_x86_64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = x86_64-linux-gnu-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary
RUSTC = rustc --target x86_64-unknown-none --emit=obj -C panic=abort -C opt-level=2 -C relocation-model=static

CSOURCEFILES = $(wildcard *.c)
ASMSOURCEFILES = $(wildcard *.asm)
COBJECTFILES = $(patsubst %.c, %.o, $(CSOURCEFILES))
ASSEMBLYOBJECTFILES = $(patsubst %.asm, %.o, $(ASMSOURCEFILES))

all: Kernel64.bin

Kernel64.bin: Kernel64.elf
	$(OBJCOPY64) $< $@

clean:
	rm -f *.bin *.elf *.o *.d *.rlib

EntryPoint.o: EntryPoint.s
	$(NASM64) -o $@ $<

%.o: %.c
	$(GCC64) -o $@ $<

%.o: %.asm
	$(NASM64) -o $@ $<

rust.o: $(wildcard *.rs)
	$(RUSTC) -o $@ lib.rs

Kernel64.elf: EntryPoint.o $(COBJECTFILES) $(ASSEMBLYOBJECTFILES) rust.o
	$(LD64) -o $@ $^

-include $(COBJECTFILES:.o=.d)
