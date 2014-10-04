#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <SDL.h>
#include "lconf2bin.h"
#include "sbmp.h"

static int mainblock2bin(LSB_Main *block,SDL_RWops *rw,int *size);
static int overrideblock2bin(LSB_Override *block,SDL_RWops *rw, int *size);
static int objectblock2bin(LSB_Objects *object,SDL_RWops *rw, int *size);
static int paletteblock2bin(LSB_Palette *block,SDL_RWops *rw, int *size);
static int iconblock2bin(SDL_Surface *block,SDL_RWops *rw, int *size);

/* Convert the level settings structure into binary data stream */
SDL_RWops *LevelSetting2bin(LevelSettings *settings,int *size) {
    SDL_RWops *rw;
    FILE *fp;
    Uint8 tmpc;
    int len;
    /* Open the file */
    fp=tmpfile();
    if(!fp) {
        perror("tmpfile");
        exit(1);
    }
    rw=SDL_RWFromFP(fp,0);
    if(!rw) {
        perror(SDL_GetError());
        exit(1);
    }
    /* Write version block */
    len=4;
    SDL_WriteLE16(rw,1);
    tmpc=1; SDL_RWwrite(rw,&tmpc,1,1);
    tmpc=2; SDL_RWwrite(rw,&tmpc,1,1);

    /* Write blocks */
    mainblock2bin(settings->mainblock,rw,&len);
    if(settings->override) overrideblock2bin(settings->override,rw,&len);
    if(settings->objects) objectblock2bin(settings->objects,rw,&len);
    if(settings->palette) paletteblock2bin(settings->palette,rw,&len);
    if(settings->icon) iconblock2bin(settings->icon,rw,&len);

    /* Rewind the file */
    SDL_RWseek(rw,0,SEEK_SET);
    *size=len;
    return rw;
}

/* Convert the main block into binary datastream */
static int mainblock2bin(LSB_Main *block,SDL_RWops *rw, int *size) {
    Uint32 i32;
    Uint16 len=0,i16;
    Uint8 type=0x02,i8;
    char tmps[4];
    int r;
    /* Block header */
    SDL_RWwrite(rw,&len,2,1);	/* We'll fix this later */
    SDL_RWwrite(rw,&type,1,1);
    /* Level name (including terminating \0) */
    if(block->name) {
        type=0x01;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,block->name,1,strlen(block->name)+1);
        len+=strlen(block->name)+1;
    }
    /* Aspect ratio */
    if(block->aspect!=1.0) {
        type=0x02;
        i32 = block->aspect*1000.0;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_WriteLE32(rw,i32); len+=4;
    }
    /* Zooming */
    if(block->zoom!=1.0) {
        type=0x03;
        i32 = block->zoom*1000.0;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_WriteLE32(rw,i32); len+=4;
    }
    /* Set proper length */
    SDL_RWseek(rw,-(len+3),SEEK_CUR);
    SDL_WriteLE16(rw,len);
    SDL_RWseek(rw,len+1,SEEK_CUR);
#if 0
    printf("Main block length: %d (+3 bytes)\n",len);
#endif
    len+=3;
    /* Done */
    *size+=len;
}

/* Convert the override block into binary datastream */
static int overrideblock2bin(LSB_Override *block,SDL_RWops *rw, int *size) {
    Uint16 len=0,i16;
    Uint8 type=0x03,i8,byte;
    int r;
    /* Block header */
    SDL_RWwrite(rw,&len,2,1);	/* We'll fix this later */
    SDL_RWwrite(rw,&type,1,1);
    /* Critters enabled */
    if(block->critters>=0) {
        byte=block->critters;
        type=0x01;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Indestructable bases */
    if(block->indstr_base>=0) {
        byte=block->indstr_base;
        type=0x02;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Stars */
    if(block->stars>=0) {
        byte=block->stars;
        type=0x03;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Snowfall */
    if(block->snowfall>=0) {
        byte=block->snowfall;
        type=0x04;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Turrets */
    if(block->turrets>=0) {
        byte=block->turrets;
        type=0x05;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Jumpgates */
    if(block->jumpgates>=0) {
        byte=block->jumpgates;
        type=0x06;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Cows */
    if(block->cows>=0) {
        byte=block->cows;
        type=0x07;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Fish */
    if(block->fish>=0) {
        byte=block->fish;
        type=0x08;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Birds */
    if(block->birds>=0) {
        byte=block->birds;
        type=0x09;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Bats */
    if(block->bats>=0) {
        byte=block->bats;
        type=0x0a;
        SDL_RWwrite(rw,&type,1,1); len++;
        SDL_RWwrite(rw,&byte,1,1); len++;
    }
    /* Set proper length */
    SDL_RWseek(rw,-(len+3),SEEK_CUR);
    SDL_WriteLE16(rw,len);
    SDL_RWseek(rw,len+1,SEEK_CUR);
#if 0
    printf("Override block length: %d (+3 bytes)\n",len);
#endif
    len+=3;
    /* Done */
    *size+=len;
}

/* Convert the objects block into binary datastream */
static int objectblock2bin(LSB_Objects *object,SDL_RWops *rw, int *size) {
  Uint16 len=0,i16;
  Uint8 type=0x04,i8;
  int r;
  /* Block header */
  SDL_RWwrite(rw,&len,2,1);	/* We'll fix this later */
  SDL_RWwrite(rw,&type,1,1);
  /* Object */
  r=19; /* Length of the block data */
  while(object) {
    SDL_WriteLE16(rw,r); len+=2;
    SDL_RWwrite(rw,&object->type,1,1); len++;
    SDL_WriteLE32(rw,object->x); len+=4;
    SDL_WriteLE32(rw,object->y); len+=4;
    SDL_RWwrite(rw,&object->ceiling_attach,1,1); len++;
    SDL_RWwrite(rw,&object->value,1,1); len++;
    SDL_WriteLE32(rw,object->id); len+=4;
    SDL_WriteLE32(rw,object->link); len+=4;
    object=object->next;
  }
  /* Set proper length */
  SDL_RWseek(rw,-(len+3),SEEK_CUR);
  SDL_WriteLE16(rw,len);
  SDL_RWseek(rw,len+1,SEEK_CUR);
#if 0
  printf("Object block length: %d (+3 bytes)\n",len);
#endif
  len+=3;
  /* Done */
  *size+=len;

}

/* Spit out the palette block */
static int paletteblock2bin(LSB_Palette *block,SDL_RWops *rw, int *size) {
  Uint16 len=256;
  Uint8 type=0x05;
  /* Block header */
  SDL_WriteLE16(rw,len);
  SDL_RWwrite(rw,&type,1,1);
  /* Data */
  SDL_RWwrite(rw,&block->entries,1,256);
  *size+=len+3;
}

/* Pack up the icon into an SBMP */
static int iconblock2bin(SDL_Surface *block,SDL_RWops *rw, int *size) {
    Uint8 hascolorkey=0;
    Uint32 colorkey=0;
    SDL_Color color;
    Uint32 len;
    Uint8 *data;
    Uint8 type=0x06;
    Uint8 notunique=1;
    int r,breakfree=0;
    /* See if the icon has a colorkey (most likely) */
    if((block->flags&SDL_SRCCOLORKEY)) {
        colorkey=block->format->colorkey;
#if 0
        printf("Icon has colorkey, pixevalue: 0x%x\n",colorkey);
#endif
        hascolorkey=1;
    }
    /* A hack to preserve the colorkey. SBMP rewrites the palette */
    /* so we need give the colorkey an unique color */
    if(block->format->BytesPerPixel==1 && hascolorkey) {
        color=block->format->palette->colors[colorkey];
        while(notunique||breakfree>10000) {
            notunique=0;
            for(r=0;r<block->format->palette->ncolors;r++) {
                if(r==colorkey) continue;
                if(color.r == block->format->palette->colors[r].r &&
                   color.g == block->format->palette->colors[r].g &&
                   color.b == block->format->palette->colors[r].b) {
                    notunique=1;
                    break;
                }
              }
              if(notunique) {
                  color.r=rand()%255;
                  color.g=rand()%255;
                  color.b=rand()%255;
              }
              breakfree++;
        }
        if(breakfree>10000) {
            printf("Couldn't find a unique colorkey!\n");
        } else {
#if 0
            printf("Unique colorkey colour is RGB(%d,%d,%d)\n",color.r,color.g,color.b);
#endif
            block->format->palette->colors[colorkey]=color;
        }
    }
    /* Convert the SDL_Surface into SBMP */
    data=surface_to_sbmp(&len,block);
    if(!data) {
        printf("Cannot pack icon!\n");
        return 0;
    }
    /* Another hack. Find the colorkey palette entry (it might have moved somewhere else) */
    if(block->format->BytesPerPixel==1 && hascolorkey) {
        Uint8 ccount;
        Uint8 *ptr;
        ptr=data+2;
        ccount=*ptr; ptr++;
#if 0
        printf("SBMP has %d colours\n",ccount);
#endif
        for(r=0;r<ccount;r++) {
            if(ptr[0]==color.r && ptr[1]==color.g && ptr[2]==color.b) {
#if 0
                printf("Colorkey found at SBMP palette entry %d !\n",r);
#endif
                colorkey=r;
            }
            ptr+=3;
        }
    }
    /* Write data to file */
    len+=9;
    SDL_WriteLE16(rw,len);
    SDL_RWwrite(rw,&type,1,1);
    SDL_RWwrite(rw,&hascolorkey,1,1);
    SDL_WriteLE32(rw,colorkey);
    SDL_RWwrite(rw,data,1,len);
#if 0
    printf("Icon block length: %d (+3 bytes)\n",len);
#endif
    *size+=len+3;
}

