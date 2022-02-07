/********************************************************************
 * dragon.c
 *
 *  DRAGON DATA computer emulator, main module.
 *  With MC6809E CPU emulation.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    "printf.h"

#include    "mem.h"
#include    "cpu.h"
#include    "rpi.h"
#include    "gpio.h"

#include    "sam.h"
#include    "vdg.h"
#include    "pia.h"
#include    "loader.h"

/* -----------------------------------------
   Dragon 32 ROM image
----------------------------------------- */
#include    "dragon/dragon.h"

/* -----------------------------------------
   Module definition
----------------------------------------- */
#define     DRAGON_ROM_START        0x8000
#define     DRAGON_ROM_END          0xfeff
#define     ESCAPE_LOADER           1       // Pressing F1
#define     LONG_RESET_DELAY        1500000 // Micro-seconds to force cold start
#define     VDG_RENDER_CYCLES       4500    // CPU cycle count for ~20mSec screen refresh rate
#define     CPU_TIME_WASTE          1500    // Results in a CPU cycle of 4uSec

/* -----------------------------------------
   Module functions
----------------------------------------- */
static int get_reset_state(uint32_t time);

/*------------------------------------------------
 * main()
 *
 */
#if (RPI_BARE_METAL==1)
    void kernel(uint32_t r0, uint32_t machid, uint32_t atags)
#else
    int main(int argc, char *argv[])
#endif
{
    int     i;
    int     emulator_escape_code;
    int     vdg_render_cycles = 0;

    if ( rpi_gpio_init() == -1 )
    {
        rpi_halt();
    }

    printf("Dragon 32 bare metal %s %s\n", __DATE__, __TIME__);
    printf("GPIO initialized.\n");

    /* ROM code load
     */
    printf("Loading ROM ... ");
    i = 0;
    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    mem_define_rom(DRAGON_ROM_START, DRAGON_ROM_END);

    /* Emulation initialization
     */
    sam_init();
    pia_init();
    vdg_init();

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    /* CPU endless execution loop.
     */
    printf("Starting CPU.\n");
    cpu_reset(1);

    /* Profiling results:
     *  Cache    Turbo   cpu_run()
     *    no       no     5.2uS
     *    no      yes     3.4uS
     *   yes       no     4.0uS
     *   yes      yes     2.5uS
     *
     *  Full loop without extra delay measured at 3uSec
     */
    for (;;)
    {
        rpi_testpoint_on();
        cpu_run();
        rpi_testpoint_off();

        bcm2835_crude_delay(2);

        switch ( get_reset_state(LONG_RESET_DELAY) )
        {
            case 0:
                cpu_reset(0);
                break;

            case 2:
                /* Cold start flag set to value that is not 0x55
                 */
                mem_write(0x71, 0);
                printf("Force cold restart.\n");
                /* no break */

            case 1:
                cpu_reset(1);
                break;

            default:
                printf("kernel(): unknown reset state.\n");
        }

        emulator_escape_code = pia_function_key();
        if ( emulator_escape_code == ESCAPE_LOADER )
            loader();

        vdg_render_cycles++;
        if ( vdg_render_cycles == VDG_RENDER_CYCLES )
        {
            //rpi_testpoint_on();
            vdg_render();
            //rpi_testpoint_off();
            pia_vsync_irq();
            vdg_render_cycles = 0;
        }
    }

#if (RPI_BARE_METAL==0)
    return 0;
#endif
}

/*------------------------------------------------
 * get_reset_state()
 *
 * Scan the reset button with rpi_reset_button() and return
 * '1' for short reset and '2' for long reset press.
 * '0' for no press.
 * Accepts 'time' in micro-seconds as a parameter for determining long
 * press.
 *
 * param:  minimum button press time in micro-seconds
 * return: '0'=no press, '1'=short reset, '2'=long reset press.
 *
 */
static int get_reset_state(uint32_t time)
{
    uint32_t    start_time;
    int         reset_type = 0;

    if ( rpi_reset_button() == 0 )  // Active low!
    {
        start_time = rpi_system_timer();
        while ( !rpi_reset_button() );
        if ( (rpi_system_timer() - start_time) >= time )
        {
            reset_type = 2;
        }
        else
        {
            reset_type = 1;
        }
    }

    return reset_type;
}
