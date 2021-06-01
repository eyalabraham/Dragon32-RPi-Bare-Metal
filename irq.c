/*
 * irq.c
 *
 *  Module implementing BCM2835 and ARM interrupt management.
 *  This module is independent of the other bcm2835 IO library modules
 *  and can be used with custom IO functions.
 *
 *  TODO Add support for (some?) FIQ interrupts.
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-Interrupt-Controller.html
 *      https://www.raspberrypi.org/forums/viewtopic.php?t=248813
 */

#include    "bcm2835.h"
#include    "irq.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */
/* Handlers and vectors
 */
#define     MAX_HANDLERS                8
#define     IRQ_VEC_ADDRESS             0x00000038      // Physical address
#define     FIQ_VEC_ADDRESS             0x0000003C      // See: start.S

/* -----------------------------------------
   Types and data structures
----------------------------------------- */
struct handler_t
{
    uint32_t            device_irq_pend_mask;
    volatile uint32_t  *pending_reg;
    void              (*handler)(void);
};

/* -----------------------------------------
   Module external static functions
----------------------------------------- */
extern void irq_handler(void);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static  int handler_cnt = 0;
static  interrupt_controller_regs_t *pIC = (interrupt_controller_regs_t*)BCM2835_INT_BASE;

static  struct handler_t dispatch_table[MAX_HANDLERS];  // Safe, .bss section is initialized

/*------------------------------------------------
 * irq_init()
 *
 *  Initialize the interrupt module.
 *  Interrupts are disabled after initialization is called.
 *
 * param:  none
 * return: none
 *
 */
void irq_init(void)
{
    uint32_t   *vector;

    disable();  // *** Safety measure

    vector = (uint32_t*)IRQ_VEC_ADDRESS;
    *vector = (uint32_t)irq_handler;
}

/*------------------------------------------------
 * irq_register_handler()
 *
 *  Register and IRQ handler/service for a device interrupt.
 *  The handler's registration order determines priority.
 *
 * param:  Device interrupt source and handler function
 * return: 1- if successful, 0- otherwise
 *
 */
int irq_register_handler(intr_source_t source, void (*handler_func)(void))
{
    volatile uint32_t *pend_reg;
    int                shift;

    /* Limit registration count
     */
    if ( handler_cnt >= MAX_HANDLERS )
        return 0;

    /* Register interrupt handler.
     */
    if ( source <= 0 )
    {
        pend_reg = &(pIC->ic_basic_pending);
        shift = -source;
    }
    else if ( source > 31 )
    {
        pend_reg = &(pIC->ic_pending2);
        shift = source % 32;
    }
    else
    {
        pend_reg = &(pIC->ic_pending1);
        shift = source % 32;
    }

    dmb();

    dispatch_table[handler_cnt].device_irq_pend_mask = (1 << shift);
    dispatch_table[handler_cnt].pending_reg = pend_reg;
    dispatch_table[handler_cnt].handler = handler_func;

    handler_cnt++;

    return 1;
}

/*------------------------------------------------
 * irq_enable()
 *
 *  Enable specific interrupt source.
 *  The interrupt is enabled in the interrupt controller,
 *  the device-specific enable action needs to be carried out in the device driver.
 *
 * param:  Device interrupt source
 * return: none
 *
 */
void irq_enable(intr_source_t source)
{
    int shift;

    dmb();

    if ( source <= 0 )
    {
        shift = -source;
        pIC->ic_basic_enable = (1 << shift);
    }
    else if ( source > 31 )
    {
        shift = source % 32;
        pIC->ic_enable2 = (1 << shift);
    }
    else
    {
        shift = source % 32;
        pIC->ic_enable1 = (1 << shift);
    }
}

/*------------------------------------------------
 * irq_disable()
 *
 *  Disable specific interrupt source.
 *  The interrupt is disabled in the interrupt controller,
 *  the device-specific disable action needs to be carried out in the device driver.
 *
 * param:  Device interrupt source
 * return: none
 *
 */
void irq_disable(intr_source_t source)
{
    int shift;

    dmb();

    if ( source <= 0 )
    {
        shift = -source;
        pIC->ic_basic_disable = (1 << shift);
    }
    else if ( source > 31 )
    {
        shift = source % 32;
        pIC->ic_disable2 = (1 << shift);
    }
    else
    {
        shift = source % 32;
        pIC->ic_disable1 = (1 << shift);
    }
}

/*------------------------------------------------
 * __irq_dispatch()
 *
 *  Called from the IRQ handler when IRQ is enabled and
 *  and an IRQ exception is registered by the ARM.
 *  This routing scans the list of registered handlers, see: see irq_register_hadler(),
 *  checks the IRQ pending bits and calls the device handler if a pending bit
 *  is set and a handler is available.
 *  Scanning is done according to user priority set by handler registration order.
 *
 * param:  none
 * return: none
 *
 */
void __irq_dispatch(void)
{
    int i;

    /* Scan the dispatch table from high to low priority
     * and call the handler if an interrupt is pending.
     */
    for ( i = 0; i < handler_cnt; i++ )
    {
        if ( *(dispatch_table[i].pending_reg) & dispatch_table[i].device_irq_pend_mask )
        {
            dispatch_table[i].handler();
        }
    }
}
