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
#define     SEMI_GRAPH8_MASK        SEMI_GRAPH4_MASK

#define     SEMIG8_SEG_HEIGHT       3       // Scan rows
#define     SEMIG12_SEG_HEIGHT      2
#define     SEMIG24_SEG_HEIGHT      1

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
    UNDEFINED,          // Undefined
} video_mode_t;

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static void vdg_draw_char(int c, int col, int row);
static void vdg_draw_semig6(int c, int col, int row);
static void vdg_draw_semig_ext(video_mode_t mode, int video_mem_base, int text_buffer_length);
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
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512  },  // ALPHA_INTERNAL, 2 color 32x16 512B Default
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512  },  // ALPHA_EXTERNAL, 4 color 32x16 512B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512  },  // SEMI_GRAPHICS_4, 8 color 64x32 512B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 512  },  // SEMI_GRAPHICS_6, 8 color 64x48 512B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 2048 },  // SEMI_GRAPHICS_8, 8 color 64x64 2048B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 3072 },  // SEMI_GRAPHICS_12, 8 color 64x96 3072B
    { SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX, 6144 },  // SEMI_GRAPHICS_24, 8 color 64x192 6144B
    {  64,  64, 1024                            },  // GRAPHICS_1C, 4 color 64x64 1024B
    { 128,  64, 1024                            },  // GRAPHICS_1R, 2 color 128x64 1024B
    { 128,  64, 2048                            },  // GRAPHICS_2C, 4 color 128x64 2048B
    { 128,  96, 1536                            },  // GRAPHICS_2R, 2 color 128x96 1536B PMODE 0
    { 128,  96, 3072                            },  // GRAPHICS_3C, 4 color 128x96 3072B PMODE 1
    { 256, 192, 3072                            },  // GRAPHICS_3R, 2 color 128x192 3072B PMODE 2
    { 256, 192, 6144                            },  // GRAPHICS_6C, 4 color 128x192 6144B PMODE 3
    { 256, 192, 6144                            },  // GRAPHICS_6R, 2 color 256x192 6144B PMODE 4
    { 256, 192, 6144                            },  // DMA, 2 color 256x192 6144B
};

static char* const mode_name[] = {
    "ALPHA_INT",  // ALPHA_INTERNAL, 2 color 32x16 512B Default
    "ALPHA_EXT",  // ALPHA_EXTERNAL, 4 color 32x16 512B
    "SEMI_GR4 ",  // SEMI_GRAPHICS_4, 8 color 64x32 512B
    "SEMI_GR6 ",  // SEMI_GRAPHICS_6, 8 color 64x48 512B
    "SEMI_GR8 ",  // SEMI_GRAPHICS_8, 8 color 64x64 2048B
    "SEMI_GR12",  // SEMI_GRAPHICS_12, 8 color 64x96 3072B
    "SEMI_GR24",  // SEMI_GRAPHICS_24, 8 color 64x192 6144B
    "GRAPH_1C ",  // GRAPHICS_1C, 4 color 64x64 1024B
    "GRAPH_1R ",  // GRAPHICS_1R, 2 color 128x64 1024B
    "GRAPH_2C ",  // GRAPHICS_2C, 4 color 128x64 2048B
    "GRAPH_2R ",  // GRAPHICS_2R, 2 color 128x96 1536B PMODE 0
    "GRAPH_3C ",  // GRAPHICS_3C, 4 color 128x96 3072B PMODE 1
    "GRAPH_3R ",  // GRAPHICS_3R, 2 color 128x192 3072B PMODE 2
    "GRAPH_6C ",  // GRAPHICS_6C, 4 color 128x192 6144B PMODE 3
    "GRAPH_6R ",  // GRAPHICS_6R, 2 color 256x192 6144B PMODE 4
    "DMA      ",  // DMA, 2 color 256x192 6144B
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
 *  A full screen rendering is performed at every invocation on the function.
 *  The function should be called periodically and will execute a screen refresh only
 *  if 20 milliseconds of more have elapsed since the last refresh (50Hz).
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_render(void)
{
    int     col, row, c;

    uint8_t vdg_data;
    int     color;
    int     element;
    int     vdg_mem_base;
    int     vdg_mem_offset;
    int     fb_offset = 0;

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

        printf("VDG mode: %s\n", mode_name[current_mode]);
    }

    /* Render screen content to RPi frame buffer
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
            vdg_draw_semig_ext(current_mode, vdg_mem_base, resolution[current_mode][RES_MEM]);
            break;

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
    uint8_t         pix_pos, bit_pattern;
    int             px, py;
    int             char_row, char_col, char_index;
    uint32_t        fg_color, bg_color;

    const uint8_t  *bit_pattern_array;
    uint32_t        byte_position;
    uint32_t        frame_buffer_index;
    register        uint32_t    pixel_group;

    /* Common initialization
     */
    byte_position = 0;
    pixel_group = 0;
    px = col * FONT_WIDTH;
    py = row * FONT_HEIGHT;
    bg_color = FB_BLACK;

    /* Mode dependent initializations
     * for text or semigraphics 4:
     * - Determine foreground and background colors
     * - Character pattern array
     * - Character code index to bit pattern array
     *
     */
    if ( (uint8_t)c & CHAR_SEMI_GRAPHICS )
    {
        fg_color = colors[(((uint8_t)c & 0b01110000) >> 4)];
        char_index = (int)(((uint8_t) c) & SEMI_GRAPH4_MASK);
        bit_pattern_array = &semi_graph_4[char_index][0];
    }
    else
    {
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
        char_index = (int)(((uint8_t) c) & ~(CHAR_SEMI_GRAPHICS | CHAR_INVERSE));
        bit_pattern_array = &font_img5x7[char_index][0];
    }

    /* Output characters
     */
    for ( char_row = 0; char_row < FONT_HEIGHT; char_row++, py++ )
    {
        bit_pattern = bit_pattern_array[char_row];

        pix_pos = 0x80;

        for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
        {
            /* Bit is set in Font, print pixel(s) in text color
             */
            if ( (bit_pattern & pix_pos) )
            {
                pixel_group += fg_color << byte_position;
            }
            /* Bit is cleared in Font
             */
            else
            {
                pixel_group += bg_color << byte_position;
            }

            /* Move to the next pixel position
             */
            pix_pos = pix_pos >> 1;

            /* Render four pixels at once
             */
            if ( byte_position == 24 )
            {
                frame_buffer_index = px + py * SCREEN_WIDTH_PIX + char_col - 3;
                *((uint32_t *)(fbp + frame_buffer_index)) = pixel_group;
                byte_position = 0;
                pixel_group = 0;
            }
            else
            {
                byte_position += 8;
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

    uint32_t    byte_position;
    uint32_t    frame_buffer_index;
    register    uint32_t    pixel_group;

    byte_position = 0;
    pixel_group = 0;

    /* Convert text column and row to pixel positions
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
                pixel_group += fg_color << byte_position;
            }
            /* Bit is cleared in Font
             */
            else
            {
                pixel_group += bg_color << byte_position;
            }

            /* Move to the next pixel position
             */
            pix_pos = pix_pos >> 1;

            /* Render four pixels at once
             */
            if ( byte_position == 24 )
            {
                frame_buffer_index = px + py * SCREEN_WIDTH_PIX + char_col - 3;
                *((uint32_t *)(fbp + frame_buffer_index)) = pixel_group;
                byte_position = 0;
                pixel_group = 0;
            }
            else
            {
                byte_position += 8;
            }
        }
    }
}

/*------------------------------------------------
 * vdg_draw_semig_ext()
 *
 * Render semigraphics-8 -12 or -24 character in the screen frame buffer.
 * Mode can only be SEMI_GRAPHICS_8, SEMI_GRAPHICS_12, and SEMI_GRAPHICS_24 as
 * this is not checked for validity.
 *
 * param:  Extended semigraphics mode, base address of video memory to scan and render, and its length
 * return: none
 *
 */
static void vdg_draw_semig_ext(video_mode_t mode, int video_mem_base, int text_buffer_length)
{
    int             i;
    uint8_t         c;
    uint8_t         pix_pos;
    uint8_t         bit_pattern[SEMIG8_SEG_HEIGHT];
    uint32_t        px, py, text_buff_index;
    int             char_row, char_col, char_index, char_row_index, segment_height;
    uint32_t        fg_color, bg_color;

    const uint8_t  *bit_pattern_array;
    uint32_t        byte_position;
    uint32_t        frame_buffer_index;
    register        uint32_t    pixel_group;

    /* Common initialization
     */
    char_row_index = 0;
    byte_position = 0;
    pixel_group = 0;

    if ( mode == SEMI_GRAPHICS_8 )
        segment_height = SEMIG8_SEG_HEIGHT;
    else if ( mode == SEMI_GRAPHICS_12 )
        segment_height = SEMIG12_SEG_HEIGHT;
    else
        segment_height = SEMIG24_SEG_HEIGHT;

    /* Outer loop reads bytes from Dragon text buffer
     */
    for ( text_buff_index = 0; text_buff_index < text_buffer_length; text_buff_index++ )
    {
        c = mem_read(text_buff_index + video_mem_base);

        /* Mode-dependent initializations
         * for text or semigraphics:
         * - Determine foreground and background colors
         * - Character pattern array
         * - Character code index to bit pattern array
         * - Use Semigraphics 4 set because according to SAM spec. L0 = L2 and L1 = L3 (can I trust this?)
         *
         */
        bg_color = FB_BLACK;

        if ( c & CHAR_SEMI_GRAPHICS )
        {
            fg_color = colors[((c & 0b01110000) >> 4)];
            char_index = (int)(c & SEMI_GRAPH8_MASK);
            bit_pattern_array = &semi_graph_4[char_index][char_row_index];
        }
        else
        {
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
            char_index = (int)(c & ~(CHAR_SEMI_GRAPHICS | CHAR_INVERSE));
            bit_pattern_array = &font_img5x7[char_index][char_row_index];
        }

        /* Pixel positions for semigraphics
         */
        px = (text_buff_index & 0x1f) * FONT_WIDTH;
        py = (text_buff_index >> 5) * segment_height * SCREEN_WIDTH_PIX;

        /* Get 3, 2 or 1 scan line of the alpha or semi4 character.
         */
        for ( i = 0; i < segment_height; i++ )
        {
            bit_pattern[i] = bit_pattern_array[i];
        }

        /* Render segment of alpha or semi4 character.
         */
        for ( char_row = 0; char_row < segment_height; char_row++ )
        {
            pix_pos = 0x80;

            for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
            {
                /* Bit is set in Font, print pixel(s) in text color
                 */
                if ( bit_pattern[char_row] & pix_pos )
                {
                    pixel_group += fg_color << byte_position;
                }
                /* Bit is cleared in Font
                 */
                else
                {
                    pixel_group += bg_color << byte_position;
                }

                /* Move to the next pixel position
                 */
                pix_pos = pix_pos >> 1;

                /* Render four pixels at once
                 */
                if ( byte_position == 24 )
                {
                    frame_buffer_index = (px + char_col - 3) + (py + (char_row * SCREEN_WIDTH_PIX));
                    //printf("px %u, py %u, frame_buffer_index %u, char_col %d, char_row %d\n", px, py, frame_buffer_index, char_col, char_row);
                    *((uint32_t *)(fbp + frame_buffer_index)) = pixel_group;
                    byte_position = 0;
                    pixel_group = 0;
                }
                else
                {
                    byte_position += 8;
                }
            }
        } /* End of render loop */

        /* Move to next character segment at the end of a 32 bytes row
         * or back to top of segment after completing character height
         */
        if ( (text_buff_index & 0x1f) == 0x1f )
        {
            char_row_index += segment_height;
            if ( char_row_index >= FONT_HEIGHT )
                char_row_index = 0;
        }

    } /* end of outer loop */
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
    video_mode_t mode = UNDEFINED;

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
