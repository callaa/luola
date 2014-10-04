/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
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
    CfgPtrType types[7];
    void *pointers[7];
    char *keys[7];

    keys[0]="collisionmap";types[0]=CFG_STRING; pointers[0]=&mainb->collmap;
    keys[1]="artwork";     types[1]=CFG_STRING; pointers[1]=&mainb->artwork;
    keys[2]="thumbnail";   types[2]=CFG_STRING; pointers[2]=&mainb->thumbnail;
    keys[3]="name";        types[3]=CFG_STRING; pointers[3]=&mainb->name;
    keys[4]="aspect";      types[4]=CFG_FLOAT;  pointers[4]=&mainb->aspect;
    keys[5]="zoom";        types[5]=CFG_FLOAT;  pointers[5]=&mainb->zoom;
    keys[6]="music";       types[6]=CFG_MULTISTRING; pointers[6]=&mainb->music;

    translate_config(values,sizeof(types)/sizeof(CfgPtrType),keys,types,pointers,0);
}

static struct LSB_Override *parse_override_block(struct dllist *values) {
    struct LSB_Override *override;
    CfgPtrType types[12];
    void *pointers[12];
    char *keys[12];

    override = malloc(sizeof(struct LSB_Override));
    if(!override) {
        perror("parse_override_block");
        return NULL;
    }
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
    override->soldiers = -1;
    override->helicopters = -1;

    keys[0]="critters"; types[0]=CFG_INT; pointers[0]=&override->critters;
    keys[1]="bases_indestructable"; types[1]=CFG_INT; pointers[1]=&override->indstr_base;
    keys[2]="snowfall"; types[2]=CFG_INT; pointers[2]=&override->snowfall;
    keys[3]="stars";    types[3]=CFG_INT; pointers[3]=&override->stars;
    keys[4]="turrets";  types[4]=CFG_INT; pointers[4]=&override->turrets;
    keys[5]="jumpgates";types[5]=CFG_INT; pointers[5]=&override->jumpgates;
    keys[6]="cows";     types[6]=CFG_INT; pointers[6]=&override->cows;
    keys[7]="fish";     types[7]=CFG_INT; pointers[7]=&override->fish;
    keys[8]="birds";    types[8]=CFG_INT; pointers[8]=&override->birds;
    keys[9]="bats";     types[9]=CFG_INT; pointers[9]=&override->bats;
    keys[10]="soldiers";types[10]=CFG_INT;pointers[10]=&override->soldiers;
    keys[11]="helicopters";types[11]=CFG_INT;pointers[11]=&override->helicopters;

    translate_config(values,sizeof(types)/sizeof(CfgPtrType),keys,types,pointers,0);

    return override;
}

static struct LSB_Object *parse_object_block(struct dllist *values) {
    struct LSB_Object *object;
    char *typestr=NULL;
    CfgPtrType types[7];
    void *pointers[7];
    char *keys[7];

    object = malloc(sizeof(struct LSB_Object));
    if(!object) {
        perror("parse_object_block");
        return NULL;
    }
    object->type = 0;
    object->x = 0; object->y = 0;
    object->ceiling_attach = 0;
    object->value = 0;
    object->id = 0; object->link = 0;

    keys[0]="type"; types[0]=CFG_STRING; pointers[0]=&typestr;
    keys[1]="x";    types[1]=CFG_INT;    pointers[1]=&object->x;
    keys[2]="y";    types[2]=CFG_INT;    pointers[2]=&object->y;
    keys[3]="ceiling_attach";types[3]=CFG_INT; pointers[3]=&object->ceiling_attach;
    keys[4]="value";types[4]=CFG_INT;    pointers[4]=&object->value;
    keys[5]="link"; types[5]=CFG_INT;    pointers[5]=&object->link;
    keys[6]="id";   types[6]=CFG_INT;    pointers[6]=&object->id;

    translate_config(values,sizeof(types)/sizeof(CfgPtrType),keys,types,pointers,0);

    if(typestr==NULL) {
        fprintf(stderr,"Warning: an object without a type!\n");
    } else if(strcmp(typestr,"turret")==0)
        object->type = OBJ_TURRET;
    else if(strcmp(typestr,"jumpgate")==0)
        object->type = OBJ_JUMPGATE;
    else if(strcmp(typestr,"cow")==0)
        object->type = OBJ_COW;
    else if(strcmp(typestr,"fish")==0)
        object->type = OBJ_FISH;
    else if(strcmp(typestr,"bird")==0)
        object->type = OBJ_BIRD;
    else if(strcmp(typestr,"bat")==0)
        object->type = OBJ_BAT;
    else if(strcmp(typestr,"ship")==0)
        object->type = OBJ_SHIP;
    else fprintf(stderr,"Warning: unknown object type %s\n",typestr);

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
    else printf("Error: unknown terrain name \"%s\" (set to TER_FREE)\n",name);
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

/* Load the configuration file from file */
static struct LevelSettings *parse_level_config(struct dllist *config,
        const char *filename)
{
    struct LevelSettings *settings;
    struct dllist *cfgptr;
    int r;

    settings = malloc (sizeof (struct LevelSettings));

    /* Set default main block values */
    settings->mainblock.artwork = NULL;
    settings->mainblock.thumbnail = NULL;
    settings->mainblock.name = NULL;
    settings->mainblock.thumbnail = NULL;
    settings->mainblock.aspect = 1;
    settings->mainblock.zoom = 1;
    settings->mainblock.music = NULL;

    /* Generate default palette */
    for(r=0;r<=LAST_TER;r++)
        settings->palette.entries[r] = r;
    for(r=LAST_TER+1;r<256;r++)
        settings->palette.entries[r] = TER_FREE;

    settings->override = NULL;
    settings->objects = NULL;
    settings->thumbnail = NULL;

    cfgptr = config;
    while(cfgptr) {
        struct ConfigBlock *block=cfgptr->data;
        if(block->title==NULL || strcmp(block->title,"main")==0)
            parse_main_block(block->values,&settings->mainblock);
        else if(strcmp(block->title,"override")==0)
            settings->override = parse_override_block(block->values);
        else if(strcmp(block->title,"object")==0) {
            if(settings->objects)
                dllist_append(settings->objects,parse_object_block(block->values));
            else
                settings->objects=dllist_append(NULL,parse_object_block(block->values));
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

