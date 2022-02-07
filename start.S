/*
 * start.S
 *
 * Startup module for Raspberry-Pi bare metal
 *
 * Resources:
 *     https://github.com/BrianSidebotham/arm-tutorial-rpi by Brian Sidebotham
 *     https://github.com/dwelch67/raspberrypi by David Welch
 *
 */

/* Standard definitions of stack locations, mode bits
 * and interrupt (I & F) flags in PSRs
 */
.equ        IRQ_stack,  0x4000
.equ        FIQ_stack,  0x6000
.equ        SVC_stack,  0x8000
.equ        SYS_stack,  0x8000

.equ        Mode_USR,   0x10
.equ        Mode_FIQ,   0x11
.equ        Mode_IRQ,   0x12
.equ        Mode_SVC,   0x13
.equ        Mode_MON,   0x16
.equ        Mode_ABT,   0x17
.equ        Mode_UND,   0x1B
.equ        Mode_SYS,   0x1F

.equ        IRD_dis,    0x80                    // When I bit is set, IRQ is disabled
.equ        FIQ_dis,    0x40                    // When F bit is set, FIQ is disabled

.equ        SCTLR_ENABLE_DATA_CACHE,        0x4
.equ        SCTLR_ENABLE_BRANCH_PREDICTION, 0x800
.equ        SCTLR_ENABLE_INSTRUCTION_CACHE, 0x1000

.section    .text.startup

.global     _start

/* Set the stack pointer above application memory space.
 * Will be available to the ARM CPU no matter what
 * settings we use to split the memory between the GPU and ARM CPU.
 */
_start:
    ldr     pc,reset_handler                    // Effectively also jump to initialization
    ldr     pc,undefined_handler
    ldr     pc,swi_handler
    ldr     pc,prefetch_handler
    ldr     pc,data_handler
    ldr     pc,unused_handler
    ldr     pc,irq_handler
    ldr     pc,fiq_handler

reset_handler:      .word   reset               // Linker adjusts this table to _start (i.e. adds 0x8000)
undefined_handler:  .word   inf_loop
swi_handler:        .word   inf_loop
prefetch_handler:   .word   inf_loop
data_handler:       .word   inf_loop
unused_handler:     .word   inf_loop
irq_handler:        .word   inf_loop
fiq_handler:        .word   inf_loop

reset:
/* Setup exception vector table
 */
    mov     r0,#0x8000
    mov     r1,#0x0000
    ldmia   r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia   r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
    ldmia   r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia   r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

/* Setup stacks for common exceptions
 */
    mov     r0,#(Mode_SVC | IRD_dis | FIQ_dis)  // Supervisor mode
    msr     cpsr_c,r0                           // Needed when starting but not after reset exception
    mov     sp,#SVC_stack

    mov     r0,#(Mode_IRQ | IRD_dis | FIQ_dis)
    msr     cpsr_c,r0
    mov     sp,#IRQ_stack

    mov     r0,#(Mode_FIQ | IRD_dis | FIQ_dis)
    msr     cpsr_c,r0
    mov     sp,#FIQ_stack

/* Run in System mode
 * Some embedded applications might run entirely in any mode other than User mode.
 * An application that requires full access to system resources usually executes in System mode.
 */
    mov     r0,#(Mode_SYS | IRD_dis | FIQ_dis)
    msr     cpsr_c,r0
    mov     sp,#SYS_stack

/* Zero out .bss section
 */
    ldr     r0,=__bss_start__                   // Trust linker with __bss_start__ <= __bss_end__
    ldr     r1,=__bss_end__
    mov     r2,#0
bss_zero_loop:
    cmp     r0,r1
    bhs     done_bss_zero                       // With optimization 'on', __bss_end__ is not always multiple of 4!
    str     r2,[r0]
    add     r0,r0,#4
    b       bss_zero_loop

done_bss_zero:

/* Enable L1 Cache
 */
    mrc     p15,0,r0,c1,c0,0                    // R0 = System Control Register

    orr     r0,#SCTLR_ENABLE_BRANCH_PREDICTION  // Enable caches and branch prediction
    orr     r0,#SCTLR_ENABLE_DATA_CACHE
    orr     r0,#SCTLR_ENABLE_INSTRUCTION_CACHE

    mcr     p15,0,r0,c1,c0,0                    // System Control Register = R0

/* Load r0, r1, r2 with entry parameters
 * and call the C application entry point kernel()
 */
	mov     r0, #0
    mrc     p15, 0, r1, c0, c0, 0               // Only care about machine ID
    mov     r2, #0
    bl      kernel

inf_loop:
    b       inf_loop                            // Should not return, but just in case.

