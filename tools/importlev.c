/*
 * importlev - Level importer for Luola
 * Copyright (C) 2005 Calle Laakkonen
 *
 * File        : importlev.c
 * Description : Level importer tool
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

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "SDL.h"

#include "ldat.h"
#include "lconf2bin.h"

#include "im_vwing.h"
#include "im_wings.h"
#include "im_tou.h"

static struct Importer importer[3];
static const int importers=3;

/*
 * Print help message and exit
 */
static void print_help(void) {
	int r;
	puts("importlev - A tool to import foreign levels to Luola\n"
		"Usage:\n"
		"importlev [options] <levelfile> [levelfile]...\n"
		"Options:\n"
		"\t--compact\tmake a compact level file\n"
		"\t--lcmap\t\tconvert collisionmap to LCMAP format\n"
		);
	fputs("Supported level formats: ",stdout);
	for(r=0;r<importers;r++) {
		fputs(importer[r].name,stdout);
		if(r+1<importers) fputs(", ",stdout);
	}
	putchar('\n');
	exit(0);
}

/* Initialize SDL, if not already */
static void init_sdl(void) {
    if(SDL_WasInit(SDL_INIT_VIDEO)==0) {
        if(SDL_Init(SDL_INIT_VIDEO)) {
            fprintf(stderr,"Couldn't initialize SDL: %s\n",SDL_GetError());
            exit(1);
        }
        atexit(SDL_Quit);
    }
}

/*
 * Strip extension (everything after the last .) from a filename
 */
static char *strip_extension(const char *filename) {
    static char path[PATH_MAX];
    char *dot = strrchr(filename,'.');
    if(dot) {
        path[0]='\0';
        strncat(path,filename,dot-filename);
    } else {
        strcpy(path,filename);
    }
    return path;
}

/*
 * Pack a file into an LDAT archive
 */
static int pack_file(const char *filename, char *ID,int index,LDAT *ldat) {
    struct stat finfo;
    SDL_RWops *rw;
    if(stat(filename,&finfo)) {
        perror(filename);
        return 1;
    }
    rw=SDL_RWFromFile(filename,"rb");
    ldat_put_item(ldat,ID,index,rw,finfo.st_size);
    return 0;
}

/* 
 * Pack level settings to LDAT
 */
static void pack_settings(LevelSettings *settings,LDAT *ldat) {
    SDL_RWops *rw;
    int size;
    rw=LevelSetting2bin(settings,&size);
    ldat_put_item(ldat,"SETTINGS",0,rw,size);
}

/*
 * Pack files to a compact level file
 */
static int make_compact(const struct Files *files,const char *filename) {
    LevelSettings *settings;
    LDAT *lev;

    /* Load settings */
    settings=load_level_config(files->cfgfile);
    if(settings==NULL) {
        printf("Couldn't load level configuration file!\n");
        return 1;
    }

    /* Create archive */
    lev = ldat_create();

    /* Pack level files */
    pack_settings(settings,lev);
    if(files->artwork)
        pack_file(files->artwork,"ARTWORK",0,lev);
    pack_file(files->collmap,"COLLISION",0,lev);
    pack_file(files->cfgfile,"SOURCE",0,lev);

    /* Save level */
    ldat_save_file(lev,filename);
    ldat_free(lev);
    return 0;
}

/*
 * Import a level
 */
static int import_file(const char *filename,int makecompact,int makelcmap) {
    struct Files saved;
    char *basename;
    int format=-1, r;
    FILE *fp;

    /* Open file and recognize format */
    fp = fopen(filename,"rb");
    if(fp==NULL) {
        perror(filename);
        return 1;
    }
    printf("%s: ",filename);
    for(r=0;r<importers;r++) {
        if(importer[r].check_format(fp)) {
            format=r;
            break;
        }
    }
    if(format==-1) {
        printf("Unknown format\n");
        return 1;
    } else {
        printf("%s format\n",importer[format].name);
    }

    /* Initialize SDL if required */
    if(importer[format].need_sdl)
        init_sdl();

    /* Load level to memory */
    if(importer[format].load_level(fp)) {
        fclose(fp);
        return 1;
    }
    fclose(fp);

    /* Save level in Luola format */
    basename = strip_extension(filename);
    if(makecompact) {
        strcat(basename,".tmp");
    }
    saved = importer[format].save_level(basename,makelcmap);
    if(saved.failed) return 1;

    if(!makecompact) {
        printf("Saved files:\n");
        if(saved.artwork) printf("\t%s\n",saved.artwork);
        printf("\t%s\n",saved.collmap);
        printf("\t%s\n",saved.cfgfile);
    }

    /* Free memory */
    importer[format].unload_level();

    /* Make compact level file */
    if(makecompact) {
        char *compactfile = strip_extension(filename);
        strcat(compactfile,".compact.lev");
        init_sdl();
        make_compact(&saved,compactfile);

        if(saved.artwork) remove(saved.artwork);
        remove(saved.collmap);
        remove(saved.cfgfile);
        printf("Saved file:\n\t%s\n",compactfile);
    }

    return 0;
}

/*
 * Main
 */
int main(int argc,char *argv[]) {
	int makecompact=0;
	int makelcmap=0;

	int r;

	/* Register importer modules */
	importer[0] = register_vwing();
	importer[1] = register_wings();
	importer[2] = register_tou();

	/* Parse command line arguments */
	if(argc==1) print_help();

    /* Loop thru command line arguments and import levels */
	for(r=1;r<argc;r++) {
		if(strcmp(argv[r],"--help")==0) print_help();
		else if(strcmp(argv[r],"--compact")==0) makecompact=1;
		else if(strcmp(argv[r],"--lcmap")==0) makelcmap=1;
		else {
			import_file(argv[r],makecompact,makelcmap);
		}
	}

	return 0;
}

