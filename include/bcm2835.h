/*
 * bcm2835.h
 *
 *  This library module provides BCM2835 Library initialization and
 *  low level register access. It is based on the Broadcom BCM 2835
 *  C library by Mike McCauley.
 *  The library was separated into functional groups and modified to
 *  be used in a bare metal environment.
 *
 *   Resources:
 *      http://www.airspayce.com/mikem/bcm2835/index.html
 *      https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 *
 */

#ifndef __BCM2835_H__
#define __BCM2835_H__

#include    <stdint.h>

/* -----------------------------------------------------
 *      ARMv6 memory barrier
 */

#ifndef barrierdefs
    #define barrierdefs
    //RPi is ARMv6. It does not have ARMv7 DMB/DSB/ISB, so go through CP15.
    #define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7,  c5, 4" : : "r" (0) : "memory")
    //use dmb() around the accesses. ("before write, after read")
    #define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
    //ARMv6 DSB (DataSynchronizationBarrier): also known as DWB (drain write buffer / data write barrier) on ARMv5
    #define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#endif

/* Pin HIGH, true, 3.3 volts on a pin.
   or pin LOW, false, 0volts on a pin.
 */
#define     HIGH                        0x1
#define     LOW                         0x0

/* -----------------------------------------------------
 *      Register bank base addresses
 */
#define     BCM2835_PERI_BASE           0x20000000      // RPi 1 and Zero
#define     BCM2835_RPI2_PERI_BASE      0x3F000000      // RPi 2 & 3
#define     BCM2835_RPI4_PERI_BASE      0xFE000000      // RPi 4

#define     BCM2835_ST_BASE             (BCM2835_PERI_BASE+0x003000)    // System Timer
#define     BCM2835_INT_BASE            (BCM2835_PERI_BASE+0x00B200)    // Interrupt controller
#define     BCM2835_MAILBOX0_BASE       (BCM2835_PERI_BASE+0x00B880 )   // Mailbox-0
// TODO #define     BCM2835_CLOCK_BASE          (BCM2835_PERI_BASE+0x101000)    // Clock/timer
#define     BCM2835_GPIO_BASE           (BCM2835_PERI_BASE+0x200000)    // GPIO
#define     BCM2835_SPI0_BASE           (BCM2835_PERI_BASE+0x204000)    // SPI0
// TODO #define     BCM2835_BSC0_BASE           (BCM2835_PERI_BASE+0x205000)    // BSC0 I2C
// TODO #define     BCM2835_GPIO_PWM            (BCM2835_PERI_BASE+0x20C000)    // PWM
#define     BCM2835_AUX_BASE            (BCM2835_PERI_BASE+0x215000)    // AUX peripherals' base
#define     BCM2835_AUX_UART1           (BCM2835_PERI_BASE+0x215040)    // AUX peripherals' Mini-UART base
#define     BCM2835_AUX_SPI1            (BCM2835_PERI_BASE+0x215080)    // AUX peripherals' SPI1 base
// TODO #define     BCM2835_BSC1_BASE           (BCM2835_PERI_BASE+0x804000)    // BSC1 I2C

/* -----------------------------------------------------
 *      Speed of the core clock core_clk
 */
#define BCM2835_CORE_CLK_HZ     250000000   // 250 MHz

/* -----------------------------------------------------
 *      System Timer
 */
typedef struct
{
    volatile uint32_t   st_cs;
    volatile uint32_t   st_clo;
    volatile uint32_t   st_chi;
    volatile uint32_t   st_c0;
    volatile uint32_t   st_c1;
    volatile uint32_t   st_c2;
    volatile uint32_t   st_c3;
} system_timer_regs_t;

/* -----------------------------------------------------
 *      Interrupt controller
 */
typedef struct
{
    volatile uint32_t   ic_basic_pending;
    volatile uint32_t   ic_pending1;
    volatile uint32_t   ic_pending2;
    volatile uint32_t   ic_FIQ_control;
    volatile uint32_t   ic_enable1;
    volatile uint32_t   ic_enable2;
    volatile uint32_t   ic_basic_enable;
    volatile uint32_t   ic_disable1;
    volatile uint32_t   ic_disable2;
    volatile uint32_t   ic_basic_disable;
} interrupt_controller_regs_t;

/* -----------------------------------------------------
 *      Mailbox
 */
typedef struct
{
    volatile uint32_t   read;
    volatile uint32_t   reserved1[((0x90 - 0x80) / 4) - 1];
    volatile uint32_t   poll;
    volatile uint32_t   sender;
    volatile uint32_t   status;
    volatile uint32_t   configuration;
    volatile uint32_t   write;
} mailbox_t;

/* -----------------------------------------------------
 *      GPIO pins
 */
typedef struct
{
    volatile uint32_t   gpfsel0;
    volatile uint32_t   gpfsel1;
    volatile uint32_t   gpfsel2;
    volatile uint32_t   gpfsel3;
    volatile uint32_t   gpfsel4;
    volatile uint32_t   gpfsel5;
    volatile uint32_t   reserved0;
    volatile uint32_t   gpset0;
    volatile uint32_t   gpset1;
    volatile uint32_t   reserved1;
    volatile uint32_t   gpclr0;
    volatile uint32_t   gpclr1;
    volatile uint32_t   reserved2;
    volatile uint32_t   gplev0;
    volatile uint32_t   gplev1;
    volatile uint32_t   reserved3;
    volatile uint32_t   gpeds0;
    volatile uint32_t   gpeds1;
    volatile uint32_t   reserved4;
    volatile uint32_t   gpren0;
    volatile uint32_t   gpren1;
    volatile uint32_t   reserved5;
    volatile uint32_t   gpfen0;
    volatile uint32_t   gpfen1;
    volatile uint32_t   reserved6;
    volatile uint32_t   gphen0;
    volatile uint32_t   gphen1;
    volatile uint32_t   reserved7;
    volatile uint32_t   gplen0;
    volatile uint32_t   gplen1;
    volatile uint32_t   reserved8;
    volatile uint32_t   gparen0;
    volatile uint32_t   gparen1;
    volatile uint32_t   reserved9;
    volatile uint32_t   gpafen0;
    volatile uint32_t   gpafen1;
    volatile uint32_t   reserved10;
    volatile uint32_t   gppud;
    volatile uint32_t   gppudclk0;
    volatile uint32_t   gppudclk1;
    volatile uint32_t   reserved11;
    volatile uint32_t   test;
} gpio_pin_regs_t;

/* Port function select modes for bcm2835_gpio_fsel()
 */
typedef enum
{
    BCM2835_GPIO_FSEL_INPT  = 0x00,     // Input 0b000
    BCM2835_GPIO_FSEL_OUTP  = 0x01,     // Output 0b001
    BCM2835_GPIO_FSEL_ALT0  = 0x04,     // Alternate function 0 0b100
    BCM2835_GPIO_FSEL_ALT1  = 0x05,     // Alternate function 1 0b101
    BCM2835_GPIO_FSEL_ALT2  = 0x06,     // Alternate function 2 0b110
    BCM2835_GPIO_FSEL_ALT3  = 0x07,     // Alternate function 3 0b111
    BCM2835_GPIO_FSEL_ALT4  = 0x03,     // Alternate function 4 0b011
    BCM2835_GPIO_FSEL_ALT5  = 0x02,     // Alternate function 5 0b010
    BCM2835_GPIO_FSEL_MASK  = 0x07      // Function select bits mask 0b111
} bcm2835FunctionSelect_t;

/* Raspberry Pin GPIO pins on P1 in terms of the underlying BCM GPIO pin numbers.
   These can be passed as a pin number to any function requiring a pin.
   Not all pins on the RPi 26 bin plug are connected to GPIO pins.
   Some can adopt an alternate function, see BCM2835 Peripherals document.
   RPi version 2 has some slightly different pints, and these are values RPI_V2_*.
   RPi B+ has yet different pints and these are defined in RPI_BPLUS_*.
 */
typedef enum
{
    /* Activity LEDs
     */
    RPIB_ACT_LED          = 16,
    RPIZ_ACT_LED          = 47,

    /* RPi P1 pin assignments
     */
    RPI_GPIO_P1_03        =  0,         // Version 1, Pin P1-03
    RPI_GPIO_P1_05        =  1,         // Version 1, Pin P1-05
    RPI_GPIO_P1_07        =  4,         // Version 1, Pin P1-07
    RPI_GPIO_P1_08        = 14,         // Version 1, Pin P1-08
    RPI_GPIO_P1_10        = 15,         // Version 1, Pin P1-10
    RPI_GPIO_P1_11        = 17,         // Version 1, Pin P1-11
    RPI_GPIO_P1_12        = 18,         // Version 1, Pin P1-12
    RPI_GPIO_P1_13        = 21,         // Version 1, Pin P1-13
    RPI_GPIO_P1_15        = 22,         // Version 1, Pin P1-15
    RPI_GPIO_P1_16        = 23,         // Version 1, Pin P1-16
    RPI_GPIO_P1_18        = 24,         // Version 1, Pin P1-18
    RPI_GPIO_P1_19        = 10,         // Version 1, Pin P1-19
    RPI_GPIO_P1_21        =  9,         // Version 1, Pin P1-21
    RPI_GPIO_P1_22        = 25,         // Version 1, Pin P1-22
    RPI_GPIO_P1_23        = 11,         // Version 1, Pin P1-23
    RPI_GPIO_P1_24        =  8,         // Version 1, Pin P1-24
    RPI_GPIO_P1_26        =  7,         // Version 1, Pin P1-26

    RPI_V2_GPIO_P1_03     =  2,         // Version 2, Pin P1-03
    RPI_V2_GPIO_P1_05     =  3,         // Version 2, Pin P1-05
    RPI_V2_GPIO_P1_07     =  4,         // Version 2, Pin P1-07
    RPI_V2_GPIO_P1_08     = 14,         // Version 2, Pin P1-08
    RPI_V2_GPIO_P1_10     = 15,         // Version 2, Pin P1-10
    RPI_V2_GPIO_P1_11     = 17,         // Version 2, Pin P1-11
    RPI_V2_GPIO_P1_12     = 18,         // Version 2, Pin P1-12
    RPI_V2_GPIO_P1_13     = 27,         // Version 2, Pin P1-13
    RPI_V2_GPIO_P1_15     = 22,         // Version 2, Pin P1-15
    RPI_V2_GPIO_P1_16     = 23,         // Version 2, Pin P1-16
    RPI_V2_GPIO_P1_18     = 24,         // Version 2, Pin P1-18
    RPI_V2_GPIO_P1_19     = 10,         // Version 2, Pin P1-19
    RPI_V2_GPIO_P1_21     =  9,         // Version 2, Pin P1-21
    RPI_V2_GPIO_P1_22     = 25,         // Version 2, Pin P1-22
    RPI_V2_GPIO_P1_23     = 11,         // Version 2, Pin P1-23
    RPI_V2_GPIO_P1_24     =  8,         // Version 2, Pin P1-24
    RPI_V2_GPIO_P1_26     =  7,         // Version 2, Pin P1-26
    RPI_V2_GPIO_P1_29     =  5,         // Version 2, Pin P1-29
    RPI_V2_GPIO_P1_31     =  6,         // Version 2, Pin P1-31
    RPI_V2_GPIO_P1_32     = 12,         // Version 2, Pin P1-32
    RPI_V2_GPIO_P1_33     = 13,         // Version 2, Pin P1-33
    RPI_V2_GPIO_P1_35     = 19,         // Version 2, Pin P1-35
    RPI_V2_GPIO_P1_36     = 16,         // Version 2, Pin P1-36
    RPI_V2_GPIO_P1_37     = 26,         // Version 2, Pin P1-37
    RPI_V2_GPIO_P1_38     = 20,         // Version 2, Pin P1-38
    RPI_V2_GPIO_P1_40     = 21,         // Version 2, Pin P1-40

    /* RPi Version 2, new plug P5
     */
    RPI_V2_GPIO_P5_03     = 28,         // Version 2, Pin P5-03
    RPI_V2_GPIO_P5_04     = 29,         // Version 2, Pin P5-04
    RPI_V2_GPIO_P5_05     = 30,         // Version 2, Pin P5-05
    RPI_V2_GPIO_P5_06     = 31,         // Version 2, Pin P5-06

    /* XXX RPi B+ J8 header, also RPi 2 40 pin GPIO header
     */
    RPI_BPLUS_GPIO_J8_03     =  2,      // B+, Pin J8-03
    RPI_BPLUS_GPIO_J8_05     =  3,      // B+, Pin J8-05
    RPI_BPLUS_GPIO_J8_07     =  4,      // B+, Pin J8-07
    RPI_BPLUS_GPIO_J8_08     = 14,      // B+, Pin J8-08
    RPI_BPLUS_GPIO_J8_10     = 15,      // B+, Pin J8-10
    RPI_BPLUS_GPIO_J8_11     = 17,      // B+, Pin J8-11
    RPI_BPLUS_GPIO_J8_12     = 18,      // B+, Pin J8-12
    RPI_BPLUS_GPIO_J8_13     = 27,      // B+, Pin J8-13
    RPI_BPLUS_GPIO_J8_15     = 22,      // B+, Pin J8-15
    RPI_BPLUS_GPIO_J8_16     = 23,      // B+, Pin J8-16
    RPI_BPLUS_GPIO_J8_18     = 24,      // B+, Pin J8-18
    RPI_BPLUS_GPIO_J8_19     = 10,      // B+, Pin J8-19
    RPI_BPLUS_GPIO_J8_21     =  9,      // B+, Pin J8-21
    RPI_BPLUS_GPIO_J8_22     = 25,      // B+, Pin J8-22
    RPI_BPLUS_GPIO_J8_23     = 11,      // B+, Pin J8-23
    RPI_BPLUS_GPIO_J8_24     =  8,      // B+, Pin J8-24
    RPI_BPLUS_GPIO_J8_26     =  7,      // B+, Pin J8-26
    RPI_BPLUS_GPIO_J8_29     =  5,      // B+, Pin J8-29
    RPI_BPLUS_GPIO_J8_31     =  6,      // B+, Pin J8-31
    RPI_BPLUS_GPIO_J8_32     = 12,      // B+, Pin J8-32
    RPI_BPLUS_GPIO_J8_33     = 13,      // B+, Pin J8-33
    RPI_BPLUS_GPIO_J8_35     = 19,      // B+, Pin J8-35
    RPI_BPLUS_GPIO_J8_36     = 16,      // B+, Pin J8-36
    RPI_BPLUS_GPIO_J8_37     = 26,      // B+, Pin J8-37
    RPI_BPLUS_GPIO_J8_38     = 20,      // B+, Pin J8-38
    RPI_BPLUS_GPIO_J8_40     = 21       // B+, Pin J8-40
} RPiGPIOPin_t;

typedef enum
{
    BCM2835_GPIO_PUD_OFF     = 0x00,    // Disable pull-up/down 0b00
    BCM2835_GPIO_PUD_DOWN    = 0x01,    // Enable Pull Down control 0b01
    BCM2835_GPIO_PUD_UP      = 0x02     // Enable Pull Up control 0b10
} bcm2835PUDControl_t;

/* -----------------------------------------------------
 *      Serial interface SPI0
 */
typedef struct
{
    volatile uint32_t   spi_cs;
    volatile uint32_t   spi_fifo;
    volatile uint32_t   spi_clk;
    volatile uint32_t   spi_dlen;
    volatile uint32_t   spi_ltoh;
    volatile uint32_t   spi_dc;
} spi0_regs_t;

/* -----------------------------------------------------
 *      Auxiliary peripheral
 */
typedef struct
{
    volatile uint32_t   aux_irq;
    volatile uint32_t   aux_enables;
} auxiliary_peripherals_regs_t;

typedef struct
{
    volatile uint32_t   aux_mu_io_reg;
    volatile uint32_t   aux_mu_ier_reg;
    volatile uint32_t   aux_mu_iir_reg;
    volatile uint32_t   aux_mu_lcr_reg;
    volatile uint32_t   aux_mu_mcr_reg;
    volatile uint32_t   aux_mu_lsr_reg;
    volatile uint32_t   aux_mu_msr_reg;
    volatile uint32_t   aux_mu_scratch;
    volatile uint32_t   aux_mu_cntl_reg;
    volatile uint32_t   aux_mu_stat_reg;
    volatile uint32_t   aux_mu_baud_reg;
} auxiliary_uart1_regs_t;

typedef struct
{
    volatile uint32_t   aux_spi1_cntl0_reg;     // xxxxxx80
    volatile uint32_t   aux_spi1_cntl1_reg;     // xxxxxx84
    volatile uint32_t   aux_spi1_stat_reg;      // xxxxxx88
    volatile uint32_t   aux_spi1_peek_reg;      // xxxxxx8c
    volatile uint32_t   reserved0;              // xxxxxx90
    volatile uint32_t   reserved1;              // xxxxxx94
    volatile uint32_t   reserved2;              // xxxxxx98
    volatile uint32_t   reserved3;              // xxxxxx9c
    volatile uint32_t   aux_spi1_io_reg;        // xxxxxxa0
    volatile uint32_t   reserved4;              // xxxxxxa4
    volatile uint32_t   reserved5;              // xxxxxxa8
    volatile uint32_t   reserved6;              // xxxxxxac
    volatile uint32_t   aux_spi1_tx_hold_reg;   // xxxxxxb0
} auxiliary_spi1_regs_t;

#endif  /* __BCM2835_H__ */
