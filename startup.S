.global _start

.section .text

_start:
    ldr sp, =STACK_TOP      // Set up the stack pointer
    bl main                  // Call main()
    b .                      // Loop forever if main returns
