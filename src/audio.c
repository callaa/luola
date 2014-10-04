/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#include "defines.h"
#include "audio.h"

#if HAVE_LIBSDL_MIXER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

/* Check SDL_Mixer version */
#if (MIX_MAJOR_VERSION >= 1 && MIX_MINOR_VERSION >= 2 && MIX_PATCHLEVEL > 0) || (MIX_MAJOR_VERSION >= 1 && MIX_MINOR_VERSION > 2)
#define MIXER_HAS_EFFECTS
#else
#warning You have an older version of SDL_Mixer, audio panning is not enabled.
#warning Upgrade to SDL_Mixer 1.2.1 to get better effects.
#endif


#include "console.h"
#include "fs.h"
#include "stringutil.h"
#include "startup.h"
#include "player.h"
#include "ship.h"
#include "game.h"

/* Playlist */
typedef struct _Playlist {
    char *file;
    struct _Playlist *next;
    struct _Playlist *prev;
} Playlist;

/* Globals */
char audio_available = 0;
char music_enabled = 0;
Uint16 audio_format;
Playlist *playlist, *playlist_original;
int song_count, original_song_count;

/* 
 * Remember, these have to be in the same order they are in the AudioSample enum
 * in audio.h
*/
static char *wavs[] = { "blip.wav", "blip2.wav", "fire.wav",
    "fire2.wav","explosion.wav", "largexpl.wav","crash.wav",
    "laser.wav", "jump.wav", "missile.wav", "zap.wav", "swoosh.wav",
    "burn.wav", "dart.wav", "snowball.wav",
    "cow.wav", "bird.wav", "steam.wav"
};

static Mix_Chunk *samples[SAMPLE_COUNT];
static Mix_Music *music;

#endif

/* Music stopped playing, move on to the next track */
void playlist_forward (void)
{
#if HAVE_LIBSDL_MIXER
    music_skip (1);
    music_play ();
#endif
}

/* Initialize */
void init_audio ()
{
#if HAVE_LIBSDL_MIXER
    int w;
    Playlist *next;

    /* Initialize SDL_mixer library */
    audio_format = MIX_DEFAULT_FORMAT;
    if (Mix_OpenAudio
        (luola_options.audio_rate, audio_format, luola_options.audio_channels,
         luola_options.audio_chunks) < 0) {
        printf ("Warning: Cannot open audio: %s\n", SDL_GetError ());
        audio_available = 0;
        return;
    } else {
        audio_available = 1;
        Mix_QuerySpec (&luola_options.audio_rate, &audio_format,
                       &luola_options.audio_channels);
    }
    /* Continue to the next song if it ends */
    Mix_HookMusicFinished (playlist_forward);

    /* load samples */
    for (w = 0; w < SAMPLE_COUNT; w++) {
        samples[w] = Mix_LoadWAV (getfullpath (SND_DIRECTORY, wavs[w]));
        if (samples[w] == NULL) {
            printf ("Error: Cannot open wave \"%s\"\n", wavs[w]);
            exit (1);
        }
    }
    /* Load playlist */
    playlist = NULL;
    song_count = 0;
    {
        FILE *fp;
        char tmps[512];
        char *line = NULL;
        Playlist *newsong;
        fp = fopen (getfullpath (HOME_DIRECTORY, PLAYLIST_FILE), "r");
        if (!fp) {
            printf ("No playlist file %s\n",
                    getfullpath (HOME_DIRECTORY, PLAYLIST_FILE));
            return;
        }
        for (; fgets (tmps, sizeof (tmps) - 1, fp); free (line)) {
            line = strip_white_space (tmps);
            if (line == NULL || strlen (line) == 0)
                continue;
            if (line[0] == '#')
                continue;
            newsong = malloc (sizeof (Playlist));
            newsong->next = NULL;
            newsong->prev = playlist;
            newsong->file = malloc (strlen (line) + 1);
            strcpy (newsong->file, line);
            if (playlist == NULL)
                playlist = newsong;
            else {
                playlist->next = newsong;
                playlist = playlist->next;
            }
            song_count++;
        }
        fclose (fp);
        /* Rewind the playlist */
        if (playlist)
            while (playlist->prev)
                playlist = playlist->prev;
        playlist_original = playlist;
        original_song_count = song_count;
        /* And load the first song */
        music = NULL;
        while (music == NULL) {
            if (playlist) {
                music = Mix_LoadMUS (playlist->file);
                if (music == NULL) {
                    printf
                        ("Warning: File \"%s\" from playlist could not be loaded, skipping.\n",
                         playlist->file);
                    printf ("-> %s\n", SDL_GetError ());
                    if (playlist->next)
                        playlist->next->prev = playlist->prev;
                    if (playlist->prev)
                        playlist->prev->next = playlist->next;
                    next = playlist->next;
                    free (playlist);
                    playlist = next;
                }
            } else
                break;
        }
    }
#endif
}

void playwave (AudioSample sample)
{
#if HAVE_LIBSDL_MIXER
    if (!audio_available || !game_settings.sounds)
        return;
    Mix_PlayChannel (-1, samples[sample], 0);
#endif
}

void playwave_3d (AudioSample sample, int x, int y)
{
#if HAVE_LIBSDL_MIXER
#ifdef MIXER_HAS_EFFECTS
    Uint8 dist;
    Sint16 angle;
#endif
    int nearest;
    if (!audio_available || !game_settings.sounds)
        return;
    nearest = hearme (x, y);
    if (nearest < 0)
        return;
#ifdef MIXER_HAS_EFFECTS
    if (players[nearest].ship) {
        x = x - players[nearest].ship->x;
        y = y - players[nearest].ship->y;
    } else {
        x = x - players[nearest].pilot.x;
        y = y - players[nearest].pilot.y;
    }
    dist = (0.5 + (hypot (x, y) / HEARINGRANGE)) * 180;
    angle = atan2 (x, y) * (180.0 / M_PI);
    Mix_SetPosition (Mix_PlayChannel (-1, samples[sample], 0), angle, dist);
#else
    Mix_PlayChannel (-1, samples[sample], 0);
#endif
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
    if (!playlist || game_settings.music == 0)
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
    Mix_HaltMusic ();
#endif
}

void music_skip (int skips)
{
#if HAVE_LIBSDL_MIXER
    int s;
    Playlist *next;
    if (!playlist || game_settings.music == 0)
        return;
    if (music)
        Mix_FreeMusic (music);
    music = NULL;
    if ((game_settings.playlist != 0 && skips != 0) || skips < 0)
        skips = rand () % song_count;
    for (s = 0; s < skips; s++)
        if (playlist->next)
            playlist = playlist->next;
        else
            while (playlist->prev)
                playlist = playlist->prev;
    while (music == NULL) {
        music = Mix_LoadMUS (playlist->file);
        if (music == NULL) {
            printf
                ("Warning: File \"%s\" from playlist does not exist, skipping.\n",
                 playlist->file);
            if (playlist->next)
                playlist->next->prev = playlist->prev;
            if (playlist->prev)
                playlist->prev->next = playlist->next;
            next = playlist->next;
            free (playlist);
            playlist = next;
        }
        if (playlist == NULL)
            break;
    }
#endif
}

void music_newplaylist (void)
{
#if HAVE_LIBSDL_MIXER
    playlist_original = playlist;
    song_count = 0;
    playlist = NULL;
#endif
}

void music_restoreplaylist (void)
{
#if HAVE_LIBSDL_MIXER
    Playlist *tmp;
    if (playlist) {
        while (playlist->prev)
            playlist = playlist->prev;
        while (playlist) {
            tmp = playlist->next;
            free (playlist);
            playlist = tmp;
        }
    }
    playlist = playlist_original;
    song_count = original_song_count;
#endif
}

void music_add_song (char *filename)
{
#if HAVE_LIBSDL_MIXER
    if (playlist == NULL) {
        playlist = malloc (sizeof (Playlist));
        memset (playlist, 0, sizeof (Playlist));
        playlist->file = filename;
    } else {
        while (playlist->next)
            playlist = playlist->next;
        playlist->next = malloc (sizeof (Playlist));
        playlist->next->prev = playlist;
        playlist = playlist->next;
        playlist->next = NULL;
        playlist->file = filename;
    }
    song_count++;
#endif
}

