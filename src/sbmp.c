/*
 * SBmp - Small bitmap library
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : sbmp.c
 * Description : Convert Small Bitmaps into and from SDL_Surfaces
 * Author(s)   : Calle Laakkonen
 *
 * SBmp is free software; you can redistribute it and/or modify
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

#include "defines.h"

#include "sbmp.h"

static inline Uint32 sbmp_getpixel (SDL_Surface * surface, int x, int y)
{
    Uint32 color = 0, temp;
    SDL_Color scolor;
    switch (surface->format->BytesPerPixel) {
    case 1:
        color = *((Uint8 *) surface->pixels + y * surface->pitch + x);
        scolor = surface->format->palette->colors[color];
        color = (scolor.r << 24) | (scolor.g << 16) | (scolor.b << 8);
        break;
    case 2:
        temp = *((Uint16 *) surface->pixels + y * surface->pitch / 2 + x * 2);
        color = temp & surface->format->Rmask;
        color = temp >> surface->format->Rshift;
        color = temp << surface->format->Rloss;
        scolor.r = color;
        color = temp & surface->format->Gmask;
        color = temp >> surface->format->Gshift;
        color = temp << surface->format->Gloss;
        scolor.g = color;
        color = temp & surface->format->Bmask;
        color = temp >> surface->format->Bshift;
        color = temp << surface->format->Bloss;
        scolor.b = color;
        color = scolor.r << 24 | scolor.g << 16 | scolor.b << 8;
        break;
    case 3:                    /* Format/endian independent */
        scolor.r =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Rshift / 8);
        scolor.g =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Gshift / 8);
        scolor.b =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Bshift / 8);
        color = scolor.r << 24 | scolor.g << 16 | scolor.b << 8;
        break;
    case 4:
        color = *((Uint32*)surface->pixels + y * (surface->pitch/4) + x);
        break;
    }
    return color;
}

Uint8 *surface_to_sbmp (Uint32 * len, SDL_Surface * surface)
{
    Uint32 palette[7], color = 0;
    Uint8 *data, colors = 0;
    Uint8 *sbmp, foundit;
    Uint8 targcol, targentry = 0;
    Uint8 targlen = 0;
    Uint8 width, height;

    Uint8 maxlen;               /* Maximium length the lenght bits can describe */
    Uint8 pshift;               /* How many bits do we need to shift the pixel value */

    int x, y, r, slen, tlen;
    if (surface->w > 255 || surface->h > 255) {
        printf ("Error! Surface is too big!\n");
        return NULL;
    }
    width = surface->w;
    height = surface->h;
    /* Construct palette */
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            if (colors >= 7)
                break;
            color = sbmp_getpixel (surface, x, y);
            if (colors == 0) {
                palette[0] = color;
                colors = 1;
            } else {
                foundit = 0;
                for (r = 0; r < colors; r++)
                    if (palette[r] == color) {
                        foundit = 1;
                        break;
                    }
                if (foundit == 0) {
                    palette[colors] = color;
                    colors++;
                }
            }
        }
        if (colors >= 254) {
            printf
                ("Warning ! Maximium number of colours in sbmp palette exceeded !\n");
            break;
        }
    }
    /* Construct bit mask */
    targcol = 0;
    for (r = 7; r >= 0; r--)
        if (colors & (1 << r)) {
            targcol++;
        }
    pshift = 8 - targcol;
    maxlen = 0;
    for (r = 0; r < pshift; r++)
        maxlen = maxlen | (1 << r);
    /* Pack pixel data */
    slen = 0;
    tlen = 0;
    targcol = 255;
    x = 0;
    y = 0;
    data = malloc (width * height);
    while (slen < width * height) {
        color = sbmp_getpixel (surface, x, y);
        for (r = 0; r < colors; r++)
            if (color == palette[r])
                targentry = r;
        if (targcol == 255) {   /* Start a new colour entry */
            targcol = targentry;
            targlen = 0;
        } else {
            if (targentry != targcol || targlen >= maxlen) {    /* End colour entry and start a new one */
                data[tlen] = ((targcol << pshift) | targlen);
                tlen++;
                targcol = targentry;
                targlen = 0;
            } else
                targlen++;
        }
        slen++;
        x++;
        if (x == width) {
            x = 0;
            y++;
        }
    }
    /* Last colour */
    data[tlen] = ((targcol << pshift) | targlen);
    tlen += 2;
    /* Calculate total length of data */
    *len = 3 + colors * 3 + 2 + tlen;
    /* Build data */
    sbmp = malloc (*len);
    sbmp[0] = width;
    sbmp[1] = height;
    sbmp[2] = colors;
    slen = 3;
    for (r = 0; r < colors; r++) {
        sbmp[slen] = (Uint8) (palette[r] >> 24);
        sbmp[slen + 1] = (Uint8) (palette[r] >> 16);
        sbmp[slen + 2] = (Uint8) (palette[r] >> 8);
        slen += 3;
    }
    memcpy (sbmp + slen, data, tlen);
    free (data);
    return sbmp;
}

SDL_Surface *sbmp_to_surface (Uint8 * sbmp)
{
    SDL_Surface *ssbmp;
    SDL_Color *colors;
    int r, xpos;
    int fpos, flen;
    Uint8 *pixels, pmask, pshift;
    Uint8 width, height;
    int pixels_drawn, total_pixels;
    /* Construct palette */
    width = sbmp[0];
    height = sbmp[1];
    colors = malloc (sizeof (SDL_Color) * sbmp[2]);
    fpos = 3;
    for (r = 0; r < sbmp[2]; r++) {
        colors[r].r = sbmp[fpos];
        colors[r].g = sbmp[fpos + 1];
        colors[r].b = sbmp[fpos + 2];
        fpos += 3;
    }
    /* Create the surface */
    ssbmp =
        SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (ssbmp == NULL) {
        printf ("Error! Could create a surface\n%s\n", SDL_GetError ());
        return NULL;
    }
    SDL_SetColors (ssbmp, colors, 0, sbmp[2]);
    /* Construct bit mask */
    flen = 0;
    for (r = 7; r >= 0; r--)
        if (sbmp[2] & (1 << r)) {
            flen++;
        }
    pmask = 0;
    for (r = flen; r < 8; r++)
        pmask = pmask | (128 >> r);
    pshift = 8 - flen;
    /* Unpack pixel data into the surface */
    pixels = (Uint8 *) ssbmp->pixels;
    flen = sbmp[fpos] & pmask;
    if (SDL_MUSTLOCK (ssbmp))
        SDL_LockSurface (ssbmp);
    xpos = 0;
    pixels_drawn = 0;
    total_pixels = ssbmp->w * ssbmp->h;
    while (pixels_drawn < total_pixels) {
        if (xpos == ssbmp->w) {
            pixels += ssbmp->pitch - ssbmp->w;
            xpos = 0;
        }
        *pixels = sbmp[fpos] >> pshift;
        if (flen == 0) {
            fpos++;
            flen = sbmp[fpos] & pmask;
        } else
            flen--;
        pixels++;
        xpos++;
        pixels_drawn++;
    }
    if (SDL_MUSTLOCK (ssbmp))
        SDL_UnlockSurface (ssbmp);
    return ssbmp;
}
