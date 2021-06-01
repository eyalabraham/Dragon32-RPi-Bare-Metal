/*
 * timer.h
 *
 *  Header file for the BCM2835 system timer.
 *
 *   Resources:
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#ifndef __TIMERT_H__
#define __TIMERT_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* System Timer compare register IDs
 */
typedef enum
{
    ST_COMPARE0 = 0,
    ST_COMPARE1 = 1,
    ST_COMPARE2 = 2,
    ST_COMPARE3 = 3,
} comp_reg_t;


uint32_t bcm2835_st_read(void);                                             // Read the System Timer Counter register.
void     bcm2835_st_delay(uint32_t micros);                                 // Micro second delay
int      bcm2835_st_set_compare(comp_reg_t compare_reg, uint32_t interval); // Setup timer compare
int      bcm2835_st_is_compare_match(comp_reg_t compare_reg);               // Check for compare match
void     bcm2835_st_clr_compare_match(comp_reg_t compare_reg);              // Clear compare match status/interrupt

#endif  /* __TIMERT_H__ */
