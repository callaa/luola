/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : lconf.c
 * Description : Level configuration file parsing
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
#include <stdio.h>
#include <string.h>

#include "level.h"
#include "fs.h"
#include "lconf.h"
#include "parser.h"

static void parse_main_block(struct dllist *values,struct LSB_Main *mainb) {
    struct Translate tr[] = {
        {"collisionmap", CFG_STRING, &mainb->collmap},
        {"artwork", CFG_STRING, &mainb->artwork},
        {"thumbnail", CFG_STRING, &mainb->thumbnail},
        {"name", CFG_STRING, &mainb->name},
        {"aspect", CFG_FLOAT, &mainb->aspect},
        {"zoom", CFG_FLOAT, &mainb->zoom},
        {"music", CFG_MULTISTRING, &mainb->music},
        {0,0,0}
    };

    translate_config(values,tr,0);
}

static struct LSB_Override *parse_override_block(struct dllist *values,
        struct LSB_Override *override) {
    struct Translate tr[] = {
        {"critters", CFG_INT, &override->critters},
        {"bases_indestructable", CFG_INT, &override->indstr_base},
        {"snowfall", CFG_INT, &override->snowfall},
        {"stars", CFG_INT, &override->stars},
        {"turrets", CFG_INT, &override->turrets},
        {"jumpgates", CFG_INT, &override->jumpgates},
        {"cows", CFG_INT, &override->cows},
        {"fish", CFG_INT, &override->fish},
        {"birds", CFG_INT, &override->birds},
        {"bats", CFG_INT, &override->bats},
        {0,0,0}
    };

    override->indstr_base = -1;
    override->critters = -1;
    override->stars = -1;
    override->snowfall = -1;
    override->turrets = -1;
    override->jumpgates = -1;
    override->cows = -1;
    override->fish = -1;
    override->birds = -1;
    override->bats = -1;

    translate_config(values,tr,0);

    return override;
}

/* Object type string */
const char *obj2str(ObjectType obj) {
    switch(obj) {
        case OBJ_TURRET: return "turret";
        case OBJ_JUMPGATE: return "jumpgate";
        case OBJ_COW: return "cow";
        case OBJ_FISH: return "fish";
        case OBJ_BIRD: return "bird";
        case OBJ_BAT: return "bat";
        case OBJ_SOLDIER: return "soldier";
        case OBJ_HELICOPTER: return "helicopter";
        case OBJ_SHIP: return "ship";
    }
    return "<unknown>";
}

static struct LSB_Object *parse_object_block(struct dllist *values,
        struct LSB_Object *object)
{
    char *typestr;
    struct Translate tr[] = {
        {"type", CFG_STRING, &typestr},
        {"x", CFG_INT, &object->x},
        {"y", CFG_INT, &object->y},
        {"value", CFG_INT, &object->value},
        {"link", CFG_INT, &object->link},
        {"id", CFG_INT, &object->id},
        {0,0,0}
    };

    object->type = 0;
    object->x = 0; object->y = 0;
    object->value = 0;
    object->id = 0; object->link = 0;


    translate_config(values,tr,0);

    if(typestr==NULL) {
        fprintf(stderr,"Warning: an object without a type!\n");
    } else {
        int r;
        for(r=0;r<OBJECT_TYPES;r++) {
            if(strcmp(typestr,obj2str(r))==0) {
                object->type = r;
                break;
            }
        }
        if(r==OBJECT_TYPES)
            fprintf(stderr,"Warning: unknown object type %s\n",typestr);
    }

    free(typestr);

    return object;
}

static int name2terrain(const char *name) {
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
    else fprintf(stderr, "%s(\"%s\"): unknown terrain name\n",__func__,name);
    return terrain;
}

static void parse_palette_block(struct dllist *values,struct LSB_Palette *palette) {

    memset(palette->entries,0,256);
    while(values) {
        struct KeyValue *pair = values->data;
        int mapto;
        if(pair==NULL || pair->key==NULL || pair->value==NULL) {
            fprintf(stderr,"Malformed palette entry:\n");
            fprintf(stderr,"\t\"%s\" = \"%s\"\n",pair->key?pair->key:"(null)",
                    pair->value?pair->value:"(null)");
            values = values->next;
            continue;

        }
        mapto = name2terrain(pair->value);
        if(strchr(pair->key,'-')) { /* Range */
            int from,to;
            if(sscanf(pair->key,"%d - %d",&from,&to)!=2) {
                fprintf(stderr,"Malformed palette entry:\n");
                fprintf(stderr,"\t\"%s\" = \"%s\"\n",pair->key,pair->value);
                values = values->next;
                continue;
            }
            if(from<0 || from>255 || to<0 || to>255) {
                fprintf(stderr,"Palette map range %d-%d out of bounds",
                    from,to);
            } else {
                if(to<from) {
                    int tmp;
                    tmp=to; to=from; from=tmp;
                }
                for(;from<=to;from++) palette->entries[from]=mapto;
            }
        } else { /* Single value */
            int index = atoi(pair->key);
            if(index<0 || index>255) {
                fprintf(stderr,"Palette map index %d out of bounds",index);
            } else {
                palette->entries[index] = mapto;
            }
        }
        values = values->next;
    }
}

/* Generate default palette */
/* Up to luola 1.3.1, this was also luola's internal palette */
static void set_default_palette(struct LSB_Palette *palette) {
    palette->entries[0]  = TER_FREE;
    palette->entries[1]  = TER_GROUND;
    palette->entries[2]  = TER_UNDERWATER;
    palette->entries[3]  = TER_INDESTRUCT;
    palette->entries[4]  = TER_WATER;
    palette->entries[5]  = TER_BASE;
    palette->entries[6]  = TER_EXPLOSIVE;
    palette->entries[7]  = TER_EXPLOSIVE2;
    palette->entries[8]  = TER_WATERFU;
    palette->entries[9]  = TER_WATERFR;
    palette->entries[10] = TER_WATERFD;
    palette->entries[11] = TER_WATERFL;
    palette->entries[12] = TER_COMBUSTABLE;
    palette->entries[13] = TER_COMBUSTABL2;
    palette->entries[14] = TER_SNOW;
    palette->entries[15] = TER_ICE;
    palette->entries[16] = TER_BASEMAT;
    palette->entries[17] = TER_TUNNEL;
    palette->entries[18] = TER_WALKWAY;
    /* Set the rest to TER_GROUND for future compatability */
    memset(palette->entries+19,TER_GROUND,256-19);
}

/* Load the configuration file from file */
static struct LevelSettings *parse_level_config(struct dllist *config,
        const char *filename)
{
    struct LevelSettings *settings;
    struct dllist *cfgptr;

    settings = malloc (sizeof (struct LevelSettings));

    /* Set default main block values */
    settings->mainblock.artwork = NULL;
    settings->mainblock.thumbnail = NULL;
    settings->mainblock.name = NULL;
    settings->mainblock.thumbnail = NULL;
    settings->mainblock.aspect = 1;
    settings->mainblock.zoom = 1;
    settings->mainblock.music = NULL;

    set_default_palette(&settings->palette);

    settings->override = NULL;
    settings->objects = NULL;
    settings->thumbnail = NULL;

    cfgptr = config;
    while(cfgptr) {
        struct ConfigBlock *block=cfgptr->data;
        if(block->title==NULL || strcmp(block->title,"main")==0)
            parse_main_block(block->values,&settings->mainblock);
        else if(strcmp(block->title,"override")==0) {
            settings->override = malloc(sizeof(struct LSB_Override));
            if(!settings->override) {
                perror("parse_level_config");
                return NULL;
            }
            parse_override_block(block->values,settings->override);
        } else if(strcmp(block->title,"object")==0) {
            struct LSB_Object *newobj = malloc(sizeof(struct LSB_Object));
            if(!newobj) {
                perror("parse_level_config");
                return NULL;
            }
            parse_object_block(block->values,newobj);
            if(settings->objects)
                dllist_append(settings->objects,newobj);
            else
                settings->objects=dllist_append(NULL,newobj);
        } else if(strcmp(block->title,"palette")==0)
            parse_palette_block(block->values,&settings->palette);
        else if(strncmp(block->title,"end",3)==0 || strcmp(block->title,"objects")==0) {
            /* Silently ignore [end*] and [objects] blocks for now */
        } else fprintf(stderr,"%s: Unknown block \"%s\"\n",filename,block->title);
            
        cfgptr = cfgptr->next;
    }

    return settings;
}

/* Load the configuration file from file */
struct LevelSettings *load_level_config (const char *filename) {
    struct LevelSettings *settings;
    struct dllist *config;

    config = read_config_file(filename,0);
    if(!config) {
        fprintf(stderr,"%s: can't parse\n",filename);
        return NULL;
    }

    settings = parse_level_config(config,filename);
    dllist_free(config,free_config_file);

    return settings;
}

/* Load the configuration file from RWops */
struct LevelSettings *load_level_config_rw (SDL_RWops * rw, size_t len,
        const char *filename)
{
    struct LevelSettings *settings;
    struct dllist *config;

    if(rw==NULL) return NULL;

    config = read_config_rw(rw,len,0);
    if(!config) {
        fprintf(stderr,"%s: can't parse\n",filename);
        return NULL;
    }

    settings = parse_level_config(config,filename);
    dllist_free(config,free_config_file);

    return settings;
}

