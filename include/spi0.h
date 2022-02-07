/*
 * spi0.h
 *
 *  Header file for the SPI0 interface of the BCM2835.
 *  The module only supports SPI0, SPI1 is available on RPi Zero 40-pin
 *  header and SPI2 is not available on the RPi header pins.
 *  The module does not provide an option to configure interrupts.
 *
 *  This device is configured by default for:
 *  - Interface in SPI mode (not TODO LoSSI mode)
 *  - CS0 selected for automatic assertion
 *  - Clock at Mode-0
 *  - Chip select lines are active low and not automatically un-asserted
 *  - No DMA
 *  - No Tx IRQ
 *  - No Rx IRQ
 *  - Single byte data length
 *  - Data rate 488.28125kHz
 *  - MSB first bit order
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#ifndef __SPI0_H__
#define __SPI0_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* SPI setup options
 * 'OR' these options to configure SPI0 for non-default operation
 */
#define     SPI0_DEFAULT                0x00000000      // See in file title
#define     SPI0_CPHA_BEGIN             0x00000001      // First SCLK transition at beginning of data bit
#define     SPI0_CPOL_HI                0x00000002      // Rest state of clock = high
#define     SPI0_CSPOL_HI               0x00000004      // Chip select lines are active high
#define     SPI0_ENA_DMA                0x00000008      // Enable DMA
#define     SPI0_ENA_TX_IRQ             0x00000010      // Enable transmission done interrupt
#define     SPI0_ENA_RX_IRQ             0x00000020      // Enable receive data interrupt
#define     SPI0_LONG_DATA              0x00000040      // Set to 32-bit data IO
#define     SPI0_LOSSI_MODE             0x00000080      // Interface will be in SoSSI mode

#define     SPI0_DEFAULT_RATE           100000          // 100Khz

/* SPI clock phase and polarity
 */
typedef enum
{
    SPI0_MODE0 = 0,                     // CPOL = 0, CPHA = 0
    SPI0_MODE1 = 1,                     // CPOL = 0, CPHA = 1
    SPI0_MODE2 = 2,                     // CPOL = 1, CPHA = 0
    SPI0_MODE3 = 3,                     // CPOL = 1, CPHA = 1
} spi0_mode_t;

/* Chip select pin(s)
 * https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#/media/File:SPI_timing_diagram2.svg
 */
typedef enum
{
    SPI0_CS0 = 0,                       // Chip Select 0
    SPI0_CS1 = 1,                       // Chip Select 1
    SPI0_CS2 = 2,                       // Chip Select 2 (XXX pins CS1 and CS2 are asserted)
    SPI0_CS_NONE = 3,                   // No automatic CS, application control
} spi0_chip_sel_t;

int  bcm2835_spi0_init(uint32_t configuration);              // Initialization
void bcm2835_spi0_close(void);                               // Close SPI0 device
int  bcm2835_spi0_set_rate(uint32_t data_rate);              // Set SPI transfer data rate
void bcm2835_spi0_clk_mode(spi0_mode_t mode);                // Set SPI clock mode (CPOL/CPHA)
void bcm2835_spi0_cs(spi0_chip_sel_t cs);                    // Select CS line
void bcm2835_spi0_cs_polarity(spi0_chip_sel_t cs, int level);// Set CS polarity

void bcm2835_spi0_transfer_Ex(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count);
void bcm2835_spi0_send_byte(uint8_t byte);                   // Transmit one byte
int  bcm2835_spi0_recv_byte(void);                           // Receive one byte
int  bcm2835_spi0_transfer_byte(uint8_t tx_byte);            // Transmit a byte and return the received byte

#endif  /* __SPI0_H__ */
