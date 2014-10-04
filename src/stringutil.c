/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : stringutil.c
 * Description : Miscallenous string operations
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL_keysym.h>

#include "defines.h"
#include "game.h"
#include "weapon.h"
#include "critter.h"
#include "stringutil.h"

/* actually strips all characters upto 0x20, \r, \n, \t... */
char *strip_white_space (const char *str)
{
    int len;
    char *newstr = NULL;
    const char *s1, *s2;
    s1 = str;
    s2 = str + strlen (str) - 1;
    while (s1 < s2 && (*s1 <= ' '))
        s1++;
    while (s2 >= s1 && (*s2 <= ' '))
        s2--;
    s2++;
    len = s2 - s1;
    if (len <= 0)
        return NULL;
    newstr = malloc (sizeof (char) * (len + 1));
    if (newstr == NULL) {
        printf
            ("Malloc error at strip_white_space ! (attempted to allocate %d bytes\n",
             len);
        exit (1);
    }
    strncpy (newstr, s1, len);
    newstr[len] = '\0';
    return newstr;
}

int split_string (char *str, char delim, char **left, char **right)
{
    int l1;
    char *tl = NULL, *tr = NULL;
    tl = strchr (str, delim);
    if(tl) l1 = tl-str; else return 1;
    tl = malloc (l1 + 1);
    strncpy (tl, str, l1);
    tl[l1] = 0;
    tr = malloc (strlen (str) - l1);
    strcpy (tr, str + l1 + 1);

    *left = strip_white_space (tl);
    *right = strip_white_space (tr);
    free (tl);
    free (tr);
    return 0;
}

const char *controller_name (int type)
{
    switch (type) {
    case Keyboard:
        return "Keyboard";
    case Pad1:
        return "Pad 1";
    case Pad2:
        return "Pad 2";
    case Pad3:
        return "Pad 3";
    case Pad4:
        return "Pad 4";
    default:
        return "huh ?";
    }
}

const char *weap2str (int weapon)
{
    switch (weapon) {
    case WCannon:
        return "Cannon";     /* This is the ordinary weapon, it should newer get here */
    case WGrenade:
        return "Grenade";
    case WMegaBomb:
        return "Mega bomb";
    case WMissile:
        return "Homing missile";
    case WCloak:
        return "Cloaking device";
    case WMagMine:
        return "Magnetic mine";
    case WMine:
        return "Mine";
    case WShield:
        return "Shield";
    case WGhostShip:
        return "Ghost Ship";
    case WAfterBurner:
        return "After Burner";
    case WWarp:
        return "Jump engine";
    case WClaybomb:
        return "Claybomb";
    case WPlastique:
        return "Plastic explosive";
    case WSnowball:
        return "Snowball";
    case WDart:
        return "Dart";
    case WLandmine:
        return "Landmine";
    case WRepair:
        return "Autorepair system";
    case WInfantry:
        return "Infantry";
    case WHelicopter:
        return "Helicopter";
    case WSpeargun:
        return "Speargun";
    case WGravGun:
        return "Grav-gun";
    case WGravMine:
        return "Gravity mine";
    case WZapper:
        return "Thunderbolt";
    case WShotgun:
        return "Shotgun";
    case WRocket:
        return "Rocket";
    case WEnergy:
        return "Microwave cannon";
    case WBoomerang:
        return "Boomerang bomb";
    case WRemote:
        return "Remote control";
    case WDivide:
        return "Dividing mine";
    case WTag:
        return "Tag-gun";
    case WMush:
        return "Mush";
    case WWatergun:
        return "Watergun";
    case WEmber:
        return "Ember";
    case WAcid:
        return "Acid";
    case WMirv:
        return "MIRV";
    case WFlame:
        return "Flamethrower";
    case WEMP:
        return "EMP";
    case WAntigrav:
        return "Gravity coil";
    }
    return "foo";
}

const char *sweap2str (int weapon)
{
    switch (weapon) {
    case SShot:
        return "Cannon";
    case S3ShotWide:
        return "Wide triple shot";
    case S3ShotTight:
        return "Tight triple shot";
    case SSweep:
        return "Sweeping shots";
    case S4Way:
        return "4 Way";
    }
    return "foo";
}

const char *critical2str (int critical)
{
    switch (critical) {
    case CRITICAL_ENGINE:
        return "Critical engine core";
    case CRITICAL_FUELCONTROL:
        return "Critical fuel control";
    case CRITICAL_LTHRUSTER:
        return "Critical left thruster";
    case CRITICAL_RTHRUSTER:
        return "Critical right thruster";
    case CRITICAL_CARGO:
        return "Critical cargo hold";
    case CRITICAL_STDWEAPON:
        return "Critical mainweapon";
    case CRITICAL_SPECIAL:
        return "Critical special weapon";
    case CRITICAL_POWER:
        return "Temporary power failure";
    }
    return "Critical FOO";
}

const char *critter2str(int critter) {
    switch(critter) {
        case Bird:
            return "bird";
        case Cow:
            return "cow";
        case Fish:
            return "fish";
        case Bat:
            return "bat";
        case Infantry:
            return "soldier";
        case Helicopter:
            return "helicopter";
    }
    return "SPOO";
}

int name2terrain(const char *name) {
    int terrain=TER_FREE;
    if(strcmp(name,"free")==0) terrain=TER_FREE;
    else if(strcmp(name,"ground")==0) terrain=TER_GROUND;
    else if(strcmp(name,"underwater")==0) terrain=TER_UNDERWATER;
    else if(strcmp(name,"indestructable")==0) terrain=TER_INDESTRUCT;
    else if(strcmp(name,"water")==0) terrain=TER_WATER;
    else if(strcmp(name,"base")==0) terrain=TER_BASE;
    else if(strcmp(name,"explosive")==0) terrain=TER_EXPLOSIVE;
    else if(strcmp(name,"explosive2")==0) terrain=TER_EXPLOSIVE2;
    else if(strcmp(name,"waterup")==0) terrain=TER_WATERFU;
    else if(strcmp(name,"waterright")==0) terrain=TER_WATERFR;
    else if(strcmp(name,"waterdown")==0) terrain=TER_WATERFD;
    else if(strcmp(name,"waterleft")==0) terrain=TER_WATERFL;
    else if(strcmp(name,"combustable")==0) terrain=TER_COMBUSTABLE;
    else if(strcmp(name,"combustable2")==0) terrain=TER_COMBUSTABL2;
    else if(strcmp(name,"snow")==0) terrain=TER_SNOW;
    else if(strcmp(name,"ice")==0) terrain=TER_ICE;
    else if(strcmp(name,"basemat")==0) terrain=TER_BASEMAT;
    else if(strcmp(name,"tunnel")==0) terrain=TER_TUNNEL;
    else if(strcmp(name,"walkway")==0) terrain=TER_WALKWAY;
    else printf("Error: unknown terrain name \"%s\" (set to TER_FREE)\n",name);
    return terrain;
}

