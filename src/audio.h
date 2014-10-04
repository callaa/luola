/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : audio.h
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

#ifndef AUDIO_H
#define AUDIO_H

/* Samples */
typedef enum { WAV_NONE=-1, WAV_BLIP=0, WAV_BLIP2, WAV_NORMALWEAP,
    WAV_SPECIALWEAP, WAV_EXPLOSION, WAV_EXPLOSION2,
    WAV_CRASH,
    WAV_LASER, WAV_JUMP, WAV_MISSILE,
    WAV_ZAP, WAV_SWOOSH, WAV_BURN,
    WAV_DART, WAV_FREEZE,
    WAV_CRITTER1, WAV_CRITTER2, WAV_STEAM
} AudioSample;

#define SAMPLE_COUNT 18 /* WAV_NONE is not counted */
#define HEARINGRANGE 350.0

/* Initialization */
extern void init_audio (void);

extern void audio_setsndvolume(int volume);
extern void audio_setmusvolume(int volume);

/* Play back a sound effect */
extern void playwave (AudioSample sample);

/* Play back a sound effect in stereo. */
/* Listener is at coordinates 0,0 and x,y are */
/* the coordinates for the sound source. */
extern void playwave_3d (AudioSample sample, int x, int y);

/* Start playing back music, if any. */
extern void music_play (void);

/* Stop music playback */
extern void music_stop (void);

/* Wait until music has stopped fading */
extern void music_wait_stopped (void);

/* Skip n songs in playlist. If skips is negative, */
/* a random number of songs are skipped. */
extern void music_skip (int skips);

/* Push the current playlist into a holding buffer */
/* and start a new playlist */
extern void music_newplaylist (void);

/* Free the current playlist and restore the old one */
/* from the buffer. */
extern void music_restoreplaylist (void);

/* Add a song to the current playlist */
extern void music_add_song (const char *filename);

#endif
