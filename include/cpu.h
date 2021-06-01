/********************************************************************
 * cpu.h
 *
 *  Header file that defines the MC6809E CPU module interface
 *
 *  July 2, 2020
 *
 *******************************************************************/

#ifndef __CPU_H__
#define __CPU_H__

#include    <stdint.h>

/********************************************************************
 *  CPU run state
 */
typedef enum
{
    NOT_DEFINED     = -1,
    CPU_EXEC        = 0,    // Normal state instruction execution
    CPU_HALTED      = 1,    // Is halted
    CPU_SYNC        = 2,    // Waiting in SYNC state (for 'sync' and 'cwai')
    CPU_RESET       = 4,    // Held in reset
    CPU_EXCEPTION   = 5,    // Signal an emulation exception (bad op-code)
} cpu_run_state_t;

/* MC6809E CPU state
 */
typedef struct
{
    /* Last command executed
     */
    cpu_run_state_t cpu_state;
    uint16_t        last_pc;
    int             last_opcode_bytes;
    int             last_opcode_cycles;

    /* Registers reflecting
     * machine state after last command execution
     */
    uint16_t    x;
    uint16_t    y;
    uint16_t    u;
    uint16_t    s;
    uint16_t    pc;
    uint8_t     a;
    uint8_t     b;
    uint8_t     dp;
    uint8_t     cc;

    /* State after last command execution
     */
    int     int_latch;
    int     nmi_armed;
    int     nmi_latched;
    int     halt_asserted;
    int     reset_asserted;
    int     irq_asserted;
    int     firq_asserted;
    int     exception_line_num;
} cpu_state_t;

/********************************************************************
 *  CPU module API
 */
int  cpu_init(int address);

void cpu_halt(int state);
void cpu_reset(int state);
void cpu_nmi_trigger(void);
void cpu_firq(int state);
void cpu_irq(int state);

cpu_run_state_t cpu_run(void);

cpu_run_state_t cpu_get_state(cpu_state_t* cpu_state);
const char*     cpu_get_menmonic(uint16_t address);

#endif  /* __CPU_H__ */
