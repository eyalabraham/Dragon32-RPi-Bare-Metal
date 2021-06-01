/********************************************************************
 * mem.h
 *
 *  Header file that defines the memory module interface
 *
 *  July 2, 2020
 *
 *******************************************************************/

#ifndef __MEM_H__
#define __MEM_H__

#include    <stdint.h>

#define     MEMORY                  65536       // 64K Byte

#define     MEM_OK                  0           // Operation ok
#define     MEM_ADD_RANGE          -1           // Address out of range
#define     MEM_ROM                -2           // Location is ROM
#define     MEM_HANDLER_ERR        -3           // Cannot hook IO handler

typedef enum
{
    MEM_READ,
    MEM_WRITE,
} mem_operation_t;

typedef uint8_t (*io_handler_callback)(uint16_t, uint8_t, mem_operation_t);

/********************************************************************
 *  Memory module API
 */

void mem_init(void);

int  mem_read(int address);
int  mem_write(int address, int data);
int  mem_define_rom(int addr_start, int addr_end);
int  mem_define_io(int addr_start, int addr_end, io_handler_callback io_handler);
int  mem_load(int addr_start, uint8_t *buffer, int length);

#endif  /* __MEM_H__ */
