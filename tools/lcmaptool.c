#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "lcmap.h"

int main(int argc,char *argv[]) {
  SDL_Surface *surface;
  FILE *fp;
  char packmode=0;
  Uint8 *mapdata;
  Uint32 maplen;
  int bytes;

  if(argc<4) {
    printf("Usage:\nlcmap <pack/unpack> <file1> <file2>\n");
    return 0;
  }
  if(strcmp(argv[1],"pack")==0) packmode=1;
  else if(strcmp(argv[1],"unpack")) {
    printf("Unrecognized mode: \"%s\"\n",argv[1]);
    return 1;
  }
  if(SDL_Init(SDL_INIT_VIDEO)) {
    printf("Error: %s\n",SDL_GetError());
    return 1;
  }
  if(packmode) {
    surface=IMG_Load(argv[2]);
    if(surface==NULL) {
      printf("IMG_Load error: %s\n",SDL_GetError());
      return 1;
    }
    mapdata=surface_to_lcmap(&maplen,surface);
    SDL_FreeSurface(surface);
    if(mapdata==NULL) {
      printf("Error occured while converting to LCMAP\n");
      return 1;
    }
    fp=fopen(argv[3],"w");
    if(fp) {
      perror(argv[3]);
      return 1;
    }
    printf("Map is %d bytes long.\n",maplen);
    bytes=fwrite(mapdata,maplen,1,fp);
    if(bytes<maplen) {
      perror("fwrite");
      return 1;
    }
    printf("Wrote %d bytes\n",bytes);
    fclose(fp);
  } else {
    struct stat finfo;
    SDL_SetVideoMode(320,240,24,0);
    if(stat(argv[2],&finfo)) {
      perror(argv[2]);
      return 1;
    }
    mapdata=malloc(finfo.st_size);
    fp=fopen(argv[2],"r");
    if(fp==NULL) {
      perror(argv[2]);
      return 1;
    }
    bytes=fread(mapdata,1,finfo.st_size,fp);
    if(bytes<finfo.st_size) {
      perror("fread");
      return 1;
    }
    printf("File size is %d bytes, read %d\n",finfo.st_size,bytes);
    fclose(fp);
    surface=lcmap_to_surface(mapdata,bytes);
    free(mapdata);
    if(surface==NULL) return 1;
    SDL_SaveBMP(surface,argv[3]);
    SDL_FreeSurface(surface);
  }
  return 0;
}
