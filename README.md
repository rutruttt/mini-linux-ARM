mini-linux-ARM is an operating system for ARM-based qemu bare-metal devices. it is compiled with gcc-arm-none-eabi, which relies on newlib implementation of libc. the command supported are:

* command parser - >,>> output redirections, 2>,2>> err redirections, < input redirection.
* cat
* zero-arguments ls
* mkdir
* rm
* cd to a child or parent (..)

does not support flags.

## emulation
to emulate you will need to install qemu-system-arm on WSL: (the packages "qemu"/"qemu-system" may not include qemu-system-arm)

> sudo apt install qemu-system-arm 

as promised: the code is emulated on a bare-metal device, namely, has no kernel at all. 
instead of a kernel executing .elf files according to their entry point (i'll elaborate on that in the next section), a bare-metal device just has a fixed start point for the CPU. 
i use versatilepb - a uart-based device, whose execution start point is at address 0x0. therefore, this is the emulation command:

> QEMU_AUDIO_DRV=none qemu-system-arm -M versatilepb -nographic -serial mon:stdio -device loader,file=mini-linux.bin,addr=0x0 2>/dev/null

(disabling qemu noise by redirecting stderr to /dev/null).

