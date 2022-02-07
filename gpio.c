/*
 * gpio.c
 *
 *  General purpose IO pin configuration and access.
 *
 *  It is based on the Broadcom BCM 2835 C library by Mike McCauley.
 *  The library was separated into functional groups and modified to
 *  be used in a bare metal environment.
 *
 *   Resources:
 *      http://www.airspayce.com/mikem/bcm2835/index.html
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#include    "gpio.h"
#include    "mailbox.h"

/* -----------------------------------------
   Module globals
----------------------------------------- */
static gpio_pin_regs_t *bcm2835_gpio = (gpio_pin_regs_t*)BCM2835_GPIO_BASE;

/*------------------------------------------------
 * bcm2835_gpio_fsel()
 *
 *  Function select pin is a BCM2835 GPIO pin number NOT RPi pin number
 *  There are 6 control registers, each control the functions of a block of 10 pins.
 *  Each control register has 10 sets of 3 bits per GPIO pin:
 *
 *  000 = GPIO Pin X is an input
 *  001 = GPIO Pin X is an output
 *  100 = GPIO Pin X takes alternate function 0
 *  101 = GPIO Pin X takes alternate function 1
 *  110 = GPIO Pin X takes alternate function 2
 *  111 = GPIO Pin X takes alternate function 3
 *  011 = GPIO Pin X takes alternate function 4
 *  010 = GPIO Pin X takes alternate function 5
 *
 *  The 3 bits for port X are: X / 10 + ((X % 10) * 3)
 *
 * param:  Pin number and function
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_gpio_fsel(RPiGPIOPin_t pin, bcm2835FunctionSelect_t function)
{
    int         shift;
    uint32_t    mask, value;

    shift = (pin % 10) * 3;
    mask = BCM2835_GPIO_FSEL_MASK << shift;
    value = function << shift;

    dmb();

    if ( pin < 10 )
    {
        bcm2835_gpio->gpfsel0 = (bcm2835_gpio->gpfsel0 & ~mask) | value;
    }
    else if ( pin < 20 )
    {
        bcm2835_gpio->gpfsel1 = (bcm2835_gpio->gpfsel1 & ~mask) | value;
    }
    else if ( pin < 30 )
    {
        bcm2835_gpio->gpfsel2 = (bcm2835_gpio->gpfsel2 & ~mask) | value;
    }
    else if ( pin < 40 )
    {
        bcm2835_gpio->gpfsel3 = (bcm2835_gpio->gpfsel3 & ~mask) | value;
    }
    else if ( pin < 50 )
    {
        bcm2835_gpio->gpfsel4 = (bcm2835_gpio->gpfsel4 & ~mask) | value;
    }
    else
        /* GPIO above #49 are not accessible */
        return 0;

    dmb();

    return 1;
}

/*------------------------------------------------
 * bcm2835_gpio_set()
 *
 *  Set output pin
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_set(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpset0 = (1 << shift);
    else
        bcm2835_gpio->gpset1 = (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr()
 *
 *  Clear output pin
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpclr0 = (1 << shift);
    else
        bcm2835_gpio->gpclr1 = (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_set_multi()
 *
 *  Set all output pins in the mask, only for GPIO 0 to 31.
 *
 * param:  Bit mask to set
 * return: none
 *
 */
void bcm2835_gpio_set_multi(uint32_t mask)
{
    dmb();
    bcm2835_gpio->gpset0 = mask;
}

/*------------------------------------------------
 * bcm2835_gpio_set_multi()
 *
 *  Clear all output pins in the mask, only for GPIO 0 to 31.
 *
 * param:  Bit mask to clear
 * return: none
 *
 */
void bcm2835_gpio_clr_multi(uint32_t mask)
{
    dmb();
    bcm2835_gpio->gpclr0 = mask;
}

/*------------------------------------------------
 * bcm2835_gpio_lev()
 *
 *  Read input pin
 *
 * param:  Pin number
 * return: Pin level
 *
 */
int bcm2835_gpio_lev(int pin)
{
    int      shift;
    uint32_t value;

    shift = pin % 32;

    if ( pin < 32 )
        value = bcm2835_gpio->gplev0;
    else
        value = bcm2835_gpio->gplev1;

    dmb();

    shift = pin % 32;

    return (value & (1 << shift)) ? HIGH : LOW;
}

/*------------------------------------------------
 * bcm2835_gpio_lev_multi()
 *
 *  Read input pins with mask, only for GPIO 0 to 31.
 *
 * param:  Pin levels mask
 * return: Pin levels
 *
 */
uint32_t bcm2835_gpio_lev_multi(uint32_t mask)
{
    uint32_t value;

    value = bcm2835_gpio->gplev0 & mask;
    dmb();

    return value;
}

/*------------------------------------------------
 * bcm2835_gpio_eds()
 *
 *  Check if an event detection bit is set
 *  TODO support interrupts
 *
 * param:  Pin number
 * return: Event detection level
 *
 */
int bcm2835_gpio_eds(int pin)
{
    int      shift;
    uint32_t value;

    shift = pin % 32;

    if ( pin < 32 )
        value = bcm2835_gpio->gpeds0;
    else
        value = bcm2835_gpio->gpeds1;

    dmb();

    shift = pin % 32;

    return (value & (1 << shift)) ? HIGH : LOW;
}

/*------------------------------------------------
 * bcm2835_gpio_eds_multi()
 *
 *  Check if an event detection bits are set, only for GPIO 0 to 31.
 *  TODO support interrupts
 *
 * param:  Detection bit mask
 * return: Event detection levels
 *
 */
uint32_t bcm2835_gpio_eds_multi(uint32_t mask)
{
    uint32_t value;

    value = bcm2835_gpio->gpeds0 & mask;
    dmb();

    return value;
}

/*------------------------------------------------
 * bcm2835_gpio_clear_eds()
 *
 *   Write a 1 to clear the bit in EDS
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_eds(int pin)
{
    int      shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpeds0 |= (1 << shift);
    else
        bcm2835_gpio->gpeds0 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clear_eds_multi()
 *
 *   Multiple clear bits in EDS, only for GPIO 0 to 31.
 *
 * param:  EDS bit mask
 * return: none
 *
 */
void bcm2835_gpio_clr_eds_multi(uint32_t mask)
{
    dmb();
    bcm2835_gpio->gpeds0 |= mask;
    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_ren()
 *
 *   Rising edge detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_ren(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpren0 |= (1 << shift);
    else
        bcm2835_gpio->gpren1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_ren()
 *
 *   Rising edge detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_ren(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpren0 &= ~(1 << shift);
    else
        bcm2835_gpio->gpren1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_fen()
 *
 *   Falling edge detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_fen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpfen0 |= (1 << shift);
    else
        bcm2835_gpio->gpfen1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_fen()
 *
 *   Falling edge detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_fen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpfen0 &= ~(1 << shift);
    else
        bcm2835_gpio->gpfen1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_hen()
 *
 *   High detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_hen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gphen0 |= (1 << shift);
    else
        bcm2835_gpio->gphen1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_hen()
 *
 *   High detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_hen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gphen0 &= ~(1 << shift);
    else
        bcm2835_gpio->gphen1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_len()
 *
 *   Low detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_len(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gplen0 |= (1 << shift);
    else
        bcm2835_gpio->gplen1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_len()
 *
 *   Low detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_len(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gplen0 &= ~(1 << shift);
    else
        bcm2835_gpio->gplen1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_aren()
 *
 *   Asynchronous rising edge detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_aren(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gparen0 |= (1 << shift);
    else
        bcm2835_gpio->gparen1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_aren()
 *
 *   Asynchronous rising edge detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_aren(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gparen0 &= ~(1 << shift);
    else
        bcm2835_gpio->gparen1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_afen()
 *
 *   Asynchronous falling edge detect enable
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_afen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpafen0 |= (1 << shift);
    else
        bcm2835_gpio->gpafen1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_afen()
 *
 *   Asynchronous falling edge detect disable (clear)
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_afen(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gpafen0 &= ~(1 << shift);
    else
        bcm2835_gpio->gpafen1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_pud()
 *
 *   Set all pull up/down resistors
 *
 * param:  Pull-up/down state
 * return: none
 *
 */
void bcm2835_gpio_pud(bcm2835PUDControl_t pud)
{
    dmb();
    bcm2835_gpio->gppud = (uint32_t)pud;
}

/*------------------------------------------------
 * bcm2835_gpio_pudclk()
 *
 *   Pull up/down clock
 *   Clocks the value of pud into the GPIO pin
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_set_pudclk(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gppudclk0 |= (1 << shift);
    else
        bcm2835_gpio->gppudclk1 |= (1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_clr_pudclk()
 *
 *   Pull up/down clock
 *   Clear the pull-up/down clock
 *
 * param:  Pin number
 * return: none
 *
 */
void bcm2835_gpio_clr_pudclk(int pin)
{
    int     shift;

    shift = pin % 32;

    dmb();

    if ( pin < 32 )
        bcm2835_gpio->gppudclk0 &= ~(1 << shift);
    else
        bcm2835_gpio->gppudclk1 &= ~(1 << shift);

    dmb();
}

/*------------------------------------------------
 * bcm2835_gpio_set_pud()
 *
 *   Set the pull up/down resistor for a pin
 *
 *   The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
 *   the respective GPIO pins. These registers must be used in conjunction with the GPPUD
 *   register to effect GPIO Pull-up/down changes. The following sequence of events is
 *   required:
 *   1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
 *      to remove the current Pull-up/down)
 *   2. Wait 150 cycles ? this provides the required set-up time for the control signal
 *   3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
 *      modify, and only the pads which receive a clock will be modified, all others will
 *      retain their previous state.
 *   4. Wait 150 cycles ? this provides the required hold time for the control signal
 *   5. XXX Write to GPPUD to remove the control signal
 *   6. Write to GPPUDCLK0/1 to remove the clock
 *
 *   RPi has P1-03 and P1-05 with 1k8 pull-up resistor
 *
 * param:  Pin number and desired state
 * return: none
 *
 */
void bcm2835_gpio_set_pud(int pin, bcm2835PUDControl_t pud)
{
    bcm2835_gpio_pud(pud);

    bcm2835_crude_delay(150);
    bcm2835_gpio_set_pudclk(pin);

    bcm2835_crude_delay(150);
    bcm2835_gpio_clr_pudclk(pin);
}

/*------------------------------------------------
 * bcm2835_gpio_write_mask()
 *
 *  Sets the first 32 GPIO output pins specified
 *  in the mask to the value given by value.
 *
 * param:  Value values required for each bit masked in by mask,
 *         for example: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
 *         Mask of pins to affect. for example: (1 << RPI_GPIO_P1_03) | (1 << RPI_GPIO_P1_05)
 * return: none
 *
 */
void bcm2835_gpio_write_mask(uint32_t value, uint32_t mask)
{
    bcm2835_gpio_set_multi(value & mask);
    bcm2835_gpio_clr_multi((~value) & mask);
}

/*------------------------------------------------
 * bcm2835_core_clk()
 *
 *  Return the core clock frequency in Hz
 *
 * param:  none
 * return: core clock frequency in Hz, 0=failed
 *
 */
uint32_t bcm2835_core_clk(void)
{
    mailbox_tag_property_t *mp;

    bcm2835_mailbox_init();
    bcm2835_mailbox_add_tag(TAG_GET_CLOCK_RATE, TAG_CLOCK_CORE);

    if ( !bcm2835_mailbox_process() )
        return 0;

    mp = bcm2835_mailbox_get_property(TAG_GET_CLOCK_RATE);

    return mp->values.fb_alloc.param2;
}

/*------------------------------------------------
 * bcm2835_crude_delay()
 *
 *  Crude delay based on incrementing loop
 *  *** Large error >10% measured on scope ***
 *
 * param:  Micro seconds
 * return: none
 *
 */
void bcm2835_crude_delay(uint32_t micro_second)
{
    volatile static uint32_t loop;

    for ( loop = 0; loop < (micro_second * 5); loop++ )
        /* Do nothing */;
}

