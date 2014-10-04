/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#ifndef L_AUDIO_H
#define L_AUDIO_H

/* Samples */
typedef enum { WAV_BLIP, WAV_BLIP2, WAV_NORMALWEAP,
    WAV_SPECIALWEAP, WAV_EXPLOSION, WAV_EXPLOSION2,
    WAV_CRASH,
    WAV_LASER, WAV_JUMP, WAV_MISSILE,
    WAV_ZAP, WAV_SWOOSH, WAV_BURN,
    WAV_DART, WAV_FREEZE,
    WAV_CRITTER1, WAV_CRITTER2, WAV_STEAM
} AudioSample;

#define SAMPLE_COUNT 18

/* Initialization */
extern void init_audio (void);

extern void audio_setsndvolume(int volume);
extern void audio_setmusvolume(int volume);

/* Playback */
extern void playwave (AudioSample sample);
extern void playwave_pan (AudioSample sample, unsigned char left,
                          unsigned char right);
extern void playwave_3d (AudioSample sample, int x, int y);

/* Music */
extern void music_play (void);
extern void music_stop (void);
extern void music_skip (int skips);
extern void music_newplaylist (void);
extern void music_restoreplaylist (void);
extern void music_add_song (char *filename);

#endif
