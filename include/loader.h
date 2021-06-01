/********************************************************************
 * loader.h
 *
 *  Header for ROM and CAS file loader
 *
 *  May 11, 2021
 *
 *******************************************************************/

#ifndef __LOADER_H__
#define __LOADER_H__

#include    "sdfat32.h"

void loader(void);
int  loader_mount_cas_file(dir_entry_t *cas_file);

#endif  /* __LOADER_H__ */
