/*
 * auxuart.c
 *
 *  Driver library module for the Auxiliary UART (UART1)
 *  of the BCM2835 auxiliary peripherals.
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *      https://elinux.org/BCM2835_datasheet_errata
 *
 *  TODO Add hardware flow control
 *  TODO Add option for a transmit interrupt-driven Auxiliary-UART
 *
 */

#include    "gpio.h"
#include    "auxuart.h"
#include    "irq.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */

/* UART setup definitions
 */
#define     BCM2835_MINI_UART_ENA       0x00000001

#define     UART_8BIT                   0x00000003
#define     UART_7BIT                   0x00000002
#define     UART_TX_EMPTY               0x00000020      // Can accept at least one byte
#define     UART_RX_OVERRUN             0x00000002      // Rx over-run
#define     UART_RX_READY               0x00000001      // Data is available in the Rx FIFO
#define     UART_TX_ENA                 0x00000002      // Enable transmitter
#define     UART_RX_ENA                 0x00000001      // Enable receiver
#define     UART_RX_TX_ENA              (UART_TX_ENA | UART_RX_ENA)
#define     UART_RX_FIFO_CLR            0x00000002
#define     UART_TX_FIFO_CLR            0x00000004
#define     UART_RXTX_FIFO_CLR          (0x00000002 | 0x00000004)

#define     UART_RX_INT_ENA             0x00000001
#define     UART_TX_INT_ENA             0x00000002
#define     UART_IRQ_PEND               0x00000001

#define     SER_IN                      256             // Circular input buffer size

/* -----------------------------------------
   Types and data structures
----------------------------------------- */

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static void bcm2835_auxuart_isr(void);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static auxiliary_peripherals_regs_t *pAux   = (auxiliary_peripherals_regs_t*)BCM2835_AUX_BASE;
static auxiliary_uart1_regs_t       *pUART1 = (auxiliary_uart1_regs_t*)BCM2835_AUX_UART1;

static uint16_t  rx_timeout = 0;
static uint16_t  tx_timeout = 0;
static int       irq_enabled = 0;

/* Serial circular receive buffer
 */
volatile static uint8_t recv_buffer[SER_IN];
volatile static int     recv_cnt;
volatile static int     recv_wr_ptr;
volatile static int     recv_rd_ptr;

/*------------------------------------------------
 * bcm2835_auxuart_init()
 *
 *  Initialize the mini-UART
 *
 * param:  Baud rate, time outs in mili-seconds,
 *         and if to enable interrupts and/or hw flow control
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_auxuart_init(baud_t baud_rate_div, uint32_t rx_tout, uint32_t tx_tout, uint32_t configuration)
{
    /* Assign UART1 functions Tx/Rx to GPIO14/15 on pins 8/10
     * and remove default pull-down.
     * *** Cannot assign UART0 here, these pins are either UART1 or UART0!
     */
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_08, BCM2835_GPIO_FSEL_ALT5);
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_10, BCM2835_GPIO_FSEL_ALT5);

    bcm2835_gpio_set_pud(RPI_V2_GPIO_P1_08, BCM2835_GPIO_PUD_OFF);
    bcm2835_gpio_set_pud(RPI_V2_GPIO_P1_10, BCM2835_GPIO_PUD_OFF);

    /* Enable the UART1 device and flush FIFOs
     */
    pAux->aux_enables |= BCM2835_MINI_UART_ENA;
    dmb();

    pUART1->aux_mu_iir_reg = UART_RXTX_FIFO_CLR;

    /* Configure UART1 device
     */
    if ( configuration & AUXUART_7BIT )
        pUART1->aux_mu_lcr_reg |= UART_7BIT;
    else
        pUART1->aux_mu_lcr_reg |= UART_8BIT;

    pUART1->aux_mu_baud_reg = (uint32_t) baud_rate_div;
    pUART1->aux_mu_cntl_reg |= UART_RX_TX_ENA;

    rx_timeout = rx_tout * 1000;
    tx_timeout = tx_tout * 1000;

    /* Configure receive interrupts if required
     */
    if ( configuration & AUXUART_ENA_RX_IRQ )
    {
        recv_cnt = 0;
        recv_wr_ptr = 0;
        recv_rd_ptr = 0;

        /* Setup interrupt controller
         */
        irq_register_handler(IRQ_AUX_SERDEV, bcm2835_auxuart_isr);

        /* Enable UART interrupt
         */
        dmb();

        pUART1->aux_mu_ier_reg |= UART_RX_INT_ENA;

        dmb();

        irq_enable(IRQ_AUX_SERDEV);

        irq_enabled = 1;
    }

    return 1;
}

/*------------------------------------------------
 * bcm2835_auxuart_close()
 *
 *  Close auxiliary mini UART
 *
 * param:  none
 * return: none
 *
 */
void bcm2835_auxuart_close(void)
{
    /* Disable Tx and Rx and reset pins back to general input
     */
    pUART1->aux_mu_cntl_reg &= ~UART_RX_TX_ENA;

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_08, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_10, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(RPI_V2_GPIO_P1_08, BCM2835_GPIO_PUD_DOWN);
    bcm2835_gpio_set_pud(RPI_V2_GPIO_P1_10, BCM2835_GPIO_PUD_DOWN);

    /* Reset interrupt if required
     */
    if ( irq_enabled )
    {
        irq_disable(IRQ_AUX_SERDEV);

        dmb();

        pUART1->aux_mu_ier_reg &= ~UART_RX_INT_ENA;
    }
}

/*------------------------------------------------
 * bcm2835_auxuart_rx_data()
 *
 *  Read data from Rx circular buffer.
 *  *** Requires interrupt driven receiver ***
 *
 * param:  Destination buffer address and count to read
 * return: Byte count read
 *
 */
int bcm2835_auxuart_rx_data(uint8_t *buffer, int count)
{
    int     rxCount;
    int     i;

    if ( recv_cnt == 0 )
        return 0;

    /* figure out how many bytes to transfer
     */
    rxCount = (count < recv_cnt) ? count : recv_cnt;

    /* transfer data to caller's buffer
     */
    for ( i = 0; i < rxCount; i++ )
    {
        buffer[i] = recv_buffer[recv_rd_ptr++];

        if ( recv_rd_ptr == SER_IN )
            recv_rd_ptr = 0;

        recv_cnt--;
    }

    return rxCount;
}

/*------------------------------------------------
 * bcm2835_auxuart_rx_byte()
 *
 *  Read byte from Rx circular buffer.
 *  *** Requires interrupt driven receiver ***
 *
 * param:  Pointer to byte
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_auxuart_rx_byte(uint8_t *byte)
{
    if ( recv_cnt == 0 )
        return 0;

    *byte = recv_buffer[recv_rd_ptr++];

    if ( recv_rd_ptr == SER_IN )
        recv_rd_ptr = 0;

    recv_cnt--;

    return 1;
}

/*------------------------------------------------
 * bcm2835_auxuart_tx_data()
 *
 *  Send data from a buffer
 *
 * param:  Source buffer address and count to send
 * return: Byte count sent
 *
 */
int bcm2835_auxuart_tx_data(uint8_t *buffer, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        bcm2835_auxuart_putchr(buffer[i]);
    }

    /* TODO This return value might change once implementing Tx time-outs
     */
    return count;
}

/*------------------------------------------------
 * bcm2835_auxuart_putchr()
 *
 *  Send a character/byte to UART transmitter.
 *
 * param:  Character/byte to send
 * return: none
 *
 */
void bcm2835_auxuart_putchr(uint8_t byte)
{
    while ( !(pUART1->aux_mu_lsr_reg & UART_TX_EMPTY) )
    {
        /* Wait for Tx to be ready for byte */
        /* TODO Implement Tx time-out       */
    }

    dmb();

    pUART1->aux_mu_io_reg = (uint32_t) byte;
}

/*------------------------------------------------
 * bcm2835_auxuart_getchr()
 *
 *  Get a character/byte directly from UART receiver without
 *  checking if one is available; use bcm2835_muart_ischar()
 *
 * param:  none
 * return: Character/byte received
 *
 */
uint8_t bcm2835_auxuart_getchr(void)
{
    uint8_t     data_in;

    dmb();

    data_in = (uint8_t)(pUART1->aux_mu_io_reg & 0x000000ff);

    return data_in;
}

/*------------------------------------------------
 * bcm2835_auxuart_waitchr()
 *
 *  Wait (block) for character directly from UART receiver.
 *
 * param:  none
 * return: Character/byte received
 *
 */
uint8_t bcm2835_auxuart_waitchr(void)
{
    uint8_t     data_in;

    while ( !(pUART1->aux_mu_lsr_reg & UART_RX_READY) )
    {
        /* Wait for character to be available */
        /* TODO Implement Rx timeout          */
    }

    dmb();

    data_in = (uint8_t)(pUART1->aux_mu_io_reg & 0x000000ff);

    return data_in;
}

/*------------------------------------------------
 * bcm2835_auxuart_ischar()
 *
 *  Check if a character is present in the receive buffer
 *
 * param:  none
 * return: 1- character/byte available, 0- otherwise
 *
 */
int bcm2835_auxuart_ischar(void)
{
    int     ready;

    ready = pUART1->aux_mu_lsr_reg & UART_RX_READY;

    dmb();

    return ready;
}

/*------------------------------------------------
 * bcm2835_auxuart_isr()
 *
 *  Character/byte receiver interrupt handler.
 *  Assumes Auxiliary UART (UART1) is fully configured.
 *
 *  TODO Declare as 'naked' because registers are saved at the handler stub.
 *
 * param:  none
 * return: none
 *
 */
void bcm2835_auxuart_isr(void)
{
    uint8_t     byte;

    dmb();

    /* Handle the interrupt request
     */
    if ( (pAux->aux_irq & UART_IRQ_PEND) &&
         (pUART1->aux_mu_lsr_reg & UART_RX_READY) )
    {
        byte = (uint8_t)(pUART1->aux_mu_io_reg & 0x000000ff);
        if ( recv_cnt < SER_IN )
        {
            recv_buffer[recv_wr_ptr] = byte;
            recv_cnt++;
            recv_wr_ptr++;
            if ( recv_wr_ptr == SER_IN )
                recv_wr_ptr = 0;
        }
    }

    dmb();
}
