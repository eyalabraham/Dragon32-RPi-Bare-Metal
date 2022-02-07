/*
 * spi0.c
 *
 *  Driver library module for the SPI0 interface of the BCM2835.
 *  The module only supports SPI0, SPI1 is available on RPi Zero 40-pin
 *  header and SPI2 is not available on the RPi header pins.
 *
 *  TODO Provide an option to configure interrupts.
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus
 *
 */

#include    "gpio.h"
#include    "spi0.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */
/* Masks for SPI0 CS register
 */
#define     SPI0_LEN_LONG               0x02000000      // Long data word in LoSSI mode if DMA_LEN is set
#define     SPI0_DMA_LEN                0x01000000      // Enable DMA in LoSSI mode
#define     SPI0_CSPOL2_ACT_HI          0x00800000      // Chip Select 2 polarity (not available in RPi)
#define     SPI0_CSPOL1_ACT_HI          0x00400000      // Chip Select 1 polarity
#define     SPI0_CSPOL0_ACT_HI          0x00200000      // Chip Select 0 polarity
#define     SPI0_RXF                    0x00100000      // Rx FIFO full
#define     SPI0_RXR                    0x00080000      // Rx FIFO half? full (see errata)
#define     SPI0_TXD                    0x00040000      // Tx FIFO can accept Data
#define     SPI0_RXD                    0x00020000      // Rx FIFO contains Data
#define     SPI0_DONE                   0x00010000      // Transfer done
#define     SPI0_LEN                    0x00002000      // LoSSI mode enable
#define     SPI0_REN                    0x00001000      // 2-wire mode read enable
#define     SPI0_ADCS                   0x00000800      // Automatically un-assert chip select
#define     SPI0_INTR                   0x00000400      // Interrupt on receive
#define     SPI0_INTD                   0x00000200      // Interrupt on transmit done
#define     SPI0_DMAEN                  0x00000100      // DMA enable
#define     SPI0_TA                     0x00000080      // Transfer active
#define     SPI0_CSPOL_ACT_HI           0x00000040      // Chip Select polarity active-high
#define     SPI0_RX_FIFO_CLR            0x00000020      // Clear Rx FIFO
#define     SPI0_TX_FIFO_CLR            0x00000010      // Clear Tx FIFO
#define     SPI0_RXTX_FIFO_CLR          (SPI0_RX_FIFO_CLR | SPI0_TX_FIFO_CLR)
#define     SPI0_CPOL                   0x00000008      // Clock polarity
#define     SPI0_CPHA                   0x00000004      // Clock phase
#define     SPI0_CS1                    0x00000001      // 00- CS0, 01- CS1, 10- CS2 (not available)
#define     SPI0_CS_MASK                0x00000003      // Chip Select mask, clear bit for CS0

#define     SPI0_MIN_RATE               32000           // ~32KHz
#define     SPI0_MAX_RATE               10000000        // actual ~9.6MHz

/* -----------------------------------------
   Types and data structures
----------------------------------------- */

/* -----------------------------------------
   Module static functions
----------------------------------------- */

/* -----------------------------------------
   Module globals
----------------------------------------- */
static spi0_regs_t *pSPI = (spi0_regs_t*)BCM2835_SPI0_BASE;

/*------------------------------------------------
 * bcm2835_spi0_init()
 *
 *  Initialize the SPI0 interface.
 *  Start SPI operations with RPi SPI0 pins P1-19 (MOSI),
 *  P1-21 (MISO), P1-23 (CLK), P1-24 (CE0) and P1-26 (CE1).
 *
 * param:  Configuration bits
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_spi0_init(uint32_t configuration)
{
    uint32_t    spi_config = 0x00000000;
    uint32_t    system_clock;
    uint32_t    spi_rate_div;

    /* Get core frequency and dynamically calculate rate divisor
     */
    if ( !(system_clock = bcm2835_core_clk()) )
        return 0;

    spi_rate_div = system_clock / SPI0_DEFAULT_RATE;

    if ( spi_rate_div & 1)
        spi_rate_div++;

    /* Set the SPI0 pins to the Alt-0 function to enable SPI0 access on them
     */
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_ALT0);  // CE1
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_ALT0);  // CE0
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_ALT0);  // MISO
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_ALT0);  // MOSI
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_ALT0);  // CLK

    /* Compile configuration from configuration bits
     * and apply to device
     */
    if ( configuration & SPI0_CPHA_BEGIN )
        spi_config |= SPI0_CPHA;

    if ( configuration & SPI0_CPOL_HI )
        spi_config |= SPI0_CPOL;

    if ( configuration & SPI0_CSPOL_HI )
        spi_config |= SPI0_CSPOL_ACT_HI | SPI0_CSPOL0_ACT_HI | SPI0_CSPOL1_ACT_HI | SPI0_CSPOL2_ACT_HI;

    dmb();

    pSPI->spi_cs = spi_config;

    pSPI->spi_clk = spi_rate_div;

    return 1;
}


/*------------------------------------------------
 * bcm2835_spi0_close()
 *
 *  Return SPI GPIO pins to default input.
 *
 * param:  none
 * return: none
 *
 */
void bcm2835_spi0_close(void)
{
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_INPT);

    dmb();

    pSPI->spi_cs = 0;
}

/*------------------------------------------------
 * bcm2835_spi0_set_rate()
 *
 *  Set the SPI data transfer rate.
 *
 * param:  Transfer rate in Hz
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_spi0_set_rate(uint32_t data_rate)
{
    uint32_t    system_clock;
    uint32_t    spi_rate_div;

    /* Limit check
     */
    if ( data_rate > SPI0_MAX_RATE )
        data_rate = SPI0_MAX_RATE;
    else if ( data_rate < SPI0_MIN_RATE )
        data_rate = SPI0_MIN_RATE;

    /* Get core frequency and dynamically calculate rate divisor
     */
    if ( !(system_clock = bcm2835_core_clk()) )
        return 0;

    spi_rate_div = system_clock / data_rate;

    if ( spi_rate_div & 1)
        spi_rate_div++;

    dmb();
    pSPI->spi_clk = spi_rate_div;

    return 1;
}

/*------------------------------------------------
 * bcm2835_spi0_clk_mode()
 *
 *  Set the SPI clock mode (CPOL/CPHA).
 *
 * param:  Mode
 * return: none
 *
 */
void bcm2835_spi0_clk_mode(spi0_mode_t mode)
{
    uint32_t    value;

    value = pSPI->spi_cs & ~(SPI0_CPHA | SPI0_CPOL);
    dmb();
    value |= (uint32_t) mode << 2;
    pSPI->spi_cs = value;
}

/*------------------------------------------------
 * bcm2835_spi0_cs()
 *
 *  Set the SPI chip select to assert during a transfer.
 *
 * param:  Chip select
 * return: none
 *
 */
void bcm2835_spi0_cs(spi0_chip_sel_t cs)
{
    uint32_t    value;

    value = pSPI->spi_cs & ~SPI0_CS_MASK;
    dmb();
    value |= (uint32_t) cs;
    pSPI->spi_cs = value;
}

/*------------------------------------------------
 * bcm2835_spi0_cs_polarity()
 *
 *  Set the SPI chip select polarity HIGH or LOW, during a transfer.
 *
 * param:  Chip select and polarity
 * return: none
 *
 */
void bcm2835_spi0_cs_polarity(spi0_chip_sel_t cs, int level)
{
    uint32_t    value;
    int         shift;

    shift = 21 + (int) cs;

    value = pSPI->spi_cs & ~(1 << shift);
    dmb();
    value |= (uint32_t) level << shift;
    pSPI->spi_cs = value;
}

/*------------------------------------------------
 * bcm2835_spi0_transfer_Ex()
 *
 *  Transfers one or more byte to and from the currently selected SPI slave.
 *  Asserts the currently selected CS pins during the transfer.
 *  Clocks 'count' 8-bit values out on MOSI, and simultaneously clocks in data from MISO.
 *  If rx_buf != NULL returns the read data byte(s) from the slave.
 *  Uses polled transfer as per section 10.6.1 of the BCM 2835 ARM Peripherals manual.
 *  *** 'tx_buf' and 'rx_buf' must be of same 'count' size.
 *
 * param:  Transmit data buffer, receive data buffer and byte count
 * return: none
 *
 */
void bcm2835_spi0_transfer_Ex(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count)
{
    uint32_t tx_count = 0;
    uint32_t rx_count = 0;
    uint8_t  byte;

    pSPI->spi_cs |= SPI0_RXTX_FIFO_CLR;

    dmb();

    pSPI->spi_cs |= SPI0_TA;

    dmb();

    while ( (tx_count < count) || (rx_count < count) )
    {
        /* Tx FIFO not full, so add some more bytes
         */
        while ( (pSPI->spi_cs & SPI0_TXD) && (tx_count < count ) )
        {
            dmb();
            pSPI->spi_fifo = tx_buf[tx_count];
            tx_count++;
        }

        /* Rx FIFO not empty, so get the next received bytes
         * if called provided a destination buffer, otherwise drop bytes
         */
        while ( (pSPI->spi_cs & SPI0_RXD) && (rx_count < count) )
        {
            dmb();
            byte = pSPI->spi_fifo;
            if ( rx_buf )
            {
                rx_buf[rx_count] = byte;
            }
            rx_count++;
        }
    }

    /* Wait for DONE to be set
     */
    while ( !(pSPI->spi_cs & SPI0_DONE) )
    {
        /* Wait */
    }

    dmb();

    /* Set TA = 0, and also set the barrier
     */
    pSPI->spi_cs &= ~SPI0_TA;
}

/*------------------------------------------------
 * bcm2835_spi0_send_byte()
 *
 *  Transmit one byte drop the return byte.
 *
 * param:  Byte to send
 * return: none
 *
 */
void bcm2835_spi0_send_byte(uint8_t byte)
{
    bcm2835_spi0_transfer_Ex(&byte, 0, 1);
}

/*------------------------------------------------
 * bcm2835_spi0_recv_byte()
 *
 *  Receive one byte by sending dummy data.
 *
 * param:  none
 * return: Data byte returned
 *
 */
int bcm2835_spi0_recv_byte(void)
{
    uint8_t byte = 0;

    bcm2835_spi0_transfer_Ex(&byte, &byte, 1);

    return byte;
}

/*------------------------------------------------
 * bcm2835_spi0_recv_byte()
 *
 *  Transmit a byte and return the received byte
 *
 * param:  Byte to send
 * return: Data byte returned
 *
 */
int bcm2835_spi0_transfer_byte(uint8_t tx_byte)
{
    uint8_t rx_byte;

    bcm2835_spi0_transfer_Ex(&tx_byte, &rx_byte, 1);

    return rx_byte;
}
