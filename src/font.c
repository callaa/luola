/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : font.c
 * Description : Font library abstraction
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
#include <string.h>

#include "console.h"
#include "fs.h"
#include "font.h"
#include "startup.h"
#include "parser.h"

#include "SFont.h"

#ifdef HAVE_LIBSDL_TTF
#include "SDL_ttf.h"
#else
typedef void TTF_Font;
#endif

/* Internally used type definitions */
typedef union {
    SFont_Font *sfont;
    TTF_Font *tfont;
} FontInfo;

/* Internally used globals */
static SFont_Font *sfont_big, *sfont_small;
static char use_sfont;
#ifdef HAVE_LIBSDL_TTF
static TTF_Font *tfont_big, *tfont_small;
static struct {
    char *bigfont;
    char *smallfont;
    int bigsize;
    int smallsize;
} TruetypeFonts;

#endif

/* Exported globals */
SDL_Color font_color_white, font_color_gray;
SDL_Color font_color_red;
SDL_Color font_color_green;
SDL_Color font_color_blue;
SDL_Color font_color_cyan;
int MENU_SPACING;

/* Pointers to functions to use */
int (*ttf_sizetext) (TTF_Font * font, const char *text, int *w, int *h);
SDL_Surface *(*ttf_rendertext_blended) (TTF_Font * font, const char *text,
                                        SDL_Color fg);

static inline void set_proper_font (FontInfo * fi, FontSize size)
{
    if (use_sfont) {
        if (size == Smallfont)
            fi->sfont = sfont_small;
        else
            fi->sfont = sfont_big;
    }
#ifdef HAVE_LIBSDL_TTF
    else {
        if (size == Smallfont)
            fi->tfont = tfont_small;
        else
            fi->tfont = tfont_big;
    }
#endif
}

#ifdef HAVE_LIBSDL_TTF
/* Load the font configuration file */
static int load_font_cfg (const char *filename) {
    struct dllist *fontcfg;
    struct ConfigBlock *block;
    struct Translate tr[] = {
        {"bigfont", CFG_STRING, &TruetypeFonts.bigfont},
        {"smallfont", CFG_STRING, &TruetypeFonts.smallfont},
        {"smallsize", CFG_INT, &TruetypeFonts.smallsize},
        {"bigsize", CFG_INT, &TruetypeFonts.bigsize},
        {0,0,0}
    };

    fontcfg = read_config_file(filename,1);
    if(!fontcfg) return 1;

    block=fontcfg->data;

    translate_config(block->values,tr,0);

    dllist_free(fontcfg,free_config_file);
    
    if(TruetypeFonts.bigfont==NULL) {
        printf("Error: bigfont not specified\n");
        return 1;
    }
    if(TruetypeFonts.smallfont==NULL) {
        printf("Error: smallfont not specified\n");
        return 1;
    }
    return 0;
}
#endif

/* Initialize the font library			*/
/* If SDL_ttf is compiled in, it is used unless */
/* the fonts are unavailable. In that case,	*/
/* Luola falls back to the built-in SFont	*/
int init_font (void) {
    use_sfont = 1;
#ifdef HAVE_LIBSDL_TTF
    /* Initialize SDL_ttf */
    if (luola_options.sfont || TTF_Init () < 0) {
        if (luola_options.sfont == 0) {
            printf ("Couldn't initialize SDL_ttf: %s\n", SDL_GetError ());
            printf ("Reverting to built-in SFont.\n");
        }
    } else {
        /* Load the font configuration file. First check the home directory,
           if there isn't one there, check the font directory. */
        const char *fcfg;
        const char *filename;
        fcfg = getfullpath (HOME_DIRECTORY, "fonts.cfg");
        if (load_font_cfg (fcfg)) {
            fcfg = getfullpath (FONT_DIRECTORY, "fonts.cfg");
            if (load_font_cfg (fcfg)) {
                fprintf(stderr,
                    "Error: Couldn't load font configuration file\n");
                return 1;
            }
        }
        if (strchr (TruetypeFonts.bigfont, '/'))
            filename = TruetypeFonts.bigfont;
        else
            filename = getfullpath (FONT_DIRECTORY, TruetypeFonts.bigfont);
        tfont_big = TTF_OpenFont (filename, TruetypeFonts.bigsize);
        if (tfont_big == NULL) {
            printf ("Could not open font \"%s\" with point size %d\n",
                    filename, TruetypeFonts.bigsize);
            printf ("SDL reports: %s\n", SDL_GetError ());
            printf ("Reverting to built-in SFont.\n");
        } else {
            if (strchr (TruetypeFonts.smallfont, '/'))
                filename = TruetypeFonts.smallfont;
            else
                filename =
                    getfullpath (FONT_DIRECTORY, TruetypeFonts.smallfont);
            tfont_small = TTF_OpenFont (filename, TruetypeFonts.smallsize);
            if (tfont_small == NULL) {
                printf ("Could not open font \"%s\" with point size %d\n",
                        filename,TruetypeFonts.smallsize);
                printf ("SDL reports: %s\n", SDL_GetError ());
                printf ("Reverting to built-in SFont.\n");
            } else
                use_sfont = 0;
            /* Set functions pointers */
            ttf_sizetext = TTF_SizeText;
            ttf_rendertext_blended = TTF_RenderText_Blended;
            MENU_SPACING = TTF_FontHeight (tfont_big);
        }
    }
#endif
    /* Initialize SFont */
    if (use_sfont) {
        sfont_big=SFont_InitFont(load_image(
                    getfullpath(FONT_DIRECTORY,"font1.png"), 0,T_ALPHA));
        sfont_small=SFont_InitFont(load_image(
                    getfullpath(FONT_DIRECTORY,"font2.png"), 0,T_ALPHA));
        MENU_SPACING = sfont_big->Surface->h;
    }
    /* Initialize font colors */
    font_color_white.r = 255;
    font_color_white.g = 255;
    font_color_white.b = 255;
    font_color_gray.r = 128;
    font_color_gray.g = 128;
    font_color_gray.b = 128;
    font_color_red.r = 255;
    font_color_red.g = 0;
    font_color_red.b = 0;
    font_color_green.r = 0;
    font_color_green.g = 255;
    font_color_green.b = 0;
    font_color_blue.r = 0;
    font_color_blue.g = 0;
    font_color_blue.b = 255;
    font_color_cyan.r = 0;
    font_color_cyan.g = 128;
    font_color_cyan.b = 255;
    return 0;
}

/* Write a string directly to the surface.		*/
/* This is slow if you use SDL_ttf because 		*/
/* an extra surface needs to be created and freed 	*/
/* Or, if you are using SFont, this is slow if color is */
/* something else than white				*/
void putstring_direct (SDL_Surface * surface, FontSize size, int x, int y,
                       const char *text, SDL_Color color)
{
    SDL_Surface *fontsurface;
    SDL_Rect rect;
    FontInfo font;
    rect.x = x;
    rect.y = y;
    set_proper_font (&font, size);
    if (use_sfont) {            /* SFont version */
        if (color.r == 255 && color.g == 255 && color.b == 255) {
            SFont_Write (surface, font.sfont, x, y, text,0);
        } else {
            fontsurface = make_surface(font.sfont->Surface,
                     SFont_TextWidth(font.sfont, text), font.sfont->Surface->h);
            SFont_Write (fontsurface, font.sfont, 0, 0, text,1);
            recolor (fontsurface, color.r / 255.0, color.g / 255.0,
                     color.b / 255.0, 1.0);
            SDL_SetColorKey (fontsurface, SDL_SRCCOLORKEY, 0);
            SDL_BlitSurface (fontsurface, NULL, surface, &rect);
            SDL_FreeSurface (fontsurface);
        }
    } else {                    /* SDL_ttf version */
        fontsurface = ttf_rendertext_blended (font.tfont, text, color);
        SDL_BlitSurface (fontsurface, NULL, surface, &rect);
        SDL_FreeSurface (fontsurface);
    }
}

/* A convenience wrapper for putstring_direct */
/* Centers the string horizontaly */
void centered_string (SDL_Surface * surface, FontSize size, int y,
                      const char *text, SDL_Color color)
{
    FontInfo font;
    int x, w, h;
    set_proper_font (&font, size);
    x = surface->w / 2;
    if (use_sfont)
        w = SFont_TextWidth(font.sfont, text);
    else
        ttf_sizetext (font.tfont, text, &w, &h);
    x -= w / 2;
    putstring_direct (surface, size, x, y, text, color);
}

/* Return a surface containing the text              */
/* This is slow with SFont when the you use a color	 */
/* other than white. The returned surface will be in */
/* the same format as the display                    */
SDL_Surface *renderstring (FontSize size, const char *text, SDL_Color color)
{
    SDL_Surface *fontsurface;
    FontInfo font;
    set_proper_font (&font, size);
    if (use_sfont) {            /* SFont version */
        fontsurface = make_surface(font.sfont->Surface,
                     SFont_TextWidth(font.sfont, text), font.sfont->Surface->h);
        SFont_Write(fontsurface, font.sfont, 0, 0, text,1);
        if (color.r != 255 || color.g != 255 || color.b != 255) {
            recolor (fontsurface, color.r / 255.0, color.g / 255.0,
                     color.b / 255.0, 1.0);
        }
        return fontsurface;
    } else {                    /* SDL_ttf version */
        return ttf_rendertext_blended (font.tfont, text, color);
    }
}

/* Get the height of the font */
int font_height (FontSize size)
{
    FontInfo font;
    set_proper_font (&font, size);
    if (use_sfont) {
        return font.sfont->Surface->h;
    }
#ifdef HAVE_LIBSDL_TTF
    else {
        return TTF_FontHeight (font.tfont);
    }
#endif
    return 0;
}

