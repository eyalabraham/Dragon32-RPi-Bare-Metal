/********************************************************************
 * pia.h
 *
 *  Header for module that implements the MC6821 PIA functionality.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#ifndef __PIA_H__
#define __PIA_H__

void pia_init(void);

void pia_vsync_irq(void);
int  pia_function_key(void);

#endif  /* __PIA_H__ */
