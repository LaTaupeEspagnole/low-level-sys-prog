#!/bin/bash

./usr/gen_init_cpio initramfs.txt > initramfs.img
qemu-system-x86_64 --enable-kvm \
                   -nographic \
                   -serial stdio \
                   -monitor none \
                   -kernel ./arch/x86/boot/bzImage \
                   -initrd initramfs.img \
                   -append "console=ttyS0"
