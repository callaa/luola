/*
 * importlev - Level importer for Luola
 * Copyright (C) 2005 Calle Laakkonen
 *
 * File        : im_tou.c
 * Description : Importer module for Tunnels Of Underworld levels
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

#include "SDL.h"

#include "lcmap.h"
#include "im_tou.h"

/* TOU level header
    0-21:     "TOU level file v1.4\013\010\026"
    22-25:   Offset to level artwork (jpeg)
    26-29:   Offset to parallax background (jpeg)
    30-33:   Offset to collisionmap
    34-161:  Author's name
    162-289: Author's email address
    290:     Has parallax
    291:     Civilians
    292:     Bombing
    293-295: Water color RGB
    296:     Disable running water
    297:     Gravity
    298:     Air resistance
    299:     Collision damage
    300:     Bouncing
    301:     Ambient sounds (0-6)
    302:     Parallax aftertouch (0-4)
    303:     Use CG theme
    304-431: CG theme name
    432:     Shaped CG theme
    433:     CG repair placing
    434:     Stuff density
    435:     Name sign density
    438-441: Random seed (32 bit int)
*/

/* Luola terrain types */
#define TER_FREE        0
#define TER_GROUND      1
#define TER_UNDERWATER  2
#define TER_INDESTRUCT  3
#define TER_WATER       4
#define TER_BASE        5
#define TER_EXPLOSIVE   6
#define TER_EXPLOSIVE2  7
#define TER_WATERFU     8
#define TER_WATERFR     9
#define TER_WATERFD     10
#define TER_WATERFL     11
#define TER_COMBUSTABLE 12
#define TER_COMBUSTABL2 13
#define TER_SNOW        14
#define TER_ICE         15
#define TER_BASEMAT     16
#define TER_TUNNEL      17
#define TER_WALKWAY     18

/* Buffers to hold the loaded level */
static unsigned char *artwork;
static size_t artwork_len;
static unsigned char *collmap;
#define CHUNKSIZE 30000 /* Load the level in chunks this big */

/* Other information extracted from the level */
static char authorname[128];
static char authoremail[128];
static SDL_Color watercolor;

/*
 * Check if file is a TOU level
 */
static int check_format(FILE *fp) {
    char buffer[14];
    fseek(fp,0,SEEK_SET);
    fread(buffer,1,14,fp);

    if(strncmp(buffer,"TOU level file",14)==0)
        return 1;
    return 0;
}

/*
 * Read len bytes into memory.
 * If len is 0, read until EOF.
 */
static char *read_block(size_t len,FILE *fp) {
    char *data;
    size_t read=0,readlen=0,next;
    data = malloc(CHUNKSIZE);
    if(!data) {
        perror("malloc");
        exit(1);
    }
    do {
        if(len>0 && readlen + CHUNKSIZE > len)
            next = len - readlen;
        else
            next = CHUNKSIZE;
        read = fread(data+readlen,1,next,fp);
        readlen += read;
        if(read == CHUNKSIZE) {
            data = realloc(data,readlen+CHUNKSIZE);
            if(!data) {
                perror("realloc");
                exit(1);
            }
        }
    } while(read==CHUNKSIZE && readlen!=len);

    return data;
}

/*
 * Load a TOU level
 */
static int load_level(FILE *fp) {
    int off_artwork,off_parallax,off_collmap;
    int r;
    /* Get offsets to artwork and collisionmap */
    fseek(fp,22,SEEK_SET);
    fread(&off_artwork,4,1,fp);
    fread(&off_parallax,4,1,fp);
    fread(&off_collmap,4,1,fp);

    artwork_len = off_parallax-off_artwork;

    /* Read author's name and email address */
    fread(authorname,1,127,fp);
    fread(authoremail,1,127,fp);

    /* Read water color */
    fseek(fp,293,SEEK_SET);
    fread(&watercolor.r,1,1,fp);
    fread(&watercolor.g,1,1,fp);
    fread(&watercolor.b,1,1,fp);

    /* Read pixel data */
    fseek(fp,off_artwork,SEEK_SET);
    artwork = read_block(artwork_len,fp);

    fseek(fp,off_collmap,SEEK_SET);
    collmap = read_block(0,fp);

    return 0;
}

/*
 * Unload level
 */
static void unload_level(void) {
    free(artwork);
    free(collmap);
}

/*
 * Write out level configuration file
 */
static int write_cfg(const char *basename,const char *artfile,const char *collfile,const char *thumbnailfile,const char *filename) {
    FILE *fp;
    const char *slash,*dot;
    char name[256];

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

    fprintf(fp,"# TOU level configuration file generated with importlev\n");
    fprintf(fp,"# Level by %s",authorname);
    if(authoremail[0])
        fprintf(fp," (%s)",authoremail);
    fprintf(fp,"\n[main]\n");
    fprintf(fp,"artwork = %s\n",artfile);
    fprintf(fp,"collisionmap = %s\n",collfile);
    fprintf(fp,"thumbnail = %s\n", thumbnailfile);
    fprintf(fp,"name = %s\n",name);

    fclose(fp);
    return 0;
}

/*
 * Simply output a stored file to disk
 */
static int write_data(const char *filename,const unsigned char *data,int len) {
    FILE *fp;
    fp=fopen(filename,"wb");
    if(!fp) {
        perror(filename);
        return 1;
    }
    if(fwrite(data,1,len,fp)!=len) {
        perror(filename);
        return 1;
    }
    fclose(fp);

    return 0;
}

/*
 * Get image width and height from JPEG header
 */
static void get_jpeg_dimensions(const unsigned char *data,int *width,int *height) {
    unsigned short type,len;
    data+=2;
    while(1) {
        type = (*(data)<<8) | *(data+1);
        len = (*(data+2)<<8) | *(data+3);
        if(type == 0xffc0) {
            *height= (*(data+5)<<8) | *(data+6);
            *width= (*(data+7)<<8) | *(data+8);
            break;
        } else {
            data += len+2;
        }
    }
}

/*
 * Decode TOU collisionmap
 */
static void decode_collmap(SDL_Surface *surface,const unsigned char *data) {
    Uint8 *pix=surface->pixels;
    Uint8 *end=(Uint8*)surface->pixels + surface->pitch*surface->h;
    /* Some sort of a header? */
    data += (*data) * 20 + 4;

    /* Loop and decode RLE */
    while(pix<end) {
        unsigned char col;
        unsigned short l,loop;

        if(*data==0xff && *(data+1)==0xff) break;

        col = (*data & 0xfc) >> 2;
        loop = (*(data+1) << 2) | (*data & 0x03);
        data+=2;

        for(l=0;l<loop;l++,pix++) {
            if(pix>=end) { printf("Over!\n"); break; }
            /* Translate TOU terrain types to Luola */
            switch(col) {
                case 0: *pix  = TER_FREE;        break; /* Air */
                case 1: *pix  = TER_GROUND;      break; /* Normal ground */
                case 2: *pix  = TER_GROUND;      break; /* Brickwall */
                case 3: *pix  = TER_INDESTRUCT;  break; /* Indestructable */
                case 4: *pix  = TER_BASE;        break; /* Base */
                case 5: *pix  = TER_EXPLOSIVE;   break; /* Explosive */
                case 6: *pix  = TER_EXPLOSIVE2;  break; /* Explosive2 */
                case 7: *pix  = TER_SNOW;        break; /* Snow */
                case 8: *pix  = TER_COMBUSTABLE; break; /* Burning material */
                case 9: *pix  = TER_COMBUSTABL2; break; /* Burning material2 */
                case 10: *pix = TER_GROUND;      break; /* Flesh */
                case 11: *pix = TER_WATER;       break; /* Water up power 2 */
                case 12: *pix = TER_WATER;       break; /* Normal water */
                case 13: *pix = TER_WATER;       break; /* Water down1 */
                case 14: *pix = TER_WATER;       break; /* Water down2 */
                case 15: *pix = TER_WATER;       break; /* Water left1 */
                case 16: *pix = TER_WATER;       break; /* Water left2 */
                case 17: *pix = TER_WATER;       break; /* Water right1 */
                case 18: *pix = TER_WATER;       break; /* Water right2 */
                case 19: *pix = TER_TUNNEL;      break; /* Air nontrasnp. */
                case 20: *pix = TER_FREE;        break; /* Air stream up */
                case 21: *pix = TER_FREE;        break; /* Air stream down */
                case 22: *pix = TER_FREE;        break; /* Air stream left */
                case 23: *pix = TER_FREE;        break; /* Air stream right */
                case 24: *pix = TER_UNDERWATER;  break; /* Underwater ground */
                case 25: *pix = TER_UNDERWATER;  break; /* brickwall UW */
                case 26: *pix = TER_EXPLOSIVE;   break; /* Explosive wall UW */
                case 27: *pix = TER_BASE;        break; /* Base UW */
                case 28: *pix = TER_BASE;        break; /* Base team 1 */
                case 29: *pix = TER_BASE;        break; /* Base team 2 */
                case 30: *pix = TER_BASE;        break; /* Base team 3 */
                case 31: *pix = TER_BASE;        break; /* Base team 4 */
                case 32: *pix = TER_BASE;        break; /* Base team 4 */
                /* Map all unknown terrain types to empty space */
                default: *pix = TER_FREE;
            }
        }
    }
}

/*
 * Write out the collisionmap as BMP or LCMAP
 */
static int write_collmap(const char *filename,int lcmap) {
    int width,height,rval=0;
    SDL_Surface *surface;
    SDL_Color palette[] = {
        {0,0,0,0},
        {0,152,0,0},
        {0,152,100,0},
        {77,77,77,0},
        {0,0,200,0},
        {168,168,168,0},
        {160,0,0,0},
        {227,0,0,0},
        {0,0,255,0},
        {150,150,255,0},
        {50,50,255,0},
        {100,100,255,0},
        {189,120,65,0},
        {133,84,45,0},
        {255,255,255,0},
        {240,240,255,0},
        {158,158,158,0},
        {0,0,0,0},
        {0,132,0,0}
    };

    /* Set water color */
    palette[TER_WATER] = watercolor;

    /* First of all, figure out the width and height of the image */
    get_jpeg_dimensions(artwork,&width,&height);

    /* Create a surface where to decode the collision map */
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,8,0xff,0xff,0xff,0);
    SDL_SetPalette(surface,SDL_LOGPAL|SDL_PHYSPAL,palette,0,18);

    if(surface==NULL) {
        fprintf(stderr,"Couldn't create surface: %s\n",SDL_GetError());
        return 1;
    }

    decode_collmap(surface,collmap);

    if(lcmap) {
        Uint32 len;
        Uint8 *data;

        data = surface_to_lcmap(&len,surface);
        if(!data)
            rval=1;
        else {
            rval = write_data(filename,data,len);
            free(data);
        }
    } else {
        if(SDL_SaveBMP(surface,filename)) {
            fprintf(stderr,"%s: %s\n",filename,SDL_GetError());
            rval=1;
        }
    }
    SDL_FreeSurface(surface);
    return rval;
}

/*
 * Save as a Luola level
 * 
 */
static struct Files save_level(const char *basename,int lcmap) {
    static char artfile[PATH_MAX],collfile[PATH_MAX],cfgfile[PATH_MAX];
    static char thumbnailfile[PATH_MAX];
    struct Files result;
    
    sprintf(artfile,"%s.luola.jpg",basename);
    sprintf(collfile,"%s.luola.%s",basename,lcmap?"lcmap":"bmp");
    sprintf(cfgfile,"%s.luola.lev",basename);
    sprintf(thumbnailfile,"%s.thumb.png",basename);

    result.failed=0;
    result.artwork=artfile;
    result.collmap=collfile;
    result.cfgfile=cfgfile;
    result.thumbnail=thumbnailfile;

    if(write_data(artfile,artwork,artwork_len)) {
        result.failed = 1;
    } else {
        if(write_collmap(collfile,lcmap)) {
            result.failed=1;
        } else {
            if(write_cfg(basename,artfile,collfile,thumbnailfile,cfgfile)) {
                remove(artfile);
                remove(collfile);
                result.failed=1;
            }
        }
    }
    
    return result;
}

/*
 * Register TOU importer
 */
struct Importer register_tou(void) {
    struct Importer i;
    i.name = "TOU";
    i.aspect = 1.0;
    i.check_format = check_format;
    i.load_level = load_level;
    i.unload_level = unload_level;
    i.save_level = save_level;

    return i;
}

