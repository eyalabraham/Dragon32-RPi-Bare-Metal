/********************************************************************
 * vdg.c
 *
 *  Module that implements the MC6847
 *  Video Display Generator (VDG) functionality.
 *
 *  https://en.wikipedia.org/wiki/Motorola_6847
 *  https://www.wikiwand.com/en/Semigraphics
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdint.h>

#include    "cpu.h"
#include    "mem.h"
#include    "vdg.h"
#include    "rpi.h"
#include    "printf.h"

#include    "dragon/font.h"
#include    "dragon/semigraph.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     VDG_REFRESH_INTERVAL    ((uint32_t)(1000000/50))

#define     SCREEN_WIDTH_PIX        256
#define     SCREEN_HEIGHT_PIX       192

#define     SCREEN_WIDTH_CHAR       32
#define     SCREEN_HEIGHT_CHAR      16

#define     FB_BLACK                0
#define     FB_BLUE                 1
#define     FB_GREEN                2
#define     FB_CYAN                 3
#define     FB_RED                  4
#define     FB_MAGENTA              5
#define     FB_BROWN                6
#define     FB_GRAY                 7
#define     FB_DARK_GRAY            8
#define     FB_LIGHT_BLUE           9
#define     FB_LIGHT_GREEN          10
#define     FB_LIGHT_CYAN           11
#define     FB_LIGHT_RED            12
#define     FB_LIGHT_MAGENTA        13
#define     FB_YELLOW               14
#define     FB_WHITE                15

#define     CHAR_SEMI_GRAPHICS      0x80
#define     CHAR_INVERSE            0x40

#define     SEMI_GRAPH4_MASK        0x0f
#define     SEMI_GRAPH6_MASK        0x1f

#define     PIA_COLOR_SET           0x01

#define     DEF_COLOR_CSS_0         0
#define     DEF_COLOR_CSS_1         4

#define     RES_HORZ_PIX            0
#define     RES_VERT_PIX            1
#define     RES_MEM                 2

typedef enum
{                       // Colors   Res.     Bytes BASIC
    ALPHA_INTERNAL = 0, // 2 color  32x16    512   Default
    ALPHA_EXTERNAL,     // 4 color  32x16    512
    SEMI_GRAPHICS_4,    // 8 color  64x32    512
    SEMI_GRAPHICS_6,    // 8 color  64x48    512
    SEMI_GRAPHICS_8,    // 8 color  64x64   2048
    SEMI_GRAPHICS_12,   // 8 color  64x96   3072
    SEMI_GRAPHICS_24,   // 8 color  64x192  6144
    GRAPHICS_1C,        // 4 color  64x64   1024
    GRAPHICS_1R,        // 2 color  128x64  1024
    GRAPHICS_2C,        // 4 color  128x64  1536
    GRAPHICS_2R,        // 2 color  128x96  1536   PMODE0
    GRAPHICS_3C,        // 4 color  128x96  3072   PMODE1
    GRAPHICS_3R,        // 2 color  128x192 3072   PMODE2
    GRAPHICS_6C,        // 4 color  128x192 6144   PMODE3
    GRAPHICS_6R,        // 2 color  256x192 6144   PMODE4
    DMA,                // 2 color  256x192 6144
} video_mode_t;

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static void vdg_put_pixel_fb(int color, int x, int y);
static void vdg_draw_char(int c, int col, int row);
static void vdg_draw_semig6(int c, int col, int row);
static video_mode_t vdg_get_mode(void);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static uint8_t  video_ram_offset;
static int      sam_video_mode;
static uint8_t  pia_video_mode;
static video_mode_t current_mode;
static video_mode_t prev_mode;

static uint8_t *fbp;

static int const resolution[][3] = {
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512 },   // ALPHA_INTERNAL, 2 color 32x16 512B Default
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512 },   // ALPHA_EXTERNAL, 4 color 32x16 512B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512 },   // SEMI_GRAPHICS_4, 8 color 64x32 512B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512 },   // SEMI_GRAPHICS_6, 8 color 64x48 512B
    {  64,  64, 2048 },                             // SEMI_GRAPHICS_8, 8 color 64x64 2048B
    {  64,  96, 3072 },                             // SEMI_GRAPHICS_12, 8 color 64x96 3072B
    {  64, 192, 6144 },                             // SEMI_GRAPHICS_24, 8 color 64x192 6144B
    {  64,  64, 1024 },                             // GRAPHICS_1C, 4 color 64x64 1024B
    { 128,  64, 1024 },                             // GRAPHICS_1R, 2 color 128x64 1024B
    { 128,  64, 2048 },                             // GRAPHICS_2C, 4 color 128x64 2048B
    { 128,  96, 1536 },                             // GRAPHICS_2R, 2 color 128x96 1536B PMODE 0
    { 128,  96, 3072 },                             // GRAPHICS_3C, 4 color 128x96 3072B PMODE 1
    { 256, 192, 3072 },                             // GRAPHICS_3R, 2 color 128x192 3072B PMODE 2
    { 256, 192, 6144 },                             // GRAPHICS_6C, 4 color 128x192 6144B PMODE 3
    { 256, 192, 6144 },                             // GRAPHICS_6R, 2 color 256x192 6144B PMODE 4
    { 256, 192, 6144 },                             // DMA, 2 color 256x192 6144B
};

static int const colors[] = {
        FB_LIGHT_GREEN,
        FB_YELLOW,
        FB_LIGHT_BLUE,
        FB_LIGHT_RED,
        FB_WHITE,        // TODO should be 'Buff'
        FB_CYAN,
        FB_LIGHT_MAGENTA,
        FB_BROWN,
};

/*------------------------------------------------
 * vdg_init()
 *
 *  Initialize the VDG device
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_init(void)
{
    video_ram_offset = 0x02;    // For offset 0x400 text screen
    sam_video_mode = 0;         // Alphanumeric

    fbp = rpi_fb_init(SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX);
    if ( fbp == 0L )
    {
        printf("vdg_init(): Frame buffer error.\n");
        rpi_halt();
    }

    /* Default startup mode of Dragon 32
     */
    current_mode = ALPHA_INTERNAL;
    prev_mode = ALPHA_INTERNAL;
}

/*------------------------------------------------
 * vdg_render()
 *
 *  Render video display.
 *
 *  TODO Consider writing to frame buffer 32-bit at a time instead of 8-bit
 *       per write. This may reduce render time. Profile render timing
 *       with GPIO pin.
 *       vdg_render() measured as running every 20mSec (~50Hz)
 *       with a render time of approximately 20uSec.
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_render(void)
{
    static  uint32_t    last_refresh_time = 0;
    int     col, row, c;

    uint8_t vdg_data;
    int     color;
    int     element;
    int     vdg_mem_base;
    int     vdg_mem_offset;
    int     fb_offset = 0;

    /* Set the refresh interval
     */
    if ( (rpi_system_timer() - last_refresh_time) < VDG_REFRESH_INTERVAL )
    {
        return;
    }

    //rpi_testpoint_on();

    last_refresh_time = rpi_system_timer();

    /* VDG/SAM mode settings
     */
    current_mode = vdg_get_mode();
    if ( current_mode != prev_mode )
    {
        fbp = rpi_fb_resolution(resolution[current_mode][RES_HORZ_PIX], resolution[current_mode][RES_VERT_PIX]);
        if ( fbp == 0L )
        {
            printf("vdg_render(): Frame buffer error.\n");
            rpi_halt();
        }

        prev_mode = current_mode;
    }

    /* Render to frame buffer with screen content
     */
    vdg_mem_base = video_ram_offset << 9;

    switch ( current_mode )
    {
        case ALPHA_INTERNAL:
        case SEMI_GRAPHICS_4:
            for ( col = 0; col < SCREEN_WIDTH_CHAR; col++ )
            {
                for ( row = 0; row < SCREEN_HEIGHT_CHAR; row++ )
                {
                    c = mem_read(col + row * SCREEN_WIDTH_CHAR + vdg_mem_base);
                    vdg_draw_char(c, col, row);
                }
            }
            break;

        case SEMI_GRAPHICS_6:
            for ( col = 0; col < SCREEN_WIDTH_CHAR; col++ )
            {
                for ( row = 0; row < SCREEN_HEIGHT_CHAR; row++ )
                {
                    c = mem_read(col + row * SCREEN_WIDTH_CHAR + vdg_mem_base);
                    vdg_draw_semig6(c, col, row);
                }
            }
            break;

        case GRAPHICS_1C:
        case GRAPHICS_2C:
        case GRAPHICS_3C:
        case GRAPHICS_6C:
            for ( vdg_mem_offset = 0; vdg_mem_offset < resolution[current_mode][RES_MEM]; vdg_mem_offset++)
            {
                vdg_data = mem_read(vdg_mem_base + vdg_mem_offset);

                for ( element = 0; element < 4; element++)
                {
                    color = (int)((vdg_data >> (2 * (3 - element))) & 0x03) + (4 * (pia_video_mode & PIA_COLOR_SET));
                    color = colors[color];
                    *((uint8_t*)(fbp + fb_offset)) = (uint8_t) color;
                    fb_offset++;

                    if ( current_mode == GRAPHICS_6C )
                    {
                        *((uint8_t*)(fbp + fb_offset)) = (uint8_t) color;
                        fb_offset++;
                    }
                }
            }
            break;

        case GRAPHICS_1R:
        case GRAPHICS_2R:
        case GRAPHICS_3R:
        case GRAPHICS_6R:
            for ( vdg_mem_offset = 0; vdg_mem_offset < resolution[current_mode][RES_MEM]; vdg_mem_offset++)
            {
                vdg_data = mem_read(vdg_mem_base + vdg_mem_offset);

                for ( element = 0; element < 8; element++)
                {
                    if ( (vdg_data >> (7 - element)) & 0x01 )
                    {
                        if ( pia_video_mode & PIA_COLOR_SET )
                        {
                            color = colors[DEF_COLOR_CSS_1];
                        }
                        else
                        {
                            color = colors[DEF_COLOR_CSS_0];
                        }
                    }
                    else
                    {
                        color = FB_BLACK;
                    }

                    *((uint8_t*)(fbp + fb_offset)) = (uint8_t) color;
                    fb_offset++;

                    if ( current_mode == GRAPHICS_3R )
                    {
                        *((uint8_t*)(fbp + fb_offset)) = (uint8_t) color;
                        fb_offset++;
                    }
                }
            }
            break;

        case SEMI_GRAPHICS_8:
        case SEMI_GRAPHICS_12:
        case SEMI_GRAPHICS_24:
        case ALPHA_EXTERNAL:
        case DMA:
            printf("vdg_render(): Mode not supported %d\n", current_mode);
            rpi_halt();
            break;

        default:
            {
                printf("vdg_render(): Illegal mode.\n");
                rpi_halt();
            }
    }

    //rpi_testpoint_off();
}

/*------------------------------------------------
 * vdg_set_video_offset()
 *
 *  Set the video display start offset in RAM.
 *  Most significant six bits of a 15 bit RAM address.
 *  Value is set by SAM device.
 *
 *  param:  Offset value.
 *  return: Nothing
 */
void vdg_set_video_offset(uint8_t offset)
{
    video_ram_offset = offset;
}

/*------------------------------------------------
 * vdg_set_mode_sam()
 *
 *  Set the video display mode from SAM device.
 *
 *  0   Alpha, S4, S6
 *  1   G1C, G1R
 *  2   G2C
 *  3   G2R
 *  4   G3C
 *  5   G3R
 *  6   G6R, G6C
 *  7   DMA
 *
 *  param:  Mode value.
 *  return: Nothing
 */
void vdg_set_mode_sam(int sam_mode)
{
    sam_video_mode = sam_mode;
}

/*------------------------------------------------
 * vdg_set_mode_pia()
 *
 *  Set the video display mode from PIA device.
 *  Mode bit are as-is for PIA shifted 3 to the right:
 *  Bit 4   O   Screen Mode G / ^A
 *  Bit 3   O   Screen Mode GM2
 *  Bit 2   O   Screen Mode GM1
 *  Bit 1   O   Screen Mode GM0 / ^INT
 *  Bit 0   O   Screen Mode CSS
 *
 *  param:  Mode value.
 *  return: Nothing
 */
void vdg_set_mode_pia(uint8_t pia_mode)
{
    pia_video_mode = pia_mode;
}

/*------------------------------------------------
 * vdg_put_pixel()
 *
 *  Put a pixel in the screen frame buffer
 *  Lowest level function to draw a pixel in the frame buffer.
 *  Assumes an 8-bit per pixel video mode is selected, and
 *  the calling function has calculated the correct (x,y) pixel coordinates.
 *  The Dragon VDG always uses a fixed 256x192 screen resolution.
 *
 * param:  pixel color, x and y coordinates
 * return: none
 *
 */
static void vdg_put_pixel_fb(int color, int x, int y)
{
    int pixel_offset;

    /* Calculate the pixel's byte offset inside the buffer
     * and update the frame buffer.
     */
    pixel_offset = x + y * SCREEN_WIDTH_PIX;
    *((uint8_t*)(fbp + pixel_offset)) = (uint8_t) color;    // The same as 'fbp[pix_offset] = value'
}

/*------------------------------------------------
 * vdg_draw_char()
 *
 * Draw a text of Semigraphics-4 character in the screen frame buffer.
 * Low level function to draw a character in the frame buffer, and
 * assumes an 8-bit per pixel video mode is selected.
 * Provide VDG character code 0..63, use fonf.h definition for
 * character bitmap.
 * Bit 6, denotes inverse video, and bit 7 denotes Semigraphics
 * character.
 * The 'pia_video_mode' bit.0 to determine color (FB_GREEN or FB_BROWN).
 *
 * NOTE: no range checks are done on c, col, and row values!
 *
 * param:  c    VDG character code or semigraphics to be printed
 *         col  horizontal text position (0..31)
 *         row  vertical text position (0..15)
 * return: none
 *
 */
void vdg_draw_char(int c, int col, int row)
{
    uint8_t     pix_pos, bit_pattern;
    int         px, py;
    int         char_row, char_col, char_index;
    int         fg_color, bg_color;

    /* Convert text col and row to pixel positions
     * and adjust for non-semigraphics
     */
    px = col * FONT_WIDTH;
    py = row * FONT_HEIGHT;

    /* Output semigraphics characters
     * code 128 through 255.
     */
    if ( (uint8_t)c & CHAR_SEMI_GRAPHICS )
    {
        /* Determine colors
         */
        bg_color = FB_BLACK;
        fg_color = colors[(int)((c & 0b01110000) >> 4)];

        /* Character output
         */
        char_index = (int)(((uint8_t) c) & SEMI_GRAPH4_MASK);

        for ( char_row = 0; char_row < FONT_HEIGHT; char_row++, py++ )
        {
            bit_pattern = semi_graph_4[char_index][char_row];

            pix_pos = 0x80;

            for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
            {
                /* Bit is set in Font, print pixel(s) in text color
                 */
                if ( (bit_pattern & pix_pos) )
                {
                    vdg_put_pixel_fb(fg_color, (px + char_col), py);
                }
                /* Bit is cleared in Font
                 */
                else
                {
                    vdg_put_pixel_fb(bg_color, (px + char_col), py);
                }

                /* Move to the next pixel position
                 */
                pix_pos = pix_pos >> 1;
            }
        }
    }

    /* Output character codes 0 through 127
     * inverse and non inverse video.
     */
    else
    {
        /* Determine colors
         */
        bg_color = FB_BLACK;

        if ( pia_video_mode & PIA_COLOR_SET )
            fg_color = colors[DEF_COLOR_CSS_1];
        else
            fg_color = colors[DEF_COLOR_CSS_0];

        if ( (uint8_t)c & CHAR_INVERSE )
        {
            char_row = fg_color;
            fg_color = bg_color;
            bg_color = char_row;
        }

        /* Character output
         */
        char_index = (int)(((uint8_t) c) & ~(CHAR_SEMI_GRAPHICS | CHAR_INVERSE));

        for ( char_row = 0; char_row < FONT_HEIGHT; char_row++, py++ )
        {
            bit_pattern = font_img5x7[char_index][char_row];

            pix_pos = 0x80;

            for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
            {
                /* Bit is set in Font, print pixel(s) in text color
                 */
                if ( (bit_pattern & pix_pos) )
                {
                    vdg_put_pixel_fb(fg_color, (px + char_col), py);
                }
                /* Bit is cleared in Font
                 */
                else
                {
                    vdg_put_pixel_fb(bg_color, (px + char_col), py);
                }

                /* Move to the next pixel position
                 */
                pix_pos = pix_pos >> 1;
            }
        }
    }
}

/*------------------------------------------------
 * vdg_draw_semig6()
 *
 * Draw a semigraphics-6 character in the screen frame buffer.
 *
 * NOTE: no range checks are done on c, col, and row values!
 *
 * param:  c    semigraphics 6 code
 *         col  horizontal text position (0..31)
 *         row  vertical text position (0..15)
 * return: none
 *
 */
static void vdg_draw_semig6(int c, int col, int row)
{
    uint8_t     pix_pos, bit_pattern;
    int         px, py;
    int         char_row, char_col, char_index;
    int         fg_color, bg_color;

    /* Convert text col and row to pixel positions
     * and adjust for non-semigraphics
     */
    px = col * FONT_WIDTH;
    py = row * FONT_HEIGHT;

    /* Determine colors
     */
    bg_color = FB_BLACK;
    fg_color = colors[(int)(((c & 0b11000000) >> 6) + (4 * (pia_video_mode & PIA_COLOR_SET)))];

    /* Character output
     */
    char_index = (int)(((uint8_t) c) & SEMI_GRAPH6_MASK);

    for ( char_row = 0; char_row < FONT_HEIGHT; char_row++, py++ )
    {
        bit_pattern = semi_graph_6[char_index][char_row];

        pix_pos = 0x80;

        for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
        {
            /* Bit is set in Font, print pixel(s) in text color
             */
            if ( (bit_pattern & pix_pos) )
            {
                vdg_put_pixel_fb(fg_color, (px + char_col), py);
            }
            /* Bit is cleared in Font
             */
            else
            {
                vdg_put_pixel_fb(bg_color, (px + char_col), py);
            }

            /* Move to the next pixel position
             */
            pix_pos = pix_pos >> 1;
        }
    }
}

/*------------------------------------------------
 * vdg_get_mode()
 *
 * Parse 'sam_video_mode' and 'pia_video_mode' and return video mode type.
 *
 * param:  None
 * return: Video mode
 *
 */
static video_mode_t vdg_get_mode(void)
{
    video_mode_t mode;

    if ( sam_video_mode == 7 )
    {
        mode = DMA;
    }
    else if ( (pia_video_mode & 0x10) )
    {
        switch ( pia_video_mode & 0x0e  )
        {
            case 0x00:
                mode = GRAPHICS_1C;
                break;
            case 0x02:
                mode = GRAPHICS_1R;
                break;
            case 0x04:
                mode = GRAPHICS_2C;
                break;
            case 0x06:
                mode = GRAPHICS_2R;
                break;
            case 0x08:
                mode = GRAPHICS_3C;
                break;
            case 0x0a:
                mode = GRAPHICS_3R;
                break;
            case 0x0c:
                mode = GRAPHICS_6C;
                break;
            case 0x0e:
                mode = GRAPHICS_6R;
                break;
        }
    }
    else if ( (pia_video_mode & 0x10) == 0 )
    {
        if ( sam_video_mode == 0 &&
             (pia_video_mode & 0x02) == 0 )
        {
            mode = ALPHA_INTERNAL;
            // Character bit.7 selects SEMI_GRAPHICS_4;
        }
        else if ( sam_video_mode == 0 &&
                (pia_video_mode & 0x02) )
        {
            mode = ALPHA_EXTERNAL;
            // Character bit.7 selects SEMI_GRAPHICS_6;
        }
        else if ( sam_video_mode == 2 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_8;
        }
        else if ( sam_video_mode == 4 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_12;
        }
        else if ( sam_video_mode == 4 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_24;
        }
    }
    else
    {
        printf("vdg_get_mode(): Cannot resolve mode.\n");
        rpi_halt();
    }

    return mode;
}
