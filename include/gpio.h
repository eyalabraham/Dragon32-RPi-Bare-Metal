/*
 * gpio.h
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

#ifndef __GPIO_H__
#define __GPIO_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* -----------------------------------------
   GPIO register access functions
------------------------------------------*/
int      bcm2835_gpio_fsel(RPiGPIOPin_t pin, bcm2835FunctionSelect_t function);
void     bcm2835_gpio_set(int pin);
void     bcm2835_gpio_clr(int pin);
void     bcm2835_gpio_set_multi(uint32_t mask);
void     bcm2835_gpio_clr_multi(uint32_t mask);
int      bcm2835_gpio_lev(int pin);
uint32_t bcm2835_gpio_lev_multi(uint32_t mask);
int      bcm2835_gpio_eds(int pin);
uint32_t bcm2835_gpio_eds_multi(uint32_t mask);
void     bcm2835_gpio_clr_eds(int pin);
void     bcm2835_gpio_clr_eds_multi(uint32_t mask);
void     bcm2835_gpio_ren(int pin);
void     bcm2835_gpio_clr_ren(int pin);
void     bcm2835_gpio_fen(int pin);
void     bcm2835_gpio_clr_fen(int pin);
void     bcm2835_gpio_hen(int pin);
void     bcm2835_gpio_clr_hen(int pin);
void     bcm2835_gpio_len(int pin);
void     bcm2835_gpio_clr_len(int pin);
void     bcm2835_gpio_aren(int pin);
void     bcm2835_gpio_clr_aren(int pin);
void     bcm2835_gpio_afen(int pin);
void     bcm2835_gpio_clr_afen(int pin);
void     bcm2835_gpio_pud(bcm2835PUDControl_t pud);
void     bcm2835_gpio_set_pudclk(int pin);
void     bcm2835_gpio_clr_pudclk(int pin);
void     bcm2835_gpio_set_pud(int pin, bcm2835PUDControl_t pud);
void     bcm2835_gpio_write_mask(uint32_t value, uint32_t mask);

uint32_t bcm2835_core_clk(void);

void     bcm2835_crude_delay(uint32_t micro_second);

#endif  /* __GPIO_H__ */
