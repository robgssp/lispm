boot.bin: kernel
	cat pure64/immed_mbr.sys pure64/pure64.sys kernel/kernel.bin > boot.bin
	./pad.sh boot.bin

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

clean:
	$(MAKE) -C kernel clean
