#ifndef LCONF2BIN_H
#define LCONF2BIN_H

#include <SDL_rwops.h>
#include "lconf.h"

SDL_RWops *LevelSetting2bin(LevelSettings *settings,int *size);

#endif

