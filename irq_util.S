/*
 * irq_utils.S
 *
 * Utility routines in assembly for the interrup module
 *
 * Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-Interrupt-Controller.html
 *      https://www.raspberrypi.org/forums/viewtopic.php?t=248813
 *
 */


.global     irq_global_enable
.global     irq_global_disable
.global     irq_handler
.global     irq_none

/*
 * Enable IRQ interrupt globally.
 * Called as C irq_global_enable()
 */
irq_global_enable:
    cpsie   i
    mov     pc, lr

/*
 * Disable IRQ interrupt globally and returns the current state.
 * Called as C irq_global_disable()
 */
irq_global_disable:
    //mrs     r0, cpsr
    cpsid   i
    mov     pc, lr

/*
 * IRQ interrups handler stub that calls the C dispatch(),
 * which checks IRQ pending bits by priority and called the
 * registered interrupt handler for a device.
 * XXX Does stack need to be 8-byte aligned when calling an externally visible function?
 * XXX Why do we need to push 'lr'? to align the stack to an 8-byte?
 * XXX Read up on entry into and return from exception.
 */
irq_handler:
    push    {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    bl      __irq_dispatch
    pop     {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    subs    pc,lr,#4

/*
 * Dummy handler for a non enabled IRQ interrupt vector
 */
irq_none:
    b       irq_none
