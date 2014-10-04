/*
 * LDAT - Luola Datafile format
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : ldat.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_endian.h"

#include "ldat.h"

#define LDAT_MAGIC_LEN 6
#define LDAT_HEADER_LEN 8

/* Utility functions, equivalent to SDL_ReadLE??, except
 * with error checking
 */
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
int Luola_ReadLE16(SDL_RWops *src,Uint16 *value)
{
    if(SDL_RWread(src, value, 2, 1)==-1) return -1;
    *value = SDL_Swap16(*value);
    return 2;

}

int Luola_ReadLE32(SDL_RWops *src,Uint32 *value)
{
    if(SDL_RWread(src, value, 4, 1)==-1) return -1;
    *value = SDL_Swap32(*value);
    return 4;

}
#else

#define Luola_ReadLE16(src,value) SDL_RWread(src,value,2,1)
#define Luola_ReadLE32(src,value) SDL_RWread(src,value,4,1)

#endif

/* Check if a file is an LDAT archive */
int is_ldat(const char *filename)
{
    char buffer[6];
    FILE *fp;
    fp = fopen(filename,"rb");
    if(!fp) {
        perror(filename);
        return 0;
    }
    if(fread(buffer,1,sizeof(buffer),fp)!=sizeof(buffer)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    if(memcmp(buffer,"LDAT",4)==0 && buffer[4]<10 && buffer[5]<10)
        return 1;
    return 0;
}

/* Create an empty LDAT structure */
LDAT *ldat_create (void)
{
    LDAT *newldat;
    newldat = malloc (sizeof (LDAT));
    memset (newldat, 0, sizeof (LDAT));
    return newldat;
}

/* Open an SDL_RWops for reading */
LDAT *ldat_open_rw (SDL_RWops * rw)
{
    char header[LDAT_MAGIC_LEN];
    LDAT *newldat;
    LDAT_Block *newblock;
    Uint16 i, itemcount;
    Uint8 idlen;
    newldat = ldat_create ();
    newldat->data = rw;
    /* Read header */
    SDL_RWread (newldat->data, header, 1, LDAT_MAGIC_LEN);
    if (strncmp (header, "LDAT", 4)) {
        fprintf (stderr,"Error: this is not an LDAT archive !\n");
        return NULL;
    }
    if (header[4] != LDAT_MAJOR) {
        fprintf (stderr, "Error: Unsupported version (%d.%d) !\n", header[4],
                header[5]);
        fprintf (stderr, "Latest supported version is %d.%d\n", LDAT_MAJOR,
                LDAT_MINOR);
        return NULL;
    }
    /* Read catalog */
    if(Luola_ReadLE16 (newldat->data,&itemcount) == -1) {
        fprintf(stderr,"Error occured while reading itemcount!\n");
    }

    for (i = 0; i < itemcount; i++) {
        newblock = malloc (sizeof (LDAT_Block));
        memset (newblock, 0, sizeof (LDAT_Block));
        if (SDL_RWread (newldat->data, &idlen, 1, 1) == -1) {
            fprintf (stderr, "(%d) Error occured while reading idlen!\n", i);
            return NULL;
        }
        newblock->ID = malloc (idlen + 1);
        if (SDL_RWread (newldat->data, newblock->ID, 1, idlen) == -1) {
            fprintf (stderr, "(%d) Error occured while reading ID string!\n", i);
            return NULL;
        }
        newblock->ID[idlen] = '\0';
        if (Luola_ReadLE16(newldat->data, &newblock->index) == -1) {
            fprintf (stderr, "(%d) Error occured while reading index number!\n", i);
            return NULL;
        }

        if (Luola_ReadLE32(newldat->data, &newblock->pos) == -1) {
            fprintf (stderr, "(%d) Error occured while reading position!\n", i);
            return NULL;
        }
        if (Luola_ReadLE32(newldat->data, &newblock->size) == -1) {
            fprintf (stderr, "(%d) Error occured while reading size!\n", i);
            return NULL;
        }
        if (newldat->catalog == NULL)
            newldat->catalog = newblock;
        else {
            newblock->prev = newldat->catalog;
            newldat->catalog->next = newblock;
            newldat->catalog = newblock;
        }
    }
    newldat->catalog_size = SDL_RWtell (newldat->data) - LDAT_HEADER_LEN;
    /* Rewing catalog */
    while (newldat->catalog->prev)
        newldat->catalog = newldat->catalog->prev;
    return newldat;
}

/* Open a file for reading */
LDAT *ldat_open_file (const char *filename)
{
    SDL_RWops *rw;
    rw = SDL_RWFromFile (filename, "rb");
    if (rw == NULL) {
        fprintf (stderr, "Could not open file \"%s\"\n", filename);
        return NULL;
    }
    return ldat_open_rw (rw);
}

/* Close the LDAT (read/write only to cache) */
void ldat_close (LDAT * ldat)
{
    if (ldat->data)
        SDL_RWclose (ldat->data);
}

/* Close and free the LDAT structure */
void ldat_free (LDAT * ldat)
{
    LDAT_Block *next;
    ldat_close (ldat);
    while (ldat->catalog) {
        next = ldat->catalog->next;
        free(ldat->catalog->ID);
        if (ldat->catalog->data)
            SDL_FreeRW (ldat->catalog->data);
        free (ldat->catalog);
        ldat->catalog = next;
    }
    free (ldat);
}

/* Put a file into the archive */
void ldat_put_item (LDAT * ldat,char *id, int item, SDL_RWops * data,
                    Uint32 len)
{
    LDAT_Block *newitem, *catalog;
    catalog = ldat->catalog;
    if (catalog)
        while (catalog->next) {
            if (catalog->index == item && strcmp (catalog->ID, id) == 0)
                fprintf (stderr,"Warning: Already found an entry with ID \"%s\" and index %d!\n", id, item);
            catalog = catalog->next;
        }
    newitem = malloc (sizeof (LDAT_Block));
    memset (newitem, 0, sizeof (LDAT_Block));
    newitem->data = data;
    newitem->ID = id;
    newitem->index = item;
    newitem->size = len;
    newitem->prev = catalog;
    if (catalog)
        catalog->next = newitem;
    else
        ldat->catalog = newitem;
    ldat->items++;
    ldat->catalog_size += 11 + strlen (id);
}

/* Fix things before saving */
/* Set the correct positions for files */
static void ldat_fixup (LDAT * ldat)
{
    LDAT_Block *block = ldat->catalog;
    Uint32 latest;
    latest = LDAT_HEADER_LEN + ldat->catalog_size;
    while (block) {
        block->pos = latest;
        latest += block->size;
        block = block->next;
    }
}

/* Copy from one SDL_RWops to another */
static int rw_to_rw_copy (SDL_RWops * dest, SDL_RWops * src, Uint32 len)
{
    Uint8 buffer[1024];
    Uint32 copied = 0, read, toread;
    while (copied < len) {
        if (len - copied < sizeof(buffer))
            toread = len - copied;
        else
            toread = sizeof(buffer);
        read = SDL_RWread (src, buffer, 1, toread);
        if(read==0) {
            fprintf(stderr,"rw_to_rw_copy:%d: %s\n",read, SDL_GetError());
            return 1;
        }
        if(SDL_RWwrite (dest, buffer, 1, read)==0) {
            fprintf(stderr,"rw_to_rw_copy:%d: %s\n",read, SDL_GetError());
            return 1;
        }
        copied += read;
    }
    return 0;
}

/* Save LDAT into an SDL_RWops */
int ldat_save_rw (LDAT * ldat, SDL_RWops * rw)
{
    LDAT_Block *block = ldat->catalog;
    char header[LDAT_MAGIC_LEN] = "LDAT00";
    Uint8 idlen;
    /* Write header */
    header[4] = LDAT_MAJOR;
    header[5] = LDAT_MINOR;
    SDL_RWwrite (rw, &header, 1, LDAT_MAGIC_LEN);
    SDL_RWwrite (rw, &ldat->items, 2, 1);
    /* Write catalog */
    ldat_fixup (ldat);
    while (block) {
        idlen = strlen (block->ID);
        SDL_RWwrite (rw, &idlen, 1, 1);
        SDL_RWwrite (rw, block->ID, 1, idlen);
        SDL_RWwrite (rw, &block->index, 2, 1);
        SDL_RWwrite (rw, &block->pos, 4, 1);
        SDL_RWwrite (rw, &block->size, 4, 1);
        block = block->next;
    }
    /* Write data */
    block = ldat->catalog;
    while (block) {
        if(rw_to_rw_copy (rw, block->data, block->size)) {
            fprintf(stderr,"Error occured in block \"%s\" %d\n",
                    block->ID,block->index);
            return 1;
        }
        block = block->next;
    }
    return 0;
}

/* Save LDAT into a file */
int ldat_save_file (LDAT * ldat,const char *filename)
{
    int r;
    SDL_RWops *rw;
    rw = SDL_RWFromFile (filename, "wb");
    if (rw == NULL) {
        printf ("Error! Could not open file \"%s\" for writing\n",
                filename);
        return 1;
    }
    r = ldat_save_rw (ldat, rw);
    SDL_RWclose (rw);
    return r;
}

/* Find an item from the archive */
LDAT_Block *ldat_find_item (const LDAT * ldat, const char *id, int item)
{
    LDAT_Block *block = ldat->catalog;
    while (block) {
        if (block->index == item && strcmp (block->ID, id) == 0)
            return block;
        block = block->next;
    }
    return NULL;
}

/* Get the number of items with the same ID */
int ldat_get_item_count (const LDAT * ldat, const char *id)
{
    LDAT_Block *block = ldat->catalog;
    int count=0;
    while (block) {
        if (strcmp (block->ID, id) == 0)
            count++;
        block = block->next;
    }
    return count;
}

/* Get a file from the archive */
SDL_RWops *ldat_get_item (const LDAT * ldat,const char *id, int item)
{
    LDAT_Block *block;
    SDL_RWops *data;
    block = ldat_find_item (ldat, id, item);
    if (!block) {
        fprintf (stderr,"Item with ID \"%s\" and index %d was not found\n",
                id, item);
        return NULL;
    }
    if (block->data)
        return block->data;
    data = ldat->data;
    SDL_RWseek (data, block->pos, SEEK_SET);
    return data;
}

/* Get the length of the item */
int ldat_get_item_length (const LDAT * ldat,const char *id, int item)
{
    LDAT_Block *block = ldat_find_item(ldat, id, item);
    if (!block) {
        fprintf (stderr,"Item with ID \"%s\" and index %d was not found\n",
                id, item);
        return 0;
    }
    return block->size;
}

