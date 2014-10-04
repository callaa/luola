/*
 * LDAT - Luola Datafile format
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : ldat.h
 * Description : Handle Luola datafiles
 * Author(s)   : Calle Laakkonen
 *
 * LDAT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Luola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef LDAT_H
#define LDAT_H

#include "SDL_rwops.h"

#define LDAT_MAJOR	0x01
#define LDAT_MINOR	0x00

/* A structure to hold a catalog item */
typedef struct LDAT_Block {
    char *ID;
    Uint16 index;
    Uint32 size;
    Uint32 pos;
    SDL_RWops *data;            /* cache */

    struct LDAT_Block *prev;
    struct LDAT_Block *next;
} LDAT_Block;

/* A structure to handle the LDAT file */
typedef struct {
    LDAT_Block *catalog;
    Uint16 items;
    Uint32 catalog_size;
    SDL_RWops *data;
} LDAT;


/** Opening/closing **/
/* Check if a file is an LDAT archive */
extern int is_ldat(const char *filename);

/* Open an SDL_RWops for reading */
extern LDAT *ldat_open_rw (SDL_RWops * rw);

/* Open a file for reading */
extern LDAT *ldat_open_file (const char *filename);

/* Creates an empty LDAT structure */
extern LDAT *ldat_create (void);

/* Save LDAT into an SDL_RWops */
extern int ldat_save_rw (LDAT * ldat, SDL_RWops * rw);

/* Save LDAT into a file */
extern int ldat_save_file (LDAT * ldat,const char *filename);

/* Close the LDAT (read/write only to cache) */
extern void ldat_close (LDAT * ldat);

/* Close and free the LDAT */
extern void ldat_free (LDAT * ldat);

/** LDAT reading functions **/
/* Get a file from the archive */
extern SDL_RWops *ldat_get_item (const LDAT * ldat, const char *id, int item);

/* Get the length of the item */
extern int ldat_get_item_length (const LDAT * ldat, const char *id, int item);

/* Get the number of items with the same ID */
extern int ldat_get_item_count (const LDAT * ldat, const char *id);

/* Find an item */
extern LDAT_Block *ldat_find_item (const LDAT * ldat, const char *id, int item);
/** LDAT writing functions **/
/* Put a file into the archive */
extern void ldat_put_item (LDAT * ldat, char *id, int item, SDL_RWops * data,
                    Uint32 len);

#endif
