/*
 * LCMAP - An image format designed to store Luola collision maps.
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : lcmap.c
 * Description : Convert LCMAPs into and from SDL_Surfaces
 * Author(s)   : Calle Laakkonen
 *
 * LCMAP is free software; you can redistribute it and/or modify
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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "lcmap.h"

#define LCMAP_FLAG_MASK		0x03
#define LCMAP_FLAG_8BIT		0x01
#define LCMAP_FLAG_16BIT	0x02
#define LCMAP_FLAG_24BIT	0x03

#define PALETTE_ENTRIES		18      /* How many palette entries to store */

Uint8 *surface_to_lcmap (Uint32 * len, SDL_Surface * surface)
{
    Uint32 dlen, flen, linelen;
    Uint16 x, y;
    Uint8 *data, *zdata, *file;
    Uint8 value, *pixel, flags;
    int r, p;
    uLongf zlen;

    if (surface->format->BitsPerPixel != 8) {
        printf ("Error! Surface has incorrect depth\n");
        return NULL;
    }
    data = malloc (surface->w * surface->h);
    dlen = 0;
    pixel = (Uint8 *) surface->pixels;
    value = *pixel;
    linelen = 0;
    /* Encode image */
    for (y = 0; y < surface->h; y++) {
        for (x = 0; x < surface->w; x++) {
            if (*pixel != value || linelen == 0xffffff) {
                if (linelen == 0)
                    flags = 0;
                else if (linelen <= 0xff)
                    flags = LCMAP_FLAG_8BIT;
                else if (linelen <= 0xffff)
                    flags = LCMAP_FLAG_16BIT;
                else
                    flags = LCMAP_FLAG_24BIT;
                data[dlen] = (value << 2) | flags;
                if (flags == LCMAP_FLAG_8BIT) {
                    data[dlen + 1] = linelen;
                    dlen += 2;
                } else if (flags == LCMAP_FLAG_16BIT) {
                    data[dlen + 1] = linelen;
                    data[dlen + 2] = linelen >> 8;
                    dlen += 3;
                } else if (flags == LCMAP_FLAG_24BIT) {
                    data[dlen + 1] = linelen;
                    data[dlen + 2] = linelen >> 8;
                    data[dlen + 3] = linelen >> 16;
                    dlen += 4;
                } else
                    dlen++;
                value = *pixel;
                linelen = 0;
            } else
                linelen++;
            pixel++;
        }
        pixel += surface->pitch - surface->w;
    }
    /* Last entry */
    if (linelen <= 1)
        flags = 0;
    else if (linelen <= 0xff)
        flags = LCMAP_FLAG_8BIT;
    else if (linelen <= 0xffff)
        flags = LCMAP_FLAG_16BIT;
    else
        flags = LCMAP_FLAG_24BIT;
    data[dlen] = (value << 2) | flags;
    if (flags == LCMAP_FLAG_8BIT) {
        data[dlen + 1] = linelen;
        dlen += 2;
    } else if (flags == LCMAP_FLAG_16BIT) {
        data[dlen + 1] = linelen;
        data[dlen + 2] = linelen >> 8;
        dlen += 3;
    } else if (flags == LCMAP_FLAG_24BIT) {
        data[dlen + 1] = linelen;
        data[dlen + 2] = linelen >> 8;
        data[dlen + 3] = linelen >> 16;
        dlen += 4;
    } else
        dlen++;
    /* Compress data */
    zlen = dlen + 200;
    zdata = malloc (zlen);
    if ((r = compress (zdata, &zlen, data, dlen)) == Z_BUF_ERROR) {
        printf ("Error! Not enough space in output buffer\n");
        if (r == Z_MEM_ERROR)
            printf ("Not enough memory\n");
        else if (r == Z_BUF_ERROR)
            printf ("Outputbuffer too small\n");
        else
            printf ("Unknown error\n");
        free (data);
        free (zdata);
        return NULL;
    }
    /* Create the file */
    flen = 5 + 13 + (PALETTE_ENTRIES * 3) + zlen;
    file = malloc (flen);
    file[0] = 'L';
    file[1] = 'C';
    file[2] = 'M';
    file[3] = 'A';
    file[4] = 'P';
    file[5] = surface->w;
    file[6] = surface->w >> 8;
    file[7] = surface->h;
    file[8] = surface->h >> 8;
    file[9] = PALETTE_ENTRIES;
    r = 10;
    for (p = 0; p < PALETTE_ENTRIES; p++) {
        file[r] = surface->format->palette->colors[p].r;
        file[r + 1] = surface->format->palette->colors[p].g;
        file[r + 2] = surface->format->palette->colors[p].b;
        r += 3;
    }
    file[r] = dlen;
    r++;
    file[r] = dlen >> 8;
    r++;
    file[r] = dlen >> 16;
    r++;
    file[r] = dlen >> 24;
    r++;
    file[r] = zlen;
    r++;
    file[r] = zlen >> 8;
    r++;
    file[r] = zlen >> 16;
    r++;
    file[r] = zlen >> 24;
    r++;
    memcpy (file + r, zdata, zlen);
    free (data);
    free (zdata);
    *len = flen;
    return file;
}

SDL_Surface *lcmap_to_surface_rw (SDL_RWops * rw)
{
    SDL_Surface *surface;
    SDL_Color *colors;
    int r, x;
    uLongf uplen;
    Uint32 pos, unpackedlen, zlen, loop;
    Uint16 width, height;
    Uint8 *zdata, *data, *pixels, value, flags, palent, *dataptr;
    Uint8 tmpbuf[8];
    /* Read the headers */
    if (!SDL_RWread (rw, tmpbuf, 1, 5)) {
        printf
            ("Error occured while attempting to read the magic number of an LCMAP file\n");
        return NULL;
    }
    if (strncmp ((char*)tmpbuf, "LCMAP", 5)) {
        printf ("Error! This file doesn't seem to be an LCMAP\n");
        return NULL;
    }
    if (!SDL_RWread (rw, tmpbuf, 1, 5)) {
        printf
            ("Error occured while attempting to read the LCMAP size info.\n");
        return NULL;
    }
    width = tmpbuf[0] | tmpbuf[1] << 8;
    height = tmpbuf[2] | tmpbuf[3] << 8;
    palent = tmpbuf[4];
    colors = malloc (sizeof (SDL_Color) * palent);
    for (r = 0; r < palent; r++) {
        if (!SDL_RWread (rw, tmpbuf, 1, 3)) {
            printf ("Error occured while reading LCMAP palette entry %d\n",
                    r);
            return NULL;
        }
        colors[r].r = tmpbuf[0];
        colors[r].g = tmpbuf[1];
        colors[r].b = tmpbuf[2];
    }
    if (!SDL_RWread (rw, tmpbuf, 1, 8)) {
        printf
            ("Error occured while attempting to read LCMAP data lengths.\n");
        return NULL;
    }
    unpackedlen =
        tmpbuf[0] | tmpbuf[1] << 8 | tmpbuf[2] << 16 | tmpbuf[3] << 24;
    zlen = tmpbuf[4] | tmpbuf[5] << 8 | tmpbuf[6] << 16 | tmpbuf[7] << 24;
    /* Build the image */
    surface =
        SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (surface == NULL) {
        printf ("Error! Couldn't create a surface!\n%s\n", SDL_GetError ());
        return NULL;
    }
    SDL_SetColors (surface, colors, 0, palent);
    /* Unpack data */
    data = malloc (unpackedlen);
    dataptr = data;
    zdata = malloc (sizeof (Uint8) * zlen);
    if (!SDL_RWread (rw, zdata, 1, zlen)) {
        printf ("Error! Could not read image data\n%s\n", SDL_GetError ());
        return NULL;
    }
    uplen = unpackedlen;
    if ((r = uncompress (data, &uplen, zdata, zlen)) != Z_OK) {
        printf ("Error occured during unpack!\n");
        if (r == Z_MEM_ERROR)
            printf ("Not enough memory\n");
        else if (r == Z_BUF_ERROR)
            printf ("Output buffer too small\n");
        else if (r == Z_STREAM_ERROR)
            printf ("Level parameter invalid\n");
        else if (r == Z_DATA_ERROR)
            printf ("Input data corrupted\n");
        else
            printf ("Unknown zlib error %d\n", r);
        return NULL;
    }
    free (zdata);
    if (uplen != unpackedlen) {
        printf
            ("Warning! Data lengths differ! ZLib says %ld, but we say %d\n",
             uplen, unpackedlen);
    }
    /* Decode pixel data */
    pixels = surface->pixels;
    if (SDL_MUSTLOCK (surface))
        SDL_LockSurface (surface);
    pos = 0;
    x = 0;
    while (pos < unpackedlen) {
        value = *data >> 2;
        flags = *data & LCMAP_FLAG_MASK;
        if (flags) {
            if (flags == LCMAP_FLAG_8BIT) {
                loop = 1 + *(data + 1);
                pos++;
                data++;
            } else if (flags == LCMAP_FLAG_16BIT) {
                loop = 1 + (*(data + 1) | *(data + 2) << 8);
                pos += 2;
                data += 2;
            } else if (flags == LCMAP_FLAG_24BIT) {
                loop =
                    0 + (*(data + 1) | *(data + 2) << 8 | *(data + 3) << 16);
                pos += 3;
                data += 3;
            } else {
                printf
                    ("Unrecognized flag in LCMAP. (This shouldn't happen, file corrupted ?)\n");
                loop = 1;
            }
        } else
            loop = 1;
        while (loop) {
            if (x == width) {
                pixels += surface->pitch - surface->w;
                x = 0;
            }
            *pixels = value;
            pixels++;
            if (pixels >=
                (Uint8 *) surface->pixels + surface->pitch * surface->h)
                loop = 1;
            x++;
            loop--;
        }
        data++;
        pos++;
    }
    if (SDL_MUSTLOCK (surface))
        SDL_UnlockSurface (surface);
    free (dataptr);
    return surface;
}

SDL_Surface *lcmap_to_surface (Uint8 * lcmap, int len)
{
    SDL_RWops *rw;
    SDL_Surface *surface;
    rw = SDL_RWFromMem (lcmap, len);
    surface = lcmap_to_surface_rw (rw);
    SDL_FreeRW (rw);
    return surface;
}
