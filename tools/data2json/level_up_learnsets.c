#define _(string) string

#include "gba/types.h"

#include "constants/moves.h"
#include "constants/species.h"

#include "structs.h"

#define LEVEL_UP_MOVE(lvl, moveLearned) {.move = moveLearned, .level = lvl}
#define LEVEL_UP_END 0xFFFF

#include "data/pokemon/level_up_learnsets.h"
#include "data/pokemon/level_up_learnset_pointers.h"
