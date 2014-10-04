#include <stdio.h>

#include "thumbnail.h"

int make_thumbnail(const char *input,const char *output,int height,float aspect) {
    /* Try: image magick, SDL_gfx rotozoom, simple pixel zoom */
    fprintf(stderr,"make_thumbnail(\"%s\", \"%s\", %d, %f): Not implemented yet.\n",
            input,output,height,aspect);
    return 1;
}

