/*
 * lcmaptool - Convert images to/from Luola Collisionmap format
 * Copyright (C) 2002-2005 Calle Laakkonen
 *
 * File        : lcmaptool.c
 * Description : LCMAP Converter
 * Author(s)   : Calle Laakkonen
 *
 * Luola is free software; you can redistribute it and/or modify
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "lcmap.h"

int main(int argc, char *argv[])
{
    SDL_Surface *surface;
    FILE *fp;
    enum {PACK,UNPACK} packmode=UNPACK;
    Uint8 *mapdata;
    Uint32 maplen;
    int bytes;

    if (argc < 4) {
        printf("Usage:\nlcmap <pack/unpack> <file1> <file2>\n");
        return 0;
    }
    if (strcmp(argv[1], "pack") == 0)
        packmode = PACK;
    else if (strcmp(argv[1], "unpack")) {
        fprintf(stderr,"Unrecognized mode: \"%s\"\n", argv[1]);
        return 1;
    }
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (packmode==PACK) {
        surface = IMG_Load(argv[2]);
        if (surface == NULL) {
            fprintf(stderr,"%s: %s\n", argv[2], SDL_GetError());
            return 1;
        }
        mapdata = surface_to_lcmap(&maplen, surface);
        SDL_FreeSurface(surface);
        if (mapdata == NULL) {
            fprintf(stderr,"Error occured while converting to LCMAP\n");
            return 1;
        }
        fp = fopen(argv[3], "wb");
        if (fp == NULL) {
            perror(argv[3]);
            return 1;
        }
        bytes = fwrite(mapdata, 1, maplen, fp);
        fclose(fp);
        if (bytes < maplen) {
            fprintf(stderr,"wrote %d bytes out of %d\n",bytes,maplen);
            perror("fwrite");
            return 1;
        }
    } else {
        struct stat finfo;
        if (stat(argv[2], &finfo)) {
            perror(argv[2]);
            return 1;
        }
        mapdata = malloc(finfo.st_size);
        fp = fopen(argv[2], "r");
        if (fp == NULL) {
            perror(argv[2]);
            return 1;
        }
        bytes = fread(mapdata, 1, finfo.st_size, fp);
        fclose(fp);
        if (bytes < finfo.st_size) {
            fprintf(stderr,"read %d bytes out of expected %d\n",bytes,
                    finfo.st_size);
            perror("fread");
        }
        surface = lcmap_to_surface(mapdata, bytes);
        free(mapdata);
        if (surface == NULL)
            return 1;
        SDL_SaveBMP(surface, argv[3]);
        SDL_FreeSurface(surface);
    }
    return 0;
}
