/********************************************************************
 * rpi.h
 *
 *  Header file that includes function stubs and definitions
 *  for RPi bare-metal option
 *
 *  January 8, 2021
 *
 *******************************************************************/

#ifndef __RPI_H__
#define __RPI_H__

#include    <stdint.h>

/* Define emulator stubs
 */
typedef enum
    {
        SD_OK,
        SD_GPIO_FAIL,
        SD_FAIL,
        SD_TIMEOUT,
        SD_BAD_CRC,
        SD_READ_FAIL,
    } sd_error_t;

/********************************************************************
 *  RPi bare meta module API
 */
int      rpi_gpio_init(void);

uint8_t *rpi_fb_init(int h, int v);
uint8_t *rpi_fb_resolution(int h, int v);

uint32_t rpi_system_timer(void);

int      rpi_keyboard_read(void);
void     rpi_keyboard_reset(void);

int      rpi_joystk_comp(void);
int      rpi_rjoystk_button(void);

int      rpi_reset_button(void);

void     rpi_audio_mux_set(int);

void     rpi_write_dac(int);

void     rpi_disable(void);
void     rpi_enable(void);

void     rpi_testpoint_on(void);
void     rpi_testpoint_off(void);

void     rpi_halt(void);

// XXX does not need to be defined void     _putchar(char character);

sd_error_t rpi_sd_init(void);
sd_error_t rpi_sd_read_block(uint32_t lba, uint8_t *buffer, uint32_t length);

#endif  /* __RPI_H__ */
