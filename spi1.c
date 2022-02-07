/*
 * spi1.c
 *
 *  Driver library module for the SPI1 interface (AUX SPI0) of the BCM2835.
 *  The SPI1 interface is available on RPi Zero 40-pin
 *  header and SPI2 is not available on the RPi header pins.
 *
 *  TODO Provide an option to configure interrupts.
 *  Default configuration:
 *  - Only CE2 is enabled on header P1 pin 36
 *  - Default clock rate 128KHZ
 *  - SPI Mode-0
 *  - MSB first
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus
 *
 */

#include    "gpio.h"
#include    "spi1.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */
/* Defines for SPI1
*/
#define     AUX_IRQ                     0x0000
#define     AUX_ENABLE                  0x0004

#define     AUX_ENABLE_SPI1             0x00000002

#define     AUX_SPI_CNTL0               0x0000
#define     AUX_SPI_CNTL1               0x0004
#define     AUX_SPI_STAT                0x0008
#define     AUX_SPI_PEEK                0x000C
#define     AUX_SPI_IO                  0x0020
#define     AUX_SPI_TXHOLD              0x0030

#define     AUX_SPI_CLOCK_MIN           30500       // Hz
#define     AUX_SPI_CLOCK_MAX           125000000   // Hz

#define     AUX_SPI_CNTL0_SPEED         0xFFF00000
#define     AUX_SPI_CNTL0_SPEED_SHIFT   20
#define     AUX_SPI_CNTL0_BYTE_SHIFT    8

#define     AUX_SPI_CNTL0_CS0_N         0x000C0000  // CS0 low
#define     AUX_SPI_CNTL0_CS1_N         0x000A0000  // CS1 low
#define     AUX_SPI_CNTL0_CS2_N         0x00060000  // CS2 low

#define     AUX_SPI_CNTL0_POSTINPUT     0x00010000
#define     AUX_SPI_CNTL0_VAR_CS        0x00008000
#define     AUX_SPI_CNTL0_VAR_WIDTH     0x00004000
#define     AUX_SPI_CNTL0_DOUTHOLD      0x00003000
#define     AUX_SPI_CNTL0_ENABLE        0x00000800
#define     AUX_SPI_CNTL0_CPHA_IN       0x00000400
#define     AUX_SPI_CNTL0_CLEARFIFO     0x00000200
#define     AUX_SPI_CNTL0_CPHA_OUT      0x00000100
#define     AUX_SPI_CNTL0_CPOL          0x00000080
#define     AUX_SPI_CNTL0_MSBF_OUT      0x00000040
#define     AUX_SPI_CNTL0_SHIFTLEN      0x0000003F

#define     AUX_SPI_CNTL1_CSHIGH        0x00000700
#define     AUX_SPI_CNTL1_IDLE          0x00000080
#define     AUX_SPI_CNTL1_TXEMPTY       0x00000040
#define     AUX_SPI_CNTL1_MSBF_IN       0x00000002
#define     AUX_SPI_CNTL1_KEEP_IN       0x00000001

#define     AUX_SPI_STAT_TX_LVL         0xF0000000
#define     AUX_SPI_STAT_RX_LVL         0x00F00000
#define     AUX_SPI_STAT_TX_FULL        0x00000400
#define     AUX_SPI_STAT_TX_EMPTY       0x00000200
#define     AUX_SPI_STAT_RX_FULL        0x00000100
#define     AUX_SPI_STAT_RX_EMPTY       0x00000080
#define     AUX_SPI_STAT_BUSY           0x00000040
#define     AUX_SPI_STAT_BITCOUNT       0x0000003F

#define     AUX_SPI_MIN_RATE            32000       // Hz
#define     AUX_SPI_MAX_RATE            10000000    // Hz

/* -----------------------------------------
   Types and data structures
----------------------------------------- */

/* -----------------------------------------
   Module static functions
----------------------------------------- */

/* -----------------------------------------
   Module globals
----------------------------------------- */
static auxiliary_peripherals_regs_t *pAux = (auxiliary_peripherals_regs_t*)BCM2835_AUX_BASE;
static auxiliary_spi1_regs_t        *pSPI = (auxiliary_spi1_regs_t*)BCM2835_AUX_SPI1;

/*------------------------------------------------
 * bcm2835_spi1_init()
 *
 *  Initialize the SPI1 interface.
 *  Start SPI operations with RPi SPI1 pins P1-38 (MOSI),
 *  P1-35 (MISO), P1-40 (CLK), P1-12 (CE0), and P1-36 (CE2).
 *
 * param:  Configuration bits
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_spi1_init(uint32_t configuration)
{
    uint32_t    spi_config0 = 0x00000000;
    uint32_t    spi_config1 = 0x00000000;
    uint32_t    system_clock;
    uint32_t    spi_rate_div;

    /* Get core frequency and dynamically calculate rate divisor
     */
    if ( !(system_clock = bcm2835_core_clk()) )
        return 0;

    spi_rate_div = ((system_clock / SPI1_DEFAULT_RATE) / 2) - 1;

    /* Set the SPI1 pins to the Alt-4 function to enable SPI1 access
     */
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_35, BCM2835_GPIO_FSEL_ALT4);  // MISO
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_38, BCM2835_GPIO_FSEL_ALT4);  // MOSI
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_40, BCM2835_GPIO_FSEL_ALT4);  // CLK
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_36, BCM2835_GPIO_FSEL_ALT4);  // CE2
/* Not using CE0 and CE2
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_11, BCM2835_GPIO_FSEL_ALT4);  // CE1
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_12, BCM2835_GPIO_FSEL_ALT4);  // CE0
*/

    /* Compile configuration from configuration bits
     * and apply to device
     */
    spi_config0 = (spi_rate_div << AUX_SPI_CNTL0_SPEED_SHIFT) |
                  AUX_SPI_CNTL0_CS2_N |
                  AUX_SPI_CNTL0_ENABLE |
                  AUX_SPI_CNTL0_CPHA_IN |
                  AUX_SPI_CNTL0_MSBF_OUT |
                  AUX_SPI_CNTL0_BYTE_SHIFT;

    spi_config1 = AUX_SPI_CNTL1_MSBF_IN;

    dmb();

    pAux->aux_enables |= AUX_ENABLE_SPI1;
    pSPI->aux_spi1_cntl0_reg = spi_config0;
    pSPI->aux_spi1_cntl1_reg = spi_config1;

    return 1;
}


/*------------------------------------------------
 * bcm2835_spi1_close()
 *
 *  Return SPI GPIO pins to default input.
 *
 * param:  none
 * return: none
 *
 */
void bcm2835_spi1_close(void)
{
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_35, BCM2835_GPIO_FSEL_INPT);  // MISO
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_38, BCM2835_GPIO_FSEL_INPT);  // MOSI
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_40, BCM2835_GPIO_FSEL_INPT);  // CLK
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_36, BCM2835_GPIO_FSEL_INPT);  // CE2
/* Not using CE0 and CE2
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_11, BCM2835_GPIO_FSEL_INPT);  // CE1
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_12, BCM2835_GPIO_FSEL_INPT);  // CE0
*/
    dmb();

    pAux->aux_enables &= ~AUX_ENABLE_SPI1;
}

/*------------------------------------------------
 * bcm2835_spi1_set_rate()
 *
 *  Set the SPI data transfer rate.
 *
 * param:  Transfer rate in Hz
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_spi1_set_rate(uint32_t data_rate)
{
    uint32_t    spi_config0;
    uint32_t    system_clock;
    uint32_t    spi_rate_div;

    /* Limit check
     */
    if ( data_rate > AUX_SPI_MAX_RATE )
        data_rate = AUX_SPI_MAX_RATE;
    else if ( data_rate < AUX_SPI_MIN_RATE )
        data_rate = AUX_SPI_MIN_RATE;

    /* Get core frequency and dynamically calculate rate divisor
     */
    if ( !(system_clock = bcm2835_core_clk()) )
        return 0;

    spi_rate_div = ((system_clock / data_rate) / 2) - 1;

    dmb();
    spi_config0 = pSPI->aux_spi1_cntl0_reg;
    spi_config0 &= ~AUX_SPI_CNTL0_SPEED;
    spi_config0 |= (spi_rate_div << AUX_SPI_CNTL0_SPEED_SHIFT);
    pSPI->aux_spi1_cntl0_reg = spi_config0;

    return 1;
}

/*------------------------------------------------
 * bcm2835_spi1_clk_mode()
 *
 *  Set the SPI clock mode (CPOL/CPHA).
 *
 * param:  Mode
 * return: none
 *
 */
void bcm2835_spi1_clk_mode(spi1_mode_t mode)
{
}

/*------------------------------------------------
 * bcm2835_spi1_cs()
 *
 *  Set the SPI chip select to assert during a transfer.
 *
 * param:  Chip select
 * return: none
 *
 */
void bcm2835_spi1_cs(spi1_chip_sel_t cs)
{
}

/*------------------------------------------------
 * bcm2835_spi1_cs_polarity()
 *
 *  Set the SPI chip select polarity HIGH or LOW, during a transfer.
 *
 * param:  Chip select and polarity
 * return: none
 *
 */
void bcm2835_spi1_cs_polarity(spi1_chip_sel_t cs, int level)
{
}

/*------------------------------------------------
 * bcm2835_spi1_transfer_Ex()
 *
 *  Transfers one or more bytes to and from the currently selected SPI device.
 *  Asserts the currently selected CS pin during the transfer.
 *  Clocks 'count' 8-bit values out on MOSI, and simultaneously clocks in data from MISO.
 *  If rx_buf != NULL returns the read data byte(s) from the device.
 *  Uses polled transfer as per section 2.3 of the BCM 2835 ARM Peripherals manual.
 *  *** 'tx_buf' and 'rx_buf' must be of same 'count' size.
 *
 * param:  Transmit data buffer, receive data buffer and byte count
 * return: none
 *
 */
void bcm2835_spi1_transfer_Ex(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count)
{
    uint32_t tx_count = 0;
    uint32_t rx_count = 0;
    uint8_t  byte;

    pSPI->aux_spi1_cntl0_reg |= AUX_SPI_CNTL0_CLEARFIFO;

    dmb();

    pSPI->aux_spi1_cntl0_reg &= ~AUX_SPI_CNTL0_CLEARFIFO;

    dmb();

    while ( (tx_count < count) || (rx_count < count) )
    {
        /* Tx FIFO not full, so add some more bytes
         */
        while ( !(pSPI->aux_spi1_stat_reg & AUX_SPI_STAT_TX_FULL) && (tx_count < count ) )
        {
            dmb();
            pSPI->aux_spi1_io_reg = tx_buf[tx_count] << 24;
            tx_count++;
        }

        /* Rx FIFO not empty, so get the next received bytes
         * if caller provided a destination buffer, otherwise drop bytes
         */
        while ( !(pSPI->aux_spi1_stat_reg & AUX_SPI_STAT_RX_EMPTY) && (rx_count < count) )
        {
            dmb();
            byte = pSPI->aux_spi1_io_reg;
            if ( rx_buf )
            {
                rx_buf[rx_count] = byte;
            }
            rx_count++;
        }
    }
}

/*------------------------------------------------
 * bcm2835_spi1_send_byte()
 *
 *  Transmit one byte drop the return byte.
 *
 * param:  Byte to send
 * return: none
 *
 */
void bcm2835_spi1_send_byte(uint8_t byte)
{
    bcm2835_spi1_transfer_Ex(&byte, 0, 1);
}

/*------------------------------------------------
 * bcm2835_spi1_recv_byte()
 *
 *  Receive one byte by sending dummy data.
 *
 * param:  none
 * return: Data byte returned
 *
 */
int bcm2835_spi1_recv_byte(void)
{
    uint8_t byte = 0xff;

    bcm2835_spi1_transfer_Ex(&byte, &byte, 1);

    return byte;
}

/*------------------------------------------------
 * bcm2835_spi1_recv_byte()
 *
 *  Transmit a byte and return the received byte
 *
 * param:  Byte to send
 * return: Data byte returned
 *
 */
int bcm2835_spi1_transfer_byte(uint8_t tx_byte)
{
    uint8_t rx_byte;

    bcm2835_spi1_transfer_Ex(&tx_byte, &rx_byte, 1);

    return rx_byte;
}
