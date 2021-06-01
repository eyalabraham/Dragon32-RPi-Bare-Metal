/********************************************************************
 * vdg.h
 *
 *  Header for module that implements the MC6847
 *  Video Display Generator (VDG) functionality.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#ifndef __VDG_H__
#define __VDG_H__

#define     VDG_REFRESH_RATE        50      // in Hz

void vdg_init(void);
void vdg_render(void);

void vdg_set_video_offset(uint8_t offset);
void vdg_set_mode_sam(int sam_mode);
void vdg_set_mode_pia(uint8_t pia_mode);

#endif  /* __VDG_H__ */
