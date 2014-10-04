/*
 * importlev - Level importer for Luola
 * Copyright (C) 2005 Calle Laakkonen
 *
 * File        : im_wings.c
 * Description : Importer module for Wings levels. Based on unmakelev
 *               by Pauli Virtanen.
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

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include <SDL.h>
#include <SDL_rwops.h>
#include <SDL_endian.h>

#include "im_wings.h"

static Uint32 lev_width, lev_height;
static SDL_Color palette[256];
static Uint8* lev_pixels;

/*
 * Check if file is a Wings level.
 * Wings levels have no magic numbers, so this is a bit tricky.
 */
static int check_format(FILE *fp) {
    char buffer[3];
    Uint16 w,h;

    /* Wings levels start out with a palette.  */
    /* The first color should always be black. */
    fseek(fp,0,SEEK_SET);
    fread(buffer,1,3,fp);
    if(buffer[0] != 0 || buffer[1] != 0 || buffer[2]!=0)
        return 0;

    /* Check level width and height */
    /* Usual level size is around 320x400, but we just check */
    /* that both dimensions are below 1000. */
    fseek(fp,0x300,SEEK_SET);
    fread(&w,2,1,fp);
    fread(&h,2,1,fp);
    
    if(SDL_SwapLE16(w)<1000 && SDL_SwapLE16(h)<1000) return 1;

    /* If none of these conditions match, this is probably not a Wings level */
    return 0;
}

/*
 * Read RLE compressed data from file and unpack it
 */
static int decode_pcx(FILE *fp,Uint8 *pixels,Uint32 len) {
    Uint32 pos=0;
    Uint8 pix;

    while(pos<len) {
        if(fread(&pix,1,1,fp)!=1) return 1;
        pos++;

        if(pix>192) {
            int count=pix-192;
            if(fread(&pix,1,1,fp)!=1) return 1;
            pos++;
            for(;count>0;count--) {
                *(pixels++) = pix;
            }
        } else {
            *(pixels++) = pix;
        }
    }
    return 0;
}

/*
 * Load level to memory
 */
static int load_level(FILE *fp) {
    Uint32 datalen;
    int i;

    /* Read level palette */
    fseek(fp,0,SEEK_SET);

    for(i=0;i<256;i++) {
        Uint8 r,g,b;
        fread(&r,1,1,fp);
        fread(&g,1,1,fp);
        fread(&b,1,1,fp);

        palette[i].r = r * 4;
        palette[i].g = g * 4;
        palette[i].b = b * 4;
    }

    /* Read level width and height */
    fseek(fp,0x300,SEEK_SET);
    fread(&lev_width,2,1,fp);
    fread(&lev_height,2,1,fp);
    lev_width = SDL_SwapLE16(lev_width);
    lev_height = SDL_SwapLE16(lev_height);

    /* Read level data length */
    fread(&datalen,4,1,fp);
    datalen = SDL_SwapLE32(datalen);

    /* Read pixel data (in PCX RLE format) */
    lev_pixels = malloc(lev_width * lev_height);
    if(!lev_pixels) {
        perror("malloc");
        return 1;
    }
    if(decode_pcx(fp,lev_pixels,datalen)) {
        printf("Error occured while decoding PCX data!\n");
        return 1;
    }

    return 0;
}

/*
 * Unload level
 */
static void unload_level(void) {
    free(lev_pixels);
}

/*
 * Save out the level as a BMP file
 */
static int write_bmp(const char *filename) {
    SDL_Surface* surface;
	unsigned int i;
	
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, lev_width, lev_height, 8,
	                               0xFF, 0xFF, 0xFF, 0);

    if(surface==NULL) {
        printf("Couldn't create surface: %s\n",SDL_GetError());
        return 1;
    }
    SDL_SetColors(surface, palette, 0, 256);
    memcpy(surface->pixels,lev_pixels,lev_width*lev_height);

	if(SDL_SaveBMP(surface, filename)) {
        printf("%s: %s\n",filename,SDL_GetError());
        return 1;
    }
    SDL_FreeSurface(surface);

    return 0;
}

/*
 * Write out level configuration file
 */
static int write_cfg(const char *basename,const char *collmap,
        const char *thumbnail,const char *filename)
{
    const char *slash,*dot;
    char name[256];
    FILE *fp;
    /* First figure out the name of this level */
    slash = strrchr(basename,'/');
    if(slash) slash++; else slash=basename;
    dot = strrchr(slash,'.');
    if(!dot) dot=slash+strlen(slash);
    name[0]=0;
    strncat(name,slash,dot-slash);

    /* Open output file and write */
    fp = fopen(filename,"w");
    if(!fp) {
        perror(filename);
        return 1;
    }

    fprintf(fp, "# Wings level configuration file generated with importlev\n");
    fprintf(fp,"[main]\n");
    fprintf(fp,"collisionmap = %s\n",collmap);
    fprintf(fp,"thumbnail = %s\n",thumbnail);
    fprintf(fp,"name = %s\n", name);
    fprintf(fp,"zoom = 3\n\n");
    fprintf(fp,"# Support for the Wings 1.40 palette according to colors.txt\n");
    fprintf(fp,"[palette]\n");
    fprintf(fp,"1-31 = ground\n"
               "32-47 = base\n"
               "48 = water\n"
               "49 = waterdown\n"
               "50 = waterleft\n"
               "51 = waterright\n"
               "52 = water\n"
               "53 = snow\n"
               "54 = ground\n"
               "55-56 = explosive\n"
               "57-63 = ground\n"
               "80-95 = indestructable\n"
               "96-111 = ground\n"
               "112-127 = combustable\n"
               "128-255 = ground\n");
    fclose(fp);
    return 0;
}

/*
 * Save as a Luola level
 * 
 * lcmap option is not used for this type of level
 */
static struct Files save_level(const char *basename,int lcmap) {
    static char bmpfile[PATH_MAX],cfgfile[PATH_MAX],thumbnailfile[PATH_MAX];
    struct Files result;

    if(lcmap) {
        printf("LCMAP not generated for Wings level.\n");
    }
    strcpy(bmpfile,basename);
    strcat(bmpfile,".luola.bmp");
    strcpy(cfgfile,basename);
    strcat(cfgfile,".luola.lev");
    strcpy(thumbnailfile,basename);
    strcat(thumbnailfile,".thumb.png");

    result.failed=0;
    result.artwork=NULL;
    result.collmap=bmpfile;
    result.cfgfile=cfgfile;

    if(write_bmp(bmpfile)) {
        result.failed = 1;
    } else if(write_cfg(basename,bmpfile,thumbnailfile,cfgfile)) {
        remove(bmpfile);
        result.failed=1;
    }

    return result;
}

/*
 * Register Wings importer
 */
struct Importer register_wings(void) {
    struct Importer i;
    i.name = "Wings";
    i.aspect = 1.0;
    i.check_format = check_format;
    i.load_level = load_level;
    i.unload_level = unload_level;
    i.save_level = save_level;

    return i;
}


