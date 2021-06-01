/*
 * irq.h
 *
 *  Header file for the BCM2835 and ARM interrupt management.
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-Interrupt-Controller.html
 *      https://www.raspberrypi.org/forums/viewtopic.php?t=248813
 *      https://github.com/xinu-os/xinu
 */

#ifndef __IRQ_H__
#define __IRQ_H__

#include    <stdint.h>

#include    "bcm2835.h"

#define     enable()                irq_global_enable()
#define     disable()               irq_global_disable()

/* Supported sources
 */
typedef enum
{
    /* Basic pending register
     */
    IRQ_ARM_TIMER     =  0,
    IRQ_ARM_MAILBOX   = -1,
    IRQ_DOORBELL0     = -2,
    IRQ_DORRBELL1     = -3,

    /* Pending 1 register
     */
    IRQ_SYSTEM_TIMER1 =  1,
    IRQ_SYSTEM_TIMER3 =  3,
    IRQ_USB           =  9,
    IRQ_AUX_SERDEV    = 29,         // UART1, SPI1, SPI2

    /* Pending 2 register
     */
    IRQ_I2C1          = 43,
    IRQ_SPI_SLAVE     = 43,
    IRQ_GPIO0         = 49,
    IRQ_GPIO1         = 50,         // XXX Are these pins accessible?
    IRQ_GPIO2         = 51,         // XXX Are these pins accessible?
    IRQ_GPIO3         = 52,         // XXX Are these pins accessible?
    IRQ_I2C0          = 53,
    IRQ_SPI0          = 54,
    IRQ_PCM           = 55,
    IRQ_UART0         = 57,
} intr_source_t;


void    irq_global_enable(void);            // Global interrupt enable
void    irq_global_disable(void);           // Global interrupt disable

void    irq_init(void);                     // Initialize the interrupt module and start services
int     irq_register_handler(intr_source_t source, void (*handler_func)(void));
void    irq_enable(intr_source_t source);   // Enable specific interrupt source
void    irq_disable(intr_source_t source);  // Disable specific interrupt source

#endif  /* __IRQ_H__ */
