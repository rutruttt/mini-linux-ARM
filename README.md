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
instead of a kernel executing .elf files according to their entry point (i'll elaborate on that in the compilation+emulation section), a bare-metal device just has a fixed start point for the CPU. 
i use versatilepb - a uart-based device, whose execution start point is at address 0x0. therefore, this is the emulation command:

> QEMU_AUDIO_DRV=none qemu-system-arm -M versatilepb -nographic -serial mon:stdio -device loader,file=mini-linux.bin,addr=0x0 2>/dev/null

(disabling qemu noise by redirecting stderr to /dev/null).

## implementation details

to install gcc-arm-none-eabi, run this command on WSL:

> sudo apt install gcc-arm-none-eabi

like i said, gcc-arm-none-eabi relies on newlib implementation of libc. to use it we need to link these two static libraries: libc.a (the libc implementation), that relies on libgcc.a; and libgcc.a (gcc internals and helpers), that invoked the newlib syscalls. THE SYSCALLS ARE THE ONES I IMPLEMENT MYSELF - according to my specific device and file system (in file newlib-syscalls.cpp).

to edit the code and view the newlib headers, will need vscode. open vscode in WSL mode, and choose the project folder. the configurations (in folder .vscode) are already set to the arm-none-eabi compiler you installed, so you can navigate to the library headers.

## compilation+emulation

like i said before, a bare-metal device just has a fixed start point for the CPU - therefore the startup code must be located exactly there. if you look at the linker versatilepb.ld you can see that the startup is explicitly placed in the beginning of section text.
to compile mini-linux.bin:

> arm-none-eabi-as -c startup.S -o startup.o
> 
> arm-none-eabi-g++ -ffreestanding -nostdlib -fno-exceptions -fno-rtti -T versatilepb.ld main.cpp StringUtilities.cpp PL011-uart.cpp FileEntry.cpp newlib-syscalls.cpp ShellUtilities.cpp startup.o -o mini-linux.elf -Wl,--start-group -lgcc -lc -Wl,--end-group
> 
> arm-none-eabi-objcopy -O binary mini-linux.elf mini-linux.bin
