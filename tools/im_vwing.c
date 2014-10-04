/*
 * importlev - Level importer for Luola
 * Copyright (C) 2005 Calle Laakkonen
 *
 * File        : im_vwing.c
 * Description : Importer module for V-Wing levels
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

#include "im_vwing.h"

/* Buffer to hold the loaded level */
static char *leveldata;
static int leveldata_len;
#define CHUNKSIZE 30000 /* Load the level in chunks this big */

/* Name of the level, extracted from the file */
/* max length is 19 characters in datafile */
static char levelname[19+1];

/*
 * Check if file is a V-Wing level
 */
static int check_format(FILE *fp) {
    char buffer[3];
    fseek(fp,0,SEEK_SET);
    fread(buffer,1,3,fp);

    if(buffer[0] == 0x76 && buffer[1] == 0x07 && isalnum(buffer[2]))
        return 1;
    return 0;
}

/*
 * Load a V-Wing level
 */
static int load_level(FILE *fp) {
    int r,read;
    /* Read level name */
    fseek(fp,2,SEEK_SET);
    for(r=0;r<sizeof(levelname)-1;r++) {
        levelname[r]=fgetc(fp);
        if(levelname[r]=='\0') {
            fprintf(stderr,"V-Wing level importer: read 0 at 2+%d bytes while reading name!\n",r);
        }
    }
    levelname[sizeof(levelname)-1]='\0';
    r=sizeof(levelname)-2;
    while(r>0 && levelname[r]==' ') levelname[r--]='\0';

    /* Read pixel data */
    fseek(fp,128,SEEK_SET);
    leveldata_len = 0;
    leveldata = malloc(CHUNKSIZE);
    if(!leveldata) {
        perror("malloc");
        exit(1);
    }
    do {
        read = fread(leveldata+leveldata_len,1,CHUNKSIZE,fp);
        leveldata_len += read;
        if(read == CHUNKSIZE) {
            leveldata = realloc(leveldata,leveldata_len+CHUNKSIZE);
            if(!leveldata) {
                perror("realloc");
                exit(1);
            }
        }
    } while(read==CHUNKSIZE);

    return 0;
}

/*
 * Unload level
 */
static void unload_level(void) {
    free(leveldata);
}

/*
 * Write out the level PCX file
 */
static int write_pcx(const char *filename) {
    FILE *fp;
    static const char header[] = {
        0x0a, 0x05, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x02, 0x1f, 0x03, 0x96, 0x00, 0x96, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x66, 0x00, 0x00, 0x99, 0x00, 0x00, 0xcc, 0xfb,
        0xfb, 0xff, 0x33, 0x00, 0x00, 0x33, 0x00, 0x33, 0x33, 0x00, 0x66, 0x33, 0x00, 0x99, 0x33, 0x00,
        0xcc, 0x33, 0x00, 0xff, 0x66, 0x00, 0x00, 0x66, 0x00, 0x33, 0x66, 0x00, 0x66, 0x66, 0x00, 0x99,
        0x00, 0x01, 0x80, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    fp = fopen(filename,"wb");
    if(!fp) {
        perror(filename);
        return 1;
    }

    if(fwrite(header,1,128,fp) != 128) {
        perror(filename);
        fclose(fp);
        return 1;
    }
    if(fwrite(leveldata,1,leveldata_len,fp) != leveldata_len) {
        perror(filename);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

/*
 * Write out level configuration file
 */
static int write_cfg(const char *collmap,const char *thumbnail,const char *filename) {
    FILE *fp;
    fp = fopen(filename,"w");
    if(!fp) {
        perror(filename);
        return 1;
    }

    fprintf(fp,"# V-Wing level configuration file generated with importlev\n");
    fprintf(fp,"[main]\n");
    fprintf(fp,"collisionmap = %s\n",collmap);
    fprintf(fp,"thumbnail = %s\n",thumbnail);
    fprintf(fp,"name = %s\n",levelname);
    fprintf(fp,"aspect = 1.6\n");
    fprintf(fp,"zoom = 2\n");
    fprintf(fp,"# Support for the V-Wing 1.91 palette format\n");
    fprintf(fp,"[palette]\n");
    fprintf(fp,"2-247 = ground\n"
            "16 = water\n"
            "17 = waterdown\n"
            "18 = waterright\n"
            "19 = waterleft\n"
            "20-30 = free\n"
            "39 = snow\n"
            "40-42 = explosive2\n"
            "43-46 = explosive\n"
            "50 = base\n"
            "52 = snow\n"
            "151-175 = combustable\n"
            "176-199 = combustable2\n"
            "201-202 = underwater\n"
            "203 = ice\n"
            "204-219 = underwater\n"
            "221-243 = indestructable\n"
            "248-255 = indestructable\n");

    fclose(fp);
    return 0;
}

/*
 * Save as a Luola level
 * 
 * lcmap option is not used for this type of level
 */
static struct Files save_level(const char *basename,int lcmap) {
    static char pcxfile[PATH_MAX],thumbnailfile[PATH_MAX],cfgfile[PATH_MAX];
    struct Files result;
    
    if(lcmap) {
        printf("LCMAP not generated for V-Wing level.\n");
    }
    strcpy(pcxfile,basename);
    strcat(pcxfile,".luola.pcx");
    strcpy(cfgfile,basename);
    strcat(cfgfile,".luola.lev");
    strcpy(thumbnailfile,basename);
    strcat(thumbnailfile,".thumb.png");

    result.failed=0;
    result.artwork=NULL;
    result.collmap=pcxfile;
    result.cfgfile=cfgfile;
    result.thumbnail=thumbnailfile;

    if(write_pcx(pcxfile)) {
        result.failed = 1;
    } else {
        if(write_cfg(pcxfile,thumbnailfile,cfgfile)) {
            remove(pcxfile);
            result.failed=1;
        }
    }
    
    return result;
}

/*
 * Register V-Wing importer
 */
struct Importer register_vwing(void) {
    struct Importer i;
    i.name = "V-Wing";
    i.aspect = 1.6;
    i.check_format = check_format;
    i.load_level = load_level;
    i.unload_level = unload_level;
    i.save_level = save_level;

    return i;
}

