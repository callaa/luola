#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "ldat.h"
#include "lconf2bin.h"

/* Internally used functions */
static void print_help(void);
static void pack_file(char *filename,char *ID,int index, LDAT *ldat);
static void pack_settings(LevelSettings *settings,LDAT *ldat);

int main(int argc,char *argv[]) {
    char *conf=NULL,*out=NULL,*ptr;
    LevelSettings *settings;
    LevelBgMusic *music;
    LDAT *clev;
    int r,c,incsrc=0;
    /* Parse command line arguments */
    if(argc<2) print_help();
    for(r=1;r<argc;r++) {
        if(strcmp(argv[r],"-o")==0) {
            r++;
            if(r>=argc) {
                printf("You did not specify the output file\n");
                return 0;
            }
            out=argv[r];
        } else if(strcmp(argv[r],"--help")==0) print_help();
        else if(strcmp(argv[r],"--include-source")==0) incsrc=1;
        else conf=argv[r]; /* Unrecognized argument, probably the configuration file name */
    }
    if(conf==NULL) {
        printf("Settings file required !\n");
        return 1;
    }
    /* Default name */
    if(out==NULL) {
        ptr=strrchr(conf,'.');
        if(ptr) {
            out=malloc(ptr-conf+8+5);
            out[0]='\0';
            strncat(out,conf,ptr-conf);
        } else {
            out=malloc(strlen(conf)+8+5);
            strcpy(out,conf);
        }
        strcat(out,".compact.lev");
    }
    printf("Level filename: %s\n",out);
    /* Load settings */
    settings=load_level_config(conf);
    if(settings==NULL) {
        printf("Error occured while loading settings file !\n");
        return 1;
    }
    printf("Configuration file loaded.\n");
    /* Check that all required options are set */
    if(settings->mainblock->collmap==NULL) {
        printf("No collisionmap defined!\n");
        return 1;
    }

    /* Initialize video if required */
    if(settings->icon) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    /* Create LDAT file */
    clev=ldat_create();
    /* Pack settings file */
    pack_settings(settings,clev);
    /* Pack artwork */
    if(settings->mainblock->artwork &&
          strcmp(settings->mainblock->artwork,settings->mainblock->collmap))
        pack_file(settings->mainblock->artwork,"ARTWORK",0,clev);

    /* Pack collisionmap */
    pack_file(settings->mainblock->collmap,"COLLISION",0,clev);

    /* Pack music */
    music=settings->mainblock->music;
    r=0;
    while(music) {
        pack_file(music->file,"MUSIC",r,clev);
    r++;
    }

    /* Pack the original level configuration file */
    if(incsrc) {
    pack_file(conf,"SOURCE",0,clev);
    }

    /* Save level */
    ldat_save_file(clev,out);
    ldat_free(clev);
    return 0;
}

static void pack_file(char *filename,char *ID,int index,LDAT *ldat) {
    struct stat finfo;
    SDL_RWops *rw;
    if(stat(filename,&finfo)) {
        perror(filename);
        exit(1);
    }
    printf("Putting item \"%s\" [%d]: %s\n",ID,index,filename);
    rw=SDL_RWFromFile(filename,"rb");
    ldat_put_item(ldat,ID,index,rw,finfo.st_size);
}

static void pack_settings(LevelSettings *settings,LDAT *ldat) {
    SDL_RWops *rw;
    int size;
    rw=LevelSetting2bin(settings,&size);
    printf("Packing binary settings block, length %d bytes\n",size);
    ldat_put_item(ldat,"SETTINGS",0,rw,size);
}

static void print_help(void) {
    printf("mkecompact\n");
    printf("This is an utility to pack luola level files into a\n");
    printf("specially formatted LDAT file.\n");
    printf("Usage:\n");
    printf("mkecompact <settingsfile> [options]\nOptions:\n");
    printf("\t-o <output.lev>\tOutput filename\n");
    printf("\t--include-source\tInclude the source .conf.lev file\n");
    exit(0);
}

