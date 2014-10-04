/*  SFont: a simple font-library that uses special .pngs as fonts
    Copyright (C) 2003 Karl Bartel

    License: GPL or LGPL (at your choice)
    WWW: http://www.linux-games.com/sfont/

    This program is free software; you can redistribute it and/or modify        
    it under the terms of the GNU General Public License as published by        
    the Free Software Foundation; either version 2 of the License, or           
    (at your option) any later version.                                         
                                                                                
    This program is distributed in the hope that it will be useful,       
    but WITHOUT ANY WARRANTY; without even the implied warranty of              
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
    GNU General Public License for more details.                
                                                                               
    You should have received a copy of the GNU General Public License           
    along with this program; if not, write to the Free Software                 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   
                                                                                
    Karl Bartel
    Cecilienstr. 14                                                    
    12307 Berlin
    GERMANY
    karlb@gmx.net                                                      
*/                                                                            
#include <assert.h>
#include <stdlib.h>
#include "SFont.h"

#include "console.h"

SFont_Font* SFont_InitFont(SDL_Surface* Surface)
{
    int x = 0, i = 0;
    Uint32 pixel;
    SFont_Font* Font;
    Uint32 pink;

    if (Surface == NULL)
	return NULL;

    Font = malloc(sizeof(SFont_Font));
    Font->Surface = Surface;

    SDL_LockSurface(Surface);

    pink = SDL_MapRGB(Surface->format, 255, 0, 255);
    while (x < Surface->w) {
	if (getpixel(Surface, x, 0) == pink) { 
    	    Font->CharPos[i++]=x;
    	    while((x < Surface->w) && (getpixel(Surface, x, 0)== pink))
		x++;
	    Font->CharPos[i++]=x;
	}
	x++;
    }
    Font->MaxPos = x-1;
    
    pixel = getpixel(Surface, 0, Surface->h-1);
    SDL_UnlockSurface(Surface);
    SDL_SetColorKey(Surface, SDL_SRCCOLORKEY, pixel);

    return Font;
}

void SFont_FreeFont(SFont_Font* FontInfo)
{
    SDL_FreeSurface(FontInfo->Surface);
    free(FontInfo);
}

/* Luola extension: Do a raw pixelcopy on areas where alpha is nonzero */
static void nonzero_pixelcopy(Uint32 * srcpix, Uint32 * pixels, int w, int h,
        int srcpitch, int pitch,Uint32 alphamask)
{
    int x,y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++,pixels++,srcpix++) {
            if ( (*srcpix & alphamask) )
                *pixels = *srcpix;
        }
        pixels += pitch - w;
        srcpix += srcpitch - w;
    }
}

void SFont_Write(SDL_Surface *Surface, const SFont_Font *Font,
		 int x, int y, const char *text,int rawcopy)
{
    const char* c;
    int charoffset;
    SDL_Rect srcrect, dstrect;

    if(text == NULL)
	return;

    // these values won't change in the loop
    srcrect.y = 1;
    dstrect.y = y;
    srcrect.h = Font->Surface->h - 1;

    for(c = text; *c != '\0' && x <= Surface->w ; c++) {
	charoffset = ((int) (*c - 33)) * 2 + 1;
	// skip spaces and nonprintable characters
	if (*c == ' ' || charoffset < 0 || charoffset > Font->MaxPos) {
	    x += Font->CharPos[2]-Font->CharPos[1];
	    continue;
	}

	srcrect.w =
	    (Font->CharPos[charoffset+2] + Font->CharPos[charoffset+1])/2 -
	    (Font->CharPos[charoffset] + Font->CharPos[charoffset-1])/2;
	srcrect.x = (Font->CharPos[charoffset]+Font->CharPos[charoffset-1])/2;
	dstrect.x = x - (float)(Font->CharPos[charoffset]
			      - Font->CharPos[charoffset-1])/2;

    if(rawcopy) {
        /* Do a raw pixel copy on areas where alpha!=0 */
        if(dstrect.x<0) {
            srcrect.x-=dstrect.x;
            dstrect.x=0;
        }
        nonzero_pixelcopy((Uint32*)Font->Surface->pixels+Font->Surface->w+
                srcrect.x,
                (Uint32*)Surface->pixels+dstrect.y*Surface->w+dstrect.x,
                srcrect.w,srcrect.h,
                Font->Surface->w,Surface->w,Font->Surface->format->Amask);
    } else {
        SDL_BlitSurface(Font->Surface, &srcrect, Surface, &dstrect); 
    }

	x += Font->CharPos[charoffset+1] - Font->CharPos[charoffset];
    }
}

int SFont_TextWidth(const SFont_Font *Font, const char *text)
{
    const char* c;
    int charoffset=0;
    int width = 0;

    if(text == NULL)
	return 0;

    for(c = text; *c != '\0'; c++) {
	charoffset = ((int) *c - 33) * 2 + 1;
	// skip spaces and nonprintable characters
        if (*c == ' ' || charoffset < 0 || charoffset > Font->MaxPos) {
            width += Font->CharPos[2]-Font->CharPos[1];
	    continue;
	}
	
	width += Font->CharPos[charoffset+1] - Font->CharPos[charoffset];
    }

    return width;
}

int SFont_TextHeight(const SFont_Font* Font)
{
    return Font->Surface->h - 1;
}

void SFont_WriteCenter(SDL_Surface *Surface, const SFont_Font *Font,
		       int y, const char *text)
{
    SFont_Write(Surface, Font, Surface->w/2 - SFont_TextWidth(Font, text)/2,
	    	y, text,0);
}

