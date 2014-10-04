/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : audio.c
 * Description : This module handles all audio playback
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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "audio.h"

#if HAVE_LIBSDL_MIXER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "console.h"
#include "fs.h"
#include "startup.h"
#include "parser.h"
#include "player.h"
#include "ship.h"
#include "game.h"

/* Globals */
static int audio_available = 0;
static struct dllist *playlist, *playlist_original;

static Mix_Chunk *samples[SAMPLE_COUNT];
static Mix_Music *music=NULL;

/* Music stopped playing, move on to the next track */
static void playlist_forward (void)
{
    music_skip (1);
    music_play ();
}

/* Load the music file at current playlist position */
static void load_music(void) {
    if(music) {
        Mix_FreeMusic (music);
        music = NULL;
    }
    while(music==NULL && playlist!=NULL) {
        music = Mix_LoadMUS (playlist->data);
        if (music == NULL) {
            struct dllist *next;
            fprintf(stderr,"%s: %s\n", (char*)playlist->data, SDL_GetError());

            free(playlist->data);
            next = playlist->next;
            if(next==NULL) next = playlist->prev;
            dllist_remove(playlist);
            playlist = next;
        }
    }
}
#endif

/* Initialize */
void init_audio ()
{
#if HAVE_LIBSDL_MIXER
    Uint16 audio_format;
    LDAT *ldat;
    int w;

    /* Initialize SDL_mixer library */
    audio_format = MIX_DEFAULT_FORMAT;
    if (Mix_OpenAudio
        (luola_options.audio_rate, audio_format, 2,
         luola_options.audio_chunks) < 0) {
        fprintf (stderr,"Cannot open audio: %s\n", SDL_GetError ());
        audio_available = 0;
        return;
    } else {
        audio_available = 1;
        Mix_QuerySpec (&luola_options.audio_rate, &audio_format,
                       NULL);
    }
    /* Continue to the next song if it ends */
    Mix_HookMusicFinished (playlist_forward);

    /* Load samples */
    ldat = ldat_open_file(getfullpath(DATA_DIRECTORY,"sfx.ldat"));
    if(!ldat) {
        fprintf(stderr,"Can't load sound effects!");
    } else {
        for (w = 0; w < SAMPLE_COUNT; w++) {
            samples[w] = Mix_LoadWAV_RW(ldat_get_item(ldat,"SFX",w),0);
            if (samples[w] == NULL) {
                fprintf (stderr,"Couldn't get SFX %d\n", w);
            }
        }
    }
    ldat_free(ldat);

    /* Load playlist */
    playlist = NULL;
    {
        FILE *fp;
        char tmps[512];
        char *line = NULL;
        fp = fopen (getfullpath (HOME_DIRECTORY, "battle.pls"), "r");
        if (!fp) {
            fprintf (stderr,"No playlist file battle.pls\n");
            return;
        }
        for (; fgets (tmps, sizeof (tmps) - 1, fp); free (line)) {
            line = strip_white_space (tmps);
            if (line == NULL || strlen (line) == 0)
                continue;
            if (line[0] == '#')
                continue;
            if(playlist)
                dllist_append(playlist,strdup(line));
            else
                playlist = dllist_append(NULL,strdup(line));
        }
        fclose (fp);

        playlist_original = NULL;

        /* Load the first song */
        load_music();
    }
#endif
}

void playwave (AudioSample sample)
{
#if HAVE_LIBSDL_MIXER
    if (!audio_available || !game_settings.sounds || sample==WAV_NONE ||
            samples[sample]==NULL)
        return;
    Mix_PlayChannel (-1, samples[sample], 0);
#endif
}

void playwave_3d (AudioSample sample, int x, int y)
{
#if HAVE_LIBSDL_MIXER
    int dist,angle;
    int nearest;
    if (!audio_available || !game_settings.sounds || sample==WAV_NONE ||
            samples[sample]==NULL)
        return;
    nearest = hearme (x, y);
    if (nearest < 0)
        return;
    if (players[nearest].ship) {
        x = x - players[nearest].ship->physics.x;
        y = y - players[nearest].ship->physics.y;
    } else {
        x = x - players[nearest].pilot.walker.physics.x;
        y = y - players[nearest].pilot.walker.physics.y;
    }
    dist = (0.5 + (hypot (x, y) / HEARINGRANGE)) * 180;
    angle = atan2 (x, y) * (180.0 / M_PI);
    Mix_SetPosition (Mix_PlayChannel (-1, samples[sample], 0), angle, dist);
#endif
}

void audio_setsndvolume(int volume)
{
#if HAVE_LIBSDL_MIXER
    if (audio_available)
        Mix_Volume(-1,volume);
#endif
}

void audio_setmusvolume(int volume)
{
#if HAVE_LIBSDL_MIXER
    if (audio_available)
        Mix_VolumeMusic(volume);
#endif
}

void music_play (void)
{
#if HAVE_LIBSDL_MIXER
    if (!playlist || game_settings.music == 0 || music==NULL)
        return;
    Mix_VolumeMusic (game_settings.music_vol);
    Mix_PlayMusic (music, 1);
#endif
}

void music_stop (void)
{
#if HAVE_LIBSDL_MIXER
    if (!playlist || game_settings.music == 0)
        return;
    Mix_FadeOutMusic (500);
#endif
}

void music_wait_stopped(void) {
#if HAVE_LIBSDL_MIXER
    while(Mix_FadingMusic()!=MIX_NO_FADING) {
        SDL_Delay(100);
    }
#endif
}

void music_skip (int skips)
{
#if HAVE_LIBSDL_MIXER
    if (playlist == NULL || game_settings.music == 0)
        return;

    if (game_settings.playlist == PLS_SHUFFLED || skips < 0)
        skips = rand () % dllist_count(playlist);
    while(skips-- > 0) {
        if (playlist->next)
            playlist = playlist->next;
        else
            while (playlist->prev)
                playlist = playlist->prev;
    }

    load_music();
#endif
}

void music_newplaylist (void)
{
#if HAVE_LIBSDL_MIXER
    if(playlist_original) {
        fprintf(stderr,"music_newplaylist: old playlist still exists!\n");
    }
    playlist_original = playlist;
    playlist = NULL;
#endif
}

void music_restoreplaylist (void)
{
#if HAVE_LIBSDL_MIXER
    if(!playlist_original) {
        fprintf(stderr,"music_restoreplaylist: restoring NULL playlist!\n");
    }
    dllist_free(playlist,free);
    playlist = playlist_original;
    playlist_original = NULL;
#endif
}

void music_add_song (const char *filename)
{
#if HAVE_LIBSDL_MIXER
    if(playlist)
        dllist_append(playlist,strdup(filename));
    else
        playlist = dllist_append(NULL,strdup(filename));
#endif
}

