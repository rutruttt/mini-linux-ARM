.global _start

.section .text

_start:
    ldr sp, =STACK_TOP      // Set up the stack pointer

    // Zero out the .bss section
    ldr r0, =_sbss          // Load start address of .bss into r0
    ldr r1, =_ebss          // Load end address of .bss into r1
    mov r2, #0              // Value to write (zero)

.L_clear_bss_loop:
    cmp r0, r1              // Compare current address with end address
    bge .L_bss_cleared      // If r0 >= r1, .bss is cleared, exit loop
    str r2, [r0]            // Store zero at current address
    add r0, r0, #4          // Increment address by 4 bytes (assuming 32-bit words)
    b .L_clear_bss_loop     // Loop

.L_bss_cleared:

    bl main                  // Call main()
    b .                      // Loop forever if main returns
