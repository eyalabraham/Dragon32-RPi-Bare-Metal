/*
 * timer.c
 *
 *  Module for the BCM2835 system timer.
 *  Does not prevent using ST_COMPARE0 or ST_COMPARE2 that are used by the GPU!
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 *  TODO Add option for timer interrupts
 */

#include    "bcm2835.h"
#include    "timer.h"
#include    "irq.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */

/* -----------------------------------------
   Types and data structures
----------------------------------------- */

/* -----------------------------------------
   Module static functions
----------------------------------------- */

/* -----------------------------------------
   Module globals
----------------------------------------- */
static system_timer_regs_t *pTimer = (system_timer_regs_t*)BCM2835_ST_BASE;

/*------------------------------------------------
 * bcm2835_st_read()
 *
 *  Read the System Timer Counter (low 32-bits)
 *
 * param:  none
 * return: Low 32-bit running counter
 *
 */
uint32_t bcm2835_st_read(void)
{
    uint32_t    timer_low;

    timer_low = pTimer->st_clo;
    dmb();

    return timer_low;
}

/*------------------------------------------------
 * bcm2835_st_delay()
 *
 *  Delays for the specified number of microseconds with
 *  1 to 4,294,967,295 (~71.5 minutes)
 *
 * param:  Microseconds to delay
 * return: none
 *
 */
void bcm2835_st_delay(uint32_t micros)
{
    uint32_t    now;

    if ( micros == 0 )
        return;

    now = bcm2835_st_read();

    while( (bcm2835_st_read() - now) < micros )
    {
        /* Wait for timer to expire.
         * This type of calculation and comparison to the delta
         * with unsigned math solved the 0-crossing
         * problem of the free running timer register.
         */
    }
}

/*------------------------------------------------
 * bcm2835_st_set_compare()
 *
 *  Setup timer compare register for a compare interval
 *  1 to 4,294,967,295 (~71.5 minutes)
 *
 * param:  Timer compare register ID, interval
 * return: 1- success, 0- otherwise
 *
 */
int bcm2835_st_set_compare(comp_reg_t compare_reg, uint32_t interval)
{
    uint32_t    next_compare_val;

    if ( interval == 0 )
        return 0;

    /* Update the timer compare match register
     */
    next_compare_val = pTimer->st_clo + interval;

    dmb();

    if ( compare_reg == ST_COMPARE1 )
    {
        pTimer->st_c1 = next_compare_val;
    }
    else             /* ST_COMPARE3 */
    {
        pTimer->st_c3 = next_compare_val;
    }

    return 1;
}

/*------------------------------------------------
 * bcm2835_st_is_compare_match()
 *
 *  Check by polling if a compare match has been achieved.
 *
 * param:  Timer compare register ID
 * return: 1- match, 0- otherwise
 *
 */
int bcm2835_st_is_compare_match(comp_reg_t compare_reg)
{
    int match;

    match = pTimer->st_cs & (1 << compare_reg);

    dmb();

    return (match ? 1 : 0);
}

/*------------------------------------------------
 * bcm2835_st_clr_compare_match()
 *
 *  Clear a compare match status.
 *
 * param:  Timer compare register ID
 * return: none
 *
 */
void bcm2835_st_clr_compare_match(comp_reg_t compare_reg)
{
    dmb();
    pTimer->st_cs = (1 << compare_reg);
}
