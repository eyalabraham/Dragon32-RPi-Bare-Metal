/********************************************************************
 * pia.c
 *
 *  Module that implements the MC6821 PIA functionality.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdint.h>
#include    <string.h>

#include    "cpu.h"
#include    "rpi.h"
#include    "mem.h"
#include    "vdg.h"
#include    "pia.h"
#include    "sdfat32.h"
#include    "loader.h"
#include    "printf.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     PIA0_PA             0xff00
#define     PIA0_CRA            0xff01
#define     PIA0_PB             0xff02
#define     PIA0_CRB            0xff03

#define     PIA1_PA             0xff20
#define     PIA1_CRA            0xff21
#define     PIA1_PB             0xff22
#define     PIA1_CRB            0xff23

#define     PIACR_CAB2_MASK     0X38
#define     PIACR_CAB2_SET      0x38
#define     PIACR_CABS_CLR      0x30

#define     KBD_ROWS            7

#define     PIA_VSYNC_INTERVAL  ((uint32_t)(1000000/50))

#define     PIA_CR_INTR         0x01    // CA1/CB1 interrupt enable bit
#define     PIA_CR_IRQ_STAT     0x80    // IRQA1/IRQB1 status bit

#define     AUDIO_MUX_DAC       0
#define     AUDIO_MUX_OTHER     1
#define     AUDIO_MUX_JSTKX     2
#define     AUDIO_MUX_JSTKY     3

#define     MOTOR_ON            0b00001000
#define     BIT_THRESHOLD_HI    4
#define     BIT_THRESHOLD_LO    20

#define     SCAN_CODE_F1        58

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static uint8_t io_handler_pia0_pa(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia0_pb(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia0_cra(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia0_crb(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia1_pa(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia1_pb(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia1_cra(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia1_crb(uint16_t address, uint8_t data, mem_operation_t op);

static uint8_t get_keyboard_row_scan(uint8_t data);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static int     pia0_cb1_int_enabled = 0;
static uint8_t pia0_cra = 0;
static uint8_t pia0_crb = 0;
static uint8_t pia1_cra = 0;
static uint8_t pia1_crb = 0;

static uint8_t audio_mux_select = AUDIO_MUX_DAC;

static dir_entry_t  cas_file;

static int     function_key = 0;

/*
    Dragon keyboard map

          LSB              $FF02                    MSB
        | PB0   PB1   PB2   PB3   PB4   PB5   PB6   PB7 | <- column
    ----|-----------------------------------------------|-----------
    PA0 |   0     1     2     3     4     5     6     7 |   LSB
    PA1 |   8     9     :     ;     ,     -     .     / |
    PA2 |   @     A     B     C     D     E     F     G |
    PA3 |   H     I     J     K     L     M     N     O | $FF00
    PA4 |   P     Q     R     S     T     U     V     W |
    PA5 |   X     Y     Z    Up  Down  Left Right Space |
    PA6 | ENT   CLR   BRK   N/C   N/C   N/C   N/C  SHFT |
    PA7 | Comparator input                              |   MSB
*/
static uint8_t scan_code_table[81][2] = {
        // Column     Row
        { 0xff,       255 }, // #0
        { 0b11111011,   6 }, //      Break (ESC key)
        { 0b11111101,   0 }, //      1
        { 0b11111011,   0 }, //      2
        { 0b11110111,   0 }, //      3
        { 0b11101111,   0 }, //      4
        { 0b11011111,   0 }, //      5
        { 0b10111111,   0 }, //      6
        { 0b01111111,   0 }, //      7
        { 0b11111110,   1 }, //      8
        { 0b11111101,   1 }, // #10  9
        { 0b11111110,   0 }, //      0
        { 0b11011111,   1 }, //      -
        { 0b11111011,   1 }, //      :
        { 0b11111101,   6 }, //      CLEAR
        { 0xff,       255 },
        { 0b11111101,   4 }, //      Q
        { 0b01111111,   4 }, //      W
        { 0b11011111,   2 }, //      E
        { 0b11111011,   4 }, //      R
        { 0b11101111,   4 }, // #20  T
        { 0b11111101,   5 }, //      Y
        { 0b11011111,   4 }, //      U
        { 0b11111101,   3 }, //      I
        { 0b01111111,   3 }, //      O
        { 0b11111110,   4 }, //      P
        { 0b11111110,   2 }, //      @
        { 0xff,       255 },
        { 0b11111110,   6 }, //      Enter
        { 0xff,       255 },
        { 0b11111101,   2 }, // #30  A
        { 0b11110111,   4 }, //      S
        { 0b11101111,   2 }, //      D
        { 0b10111111,   2 }, //      F
        { 0b01111111,   2 }, //      G
        { 0b11111110,   3 }, //      H
        { 0b11111011,   3 }, //      J
        { 0b11110111,   3 }, //      K
        { 0b11101111,   3 }, //      L
        { 0b11110111,   1 }, //      ;
        { 0xff,       255 }, // #40
        { 0xff,       255 },
        { 0b01111111,   6 }, //      Shift key
        { 0xff,       255 },
        { 0b11111011,   5 }, //      Z
        { 0b11111110,   5 }, //      X
        { 0b11110111,   2 }, //      C
        { 0b10111111,   4 }, //      V
        { 0b11111011,   2 }, //      B
        { 0b10111111,   3 }, //      N
        { 0b11011111,   3 }, // #50  M
        { 0b11101111,   1 }, //      ,
        { 0b10111111,   1 }, //      .
        { 0b01111111,   1 }, //      /
        { 0xff,       255 },
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b01111111,   5 }, //      Space bar
        { 0xff,       255 },
        { 0xff,       255 }, //      F1
        { 0xff,       255 }, // #60  F2
        { 0xff,       255 }, //      F3
        { 0xff,       255 }, //      F4
        { 0xff,       255 }, //      F5
        { 0xff,       255 }, //      F6
        { 0xff,       255 }, //      F7
        { 0xff,       255 }, //      F8
        { 0xff,       255 }, //      F9
        { 0xff,       255 }, //      F10
        { 0xff,       255 },
        { 0xff,       255 }, // #70
        { 0xff,       255 },
        { 0b11110111,   5 }, //      Up arrow
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b11011111,   5 }, //      Left arrow
        { 0xff,       255 },
        { 0b10111111,   5 }, //      Right arrow
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b11101111,   5 }, // #80  Down arrow
};

static uint8_t keyboard_rows[KBD_ROWS] = {
        255,    // row PIA0_PA0
        255,    // row PIA0_PA1
        255,    // row PIA0_PA2
        255,    // row PIA0_PA3
        255,    // row PIA0_PA4
        255,    // row PIA0_PA5
        255,    // row PIA0_PA6
};

/*------------------------------------------------
 * pia_init()
 *
 *  Initialize the PIA device
 *
 *  param:  Nothing
 *  return: Nothing
 */
void pia_init(void)
{
    /* Link IO call-backs
     */
    mem_write(PIA0_PA, 0x7f);
    mem_define_io(PIA0_PA, PIA0_PA, io_handler_pia0_pa);    // Joystick comparator, keyboard row input
    mem_define_io(PIA0_PB, PIA0_PB, io_handler_pia0_pb);    // Keyboard column output
    mem_define_io(PIA0_CRA, PIA0_CRA, io_handler_pia0_cra); // Audio multiplexer select bit.0
    mem_define_io(PIA0_CRB, PIA0_CRB, io_handler_pia0_crb); // Field sync interrupt

    mem_define_io(PIA1_PA, PIA1_PA, io_handler_pia1_pa);    // 6-bit DAC output, cassette interface input bit
    mem_define_io(PIA1_PB, PIA1_PB, io_handler_pia1_pb);    // VDG mode bits output
    mem_define_io(PIA1_CRA, PIA1_CRA, io_handler_pia1_cra); // Cassette tape motor control
    mem_define_io(PIA1_CRB, PIA1_CRB, io_handler_pia1_crb); // Audio multiplexer select bit.1

    memset(&cas_file, 0, sizeof(dir_entry_t));
}

/*------------------------------------------------
 * pia_hsync_irq()
 *
 *  Assert an IRQ interrupt at Field Sync refresh rate
 *
 *  param:  Nothing
 *  return: Nothing
 */
void pia_vsync_irq(void)
{
    static  uint32_t    last_vsync_time = 0;

    /* Check interrupt cycle time
     */
    if ( (rpi_system_timer() - last_vsync_time) < PIA_VSYNC_INTERVAL )
    {
        return;
    }

    last_vsync_time = rpi_system_timer();

    /* Assert interrupt if enabled
     */
    if ( pia0_cb1_int_enabled )
    {
        pia0_crb |= PIA_CR_IRQ_STAT;
        cpu_irq(1);
    }
}

/*------------------------------------------------
 * pia_function_key()
 *
 *  Check if function key was pressed to escape the emulation.
 *  Return '0' if not, or function key value from 1 to 10
 *  if a key was pressed and latched.
 *  If a key was pressed, reset the value latch.
 *
 *  param:  Nothing
 *  return: Function key value 1 to 10, o if none pressed
 */
int pia_function_key(void)
{
    int key_code;

    key_code = function_key;
    function_key = 0;

    return key_code;
}

/*------------------------------------------------
 * io_handler_pia0_pa()
 *
 *  IO call-back handler 0xFF00 PIA0-A Data read:
 *
 *  Bit 0..6 keyboard row input
 *  Bit 0    Right joystick button input
 *  Bit 1    Left joystick button input ** not implemented **
 *  Bit 7    Joystick comparator input
 *
 *  This call-back will only deal with joystick comparator input read.
 *  Keyboard row inputs are handled by io_handler_pia0_pb()
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia0_pa(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_READ )
    {
        /* Check joystick comparator and button GPIO and set bits
         */
        if ( rpi_joystk_comp() )
            data |= 0x80;
        else
            data &= 0x7f;

        /* Do not force a '1' if joystick button is not pressed
         * this will interfere with keyboard scan.
         */
        if ( rpi_rjoystk_button() == 0 )
            data &= 0xfe;

        /* Keyboard row scan inputs are set by 'io_handler_pia0_pb()'
         */
    }

    return data;
}

/*------------------------------------------------
 * io_handler_pia0_pb()
 *
 *  IO call-back handler 0xFF02 PIA0-B Data
 *  Bit 0..7 Output to keyboard columns
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia0_pb(uint16_t address, uint8_t data, mem_operation_t op)
{
    uint8_t scan_code = 0;
    uint8_t row_switch_bits;
    int     row_index;

    /* Activate the call back to read keyboard scan code from
     * external AVR interface only when testing the keyboard columns
     * for a key press.
     */
    if ( op == MEM_WRITE )
    {
        /* When writing to the port, the ROM code is checking if any
         * key is pressed. So a good opportunity
         * to read the keyboard scan code.
         */
        scan_code = (uint8_t) rpi_keyboard_read();

        //if ( (scan_code & 0x7f) >= 59 && (scan_code & 0x7f) <= 68 )
        if ( scan_code >= 59 && scan_code <= 68 )
        {
            /* Store special function keys as emulator escapes
             * values between 1 an 10 for F1 to F10 keys
             */
            if ( function_key == 0 )
                function_key = scan_code - SCAN_CODE_F1;
        }
        else if ( scan_code != 0 )
        {
            /* Sanity check
             */
            if ( (row_index = scan_code_table[(scan_code & 0x7f)][1]) == 255 )
            {
                printf("io_handler_pia0_pb(): Illegal scan code.\n");
                rpi_halt();
            }

            /* Generate row bit patterns emulating row key closures
             * and match to 'make' or 'break' codes (bit.7 of scan code)
             */
            row_switch_bits = scan_code_table[(scan_code & 0x7f)][0];

            if ( scan_code & 0x80 )
            {
                keyboard_rows[row_index] |= ~row_switch_bits;
            }
            else
            {
                keyboard_rows[row_index] &= row_switch_bits;
            }
        }

        /* Store the appropriate row bit value
         * for PIA0_PA bit pattern after merging with comparator input
         */
        row_switch_bits = get_keyboard_row_scan(data);
        if ( rpi_joystk_comp() )
            row_switch_bits |= 0x80;
        else
            row_switch_bits &= 0x7f;

        mem_write(PIA0_PA, (int) row_switch_bits);
    }

    /* A read to the port address has the effect of resetting
     * the IRQ status line
     */
    else
    {
        pia0_crb &= ~PIA_CR_IRQ_STAT;
        cpu_irq(0);
    }

    return data;
}

/*------------------------------------------------
 * io_handler_pia0_cra()
 *
 *  IO call-back handler 0xFF03 PIA0-A Control register
 *  responding the audio multiplexer select bits
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia0_cra(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_WRITE )
    {
        pia0_cra = data;

        if ( (pia0_cra & PIACR_CAB2_MASK) == PIACR_CAB2_SET )
            audio_mux_select |= 0x01;
        else
            audio_mux_select &= 0xfe;

        rpi_audio_mux_set((int) audio_mux_select);
    }

    return pia0_cra;
}

/*------------------------------------------------
 * io_handler_pia0_crb()
 *
 *  IO call-back handler 0xFF03 PIA0-B Control register
 *  to enabled/disable IRQ interrupt source.
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia0_crb(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_WRITE )
    {
        pia0_crb = data;

        if ( pia0_crb & PIA_CR_INTR )
            pia0_cb1_int_enabled = 1;
        else
            pia0_cb1_int_enabled = 0;
    }

    return pia0_crb;
}

/*------------------------------------------------
 * io_handler_pia1_pa()
 *
 *  IO call-back handler 0xFF20 Dir PIA1-A output to 6-bit DAC
 *  Traps and handles writes to PA bit.2 to bit.7
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia1_pa(uint16_t address, uint8_t data, mem_operation_t op)
{
    static  uint8_t byte = 0;
    static  int     bit_index = 0;
    static  int     bit_timing_threshold = 0;
    static  int     bit_timing_count = 0;

    int     cas_eof;
    int     dac_output;

    if ( op == MEM_WRITE )
    {
        dac_output = (data >> 2) & 0x3f;
        rpi_write_dac(dac_output);
    }
    else
    {
        /* Reading the cassette tape input bit PIA1-PA0:
         * 1) Bits are fed into PA0 with LSB first
         * 2) a '1' bit toggles PA0 to '0' then '1' for BIT_THRESHOLD_HI/2 reads of PA0
         * 3) a '0' bit toggles PA0 to '0' then '1' for BIT_THRESHOLD_LO/2 reads of PA0
         * 4) The read count threshold of PA0 that determines the bit state is 18
         *    according to the Dragon ROM listing
         * 5) The normal PA0 state is '0'
         *
         * This process fakes the bit stream coming from the cassette tape interface
         * with the advantage that it can synchronize on the bit reads. The interface
         * can be hacked to speed up the load time by changing the threshold of 18
         * in Dragon RAM location 0x0092 to a lower number.
         *
         */
        if ( bit_index == 0 )
        {
            cas_eof = !fat32_fread(&byte, 1);

            bit_index = 9;
            bit_timing_threshold = 0;
            bit_timing_count = 0;

            /* TODO Will we see an EOF because EOF-CAS-block would be read first?
             *      Not sure how we handle and EOF.
             *      There is also no need to fat32_fclose() the file.
             */
            if ( cas_eof )
            {
                byte = 0x55;
            }
        }

        if ( bit_timing_count == bit_timing_threshold )
        {
            if ( byte & 0b00000001 )
            {
                bit_timing_threshold = BIT_THRESHOLD_HI;
            }
            else
            {
                bit_timing_threshold = BIT_THRESHOLD_LO;
            }

            bit_timing_count = 0;

            byte = byte >> 1;
            bit_index--;
        }

        if ( bit_timing_count < (bit_timing_threshold / 2) )
        {
            data &= 0b11111110;
        }
        else
        {
            data |= 0b00000001;
        }

        bit_timing_count++;
    }

    return data;
}

/*------------------------------------------------
 * io_handler_pia1_pb()
 *
 *  IO call-back handler 0xFF22 Dir PIA1-B Data
 *  Bit 7   O   Screen Mode G/^A
 *  Bit 6   O   Screen Mode GM2
 *  Bit 5   O   Screen Mode GM1
 *  Bit 4   O   Screen Mode GM0 / INT
 *  Bit 3   O   Screen Mode CSS
 *  Bit 2   I   Ram Size (1=16k 0=32/64k), not implemented
 *  Bit 1   I   TODO Single bit sound
 *  Bit 0   I   Rs232 In / Printer Busy, not implemented
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia1_pb(uint16_t address, uint8_t data, mem_operation_t op)
{
    vdg_set_mode_pia(((data >> 3) & 0x1f));

    return data;
}

/*------------------------------------------------
 * io_handler_pia1_cra()
 *
 *  IO call-back handler 0xFF21 PIA1-A Control register
 *  responding the cassette motor on-off select bit CA2
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia1_cra(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_WRITE )
    {
        pia1_cra = data;

        if ( data & 0b00110000 )
        {
            if ( data & MOTOR_ON )
            {
                /* If the motor is turned on then get the mounted
                 * CAS file and open it. Not checking errors, if the file
                 * is open then ok as it will never be a directory either.
                 * Reopening a file does not reset the read pointer so no harm there either.
                 */
                if ( loader_mount_cas_file(&cas_file) )
                {
                    fat32_fopen(&cas_file);
                }
            }
            else
            {
                /* Do nothing with motor-off, not even fat32_fclose()
                 */
            }
        }
    }

    return pia1_cra;
}

/*------------------------------------------------
 * io_handler_pia1_crb()
 *
 *  IO call-back handler 0xFF23 PIA1-B Control register
 *  responding the audio multiplexer select bits
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia1_crb(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_WRITE )
    {
        pia1_crb = data;

        if ( (pia1_crb & PIACR_CAB2_MASK) == PIACR_CAB2_SET )
            audio_mux_select |= 0x02;
        else
            audio_mux_select &= 0xfd;

        rpi_audio_mux_set((int) audio_mux_select);
    }

    return pia1_crb;
}

/*------------------------------------------------
 * get_keyboard_row_scan()
 *
 *  Using the Row scan bit pattern and the key closure
 *  matrix in 'keyboard_rows', generate the row scan bit pattern
 *
 *  param:  Row scan bit pattern
 *  return: Column scan bit pattern
 */
static uint8_t get_keyboard_row_scan(uint8_t row_scan)
{
    uint8_t result = 0;
    uint8_t bit_position = 0x01;
    uint8_t test;
    int     row;

    for ( row = 0; row < KBD_ROWS; row++ )
    {
        test = (~row_scan) & keyboard_rows[row];

        if ( test == (uint8_t)(~row_scan) )
        {
            result |= bit_position;
        }

        bit_position = bit_position << 1;
    }

    return result;
}
