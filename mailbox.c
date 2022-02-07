/*
 * mailbox.c
 *
 *  Driver library module BCM2835 ARM->VC mailbox interface
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
#include    <stdarg.h>
#include    <string.h>

#include    "mailbox.h"

/* -----------------------------------------
   Definitions
----------------------------------------- */
#define     PT_OSIZE                    0
#define     PT_OREQUEST_OR_RESPONSE     1

#define     T_OIDENTIFIER               0
#define     T_OVALUE_SIZE               1
#define     T_ORESPONSE                 2
#define     T_OVALUE                    3

#define     T_ORESPONSE_FLAG_MASK       0x8000000

/* -----------------------------------------
   Types and data structures
----------------------------------------- */

/* -----------------------------------------
   Module static functions
----------------------------------------- */

/* -----------------------------------------
   Module globals
----------------------------------------- */
static mailbox_t   *pMailbox0 = (mailbox_t*)BCM2835_MAILBOX0_BASE;
static int          property_index = 0;

/* Probably a large enough buffer for staging mailbox property tags
 * but I don't like the fact that there is a buffer overrun risk.
 * TODO Find an elegant way to allocate space, or protect from overrun.
 */
static uint32_t     pt[8192] __attribute__((aligned(16)));

/*------------------------------------------------
 * bcm2835_mailbox0_write()
 *
 *  Send (post) a mailbox message from ARM to VC.
 *  'mailbox_buffer_address' must be 16-byte aligned.
 *
 * param:  Channel and mailbox message buffer address
 * return: 1- if successful, 0- otherwise
 *
 */
int bcm2835_mailbox0_write(mailbox0_channel_t channel, uint32_t mailbox_buffer_address)
{
    /* Check address alignment
     */
    if ( mailbox_buffer_address & 0x0000000f )
        return 0;

    /* Merge channel number into the lower 4 bits
     */
    mailbox_buffer_address |= channel;

    /* Wait until the mailbox becomes available and then write to the mailbox
     */
    while( ( pMailbox0->status & MB_STATUS_FULL ) != 0 )
    {
        /* Wait */
    }

    dmb();

    /* Write the modified value + channel number into the write register
     */
    pMailbox0->write = mailbox_buffer_address;

    return 1;
}

/*------------------------------------------------
 * bcm2835_mailbox0_read()
 *
 *  Block and wait to read a mailbox response from VC to ARM
 *
 * param:  Channel number
 * return: Buffer address of response
 *
 */
uint32_t bcm2835_mailbox0_read(mailbox0_channel_t channel)
{
    uint32_t    value = 0xffffffff;

    /* Keep reading the register until the desired channel gives us a value
     */
    while ( (value & 0xF) != channel )
    {
        /* Wait while the mailbox is empty
         */
        while( pMailbox0->status & MB_STATUS_EMPTY )
        {
            /* Wait */
        }

        dmb();

        /* Extract the value from the Read register of the mailbox.
         * The TODO What is in the upper 28 bits?
         */
        value = pMailbox0->read;
        dmb();
    }

    return value >> 4;
}

/*------------------------------------------------
 * bcm2835_mailbox_init()
 *
 *  Initialize the mailbox properties' buffer. The function
 *  creates an empty entry in the mailbox properties' buffer.
 *  Must call this function before any bcm2835_mailbox_add_tag()
 *
 * param:  none
 * return: none
 *
 */
void bcm2835_mailbox_init(void)
{
    memset(pt, 0, sizeof(pt));
    pt[PT_OSIZE] = 12;
    pt[PT_OREQUEST_OR_RESPONSE] = MB_REQUEST;
    property_index = 2;
    pt[property_index] = 0;
}

/*------------------------------------------------
 * bcm2835_mailbox_init()
 *
 *  Add tags and tag parameters to the mailbox buffer.
 *  Reference: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 *  TODO Add buffer overrun protection for pt[]
 *
 * param:  Tag value to add and list of tag parameters (see reference URL)
 * return: none
 *
 */
void bcm2835_mailbox_add_tag(uint32_t tag, ...)
{
    uint32_t    i, offset, length;
    uint32_t   *palette;
    va_list     vl;

    va_start(vl, tag);

    pt[property_index++] = tag;

    switch (tag)
    {
        case TAG_VC_REV:
        case TAG_BOARD_MODEL:
        case TAG_BOARD_REV:
            pt[property_index++] = 4;
            pt[property_index++] = MB_REQUEST;
            property_index += 1;
            break;

        case TAG_MAC_ADDRESS:
        case TAG_BOARD_SN:
        case TAG_ARM_MEMORY:
        case TAG_VC_MEMORY:
            pt[property_index++] = 8;
            pt[property_index++] = MB_REQUEST;
            property_index += 2;
            break;

        case TAG_FB_ALLOCATE:
        case TAG_GET_CLOCK_RATE:
            pt[property_index++] = 8;
            pt[property_index++] = MB_REQUEST;
            pt[property_index++] = va_arg(vl, uint32_t);
            property_index += 1;
            break;

        case TAG_FB_RELEASE:
            pt[property_index++] = 0;
            pt[property_index++] = MB_REQUEST;
            break;

        case TAG_FB_SET_PHYS_DISPLAY:
        case TAG_FB_SET_VIRT_DISPLAY:
        case TAG_FB_SET_VIRT_OFFSET:
            pt[property_index++] = 8;
            pt[property_index++] = MB_REQUEST;
            pt[property_index++] = va_arg(vl, uint32_t);    // Width / x
            pt[property_index++] = va_arg(vl, uint32_t);    // Height / y
            break;

        case TAG_FB_GET_PHYS_DISPLAY:
        case TAG_FB_GET_VIRT_DISPLAY:
        case TAG_FB_GET_VIRT_OFFSET:
            pt[property_index++] = 8;
            pt[property_index++] = MB_REQUEST;
            property_index += 2;
            break;

        case TAG_FB_SET_ALPHA_MODE:
        case TAG_FB_SET_DEPTH:
        case TAG_FB_SET_PIXEL_ORDER:
            pt[property_index++] = 4;
            pt[property_index++] = MB_REQUEST;
            pt[property_index++] = va_arg(vl, uint32_t);    // Color Depth, bits-per-pixel \ Pixel Order State
            break;

        case TAG_FB_GET_ALPHA_MODE:
        case TAG_FB_GET_DEPTH:
        case TAG_FB_GET_PIXEL_ORDER:
        case TAG_FB_GET_PITCH:
            pt[property_index++] = 4;
            pt[property_index++] = MB_REQUEST;
            property_index += 1;
            break;

        case TAG_FB_SET_OVERSCAN:
            pt[property_index++] = 16;
            pt[property_index++] = MB_REQUEST;
            pt[property_index++] = va_arg(vl, uint32_t);    // Top pixels
            pt[property_index++] = va_arg(vl, uint32_t);    // Bottom pixels
            pt[property_index++] = va_arg(vl, uint32_t);    // Left pixels
            pt[property_index++] = va_arg(vl, uint32_t);    // Right pixels
            break;

        case TAG_FB_GET_OVERSCAN:
            pt[property_index++] = 16;
            pt[property_index++] = MB_REQUEST;
            property_index += 4;
            break;

        case TAG_FB_SET_PALETTE:
            offset = va_arg(vl, uint32_t);
            length = va_arg(vl, uint32_t);
            if ( (offset > 255) || (length > (256 - offset)) )
            {
                property_index--;
                break;
            }
            palette = (uint32_t*)va_arg(vl, uint32_t);
            pt[property_index++] = 4*(2 + length);
            pt[property_index++] = MB_REQUEST;
            pt[property_index++] = offset;
            pt[property_index++] = length;
            for (i = 0; i < length; i++)
                pt[property_index++] = palette[i];
            break;

        case TAG_FB_GET_PALETTE:
            pt[property_index++] = 1024;
            pt[property_index++] = MB_REQUEST;
            property_index += 256;
            break;

        default:
            /* Unsupported tags, just remove the tag from the list
             */
            property_index--;
            break;
    }

    /* Make sure the tags are 0 terminated to end the list and update the buffer size
     */
    pt[property_index] = 0;

    va_end(vl);
}

/*------------------------------------------------
 * bcm2835_mailbox_process()
 *
 *  Send mailbox buffer to mailbox for processing by VideoCore GPU
 *
 * param:  none
 * return: buffer address of response if successful, NULL- otherwise
 *
 */
uint32_t *bcm2835_mailbox_process(void)
{
    uint32_t result;

    /* Fill in the size of the buffer
     */
    pt[PT_OSIZE] = (property_index + 1) << 2;
    pt[PT_OREQUEST_OR_RESPONSE] = 0;

    if ( !bcm2835_mailbox0_write(MB0_TAGS_ARM_TO_VC, (uint32_t)pt) )
        return NULL;

    result = bcm2835_mailbox0_read(MB0_TAGS_ARM_TO_VC);

    return (uint32_t*) result;
}

/*------------------------------------------------
 * bcm2835_mailbox_get_property()
 *
 *  Search for a tag property in the processed mailbox buffer
 *  and return a pointer to the tag's properties.
 *
 * param:  Tag to search
 * return: Pointer to tag properties/values or NULL if not found
 *
 */
mailbox_tag_property_t *bcm2835_mailbox_get_property(uint32_t tag)
{
    uint32_t   *tag_buffer = NULL;
    int         index = 2;

    if ( pt[PT_OREQUEST_OR_RESPONSE] & MB_RESPONSE_ERR )
        return NULL;

    /* Search for the tag in the return buffer and
     * return NULL if the property tag cannot be found in the buffer
     */
    while ( index < (pt[PT_OSIZE] >> 2) )
    {
        if( pt[index] == tag )
        {
            tag_buffer = &pt[index];
            break;
        }

        index += ( pt[index + 1] >> 2 ) + 3;
    }

    if( tag_buffer == NULL )
        return NULL;

    /* Return the required data
     */
    return (mailbox_tag_property_t*)tag_buffer;
}
