/*
 * spi1.h
 *
 *  Header for the SPI1 interface (AUX SPI0) of the BCM2835.
 *  The SPI1 interface is available on RPi Zero 40-pin
 *  header and SPI2 is not available on the RPi header pins.
 *
 *  This device is configured by default for:
 *  - Only CE2 is enabled on header P1 pin 36
 *  - Default clock rate 128KHZ
 *  - SPI Mode-0
 *  - MSB first
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#ifndef __SPI1_H__
#define __SPI1_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* SPI setup options
 * 'OR' these options to configure SPI0 for non-default operation
 */
#define     SPI1_DEFAULT            0x00000000  // See in file title
#define     SPI1_CPHA_BEGIN         0x00000001  // First SCLK transition at beginning of data bit
#define     SPI1_CPOL_HI            0x00000002  // Rest state of clock = high
#define     SPI1_CSPOL_HI           0x00000004  // Chip select lines are active high
#define     SPI1_ENA_DMA            0x00000008  // Enable DMA
#define     SPI1_ENA_TX_IRQ         0x00000010  // Enable transmission done interrupt
#define     SPI1_ENA_RX_IRQ         0x00000020  // Enable receive data interrupt
#define     SPI1_LONG_DATA          0x00000040  // Set to 32-bit data IO
#define     SPI1_LOSSI_MODE         0x00000080  // Interface will be in LoSSI mode

#define     SPI1_DEFAULT_RATE       128000      // Hz

/* SPI clock phase and polarity
 */
typedef enum
{
    SPI1_MODE0 = 0,     // CPOL = 0, CPHA = 0
    SPI1_MODE1 = 1,     // CPOL = 0, CPHA = 1
    SPI1_MODE2 = 2,     // CPOL = 1, CPHA = 0
    SPI1_MODE3 = 3,     // CPOL = 1, CPHA = 1
} spi1_mode_t;

/* Chip select pin(s)
 * https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#/media/File:SPI_timing_diagram2.svg
 */
typedef enum
{
    SPI1_CS0 = 0,       // Chip Select 0
    SPI1_CS1 = 1,       // Chip Select 1
    SPI1_CS2 = 2,       // Chip Select 2 (XXX pins CS1 and CS2 are asserted)
    SPI1_CS_NONE = 3,       // No automatic CS, application control
} spi1_chip_sel_t;

int  bcm2835_spi1_init(uint32_t configuration);              // Initialization
void bcm2835_spi1_close(void);                               // Close SPI1 device
int  bcm2835_spi1_set_rate(uint32_t data_rate);              // Set SPI transfer data rate
void bcm2835_spi1_clk_mode(spi1_mode_t mode);                // Set SPI clock mode (CPOL/CPHA)
void bcm2835_spi1_cs(spi1_chip_sel_t cs);                    // Select CS line
void bcm2835_spi1_cs_polarity(spi1_chip_sel_t cs, int level);// Set CS polarity

void bcm2835_spi1_transfer_Ex(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count);
void bcm2835_spi1_send_byte(uint8_t byte);                   // Transmit one byte
int  bcm2835_spi1_recv_byte(void);                           // Receive one byte
int  bcm2835_spi1_transfer_byte(uint8_t tx_byte);            // Transmit a byte and return the received byte

#endif  /* __SPI1_H__ */
