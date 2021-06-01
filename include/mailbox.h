/*
 * mailbox.h
 *
 *  Driver library module BCM2835 ARM->VC mailbox interface
 *
 *  Resources:
 *      https://github.com/raspberrypi/firmware/wiki/Mailboxes
 *      https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 *
 */

/*

    Part of the Raspberry-Pi Bare Metal Tutorials
    Copyright (c) 2015, Brian Sidebotham
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef     __MAILBOX_H__
#define     __MAILBOX_H__

#include    <stdint.h>

#include    "bcm2835.h"

/* Mailbox request and response status
 */
#define     MB_REQUEST                  0x00000000
#define     MB_RESPONSE_OK              0x80000000
#define     MB_RESPONSE_ERR             0x00000001
#define     MB_STATUS_FULL              0x80000000
#define     MB_STATUS_EMPTY             0x40000000
#define     MB_STATUS_LEVEL             0x400000FF

/* General system tags (channel 8)
 */
#define     TAG_VC_REV                  0x00000001
#define     TAG_BOARD_MODEL             0x00010001
#define     TAG_BOARD_REV               0x00010002
#define     TAG_MAC_ADDRESS             0x00010003
#define     TAG_BOARD_SN                0x00010004
#define     TAG_ARM_MEMORY              0x00010005
#define     TAG_VC_MEMORY               0x00010006

/* Frame buffer tags (channel 8)
 */
#define     TAG_FB_ALLOCATE             0x00040001
#define     TAG_FB_RELEASE              0x00048001
#define     TAG_FB_GET_PHYS_DISPLAY     0x00040003  // Display width (param1) height (param2) in pixels
#define     TAG_FB_GET_VIRT_DISPLAY     0x00040004  // Virtual display width (param1) height (param2) in pixels
#define     TAG_FB_GET_DEPTH            0x00040005  // Display depth bits per pixel (param1)
#define     TAG_FB_GET_PIXEL_ORDER      0x00040006  // 0- BRG, 1- RGB (param1)
#define     TAG_FB_GET_ALPHA_MODE       0x00040007  // 0- enabled, 1- reverse, 2- ignored (param1)
#define     TAG_FB_GET_PITCH            0x00040008  // Bytes per line (param1)
#define     TAG_FB_GET_VIRT_OFFSET      0x00040009  // X pixels (param1), Y pixels (param2)
#define     TAG_FB_GET_OVERSCAN         0x0004000a  // In pixels: top (param1), bottom (param2), left (param3), right (param4)
#define     TAG_FB_GET_PALETTE          0x0004000b  // Get color palette for 8-bit per pixel graphics
#define     TAG_FB_SET_BLANK            0x00040002  // Blank the screen 0- off, 1- on (param1)
#define     TAG_FB_SET_PHYS_DISPLAY     0x00048003  // Display width (param1) height (param2) in pixels
#define     TAG_FB_SET_VIRT_DISPLAY     0x00048004  // Virtual display width (param1) height (param2) in pixels
#define     TAG_FB_SET_DEPTH            0x00048005  // Bits per pixel (param1)
#define     TAG_FB_SET_PIXEL_ORDER      0x00048006  // 0- BRG, 1- RGB (param1)
#define     TAG_FB_SET_ALPHA_MODE       0x00048007  // 0- enabled, 1- reverse, 2- ignored (param1)
#define     TAG_FB_SET_VIRT_OFFSET      0x00048009  // X pixels (param1), Y pixels (param2)
#define     TAG_FB_SET_OVERSCAN         0x0004800a  // In pixels: top (param1), bottom (param2), left (param3), right (param4)
#define     TAG_FB_SET_PALETTE          0x0004800b  // Set color palette for 8-bit per pixel graphics

/* Tag structures
 */
struct versions_t
{
    uint32_t    fw_rev;             // Firmware rev in the response
};

struct mac_t
{
    uint8_t     mac[6];             // MAC address in network byte order
    uint16_t    pad;                // 0x0000
};

struct memory_t
{
    uint32_t    base_address;
    uint32_t    size;
};

struct fb_allocate_t
{
    uint32_t    param1;             // request: alignment, response: address
    uint32_t    param2;             //                     response: size
};

struct fb_get_t
{
    uint32_t    param1;
    uint32_t    param2;
    uint32_t    param3;
    uint32_t    param4;
};

struct fb_set_t
{
    uint32_t    param1;
    uint32_t    param2;
    uint32_t    param3;
    uint32_t    param4;
};

struct fb_get_palette_t
{
    uint32_t   palette[256];        // Palette indexes 0 to 255
};

struct fb_set_palette_t
{
    uint32_t    palette_offset;     // Request: first palette index, response: 0- valid or 1- not valid
    uint32_t    palette_length;
    uint32_t    palette[256];       // 'palette_length' x palette values for indexes starting at 'palette_offset'
};

/* general mailbox and tag structure
 */
typedef struct
{
    uint32_t    tag;                // See define TAG_* list
    uint32_t    values_length;      // Value buffer size in bytes
    uint32_t    req_resp_status;    // Request: b31 clear,  Response: b31 set, b30-b0: value length in bytes
    union values_t                  // One structure in the union per Mailbox tag call
    {
        struct versions_t version;
        struct mac_t mac_address;
        struct memory_t memory;
        struct fb_allocate_t fb_alloc;
        struct fb_get_t fb_get;
        struct fb_set_t fb_set;
        struct fb_get_palette_t fb_get_palette;
        // struct fb_set_palette_t fb_set_palette;
    } values;
} mailbox_tag_property_t;

/* The available mailbox channels in the BCM2835 Mailbox interface.
   See: https://github.com/raspberrypi/firmware/wiki/Mailboxes for
 */
typedef enum {
    MB0_POWER_MANAGEMENT = 0,
    MB0_FRAMEBUFFER,    // *** Deprecated, use channel 8 instead ***
    MB0_VIRTUAL_UART,
    MB0_VCHIQ,
    MB0_LEDS,
    MB0_BUTTONS,
    MB0_TOUCHSCREEN,
    MB0_UNUSED,
    MB0_TAGS_ARM_TO_VC,
    MB0_TAGS_VC_TO_ARM,
} mailbox0_channel_t;


int       bcm2835_mailbox0_write(mailbox0_channel_t channel, uint32_t mailbox_buffer_address);
uint32_t  bcm2835_mailbox0_read(mailbox0_channel_t channel);

void      bcm2835_mailbox_init(void);
void      bcm2835_mailbox_add_tag(uint32_t tag, ...);
uint32_t *bcm2835_mailbox_process(void);
mailbox_tag_property_t *bcm2835_mailbox_get_property(uint32_t tag);

#endif  /* __MAILBOX_H__ */
