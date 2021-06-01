/*
 * auxuart.h
 *
 *  Header file for the Auxiliary UART (UART1) driver of the BCM2835
 *  auxiliary peripherals.
 *
 *  This device is configured by default for:
 *  - 8-bit data, 1 start bit, one stop bit, no parity,
 *  - no hardware flow control
 *  - no interrupt on receive
 *  - no interrupt on transmit
 *  - no timeout, wait forever
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#ifndef __AUXUART_H__
#define __AUXUART_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* UART1 setup options
 * 'OR' these options to configure UART1 for non-default operation
 */
#define     AUXUART_DEFAULT             0x00000000
#define     AUXUART_7BIT                0x00000001              // Default is 8-bit
#define     AUXUART_ENA_HW_FLOW         0x00000002              // Default no hardware flow control
#define     AUXUART_ENA_RX_IRQ          0x00000004              // Default is without receiver interrupts
#define     AUXUART_ENA_TX_IRQ          0x00000008              // Default is without receiver interrupts

/* Values are BAUD rate clock divider register,
 * and used here to save the need for division calculation.
 * Baud rate divisors are based on 250MHz system clock.
 */
typedef enum
{
    BAUD_9600   = 3254,
    BAUD_19200  = 1627,
    BAUD_38400  = 813,
    BAUD_57600  = 542,
    BAUD_115200 = 270
} baud_t;


/* UART management
 */
int     bcm2835_auxuart_init(baud_t baud_rate_div,
                             uint32_t rx_tout, uint32_t tx_tout,
                             uint32_t configuration);           // Initialization
void    bcm2835_auxuart_close(void);                            // Close auxiliary mini UART

/* Buffered, interrupt driven
 */
int     bcm2835_auxuart_rx_data(uint8_t *buffer, int count);    // Receive data from Rx buffer
int     bcm2835_auxuart_rx_byte(uint8_t *byte);                 // Read byte from Rx circular buffer.
int     bcm2835_auxuart_tx_data(uint8_t *buffer, int count);    // Transmit data through Tx buffer

/* Non-buffered, UART direct Rx and Tx
 */
void    bcm2835_auxuart_putchr(uint8_t byte);                   // Output a character
uint8_t bcm2835_auxuart_getchr(void);                           // Get a character from receive buffer
uint8_t bcm2835_auxuart_waitchr(void);                          // Wait (block) for character from receive buffer
int     bcm2835_auxuart_ischar(void);                           // Check if a character is present in the receive buffer

#endif  /* __AUXUART_H__ */
