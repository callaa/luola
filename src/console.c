/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : console.c
 * Description : 
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

#include "SDL_endian.h"

#include "fs.h"
#include "console.h"
#include "animation.h"

#include "game.h"
#include "player.h"
#include "particle.h"
#include "special.h"
#include "critter.h"
#include "hotseat.h"
#include "audio.h"
#include "font.h"
#include "demo.h"
#include "startup.h"

/* The Bit Depth to use */
#define SCREEN_DEPTH 32

/** Globals **/
SDL_Surface *screen;

Uint32 col_gray, col_grenade, col_snow, col_clay, col_clay_uw, col_default,
    col_yellow, col_black, col_red, col_cyan, col_white, col_rope, col_plrs[4];
Uint32 col_pause_backg, col_green, col_blue, col_translucent;

/* Internally used globals */
static SDL_Joystick *pad_0; /* The first joypad is always opened,
                               for it can be used in menus */

/** Cleanup **/
void con_cleanup (void)
{
    if (pad_0)
        SDL_JoystickClose (pad_0);
    SDL_Quit ();
}

/** Initalize SDL **/
void init_sdl ()
{
    Uint32 initflags = SDL_INIT_VIDEO;
    if (luola_options.joystick)
        initflags = initflags | SDL_INIT_JOYSTICK;
#if HAVE_LIBSDL_MIXER
    if (luola_options.sounds)
        initflags = initflags | SDL_INIT_AUDIO;
#endif
    if (SDL_Init (initflags)) {
        fprintf (stderr,"Unable to initialize SDL: %s\n", SDL_GetError ());
        exit (1);
    }

    /* Try opening the first joystick */
    pad_0 = NULL;
    if (luola_options.joystick) {
        int jscount = SDL_NumJoysticks ();
        if (jscount == 0) {
            fprintf (stderr,"No joysticks available\n");
        } else {
            pad_0 = SDL_JoystickOpen (0);
            if (pad_0 == NULL)
                fprintf (stderr,"Unable to open the first joypad: %s\n",
                        SDL_GetError ());
        }
    }
    atexit (con_cleanup);
}

/* Open player joypads */
void open_joypads() {
    int r;
    for(r=0;r<4;r++) {
        if(game_settings.controller[r].number>1) {
            game_settings.controller[r].device =
                SDL_JoystickOpen(game_settings.controller[r].number-1);
            if(pad_0 == NULL) {
                fprintf(stderr,"Couldn't open joypad %d",
                        game_settings.controller[r].number-1);
            }
        }
    }
}

/* Close player joypads */
void close_joypads() {
    int r;
    for(r=0;r<4;r++) {
        if(game_settings.controller[r].device) {
            SDL_JoystickClose (game_settings.controller[r].device);
            game_settings.controller[r].device = NULL;
        }
    }
}

/** Initialize video **/
void init_video () {
    int swidth,sheight;
    switch(luola_options.videomode) {
        case VID_640:
            swidth = 640;
            sheight = 480;
            break;
        case VID_800:
            swidth = 800;
            sheight = 600;
            break;
        case VID_1024:
            swidth = 1024;
            sheight = 768;
            break;
        default:
            fprintf(stderr,"Bug! init_video(): unknown video mode %d!\n",luola_options.videomode);
            exit(1);
    }
    screen =
        SDL_SetVideoMode (swidth, sheight, SCREEN_DEPTH,
                          (luola_options.fullscreen?SDL_FULLSCREEN:0)|SDL_SWSURFACE);
    if (screen == NULL) {
        fprintf (stderr,"Unable to set video mode: %s\n", SDL_GetError ());
        exit (1);
    }
    if (luola_options.hidemouse)
        SDL_ShowCursor (SDL_DISABLE);
    /* Set window caption */
    SDL_WM_SetCaption (PACKAGE_STRING, PACKAGE);
    /* Initialize colours */
    col_black = map_rgba (0, 0, 0, 255);
    col_gray = map_rgba (128, 128, 128, 255);
    col_grenade = map_rgba (160, 160, 160, 255);
    col_snow = map_rgba (176, 193, 255, 255);
    col_clay = map_rgba (255, 200, 128, 255);
    col_default = map_rgba (255, 100, 100, 255);
    col_yellow = map_rgba (255, 255, 100, 255);
    col_red = map_rgba (255, 0, 0, 255);
    col_blue = map_rgba (0, 0, 255, 255);
    col_cyan = map_rgba (0, 128, 255, 255);
    col_white = map_rgba (255, 255, 255, 255);
    col_rope = map_rgba (178, 159, 103, 255);
    col_plrs[0] = map_rgba (255, 0, 0, 255);
    col_plrs[1] = map_rgba (0, 0, 255, 255);
    col_plrs[2] = map_rgba (0, 255, 0, 255);
    col_plrs[3] = map_rgba (255, 255, 0, 255);
    col_pause_backg = map_rgba (128, 128, 128, 128);
    col_green = map_rgba (0, 158, 0, 255);
    col_translucent = map_rgba(255,255,255,128);
}

/* Map RGBA colors using either plain SDL or SDL_gfx */
Uint32 map_rgba(Uint8 r,Uint8 g,Uint8 b,Uint8 a)
{
#if HAVE_LIBSDL_GFX
        return (r << 24) + (g << 16) + (b << 8) + a;
#else
        r = r/255.0 * a;
        g = g/255.0 * a;
        b = b/255.0 * a;
        return SDL_MapRGB (screen->format, r, g, b);
#endif
}

/* Unmap RGBA colors using either plain SDL or SDL_gfx */
void unmap_rgba(Uint32 c,Uint8 *r,Uint8 *g,Uint8 *b,Uint8 *a)
{
#if HAVE_LIBSDL_GFX
        *r = c >> 24;
        *g = c >> 16;
        *b = c >> 8;
        *a = c;
#else
        SDL_GetRGBA (c,screen->format, r, g, b, a);
#endif
}

/* Draw a box */
void draw_box (int x, int y, int w, int h, int width, Uint32 color) {
#if HAVE_LIBSDL_GFX
    int r;
    for (r = 0; r < width; r++)
        rectangleColor (screen, x + r, y + r, x + w - r-1, y + h - r-1, color);
#else
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = width;
    SDL_FillRect (screen, &rect, color);
    rect.y += h - width;
    SDL_FillRect (screen, &rect, color);
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = h;
    SDL_FillRect (screen, &rect, color);
    rect.x += w - width;
    SDL_FillRect (screen, &rect, color);
#endif
}

/* Draw a filled box */
void fill_box (SDL_Surface *surface,int x, int y, int w, int h, Uint32 color) {
#if HAVE_LIBSDL_GFX
    boxColor(surface, x, y, x+w, y+h, color);
#else
    SDL_Rect rect = {x,y,w,h};
    SDL_FillRect(surface, &rect, color);
#endif
}

/* Do a raw 32bit pixel copy */
void pixelcopy (Uint32 * srcpix, Uint32 * pixels, int w, int h, int srcpitch,
                int pitch) {
    int y;
    for (y = 0; y < h; y++) {
        memcpy(pixels,srcpix,w*sizeof(Uint32));
        pixels += pitch;
        srcpix += srcpitch;
    }
}

/* Returns a copy of the surface */
SDL_Surface *copy_surface (SDL_Surface * original) {
    SDL_Surface *newsurface;

    newsurface = make_surface(original,0,0);
    memcpy (newsurface->pixels, original->pixels,original->pitch*original->h);

    return newsurface;
}

/* Make a new surface that is like the one given */
SDL_Surface *make_surface(SDL_Surface * likethis,int w,int h) {
    if(w==0 || h==0) {
        w = likethis->w;
        h = likethis->h;
    }
    SDL_Surface *surface = SDL_CreateRGBSurface (likethis->flags,
                                  w, h,
                                  likethis->format->BitsPerPixel,
                                  likethis->format->Rmask,
                                  likethis->format->Gmask,
                                  likethis->format->Bmask,
                                  likethis->format->Amask);
    if (likethis->format->BytesPerPixel == 1) /* Copy the palette */
        SDL_SetPalette (surface, SDL_LOGPAL,
                        likethis->format->palette->colors, 0,
                        likethis->format->palette->ncolors);

    return surface;
}

/* Rectangle clipping */
SDL_Rect cliprect (int x1, int y1, int w1, int h1, int x2, int y2, int x3,
                   int y3) {
    int d;
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = w1;
    r.h = h1;
    if (x1 < x2) {
        d = x2 - x1;
        r.x += d;
        r.w -= d;
    }
    if (x1 + w1 > x3) {
        r.w -= x1 + w1 - x3;
    }
    if (y1 < y2) {
        d = y2 - y1;
        r.y += d;
        r.h -= d;
    }
    if (y1 + h1 > y3) {
        r.h -= y1 + h1 - y3;
    }
    return r;
}

int clip_line (int *x1, int *y1, int *x2, int *y2, int left, int top,
                int right, int bottom) {
    double m;
    if ((*x1 < left && *x2 < left) || (*x1 > right && *x2 > right))
        return 0;
    if ((*y1 < top && *y2 < top) || (*y1 > bottom && *y2 > bottom))
        return 0;
    if (*x1 != *x2) {
        m = (double) (*y2 - *y1) / (double) (*x2 - *x1);
    } else {
        m = 1.0;
    }
    if (*x1 < left) {
        *y1 += (left - *x1) * m;
        *x1 = left;
    } else if (*x2 < left) {
        *y2 += (left - *x2) * m;
        *x2 = left;
    }
    if (*x2 > right) {
        *y2 += (right - *x2) * m;
        *x2 = right;
    } else if (*x1 > right) {
        *y1 += (right - *x1) * m;
        *x1 = right;
    }
    if (*y1 > bottom) {
        if (*x2 != *x1) {
            *x1 += (bottom - *y1) / m;
        }
        *y1 = bottom;
    } else if (*y2 > bottom) {
        if (*x2 != *x1) {
            *x2 += (bottom - *y2) / m;
        }
        *y2 = bottom;
    }
    if (*y1 < top) {
        if (*x2 != *x1) {
            *x1 += (top - *y1) / m;
        }
        *y1 = top;
    } else if (*y2 < top) {
        if (*x2 != *x1) {
            *x2 += (top - *y2) / m;
        }
        *y2 = top;
    }
    return 1;
}

/* Modify the colors of a surface (Used to change the player colors in ship sprites and such) */
void recolor (SDL_Surface * surface, float red, float green, float blue,
              float alpha) {
    Uint8 *pos, *end;
    if (surface->format->BytesPerPixel != 4) {
        fprintf(stderr,"Unsupported color depth (%d bytes per pixel), currently only 4 bps is supported. (fix this at recolor(), console.c)\n",
             screen->format->BytesPerPixel);
        exit (1);
    }

    pos = (Uint8 *) surface->pixels;
    end = pos + 4 * surface->w * surface->h;
    while (pos < end) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        (*pos++) *= blue;
        (*pos++) *= green;
        (*pos++) *= red;
        (*pos++) *= alpha;
#else
        (*pos++) *= alpha;
        (*pos++) *= red;
        (*pos++) *= green;
        (*pos++) *= blue;
#endif
    }
}

/* Return a scaled version of the surface */
SDL_Surface *zoom_surface(SDL_Surface *original, float aspect, float zoom) {
    SDL_Surface *scaled;
    Uint8 *targ, bpp;
    int x, y;

    bpp = original->format->BytesPerPixel;
    if(bpp!=1 && bpp!=4) {
        fprintf(stderr,"zoom_surface(): surface has %d bytes per pixel, only 1 and 4 are supported!\n",bpp);
        return NULL;
    }
    scaled = make_surface(original,original->w * aspect * zoom,
            original->h * zoom);

    targ = ((Uint8 *) scaled->pixels);
    for (y = 0; y < scaled->h; y++) {
        for (x = 0; x < scaled->w; x++) {
#if 1 /* This works with a limited set of bitdepths, but is faster than using memcpy */
            switch (bpp) {
            case 1:
                *((Uint8 *) (targ)) =
                    *((Uint8 *) (original->pixels)+((int)(y/zoom))*original->w+(int)(x/(aspect*zoom)));
                targ++;
                break;
            case 4:
                *((Uint32 *) (targ)) =
                    *((Uint32 *) (original->pixels)+((int)(y/zoom))*original->w+(int)(x/(aspect*zoom)));
                targ += 4;
                break;
            }
#else /* This works with all bitdepths, but is a bit slower */
            memcpy(targ,original->pixels+(int)(y/zoom)*original->pitch+
                    (int)(x/(aspect*zoom))*bpp,bpp);
            targ+=bpp;
#endif
        }
        targ += scaled->pitch - scaled->w*bpp;
    }
    return scaled;
}

/* Wait for a keypress */
/* Also handles screenshot and toggling between fullscreen
 * and windowed mode.
 */
void wait_for_enter(void) {
    SDL_Event e;
    while(1) {
        SDL_WaitEvent(&e);
        if(e.type==SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                if((e.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                    toggle_fullscreen();
                else
                    break;
            } else if(e.key.keysym.sym == SDLK_F11) {
                screenshot();
            }
        } else if(e.type==SDL_JOYBUTTONDOWN)
            break;
        else if(e.type==SDL_QUIT)
            exit(0);
    }
}

/* Toggle between fullscreen and windowed mode */
void toggle_fullscreen(void) {
#if (defined(unix) || defined(__unix__) || defined(_AIX) || \
     defined(__OpenBSD__)) && (!defined(DISABLE_X11) && \
     !defined(__CYGWIN32__) && !defined(ENABLE_NANOX) && \
     !defined(__QNXNTO__))
    /* This is the easy way, but it only works under X11 */
    SDL_WM_ToggleFullScreen(screen);
#else
    int fullscreen = !(screen->flags & SDL_FULLSCREEN);
    int w=screen->w,h=screen->h;
    char *pixels;
    /* The pixel data on screen will be lost, so copy it first */
    pixels = malloc(screen->h*screen->pitch);
    memcpy(pixels,screen->pixels,screen->h*screen->pitch);

    /* Create a new screen */
    SDL_FreeSurface(screen);
    screen =
        SDL_SetVideoMode (w, h, SCREEN_DEPTH,
                          (fullscreen?SDL_FULLSCREEN:0)|SDL_SWSURFACE);
    if (screen == NULL) {
        fprintf (stderr,"Unable to set video mode: %s\n", SDL_GetError ());
        exit (1);
    }

    /* Copy back the pixel data */
    memcpy(screen->pixels,pixels,screen->h*screen->pitch);
    free(pixels);
    SDL_UpdateRect(screen,0,0,0,0);
#endif
}

/* Display an error message */
void error_screen(const char *title, const char *exitmsg, const char *message[]) {
    SDL_Rect r1, r2;
    int txty;

    r1.x = 10;
    r1.w = screen->w - 20;
    r1.y = 10;
    r1.h = screen->h - 20;
    r2.x = r1.x + 2;
    r2.y = r1.y + 2;
    r2.w = r1.w - 4;
    r2.h = r1.h - 4;
    txty= r2.y + 10;

    SDL_FillRect (screen, &r1, SDL_MapRGB (screen->format, 255, 0, 0));
    SDL_FillRect (screen, &r2, SDL_MapRGB (screen->format, 0, 0, 0));
    centered_string (screen, Bigfont, txty, title, font_color_red);
    txty += 40;

    while(*message) {
        if(**message!='\0')
            centered_string (screen, Bigfont, txty,*message, font_color_white);
        txty += font_height(Bigfont);
        message++;
    }
    
    centered_string(screen,Bigfont, r2.y + r2.h - font_height(Bigfont),
            exitmsg, font_color_red);
    SDL_UpdateRect (screen, r1.x, r1.y, r1.w, r1.h);
    wait_for_enter();

}

/* Translate joystick button press to keypress */
void joystick_button (SDL_JoyButtonEvent * btn) {
    SDL_Event event;
    event.key.state = btn->state;
    event.key.keysym.mod = 0;
    event.key.type =
        (btn->type == SDL_JOYBUTTONDOWN) ? SDL_KEYDOWN : SDL_KEYUP;
    if (btn->button == 0 || btn->button == 3)
        event.key.keysym.sym = SDLK_RETURN;
    else
        return;
    SDL_PushEvent (&event);
}

/* Translate joystick motion to keypress */
void joystick_motion (SDL_JoyAxisEvent * axis, int plrmode) {
    SDL_Event event;
    int p, plr = -1;
    if (axis->axis > 1)
        return;                 /* Only the first two axises are handled */
    if (abs (axis->value) > 16384) {
        event.key.state = SDL_PRESSED;
        event.key.type = SDL_KEYDOWN;
    } else {
        event.key.state = SDL_RELEASED;
        event.key.state = SDL_KEYUP;
    }
    for (p = 0; p < 4; p++)
        if (game_settings.controller[p].number - 1 == axis->which) {
            plr = p;
            break;
        }
    if (axis->axis == 0)
        p = 2;
    else
        p = 0;
    if (axis->value > 0)
        p++;
    if (p == 0) {
        if (plrmode && plr > -1)
            event.key.keysym.sym = game_settings.controller[plr].keys[0];
        else
            event.key.keysym.sym = SDLK_UP;
    } else if (p == 1) {
        if (plrmode && plr > -1)
            event.key.keysym.sym = game_settings.controller[plr].keys[1];
        else
            event.key.keysym.sym = SDLK_DOWN;
    } else if (p == 2) {
        if (plrmode && plr > -1)
            event.key.keysym.sym = game_settings.controller[plr].keys[2];
        else
            event.key.keysym.sym = SDLK_LEFT;
    } else {
        if (plrmode && plr > -1)
            event.key.keysym.sym = game_settings.controller[plr].keys[3];
        else
            event.key.keysym.sym = SDLK_RIGHT;
    }
    event.key.keysym.mod = 0;
    SDL_PushEvent (&event);
}

#ifndef HAVE_LIBSDL_GFX
/* Draw a single pixel */
/* Note. The support code for other bit depths than 32bit is currently commented out.
 * this is because, this function is currently used only with 32bit surfaces.
 * Should this change, remove the comments. */

inline void putpixel (SDL_Surface * surface, int x, int y, Uint32 color) {
#if SCREEN_DEPTH != 32
#error update putpixel() with bitdepths other than 32!
#endif
    Uint8 *bits, bpp;
    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h)
        return;
    bpp = surface->format->BytesPerPixel;
    bits = ((Uint8 *) surface->pixels) + y * surface->pitch + x * bpp;
#if 0
    switch(bpp) {
        case 1:
            *((Uint8 *)(bits)) = (Uint8)color;
            break;
        case 2:
            *((Uint16 *)(bits)) = (Uint16)color;
            break;
        case 3: {
            Uint8 r, g, b;
            r = (color>>surface->format->Rshift)&0xFF;
            g = (color>>surface->format->Gshift)&0xFF;
            b = (color>>surface->format->Bshift)&0xFF;
            *((bits)+surface->format->Rshift/8) = r;
            *((bits)+surface->format->Gshift/8) = g;
            *((bits)+surface->format->Bshift/8) = b;
            }
            break;
        case 4:
#endif
            *((Uint32 *) (bits)) = (Uint32) color;
#if 0
    }
#endif
}
#endif

/*
 * Get a single pixel
 */
Uint32 getpixel (SDL_Surface * surface, int x, int y)
{
    Uint32 color = 0, temp;
    SDL_Color scolor;
    switch (surface->format->BytesPerPixel) {
    case 1:
        color = *((Uint8 *) surface->pixels + y * surface->pitch + x);
        scolor = surface->format->palette->colors[color];
        color = (scolor.r << 24) | (scolor.g << 16) | (scolor.b << 8);
        break;
    case 2:
        temp = *((Uint16 *) surface->pixels + y * surface->pitch / 2 + x * 2);
        color = temp & surface->format->Rmask;
        color = temp >> surface->format->Rshift;
        color = temp << surface->format->Rloss;
        scolor.r = color;
        color = temp & surface->format->Gmask;
        color = temp >> surface->format->Gshift;
        color = temp << surface->format->Gloss;
        scolor.g = color;
        color = temp & surface->format->Bmask;
        color = temp >> surface->format->Bshift;
        color = temp << surface->format->Bloss;
        scolor.b = color;
        color = scolor.r << 24 | scolor.g << 16 | scolor.b << 8;
        break;
    case 3:                    /* Format/endian independent */
        scolor.r =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Rshift / 8);
        scolor.g =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Gshift / 8);
        scolor.b =
            *(((Uint8 *) surface->pixels + y * surface->pitch + x * 3) +
              surface->format->Bshift / 8);
        color = scolor.r << 24 | scolor.g << 16 | scolor.b << 8;
        break;
    case 4:
        color = *((Uint32*)surface->pixels + y * (surface->pitch/4) + x);
        break;
    }
    return color;
}

#ifndef HAVE_LIBSDL_GFX
/* Draw a line from point A to point B */
void draw_line (SDL_Surface * screen, int x1, int y1, int x2, int y2,
                Uint32 pixel)
{
    Uint8 *bits, bpp;
#if 0
    Uint8 r, g, b;
#endif
    int dx, dy;
    int ax, ay;
    int sx, sy;
    int x, y;

    if (x1 < 0)
        x1 = 0;
    else if (x1 >= screen->w)
        x1 = screen->w-1;
    if (x2 < 0)
        x2 = 0;
    else if (x2 >= screen->w)
        x2 = screen->w-1;
    if (y1 < 0)
        y1 = 0;
    else if (y1 >= screen->h)
        y1 = screen->h-1;
    if (y2 < 0)
        y2 = 0;
    else if (y2 >= screen->h)
        y2 = screen->h-1;

    dx = x2 - x1;
    dy = y2 - y1;
    ax = abs (dx) << 1;
    ay = abs (dy) << 1;
    sx = (dx >= 0) ? 1 : -1;
    sy = (dy >= 0) ? 1 : -1;
    x = x1;
    y = y1;
#if SCREEN_DEPTH != 32
#error update draw_line() with bitdepths other than 32!
#endif
    bpp = screen->format->BytesPerPixel;
    if (ax > ay) {
        int d = ay - (ax >> 1);
        while (x != x2) {
                /***  DRAW PIXEL HERE ***/
            bits = ((Uint8 *) screen->pixels) + y * screen->pitch + x * bpp;
#if 0
            switch (bpp) {
                case 1:
                    *((Uint8 *) (bits)) = (Uint8) pixel;
                    break;
                case 2:
                    *((Uint16 *) (bits)) = (Uint16) pixel;
                    break;
                case 3:{           /* Format/endian independent */
                    r = (pixel >> screen->format->Rshift) & 0xFF;
                    g = (pixel >> screen->format->Gshift) & 0xFF;
                    b = (pixel >> screen->format->Bshift) & 0xFF;
                    *((bits) + screen->format->Rshift / 8) = r;
                    *((bits) + screen->format->Gshift / 8) = g;
                    *((bits) + screen->format->Bshift / 8) = b;
                    }
                    break;
                case 4:
#endif
                    *((Uint32 *) (bits)) = (Uint32) pixel;
#if 0
            }
#endif
                /*** END DRAWING PIXEL ***/
            if (d > 0 || (d == 0 && sx == 1)) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    } else {
        int d = ax - (ay >> 1);
        while (y != y2) {
                /*** DRAW PIXEL HERE ***/
            bits = ((Uint8 *) screen->pixels) + y * screen->pitch + x * bpp;
#if 0
            switch (bpp) {
                case 1:
                    *((Uint8 *) (bits)) = (Uint8) pixel;
                    break;
                case 2:
                    *((Uint16 *) (bits)) = (Uint16) pixel;
                    break;
                case 3:{           /* Format/endian independent */
                        r = (pixel >> screen->format->Rshift) & 0xFF;
                        g = (pixel >> screen->format->Gshift) & 0xFF;
                        b = (pixel >> screen->format->Bshift) & 0xFF;
                        *((bits) + screen->format->Rshift / 8) = r;
                        *((bits) + screen->format->Gshift / 8) = g;
                        *((bits) + screen->format->Bshift / 8) = b;
                    }
                    break;
                case 4:
#endif
                    *((Uint32 *) (bits)) = (Uint32) pixel;
#if 0
            }
#endif

                /*** END DRAWING PIXEL ***/

            if (d > 0 || (d == 0 && sy == 1)) {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
    /*** DRAW PIXEL HERE ***/
    bits = ((Uint8 *) screen->pixels) + y * screen->pitch + x * bpp;
#if 0
    switch (bpp) {
        case 1:
            *((Uint8 *) (bits)) = (Uint8) pixel;
            break;
        case 2:
            *((Uint16 *) (bits)) = (Uint16) pixel;
            break;
        case 3:{                   /* Format/endian independent */
                r = (pixel >> screen->format->Rshift) & 0xFF;
                g = (pixel >> screen->format->Gshift) & 0xFF;
                b = (pixel >> screen->format->Bshift) & 0xFF;
                *((bits) + screen->format->Rshift / 8) = r;
                *((bits) + screen->format->Gshift / 8) = g;
                *((bits) + screen->format->Bshift / 8) = b;
            }
            break;
        case 4:
#endif
            *((Uint32 *) (bits)) = (Uint32) pixel;
#if 0
            break;
    }
#endif
    /*** END DRAWING PIXEL HERE ***/
}

#endif

