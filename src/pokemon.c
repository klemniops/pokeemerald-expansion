#include "global.h"
#include "malloc.h"
#include "apprentice.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "data.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_specials.h"
#include "field_weather.h"
#include "item.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "m4a.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokeblock.h"
#include "pokemon.h"
#include "pokemon_animation.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "recorded_battle.h"
#include "rtc.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trainer_hill.h"
#include "util.h"
#include "constants/abilities.h"
#include "constants/battle_frontier.h"
#include "constants/battle_move_effects.h"
#include "constants/hold_effects.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/weather.h"
#include "constants/battle_config.h"

struct SpeciesItem
{
    u16 species;
    u16 item;
};

// this file's functions
static u16 CalculateBoxMonChecksum(struct BoxPokemon *boxMon);
static union PokemonSubstruct *GetSubstruct(struct BoxPokemon *boxMon, u32 personality, u8 substructType);
static void EncryptBoxMon(struct BoxPokemon *boxMon);
static void DecryptBoxMon(struct BoxPokemon *boxMon);
static void Task_PlayMapChosenOrBattleBGM(u8 taskId);
static u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move);
static bool8 ShouldSkipFriendshipChange(void);

// EWRAM vars
EWRAM_DATA static u8 sLearningMoveTableID = 0;
EWRAM_DATA u8 gPlayerPartyCount = 0;
EWRAM_DATA u8 gEnemyPartyCount = 0;
EWRAM_DATA struct Pokemon gPlayerParty[PARTY_SIZE] = {0};
EWRAM_DATA struct Pokemon gEnemyParty[PARTY_SIZE] = {0};
EWRAM_DATA struct SpriteTemplate gMultiuseSpriteTemplate = {0};
EWRAM_DATA struct Unknown_806F160_Struct *gUnknown_020249B4[2] = {NULL};

// const rom data
#include "data/battle_moves.h"

// Used in an unreferenced function in RS.
// Unreferenced here and in FRLG.
struct CombinedMove
{
    u16 move1;
    u16 move2;
    u16 newMove;
};

static const struct CombinedMove sCombinedMoves[2] =
{
    {MOVE_EMBER, MOVE_GUST, MOVE_HEAT_WAVE},
    {0xFFFF, 0xFFFF, 0xFFFF}
};

#include "data/pokemon/pokedex_numbers.h"

const struct SpindaSpot gSpindaSpotGraphics[] =
{
    {16, 7, INCBIN_U16("graphics/spinda_spots/spot_0.bin")},
    {40, 8, INCBIN_U16("graphics/spinda_spots/spot_1.bin")},
    {22, 25, INCBIN_U16("graphics/spinda_spots/spot_2.bin")},
    {34, 26, INCBIN_U16("graphics/spinda_spots/spot_3.bin")}
};

#include "data/pokemon/item_effects.h"

const s8 gNatureStatTable[NUM_NATURES][NUM_NATURE_STATS] =
{
                       // Atk Def Spd Sp.Atk Sp.Def
    [NATURE_HARDY]   = {    0,  0,  0,     0,     0},
    [NATURE_LONELY]  = {   +1, -1,  0,     0,     0},
    [NATURE_BRAVE]   = {   +1,  0, -1,     0,     0},
    [NATURE_ADAMANT] = {   +1,  0,  0,    -1,     0},
    [NATURE_NAUGHTY] = {   +1,  0,  0,     0,    -1},
    [NATURE_BOLD]    = {   -1, +1,  0,     0,     0},
    [NATURE_DOCILE]  = {    0,  0,  0,     0,     0},
    [NATURE_RELAXED] = {    0, +1, -1,     0,     0},
    [NATURE_IMPISH]  = {    0, +1,  0,    -1,     0},
    [NATURE_LAX]     = {    0, +1,  0,     0,    -1},
    [NATURE_TIMID]   = {   -1,  0, +1,     0,     0},
    [NATURE_HASTY]   = {    0, -1, +1,     0,     0},
    [NATURE_SERIOUS] = {    0,  0,  0,     0,     0},
    [NATURE_JOLLY]   = {    0,  0, +1,    -1,     0},
    [NATURE_NAIVE]   = {    0,  0, +1,     0,    -1},
    [NATURE_MODEST]  = {   -1,  0,  0,    +1,     0},
    [NATURE_MILD]    = {    0, -1,  0,    +1,     0},
    [NATURE_QUIET]   = {    0,  0, -1,    +1,     0},
    [NATURE_BASHFUL] = {    0,  0,  0,     0,     0},
    [NATURE_RASH]    = {    0,  0,  0,    +1,    -1},
    [NATURE_CALM]    = {   -1,  0,  0,     0,    +1},
    [NATURE_GENTLE]  = {    0, -1,  0,     0,    +1},
    [NATURE_SASSY]   = {    0,  0, -1,     0,    +1},
    [NATURE_CAREFUL] = {    0,  0,  0,    -1,    +1},
    [NATURE_QUIRKY]  = {    0,  0,  0,     0,     0},
};

#include "data/pokemon/tmhm_learnsets.h"
#include "data/pokemon/trainer_class_lookups.h"
#include "data/pokemon/experience_tables.h"
#include "data/pokemon/base_stats.h"
#include "data/pokemon/level_up_learnsets.h"
#include "data/pokemon/evolution.h"
#include "data/pokemon/level_up_learnset_pointers.h"
#include "data/pokemon_graphics/front_animation.h"

#include "data/pokemon/form_species_tables.h"
#include "data/pokemon/form_species_table_pointers.h"


const u8 gPPUpGetMask[] = {0x03, 0x0c, 0x30, 0xc0}; // Masks for getting PP Up count, also PP Max values
const u8 gPPUpSetMask[] = {0xfc, 0xf3, 0xcf, 0x3f}; // Masks for setting PP Up count
const u8 gPPUpAddMask[] = {0x01, 0x04, 0x10, 0x40}; // Values added to PP Up count

const u8 gStatStageRatios[MAX_STAT_STAGE + 1][2] =
{
    {10, 40}, // -6, MIN_STAT_STAGE
    {10, 35}, // -5
    {10, 30}, // -4
    {10, 25}, // -3
    {10, 20}, // -2
    {10, 15}, // -1
    {10, 10}, //  0, DEFAULT_STAT_STAGE
    {15, 10}, // +1
    {20, 10}, // +2
    {25, 10}, // +3
    {30, 10}, // +4
    {35, 10}, // +5
    {40, 10}, // +6, MAX_STAT_STAGE
};

const u16 gLinkPlayerFacilityClasses[NUM_MALE_LINK_FACILITY_CLASSES + NUM_FEMALE_LINK_FACILITY_CLASSES] =
{
    // Male classes
    FACILITY_CLASS_COOLTRAINER_M,
    FACILITY_CLASS_BLACK_BELT,
    FACILITY_CLASS_CAMPER,
    FACILITY_CLASS_YOUNGSTER,
    FACILITY_CLASS_PSYCHIC_M,
    FACILITY_CLASS_BUG_CATCHER,
    FACILITY_CLASS_PKMN_BREEDER_M,
    FACILITY_CLASS_GUITARIST,
    // Female Classes
    FACILITY_CLASS_COOLTRAINER_F,
    FACILITY_CLASS_HEX_MANIAC,
    FACILITY_CLASS_PICNICKER,
    FACILITY_CLASS_LASS,
    FACILITY_CLASS_PSYCHIC_F,
    FACILITY_CLASS_BATTLE_GIRL,
    FACILITY_CLASS_PKMN_BREEDER_F,
    FACILITY_CLASS_BEAUTY
};

const struct SpriteTemplate gBattlerSpriteTemplates[MAX_BATTLERS_COUNT] =
{
    [B_POSITION_PLAYER_LEFT] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gBattlerPicTable_PlayerLeft,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [B_POSITION_OPPONENT_LEFT] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpriteOpponentSide,
        .anims = NULL,
        .images = gBattlerPicTable_OpponentLeft,
        .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
        .callback = SpriteCb_WildMon,
    },
    [B_POSITION_PLAYER_RIGHT] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gBattlerPicTable_PlayerRight,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [B_POSITION_OPPONENT_RIGHT] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpriteOpponentSide,
        .anims = NULL,
        .images = gBattlerPicTable_OpponentRight,
        .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
        .callback = SpriteCb_WildMon
    },
};

static const struct SpriteTemplate sTrainerBackSpriteTemplates[] =
{
    [TRAINER_BACK_PIC_BRENDAN] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_Brendan,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_MAY] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_May,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_RED] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_Red,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_LEAF] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_Leaf,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_RubySapphireBrendan,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_RUBY_SAPPHIRE_MAY] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_RubySapphireMay,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_WALLY] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_Wally,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [TRAINER_BACK_PIC_STEVEN] = {
        .tileTag = 0xFFFF,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gTrainerBackPicTable_Steven,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
};

static const u8 sSecretBaseFacilityClasses[2][5] =
{
    {FACILITY_CLASS_YOUNGSTER, FACILITY_CLASS_BUG_CATCHER, FACILITY_CLASS_RICH_BOY, FACILITY_CLASS_CAMPER, FACILITY_CLASS_COOLTRAINER_M},
    {FACILITY_CLASS_LASS, FACILITY_CLASS_SCHOOL_KID_F, FACILITY_CLASS_LADY, FACILITY_CLASS_PICNICKER, FACILITY_CLASS_COOLTRAINER_F}
};

static const u8 sGetMonDataEVConstants[] =
{
    MON_DATA_HP_EV,
    MON_DATA_ATK_EV,
    MON_DATA_DEF_EV,
    MON_DATA_SPEED_EV,
    MON_DATA_SPDEF_EV,
    MON_DATA_SPATK_EV
};

// For stat-raising items
static const u8 sStatsToRaise[] =
{
#ifndef ITEM_EXPANSION
    STAT_ATK, STAT_ATK, STAT_SPEED, STAT_DEF, STAT_SPATK, STAT_ACC
#else
    STAT_ATK, STAT_ATK, STAT_DEF, STAT_SPEED, STAT_SPATK, STAT_SPDEF, STAT_ACC
#endif
};

// 3 modifiers each for how much to change friendship for different ranges
// 0-99, 100-199, 200+
static const s8 sFriendshipEventModifiers[][3] =
{
    [FRIENDSHIP_EVENT_GROW_LEVEL]      = { 5,  3,  2},
    [FRIENDSHIP_EVENT_VITAMIN]         = { 5,  3,  2},
    [FRIENDSHIP_EVENT_BATTLE_ITEM]     = { 1,  1,  0},
    [FRIENDSHIP_EVENT_LEAGUE_BATTLE]   = { 3,  2,  1},
    [FRIENDSHIP_EVENT_LEARN_TMHM]      = { 1,  1,  0},
    [FRIENDSHIP_EVENT_WALKING]         = { 1,  1,  1},
    [FRIENDSHIP_EVENT_FAINT_SMALL]     = {-1, -1, -1},
    [FRIENDSHIP_EVENT_FAINT_FIELD_PSN] = {-5, -5, -10},
    [FRIENDSHIP_EVENT_FAINT_LARGE]     = {-5, -5, -10},
};

static const u16 sHMMoves[] =
{
    MOVE_CUT, MOVE_FLY, MOVE_SURF, MOVE_STRENGTH, MOVE_FLASH,
    MOVE_ROCK_SMASH, MOVE_WATERFALL, MOVE_DIVE, 0xFFFF
};

static const struct SpeciesItem sAlteringCaveWildMonHeldItems[] =
{
    {SPECIES_NONE,      ITEM_NONE},
    {SPECIES_MAREEP,    ITEM_GANLON_BERRY},
    {SPECIES_PINECO,    ITEM_APICOT_BERRY},
    {SPECIES_HOUNDOUR,  ITEM_BIG_MUSHROOM},
    {SPECIES_TEDDIURSA, ITEM_PETAYA_BERRY},
    {SPECIES_AIPOM,     ITEM_BERRY_JUICE},
    {SPECIES_SHUCKLE,   ITEM_BERRY_JUICE},
    {SPECIES_STANTLER,  ITEM_PETAYA_BERRY},
    {SPECIES_SMEARGLE,  ITEM_SALAC_BERRY},
};

static const struct OamData sOamData_8329F20 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct SpriteTemplate gUnknown_08329F28 =
{
    .tileTag = 0xFFFF,
    .paletteTag = 0xFFFF,
    .oam = &sOamData_8329F20,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// code
void ZeroBoxMonData(struct BoxPokemon *boxMon)
{
    u8 *raw = (u8 *)boxMon;
    u32 i;
    for (i = 0; i < sizeof(struct BoxPokemon); i++)
        raw[i] = 0;
}

void ZeroMonData(struct Pokemon *mon)
{
    u32 arg;
    ZeroBoxMonData(&mon->box);
    arg = 0;
    SetMonData(mon, MON_DATA_STATUS, &arg);
    SetMonData(mon, MON_DATA_LEVEL, &arg);
    SetMonData(mon, MON_DATA_HP, &arg);
    SetMonData(mon, MON_DATA_MAX_HP, &arg);
    SetMonData(mon, MON_DATA_ATK, &arg);
    SetMonData(mon, MON_DATA_DEF, &arg);
    SetMonData(mon, MON_DATA_SPEED, &arg);
    SetMonData(mon, MON_DATA_SPATK, &arg);
    SetMonData(mon, MON_DATA_SPDEF, &arg);
    arg = 255;
    SetMonData(mon, MON_DATA_MAIL, &arg);
}

void ZeroPlayerPartyMons(void)
{
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gPlayerParty[i]);
}

void ZeroEnemyPartyMons(void)
{
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gEnemyParty[i]);
}

void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    u32 arg;
    ZeroMonData(mon);
    CreateBoxMon(&mon->box, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_LEVEL, &level);
    arg = 255;
    SetMonData(mon, MON_DATA_MAIL, &arg);
    CalculateMonStats(mon);
}

void CreateBoxMon(struct BoxPokemon *boxMon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    u8 speciesName[POKEMON_NAME_LENGTH + 1];
    u32 personality;
    u32 value;
    u16 checksum;

    ZeroBoxMonData(boxMon);

    if (hasFixedPersonality)
        personality = fixedPersonality;
    else
        personality = Random32();

    //Determine original trainer ID
    if (otIdType == OT_ID_RANDOM_NO_SHINY) //Pokemon cannot be shiny
    {
        u32 shinyValue;
        do
        {
            value = Random32();
            shinyValue = HIHALF(value) ^ LOHALF(value) ^ HIHALF(personality) ^ LOHALF(personality);
        } while (shinyValue < SHINY_ODDS);
    }
    else if (otIdType == OT_ID_PRESET) //Pokemon has a preset OT ID
    {
        value = fixedOtId;
    }
    else //Player is the OT
    {
        value = gSaveBlock2Ptr->playerTrainerId[0]
              | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
              | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
              | (gSaveBlock2Ptr->playerTrainerId[3] << 24);
        
        if (CheckBagHasItem(ITEM_SHINY_CHARM, 1))
        {
            u32 shinyValue;
            u32 rolls = 0;
            do
            {
                personality = Random32();
                shinyValue = HIHALF(value) ^ LOHALF(value) ^ HIHALF(personality) ^ LOHALF(personality);
                rolls++;
            } while (shinyValue >= SHINY_ODDS && rolls < I_SHINY_CHARM_REROLLS);
        }
    }

    SetBoxMonData(boxMon, MON_DATA_PERSONALITY, &personality);
    SetBoxMonData(boxMon, MON_DATA_OT_ID, &value);

    checksum = CalculateBoxMonChecksum(boxMon);
    SetBoxMonData(boxMon, MON_DATA_CHECKSUM, &checksum);
    EncryptBoxMon(boxMon);
    GetSpeciesName(speciesName, species);
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, speciesName);
    SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &gGameLanguage);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetBoxMonData(boxMon, MON_DATA_SPECIES, &species);
    SetBoxMonData(boxMon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &gBaseStats[species].friendship);
    value = GetCurrentRegionMapSectionId();
    SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &value);
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &level);
    SetBoxMonData(boxMon, MON_DATA_MET_GAME, &gGameVersion);
    value = ITEM_POKE_BALL;
    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &value);
    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);

    if (fixedIV < USE_RANDOM_IVS)
    {
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &fixedIV);
    }
    else
    {
        u32 iv;
        value = Random();

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);

        value = Random();

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);
    }

    if (gBaseStats[species].abilities[1])
    {
        value = personality & 1;
        SetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, &value);
    }

    GiveBoxMonInitialMoveset(boxMon);
}

void CreateMonWithNature(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 nature)
{
    u32 personality;

    do
    {
        personality = Random32();
    }
    while (nature != GetNatureFromPersonality(personality));

    CreateMon(mon, species, level, fixedIV, 1, personality, OT_ID_PLAYER_ID, 0);
}

void CreateMonWithGenderNatureLetter(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 gender, u8 nature, u8 unownLetter)
{
    u32 personality;

    if ((u8)(unownLetter - 1) < NUM_UNOWN_FORMS)
    {
        u16 actualLetter;

        do
        {
            personality = Random32();
            actualLetter = GET_UNOWN_LETTER(personality);
        }
        while (nature != GetNatureFromPersonality(personality)
            || gender != GetGenderFromSpeciesAndPersonality(species, personality)
            || actualLetter != unownLetter - 1);
    }
    else
    {
        do
        {
            personality = Random32();
        }
        while (nature != GetNatureFromPersonality(personality)
            || gender != GetGenderFromSpeciesAndPersonality(species, personality));
    }

    CreateMon(mon, species, level, fixedIV, 1, personality, OT_ID_PLAYER_ID, 0);
}

// This is only used to create Wally's Ralts.
void CreateMaleMon(struct Pokemon *mon, u16 species, u8 level)
{
    u32 personality;
    u32 otId;

    do
    {
        otId = Random32();
        personality = Random32();
    }
    while (GetGenderFromSpeciesAndPersonality(species, personality) != MON_MALE);
    CreateMon(mon, species, level, USE_RANDOM_IVS, 1, personality, OT_ID_PRESET, otId);
}

void CreateMonWithIVsPersonality(struct Pokemon *mon, u16 species, u8 level, u32 ivs, u32 personality)
{
    CreateMon(mon, species, level, 0, 1, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(mon, MON_DATA_IVS, &ivs);
    CalculateMonStats(mon);
}

void CreateMonWithIVsOTID(struct Pokemon *mon, u16 species, u8 level, u8 *ivs, u32 otId)
{
    CreateMon(mon, species, level, 0, 0, 0, OT_ID_PRESET, otId);
    SetMonData(mon, MON_DATA_HP_IV, &ivs[0]);
    SetMonData(mon, MON_DATA_ATK_IV, &ivs[1]);
    SetMonData(mon, MON_DATA_DEF_IV, &ivs[2]);
    SetMonData(mon, MON_DATA_SPEED_IV, &ivs[3]);
    SetMonData(mon, MON_DATA_SPATK_IV, &ivs[4]);
    SetMonData(mon, MON_DATA_SPDEF_IV, &ivs[5]);
    CalculateMonStats(mon);
}

void CreateMonWithEVSpread(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 evSpread)
{
    s32 i;
    s32 statCount = 0;
    u16 evAmount;
    u8 evsBits;

    CreateMon(mon, species, level, fixedIV, 0, 0, OT_ID_PLAYER_ID, 0);

    evsBits = evSpread;

    for (i = 0; i < NUM_STATS; i++)
    {
        if (evsBits & 1)
            statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;

    evsBits = 1;

    for (i = 0; i < NUM_STATS; i++)
    {
        if (evSpread & evsBits)
            SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }

    CalculateMonStats(mon);
}

void CreateBattleTowerMon(struct Pokemon *mon, struct BattleTowerPokemon *src)
{
    s32 i;
    u8 nickname[30];
    u8 language;
    u8 value;

    CreateMon(mon, src->species, src->level, 0, 1, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN)
    {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    }
    else
    {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateBattleTowerMon2(struct Pokemon *mon, struct BattleTowerPokemon *src, bool8 lvl50)
{
    s32 i;
    u8 nickname[30];
    u8 level;
    u8 language;
    u8 value;

    if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_50)
        level = GetFrontierEnemyMonLevel(gSaveBlock2Ptr->frontier.lvlMode);
    else if (lvl50)
        level = 50;
    else
        level = src->level;

    CreateMon(mon, src->species, level, 0, 1, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN)
    {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    }
    else
    {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateApprenticeMon(struct Pokemon *mon, const struct Apprentice *src, u8 monId)
{
    s32 i;
    u16 evAmount;
    u8 language;
    u32 otId = gApprentices[src->id].otId;
    u32 personality = ((gApprentices[src->id].otId >> 8) | ((gApprentices[src->id].otId & 0xFF) << 8))
                    + src->party[monId].species + src->number;

    CreateMon(mon,
              src->party[monId].species,
              GetFrontierEnemyMonLevel(src->lvlMode - 1),
              MAX_PER_STAT_IVS,
              TRUE,
              personality,
              OT_ID_PRESET,
              otId);

    SetMonData(mon, MON_DATA_HELD_ITEM, &src->party[monId].item);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->party[monId].moves[i], i);

    evAmount = MAX_TOTAL_EVS / NUM_STATS;
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);

    language = src->language;
    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_OT_NAME, GetApprenticeNameInLanguage(src->id, language));
    CalculateMonStats(mon);
}

void CreateMonWithEVSpreadNatureOTID(struct Pokemon *mon, u16 species, u8 level, u8 nature, u8 fixedIV, u8 evSpread, u32 otId)
{
    s32 i;
    s32 statCount = 0;
    u8 evsBits;
    u16 evAmount;

    // i is reused as personality value
    do
    {
        i = Random32();
    } while (nature != GetNatureFromPersonality(i));

    CreateMon(mon, species, level, fixedIV, TRUE, i, OT_ID_PRESET, otId);
    evsBits = evSpread;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (evsBits & 1)
            statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;
    evsBits = 1;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (evSpread & evsBits)
            SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }

    CalculateMonStats(mon);
}

void ConvertPokemonToBattleTowerPokemon(struct Pokemon *mon, struct BattleTowerPokemon *dest)
{
    s32 i;
    u16 heldItem;

    dest->species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);

    if (heldItem == ITEM_ENIGMA_BERRY)
        heldItem = ITEM_NONE;

    dest->heldItem = heldItem;

    for (i = 0; i < MAX_MON_MOVES; i++)
        dest->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);

    dest->level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    dest->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    dest->otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    dest->hpEV = GetMonData(mon, MON_DATA_HP_EV, NULL);
    dest->attackEV = GetMonData(mon, MON_DATA_ATK_EV, NULL);
    dest->defenseEV = GetMonData(mon, MON_DATA_DEF_EV, NULL);
    dest->speedEV = GetMonData(mon, MON_DATA_SPEED_EV, NULL);
    dest->spAttackEV = GetMonData(mon, MON_DATA_SPATK_EV, NULL);
    dest->spDefenseEV = GetMonData(mon, MON_DATA_SPDEF_EV, NULL);
    dest->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
    dest->hpIV = GetMonData(mon, MON_DATA_HP_IV, NULL);
    dest->attackIV = GetMonData(mon, MON_DATA_ATK_IV, NULL);
    dest->defenseIV = GetMonData(mon, MON_DATA_DEF_IV, NULL);
    dest->speedIV  = GetMonData(mon, MON_DATA_SPEED_IV, NULL);
    dest->spAttackIV  = GetMonData(mon, MON_DATA_SPATK_IV, NULL);
    dest->spDefenseIV  = GetMonData(mon, MON_DATA_SPDEF_IV, NULL);
    dest->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    dest->personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    GetMonData(mon, MON_DATA_NICKNAME, dest->nickname);
}

void CreateEventLegalMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    bool32 isEventLegal = TRUE;

    CreateMon(mon, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_EVENT_LEGAL, &isEventLegal);
}

// If FALSE, should load this game's Deoxys form. If TRUE, should load normal Deoxys form
bool8 ShouldIgnoreDeoxysForm(u8 caseId, u8 battlerId)
{
    switch (caseId)
    {
    case 0:
    default:
        return FALSE;
    case 1: // Player's side in battle
        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            return FALSE;
        if (!gMain.inBattle)
            return FALSE;
        if (gLinkPlayers[GetMultiplayerId()].id == battlerId)
            return FALSE;
        break;
    case 2:
        break;
    case 3: // Summary Screen
        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            return FALSE;
        if (!gMain.inBattle)
            return FALSE;
        if (battlerId == 1 || battlerId == 4 || battlerId == 5)
            return TRUE;
        return FALSE;
    case 4:
        break;
    case 5: // In move animation, e.g. in Role Play or Snatch
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            if (!gMain.inBattle)
                return FALSE;
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
            {
                if (gLinkPlayers[GetMultiplayerId()].id == battlerId)
                    return FALSE;
            }
            else
            {
                if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
                    return FALSE;
            }
        }
        else
        {
            if (!gMain.inBattle)
                return FALSE;
            if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
                return FALSE;
        }
        break;
    }

    return TRUE;
}

u16 GetUnionRoomTrainerPic(void)
{
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId & 7;
    arrId |= gLinkPlayers[linkId].gender << 3;
    return FacilityClassToPicIndex(gLinkPlayerFacilityClasses[arrId]);
}

u16 GetUnionRoomTrainerClass(void)
{
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId & 7;
    arrId |= gLinkPlayers[linkId].gender << 3;
    return gFacilityClassToTrainerClass[gLinkPlayerFacilityClasses[arrId]];
}

void CreateEventLegalEnemyMon(void)
{
    s32 species = gSpecialVar_0x8004;
    s32 level = gSpecialVar_0x8005;
    s32 itemId = gSpecialVar_0x8006;

    ZeroEnemyPartyMons();
    CreateEventLegalMon(&gEnemyParty[0], species, level, USE_RANDOM_IVS, 0, 0, 0, 0);
    if (itemId)
    {
        u8 heldItem[2];
        heldItem[0] = itemId;
        heldItem[1] = itemId >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem);
    }
}

static u16 CalculateBoxMonChecksum(struct BoxPokemon *boxMon)
{
    u16 checksum = 0;
    union PokemonSubstruct *substruct0 = GetSubstruct(boxMon, boxMon->personality, 0);
    union PokemonSubstruct *substruct1 = GetSubstruct(boxMon, boxMon->personality, 1);
    union PokemonSubstruct *substruct2 = GetSubstruct(boxMon, boxMon->personality, 2);
    union PokemonSubstruct *substruct3 = GetSubstruct(boxMon, boxMon->personality, 3);
    s32 i;

    for (i = 0; i < 6; i++)
        checksum += substruct0->raw[i];

    for (i = 0; i < 6; i++)
        checksum += substruct1->raw[i];

    for (i = 0; i < 6; i++)
        checksum += substruct2->raw[i];

    for (i = 0; i < 6; i++)
        checksum += substruct3->raw[i];

    return checksum;
}

#define CALC_STAT(base, iv, ev, statIndex, field)               \
{                                                               \
    u8 baseStat = gBaseStats[species].base;                     \
    s32 n = (((2 * baseStat + iv + ev / 4) * level) / 100) + 5; \
    u8 nature = GetNature(mon);                                 \
    n = ModifyStatByNature(nature, n, statIndex);               \
    SetMonData(mon, field, &n);                                 \
}

void CalculateMonStats(struct Pokemon *mon)
{
    s32 oldMaxHP = GetMonData(mon, MON_DATA_MAX_HP, NULL);
    s32 currentHP = GetMonData(mon, MON_DATA_HP, NULL);
    s32 hpIV = GetMonData(mon, MON_DATA_HP_IV, NULL);
    s32 hpEV = GetMonData(mon, MON_DATA_HP_EV, NULL);
    s32 attackIV = GetMonData(mon, MON_DATA_ATK_IV, NULL);
    s32 attackEV = GetMonData(mon, MON_DATA_ATK_EV, NULL);
    s32 defenseIV = GetMonData(mon, MON_DATA_DEF_IV, NULL);
    s32 defenseEV = GetMonData(mon, MON_DATA_DEF_EV, NULL);
    s32 speedIV = GetMonData(mon, MON_DATA_SPEED_IV, NULL);
    s32 speedEV = GetMonData(mon, MON_DATA_SPEED_EV, NULL);
    s32 spAttackIV = GetMonData(mon, MON_DATA_SPATK_IV, NULL);
    s32 spAttackEV = GetMonData(mon, MON_DATA_SPATK_EV, NULL);
    s32 spDefenseIV = GetMonData(mon, MON_DATA_SPDEF_IV, NULL);
    s32 spDefenseEV = GetMonData(mon, MON_DATA_SPDEF_EV, NULL);
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    s32 level = GetLevelFromMonExp(mon);
    s32 newMaxHP;

    SetMonData(mon, MON_DATA_LEVEL, &level);

    if (species == SPECIES_SHEDINJA)
    {
        newMaxHP = 1;
    }
    else
    {
        s32 n = 2 * gBaseStats[species].baseHP + hpIV;
        newMaxHP = (((n + hpEV / 4) * level) / 100) + level + 10;
    }

    gBattleScripting.levelUpHP = newMaxHP - oldMaxHP;
    if (gBattleScripting.levelUpHP == 0)
        gBattleScripting.levelUpHP = 1;

    SetMonData(mon, MON_DATA_MAX_HP, &newMaxHP);

    CALC_STAT(baseAttack, attackIV, attackEV, STAT_ATK, MON_DATA_ATK)
    CALC_STAT(baseDefense, defenseIV, defenseEV, STAT_DEF, MON_DATA_DEF)
    CALC_STAT(baseSpeed, speedIV, speedEV, STAT_SPEED, MON_DATA_SPEED)
    CALC_STAT(baseSpAttack, spAttackIV, spAttackEV, STAT_SPATK, MON_DATA_SPATK)
    CALC_STAT(baseSpDefense, spDefenseIV, spDefenseEV, STAT_SPDEF, MON_DATA_SPDEF)

    if (species == SPECIES_SHEDINJA)
    {
        if (currentHP != 0 || oldMaxHP == 0)
            currentHP = 1;
        else
            return;
    }
    else
    {
        if (currentHP == 0 && oldMaxHP == 0)
            currentHP = newMaxHP;
        else if (currentHP != 0) {
            // BUG: currentHP is unintentionally able to become <= 0 after the instruction below. This causes the pomeg berry glitch.
            currentHP += newMaxHP - oldMaxHP;
            #ifdef BUGFIX
            if (currentHP <= 0)
                currentHP = 1;
            #endif
        }
        else
            return;
    }

    SetMonData(mon, MON_DATA_HP, &currentHP);
}

void BoxMonToMon(const struct BoxPokemon *src, struct Pokemon *dest)
{
    u32 value = 0;
    dest->box = *src;
    SetMonData(dest, MON_DATA_STATUS, &value);
    SetMonData(dest, MON_DATA_HP, &value);
    SetMonData(dest, MON_DATA_MAX_HP, &value);
    value = 255;
    SetMonData(dest, MON_DATA_MAIL, &value);
    CalculateMonStats(dest);
}

u8 GetLevelFromMonExp(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 exp = GetMonData(mon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gBaseStats[species].growthRate][level] <= exp)
        level++;

    return level - 1;
}

u8 GetLevelFromBoxMonExp(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 exp = GetBoxMonData(boxMon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gBaseStats[species].growthRate][level] <= exp)
        level++;

    return level - 1;
}

u16 GiveMoveToMon(struct Pokemon *mon, u16 move)
{
    return GiveMoveToBoxMon(&mon->box, move);
}

static u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move)
{
    s32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 existingMove = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, NULL);
        if (existingMove == MOVE_NONE)
        {
            SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
            SetBoxMonData(boxMon, MON_DATA_PP1 + i, &gBattleMoves[move].pp);
            return move;
        }
        if (existingMove == move)
            return MON_ALREADY_KNOWS_MOVE;
    }
    return MON_HAS_MAX_MOVES;
}

u16 GiveMoveToBattleMon(struct BattlePokemon *mon, u16 move)
{
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (!mon->moves[i])
        {
            mon->moves[i] = move;
            mon->pp[i] = gBattleMoves[move].pp;
            return move;
        }
    }

    return 0xFFFF;
}

void SetMonMoveSlot(struct Pokemon *mon, u16 move, u8 slot)
{
    SetMonData(mon, MON_DATA_MOVE1 + slot, &move);
    SetMonData(mon, MON_DATA_PP1 + slot, &gBattleMoves[move].pp);
}

void SetBattleMonMoveSlot(struct BattlePokemon *mon, u16 move, u8 slot)
{
    mon->moves[slot] = move;
    mon->pp[slot] = gBattleMoves[move].pp;
}

void GiveMonInitialMoveset(struct Pokemon *mon)
{
    GiveBoxMonInitialMoveset(&mon->box);
}

void GiveBoxMonInitialMoveset(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    s32 level = GetLevelFromBoxMonExp(boxMon);
    s32 i;

    for (i = 0; gLevelUpLearnsets[species][i].move != LEVEL_UP_END; i++)
    {
        if (gLevelUpLearnsets[species][i].level > level)
            break;
        if (GiveMoveToBoxMon(boxMon, gLevelUpLearnsets[species][i].move) == MON_HAS_MAX_MOVES)
            DeleteFirstMoveAndGiveMoveToBoxMon(boxMon, gLevelUpLearnsets[species][i].move);
    }
}

u16 MonTryLearningNewMove(struct Pokemon *mon, bool8 firstMove)
{
    u32 retVal = MOVE_NONE;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);

    // since you can learn more than one move per level
    // the game needs to know whether you decided to
    // learn it or keep the old set to avoid asking
    // you to learn the same move over and over again
    if (firstMove)
    {
        sLearningMoveTableID = 0;

        while (gLevelUpLearnsets[species][sLearningMoveTableID].level != level)
        {
            sLearningMoveTableID++;
            if (gLevelUpLearnsets[species][sLearningMoveTableID].move == LEVEL_UP_END)
                return MOVE_NONE;
        }
    }

    if (gLevelUpLearnsets[species][sLearningMoveTableID].level == level)
    {
        gMoveToLearn = gLevelUpLearnsets[species][sLearningMoveTableID].move;
        sLearningMoveTableID++;
        retVal = GiveMoveToMon(mon, gMoveToLearn);
    }

    return retVal;
}

void DeleteFirstMoveAndGiveMoveToMon(struct Pokemon *mon, u16 move)
{
    s32 i;
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++)
    {
        moves[i] = GetMonData(mon, MON_DATA_MOVE2 + i, NULL);
        pp[i] = GetMonData(mon, MON_DATA_PP2 + i, NULL);
    }

    ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[3] = move;
    pp[3] = gBattleMoves[move].pp;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void DeleteFirstMoveAndGiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move)
{
    s32 i;
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++)
    {
        moves[i] = GetBoxMonData(boxMon, MON_DATA_MOVE2 + i, NULL);
        pp[i] = GetBoxMonData(boxMon, MON_DATA_PP2 + i, NULL);
    }

    ppBonuses = GetBoxMonData(boxMon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[3] = move;
    pp[3] = gBattleMoves[move].pp;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &moves[i]);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetBoxMonData(boxMon, MON_DATA_PP_BONUSES, &ppBonuses);
}

u8 CountAliveMonsInBattle(u8 caseId)
{
    s32 i;
    u8 retVal = 0;

    switch (caseId)
    {
    case BATTLE_ALIVE_EXCEPT_ACTIVE:
        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            if (i != gActiveBattler && !(gAbsentBattlerFlags & gBitTable[i]))
                retVal++;
        }
        break;
    case BATTLE_ALIVE_ATK_SIDE:
        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            if (GetBattlerSide(i) == GetBattlerSide(gBattlerAttacker) && !(gAbsentBattlerFlags & gBitTable[i]))
                retVal++;
        }
        break;
    case BATTLE_ALIVE_DEF_SIDE:
        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            if (GetBattlerSide(i) == GetBattlerSide(gBattlerTarget) && !(gAbsentBattlerFlags & gBitTable[i]))
                retVal++;
        }
        break;
    }

    return retVal;
}

u8 GetDefaultMoveTarget(u8 battlerId)
{
    u8 opposing = BATTLE_OPPOSITE(GetBattlerPosition(battlerId) & BIT_SIDE);

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        return GetBattlerAtPosition(opposing);
    if (CountAliveMonsInBattle(BATTLE_ALIVE_EXCEPT_ACTIVE) > 1)
    {
        u8 position;

        if ((Random() & 1) == 0)
            position = BATTLE_PARTNER(opposing);
        else
            position = opposing;

        return GetBattlerAtPosition(position);
    }
    else
    {
        if ((gAbsentBattlerFlags & gBitTable[opposing]))
            return GetBattlerAtPosition(BATTLE_PARTNER(opposing));
        else
            return GetBattlerAtPosition(opposing);
    }
}

u8 GetMonGender(struct Pokemon *mon)
{
    return GetBoxMonGender(&mon->box);
}

u8 GetBoxMonGender(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL);

    switch (gBaseStats[species].genderRatio)
    {
    case MON_MALE:
    case MON_FEMALE:
    case MON_GENDERLESS:
        return gBaseStats[species].genderRatio;
    }

    if (gBaseStats[species].genderRatio > (personality & 0xFF))
        return MON_FEMALE;
    else
        return MON_MALE;
}

u8 GetGenderFromSpeciesAndPersonality(u16 species, u32 personality)
{
    switch (gBaseStats[species].genderRatio)
    {
    case MON_MALE:
    case MON_FEMALE:
    case MON_GENDERLESS:
        return gBaseStats[species].genderRatio;
    }

    if (gBaseStats[species].genderRatio > (personality & 0xFF))
        return MON_FEMALE;
    else
        return MON_MALE;
}

u32 GetUnownSpeciesId(u32 personality)
{
    return GetUnownLetterByPersonality(personality) + SPECIES_UNOWN_B - 1; //TODO
    /*
    u16 unownLetter = GetUnownLetterByPersonality(personality);
    if (unownLetter == 0)
        return SPECIES_UNOWN;
    return unownLetter + SPECIES_UNOWN_B - 1;
    */
}

void SetMultiuseSpriteTemplateToPokemon(u16 speciesTag, u8 battlerPosition)
{
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else if (gUnknown_020249B4[0])
        gMultiuseSpriteTemplate = gUnknown_020249B4[0]->templates[battlerPosition];
    else if (gUnknown_020249B4[1])
        gMultiuseSpriteTemplate = gUnknown_020249B4[1]->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = speciesTag;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT)
        gMultiuseSpriteTemplate.anims = gUnknown_082FF70C;
    else if (speciesTag > SPECIES_SHINY_TAG)
        gMultiuseSpriteTemplate.anims = gMonFrontAnimsPtrTable[speciesTag - SPECIES_SHINY_TAG];
    else
        gMultiuseSpriteTemplate.anims = gMonFrontAnimsPtrTable[speciesTag];
}

void SetMultiuseSpriteTemplateToTrainerBack(u16 trainerSpriteId, u8 battlerPosition)
{
    gMultiuseSpriteTemplate.paletteTag = trainerSpriteId;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT)
    {
        gMultiuseSpriteTemplate = sTrainerBackSpriteTemplates[trainerSpriteId];
        gMultiuseSpriteTemplate.anims = gTrainerBackAnimsPtrTable[trainerSpriteId];
    }
    else
    {
        if (gMonSpritesGfxPtr != NULL)
            gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
        else
            gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];
        gMultiuseSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[trainerSpriteId];
    }
}

void SetMultiuseSpriteTemplateToTrainerFront(u16 arg0, u8 battlerPosition)
{
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = arg0;
    gMultiuseSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[arg0];
}

static void EncryptBoxMon(struct BoxPokemon *boxMon)
{
    u32 i;
    for (i = 0; i < 12; i++)
    {
        boxMon->secure.raw[i] ^= boxMon->personality;
        boxMon->secure.raw[i] ^= boxMon->otId;
    }
}

static void DecryptBoxMon(struct BoxPokemon *boxMon)
{
    u32 i;
    for (i = 0; i < 12; i++)
    {
        boxMon->secure.raw[i] ^= boxMon->otId;
        boxMon->secure.raw[i] ^= boxMon->personality;
    }
}

#define SUBSTRUCT_CASE(n, v1, v2, v3, v4)                               \
case n:                                                                 \
    {                                                                   \
    union PokemonSubstruct *substructs0 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs1 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs2 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs3 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs4 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs5 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs6 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs7 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs8 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs9 = boxMon->secure.substructs;    \
    union PokemonSubstruct *substructs10 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs11 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs12 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs13 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs14 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs15 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs16 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs17 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs18 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs19 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs20 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs21 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs22 = boxMon->secure.substructs;   \
    union PokemonSubstruct *substructs23 = boxMon->secure.substructs;   \
                                                                        \
        switch (substructType)                                          \
        {                                                               \
        case 0:                                                         \
            substruct = &substructs ## n [v1];                          \
            break;                                                      \
        case 1:                                                         \
            substruct = &substructs ## n [v2];                          \
            break;                                                      \
        case 2:                                                         \
            substruct = &substructs ## n [v3];                          \
            break;                                                      \
        case 3:                                                         \
            substruct = &substructs ## n [v4];                          \
            break;                                                      \
        }                                                               \
        break;                                                          \
    }                                                                   \


static union PokemonSubstruct *GetSubstruct(struct BoxPokemon *boxMon, u32 personality, u8 substructType)
{
    union PokemonSubstruct *substruct = NULL;

    switch (personality % 24)
    {
        SUBSTRUCT_CASE( 0,0,1,2,3)
        SUBSTRUCT_CASE( 1,0,1,3,2)
        SUBSTRUCT_CASE( 2,0,2,1,3)
        SUBSTRUCT_CASE( 3,0,3,1,2)
        SUBSTRUCT_CASE( 4,0,2,3,1)
        SUBSTRUCT_CASE( 5,0,3,2,1)
        SUBSTRUCT_CASE( 6,1,0,2,3)
        SUBSTRUCT_CASE( 7,1,0,3,2)
        SUBSTRUCT_CASE( 8,2,0,1,3)
        SUBSTRUCT_CASE( 9,3,0,1,2)
        SUBSTRUCT_CASE(10,2,0,3,1)
        SUBSTRUCT_CASE(11,3,0,2,1)
        SUBSTRUCT_CASE(12,1,2,0,3)
        SUBSTRUCT_CASE(13,1,3,0,2)
        SUBSTRUCT_CASE(14,2,1,0,3)
        SUBSTRUCT_CASE(15,3,1,0,2)
        SUBSTRUCT_CASE(16,2,3,0,1)
        SUBSTRUCT_CASE(17,3,2,0,1)
        SUBSTRUCT_CASE(18,1,2,3,0)
        SUBSTRUCT_CASE(19,1,3,2,0)
        SUBSTRUCT_CASE(20,2,1,3,0)
        SUBSTRUCT_CASE(21,3,1,2,0)
        SUBSTRUCT_CASE(22,2,3,1,0)
        SUBSTRUCT_CASE(23,3,2,1,0)
    }

    return substruct;
}

u32 GetMonData(struct Pokemon *mon, s32 field, u8* data)
{
    u32 ret;

    switch (field)
    {
    case MON_DATA_STATUS:
        ret = mon->status;
        break;
    case MON_DATA_LEVEL:
        ret = mon->level;
        break;
    case MON_DATA_HP:
        ret = mon->hp;
        break;
    case MON_DATA_MAX_HP:
        ret = mon->maxHP;
        break;
    case MON_DATA_ATK:
        ret = mon->attack;
        break;
    case MON_DATA_DEF:
        ret = mon->defense;
        break;
    case MON_DATA_SPEED:
        ret = mon->speed;
        break;
    case MON_DATA_SPATK:
        ret = mon->spAttack;
        break;
    case MON_DATA_SPDEF:
        ret = mon->spDefense;
        break;
    case MON_DATA_ATK2:
        ret = mon->attack;
        break;
    case MON_DATA_DEF2:
        ret = mon->defense;
        break;
    case MON_DATA_SPEED2:
        ret = mon->speed;
        break;
    case MON_DATA_SPATK2:
        ret = mon->spAttack;
        break;
    case MON_DATA_SPDEF2:
        ret = mon->spDefense;
        break;
    case MON_DATA_MAIL:
        ret = mon->mail;
        break;
    default:
        ret = GetBoxMonData(&mon->box, field, data);
        break;
    }
    return ret;
}

u32 GetBoxMonData(struct BoxPokemon *boxMon, s32 field, u8 *data)
{
    s32 i;
    u32 retVal = 0;
    struct PokemonSubstruct0 *substruct0 = NULL;
    struct PokemonSubstruct1 *substruct1 = NULL;
    struct PokemonSubstruct2 *substruct2 = NULL;
    struct PokemonSubstruct3 *substruct3 = NULL;

    // Any field greater than MON_DATA_ENCRYPT_SEPARATOR is encrypted and must be treated as such
    if (field > MON_DATA_ENCRYPT_SEPARATOR)
    {
        substruct0 = &(GetSubstruct(boxMon, boxMon->personality, 0)->type0);
        substruct1 = &(GetSubstruct(boxMon, boxMon->personality, 1)->type1);
        substruct2 = &(GetSubstruct(boxMon, boxMon->personality, 2)->type2);
        substruct3 = &(GetSubstruct(boxMon, boxMon->personality, 3)->type3);

        DecryptBoxMon(boxMon);

        if (CalculateBoxMonChecksum(boxMon) != boxMon->checksum)
        {
            boxMon->isBadEgg = 1;
            boxMon->isEgg = 1;
            substruct3->isEgg = 1;
        }
    }

    switch (field)
    {
    case MON_DATA_PERSONALITY:
        retVal = boxMon->personality;
        break;
    case MON_DATA_OT_ID:
        retVal = boxMon->otId;
        break;
    case MON_DATA_NICKNAME:
    {
        if (boxMon->isBadEgg)
        {
            for (retVal = 0;
                retVal < POKEMON_NAME_LENGTH && gText_BadEgg[retVal] != EOS;
                data[retVal] = gText_BadEgg[retVal], retVal++) {}

            data[retVal] = EOS;
        }
        else if (boxMon->isEgg)
        {
            StringCopy(data, gText_EggNickname);
            retVal = StringLength(data);
        }
        else if (boxMon->language == LANGUAGE_JAPANESE)
        {
            data[0] = EXT_CTRL_CODE_BEGIN;
            data[1] = EXT_CTRL_CODE_JPN;

            for (retVal = 2, i = 0;
                i < 5 && boxMon->nickname[i] != EOS;
                data[retVal] = boxMon->nickname[i], retVal++, i++) {}

            data[retVal++] = EXT_CTRL_CODE_BEGIN;
            data[retVal++] = EXT_CTRL_CODE_ENG;
            data[retVal] = EOS;
        }
        else
        {
            for (retVal = 0;
                retVal < POKEMON_NAME_LENGTH;
                data[retVal] = boxMon->nickname[retVal], retVal++){}

            data[retVal] = EOS;
        }
        break;
    }
    case MON_DATA_LANGUAGE:
        retVal = boxMon->language;
        break;
    case MON_DATA_SANITY_IS_BAD_EGG:
        retVal = boxMon->isBadEgg;
        break;
    case MON_DATA_SANITY_HAS_SPECIES:
        retVal = boxMon->hasSpecies;
        break;
    case MON_DATA_SANITY_IS_EGG:
        retVal = boxMon->isEgg;
        break;
    case MON_DATA_OT_NAME:
    {
        retVal = 0;

        while (retVal < PLAYER_NAME_LENGTH)
        {
            data[retVal] = boxMon->otName[retVal];
            retVal++;
        }

        data[retVal] = EOS;
        break;
    }
    case MON_DATA_MARKINGS:
        retVal = boxMon->markings;
        break;
    case MON_DATA_CHECKSUM:
        retVal = boxMon->checksum;
        break;
    case MON_DATA_ENCRYPT_SEPARATOR:
        retVal = boxMon->unknown;
        break;
    case MON_DATA_SPECIES:
        retVal = boxMon->isBadEgg ? SPECIES_EGG : substruct0->species;
        break;
    case MON_DATA_HELD_ITEM:
        retVal = substruct0->heldItem;
        break;
    case MON_DATA_EXP:
        retVal = substruct0->experience;
        break;
    case MON_DATA_PP_BONUSES:
        retVal = substruct0->ppBonuses;
        break;
    case MON_DATA_FRIENDSHIP:
        retVal = substruct0->friendship;
        break;
    case MON_DATA_MOVE1:
    case MON_DATA_MOVE2:
    case MON_DATA_MOVE3:
    case MON_DATA_MOVE4:
        retVal = substruct1->moves[field - MON_DATA_MOVE1];
        break;
    case MON_DATA_PP1:
    case MON_DATA_PP2:
    case MON_DATA_PP3:
    case MON_DATA_PP4:
        retVal = substruct1->pp[field - MON_DATA_PP1];
        break;
    case MON_DATA_HP_EV:
        retVal = substruct2->hpEV;
        break;
    case MON_DATA_ATK_EV:
        retVal = substruct2->attackEV;
        break;
    case MON_DATA_DEF_EV:
        retVal = substruct2->defenseEV;
        break;
    case MON_DATA_SPEED_EV:
        retVal = substruct2->speedEV;
        break;
    case MON_DATA_SPATK_EV:
        retVal = substruct2->spAttackEV;
        break;
    case MON_DATA_SPDEF_EV:
        retVal = substruct2->spDefenseEV;
        break;
    case MON_DATA_COOL:
        retVal = substruct2->cool;
        break;
    case MON_DATA_BEAUTY:
        retVal = substruct2->beauty;
        break;
    case MON_DATA_CUTE:
        retVal = substruct2->cute;
        break;
    case MON_DATA_SMART:
        retVal = substruct2->smart;
        break;
    case MON_DATA_TOUGH:
        retVal = substruct2->tough;
        break;
    case MON_DATA_SHEEN:
        retVal = substruct2->sheen;
        break;
    case MON_DATA_POKERUS:
        retVal = substruct3->pokerus;
        break;
    case MON_DATA_MET_LOCATION:
        retVal = substruct3->metLocation;
        break;
    case MON_DATA_MET_LEVEL:
        retVal = substruct3->metLevel;
        break;
    case MON_DATA_MET_GAME:
        retVal = substruct3->metGame;
        break;
    case MON_DATA_POKEBALL:
        retVal = substruct0->pokeball;
        break;
    case MON_DATA_OT_GENDER:
        retVal = substruct3->otGender;
        break;
    case MON_DATA_HP_IV:
        retVal = substruct3->hpIV;
        break;
    case MON_DATA_ATK_IV:
        retVal = substruct3->attackIV;
        break;
    case MON_DATA_DEF_IV:
        retVal = substruct3->defenseIV;
        break;
    case MON_DATA_SPEED_IV:
        retVal = substruct3->speedIV;
        break;
    case MON_DATA_SPATK_IV:
        retVal = substruct3->spAttackIV;
        break;
    case MON_DATA_SPDEF_IV:
        retVal = substruct3->spDefenseIV;
        break;
    case MON_DATA_IS_EGG:
        retVal = substruct3->isEgg;
        break;
    case MON_DATA_ABILITY_NUM:
        retVal = substruct3->abilityNum;
        break;
    case MON_DATA_COOL_RIBBON:
        retVal = substruct3->coolRibbon;
        break;
    case MON_DATA_BEAUTY_RIBBON:
        retVal = substruct3->beautyRibbon;
        break;
    case MON_DATA_CUTE_RIBBON:
        retVal = substruct3->cuteRibbon;
        break;
    case MON_DATA_SMART_RIBBON:
        retVal = substruct3->smartRibbon;
        break;
    case MON_DATA_TOUGH_RIBBON:
        retVal = substruct3->toughRibbon;
        break;
    case MON_DATA_CHAMPION_RIBBON:
        retVal = substruct3->championRibbon;
        break;
    case MON_DATA_WINNING_RIBBON:
        retVal = substruct3->winningRibbon;
        break;
    case MON_DATA_VICTORY_RIBBON:
        retVal = substruct3->victoryRibbon;
        break;
    case MON_DATA_ARTIST_RIBBON:
        retVal = substruct3->artistRibbon;
        break;
    case MON_DATA_EFFORT_RIBBON:
        retVal = substruct3->effortRibbon;
        break;
    case MON_DATA_MARINE_RIBBON:
        retVal = substruct3->marineRibbon;
        break;
    case MON_DATA_LAND_RIBBON:
        retVal = substruct3->landRibbon;
        break;
    case MON_DATA_SKY_RIBBON:
        retVal = substruct3->skyRibbon;
        break;
    case MON_DATA_COUNTRY_RIBBON:
        retVal = substruct3->countryRibbon;
        break;
    case MON_DATA_NATIONAL_RIBBON:
        retVal = substruct3->nationalRibbon;
        break;
    case MON_DATA_EARTH_RIBBON:
        retVal = substruct3->earthRibbon;
        break;
    case MON_DATA_WORLD_RIBBON:
        retVal = substruct3->worldRibbon;
        break;
    case MON_DATA_UNUSED_RIBBONS:
        retVal = substruct3->unusedRibbons;
        break;
    case MON_DATA_EVENT_LEGAL:
        retVal = substruct3->eventLegal;
        break;
    case MON_DATA_SPECIES2:
        retVal = substruct0->species;
        if (substruct0->species && (substruct3->isEgg || boxMon->isBadEgg))
            retVal = SPECIES_EGG;
        break;
    case MON_DATA_IVS:
        retVal = substruct3->hpIV | (substruct3->attackIV << 5) | (substruct3->defenseIV << 10) | (substruct3->speedIV << 15) | (substruct3->spAttackIV << 20) | (substruct3->spDefenseIV << 25);
        break;
    case MON_DATA_KNOWN_MOVES:
        if (substruct0->species && !substruct3->isEgg)
        {
            u16 *moves = (u16 *)data;
            s32 i = 0;

            while (moves[i] != MOVES_COUNT)
            {
                u16 move = moves[i];
                if (substruct1->moves[0] == move
                    || substruct1->moves[1] == move
                    || substruct1->moves[2] == move
                    || substruct1->moves[3] == move)
                    retVal |= gBitTable[i];
                i++;
            }
        }
        break;
    case MON_DATA_RIBBON_COUNT:
        retVal = 0;
        if (substruct0->species && !substruct3->isEgg)
        {
            retVal += substruct3->coolRibbon;
            retVal += substruct3->beautyRibbon;
            retVal += substruct3->cuteRibbon;
            retVal += substruct3->smartRibbon;
            retVal += substruct3->toughRibbon;
            retVal += substruct3->championRibbon;
            retVal += substruct3->winningRibbon;
            retVal += substruct3->victoryRibbon;
            retVal += substruct3->artistRibbon;
            retVal += substruct3->effortRibbon;
            retVal += substruct3->marineRibbon;
            retVal += substruct3->landRibbon;
            retVal += substruct3->skyRibbon;
            retVal += substruct3->countryRibbon;
            retVal += substruct3->nationalRibbon;
            retVal += substruct3->earthRibbon;
            retVal += substruct3->worldRibbon;
        }
        break;
    case MON_DATA_RIBBONS:
        retVal = 0;
        if (substruct0->species && !substruct3->isEgg)
        {
            retVal = substruct3->championRibbon
                | (substruct3->coolRibbon << 1)
                | (substruct3->beautyRibbon << 4)
                | (substruct3->cuteRibbon << 7)
                | (substruct3->smartRibbon << 10)
                | (substruct3->toughRibbon << 13)
                | (substruct3->winningRibbon << 16)
                | (substruct3->victoryRibbon << 17)
                | (substruct3->artistRibbon << 18)
                | (substruct3->effortRibbon << 19)
                | (substruct3->marineRibbon << 20)
                | (substruct3->landRibbon << 21)
                | (substruct3->skyRibbon << 22)
                | (substruct3->countryRibbon << 23)
                | (substruct3->nationalRibbon << 24)
                | (substruct3->earthRibbon << 25)
                | (substruct3->worldRibbon << 26);
        }
        break;
    default:
        break;
    }

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
        EncryptBoxMon(boxMon);

    return retVal;
}

#define SET8(lhs) (lhs) = *data
#define SET16(lhs) (lhs) = data[0] + (data[1] << 8)
#define SET32(lhs) (lhs) = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)

void SetMonData(struct Pokemon *mon, s32 field, const void *dataArg)
{
    const u8 *data = dataArg;

    switch (field)
    {
    case MON_DATA_STATUS:
        SET32(mon->status);
        break;
    case MON_DATA_LEVEL:
        SET8(mon->level);
        break;
    case MON_DATA_HP:
        SET16(mon->hp);
        break;
    case MON_DATA_MAX_HP:
        SET16(mon->maxHP);
        break;
    case MON_DATA_ATK:
        SET16(mon->attack);
        break;
    case MON_DATA_DEF:
        SET16(mon->defense);
        break;
    case MON_DATA_SPEED:
        SET16(mon->speed);
        break;
    case MON_DATA_SPATK:
        SET16(mon->spAttack);
        break;
    case MON_DATA_SPDEF:
        SET16(mon->spDefense);
        break;
    case MON_DATA_MAIL:
        SET8(mon->mail);
        break;
    case MON_DATA_SPECIES2:
        break;
    default:
        SetBoxMonData(&mon->box, field, data);
        break;
    }
}

void SetBoxMonData(struct BoxPokemon *boxMon, s32 field, const void *dataArg)
{
    const u8 *data = dataArg;

    struct PokemonSubstruct0 *substruct0 = NULL;
    struct PokemonSubstruct1 *substruct1 = NULL;
    struct PokemonSubstruct2 *substruct2 = NULL;
    struct PokemonSubstruct3 *substruct3 = NULL;

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
    {
        substruct0 = &(GetSubstruct(boxMon, boxMon->personality, 0)->type0);
        substruct1 = &(GetSubstruct(boxMon, boxMon->personality, 1)->type1);
        substruct2 = &(GetSubstruct(boxMon, boxMon->personality, 2)->type2);
        substruct3 = &(GetSubstruct(boxMon, boxMon->personality, 3)->type3);

        DecryptBoxMon(boxMon);

        if (CalculateBoxMonChecksum(boxMon) != boxMon->checksum)
        {
            boxMon->isBadEgg = 1;
            boxMon->isEgg = 1;
            substruct3->isEgg = 1;
            EncryptBoxMon(boxMon);
            return;
        }
    }

    switch (field)
    {
    case MON_DATA_PERSONALITY:
        SET32(boxMon->personality);
        break;
    case MON_DATA_OT_ID:
        SET32(boxMon->otId);
        break;
    case MON_DATA_NICKNAME:
    {
        s32 i;
        for (i = 0; i < POKEMON_NAME_LENGTH; i++)
            boxMon->nickname[i] = data[i];
        break;
    }
    case MON_DATA_LANGUAGE:
        SET8(boxMon->language);
        break;
    case MON_DATA_SANITY_IS_BAD_EGG:
        SET8(boxMon->isBadEgg);
        break;
    case MON_DATA_SANITY_HAS_SPECIES:
        SET8(boxMon->hasSpecies);
        break;
    case MON_DATA_SANITY_IS_EGG:
        SET8(boxMon->isEgg);
        break;
    case MON_DATA_OT_NAME:
    {
        s32 i;
        for (i = 0; i < PLAYER_NAME_LENGTH; i++)
            boxMon->otName[i] = data[i];
        break;
    }
    case MON_DATA_MARKINGS:
        SET8(boxMon->markings);
        break;
    case MON_DATA_CHECKSUM:
        SET16(boxMon->checksum);
        break;
    case MON_DATA_ENCRYPT_SEPARATOR:
        SET16(boxMon->unknown);
        break;
    case MON_DATA_SPECIES:
    {
        SET16(substruct0->species);
        if (substruct0->species)
            boxMon->hasSpecies = 1;
        else
            boxMon->hasSpecies = 0;
        break;
    }
    case MON_DATA_HELD_ITEM:
        SET16(substruct0->heldItem);
        break;
    case MON_DATA_EXP:
        SET32(substruct0->experience);
        break;
    case MON_DATA_PP_BONUSES:
        SET8(substruct0->ppBonuses);
        break;
    case MON_DATA_FRIENDSHIP:
        SET8(substruct0->friendship);
        break;
    case MON_DATA_MOVE1:
    case MON_DATA_MOVE2:
    case MON_DATA_MOVE3:
    case MON_DATA_MOVE4:
        SET16(substruct1->moves[field - MON_DATA_MOVE1]);
        break;
    case MON_DATA_PP1:
    case MON_DATA_PP2:
    case MON_DATA_PP3:
    case MON_DATA_PP4:
        SET8(substruct1->pp[field - MON_DATA_PP1]);
        break;
    case MON_DATA_HP_EV:
        SET8(substruct2->hpEV);
        break;
    case MON_DATA_ATK_EV:
        SET8(substruct2->attackEV);
        break;
    case MON_DATA_DEF_EV:
        SET8(substruct2->defenseEV);
        break;
    case MON_DATA_SPEED_EV:
        SET8(substruct2->speedEV);
        break;
    case MON_DATA_SPATK_EV:
        SET8(substruct2->spAttackEV);
        break;
    case MON_DATA_SPDEF_EV:
        SET8(substruct2->spDefenseEV);
        break;
    case MON_DATA_COOL:
        SET8(substruct2->cool);
        break;
    case MON_DATA_BEAUTY:
        SET8(substruct2->beauty);
        break;
    case MON_DATA_CUTE:
        SET8(substruct2->cute);
        break;
    case MON_DATA_SMART:
        SET8(substruct2->smart);
        break;
    case MON_DATA_TOUGH:
        SET8(substruct2->tough);
        break;
    case MON_DATA_SHEEN:
        SET8(substruct2->sheen);
        break;
    case MON_DATA_POKERUS:
        SET8(substruct3->pokerus);
        break;
    case MON_DATA_MET_LOCATION:
        SET8(substruct3->metLocation);
        break;
    case MON_DATA_MET_LEVEL:
    {
        u8 metLevel = *data;
        substruct3->metLevel = metLevel;
        break;
    }
    case MON_DATA_MET_GAME:
        SET8(substruct3->metGame);
        break;
    case MON_DATA_POKEBALL:
    {
        u8 pokeball = *data;
        substruct0->pokeball = pokeball;
        break;
    }
    case MON_DATA_OT_GENDER:
        SET8(substruct3->otGender);
        break;
    case MON_DATA_HP_IV:
        SET8(substruct3->hpIV);
        break;
    case MON_DATA_ATK_IV:
        SET8(substruct3->attackIV);
        break;
    case MON_DATA_DEF_IV:
        SET8(substruct3->defenseIV);
        break;
    case MON_DATA_SPEED_IV:
        SET8(substruct3->speedIV);
        break;
    case MON_DATA_SPATK_IV:
        SET8(substruct3->spAttackIV);
        break;
    case MON_DATA_SPDEF_IV:
        SET8(substruct3->spDefenseIV);
        break;
    case MON_DATA_IS_EGG:
        SET8(substruct3->isEgg);
        if (substruct3->isEgg)
            boxMon->isEgg = 1;
        else
            boxMon->isEgg = 0;
        break;
    case MON_DATA_ABILITY_NUM:
        SET8(substruct3->abilityNum);
        break;
    case MON_DATA_COOL_RIBBON:
        SET8(substruct3->coolRibbon);
        break;
    case MON_DATA_BEAUTY_RIBBON:
        SET8(substruct3->beautyRibbon);
        break;
    case MON_DATA_CUTE_RIBBON:
        SET8(substruct3->cuteRibbon);
        break;
    case MON_DATA_SMART_RIBBON:
        SET8(substruct3->smartRibbon);
        break;
    case MON_DATA_TOUGH_RIBBON:
        SET8(substruct3->toughRibbon);
        break;
    case MON_DATA_CHAMPION_RIBBON:
        SET8(substruct3->championRibbon);
        break;
    case MON_DATA_WINNING_RIBBON:
        SET8(substruct3->winningRibbon);
        break;
    case MON_DATA_VICTORY_RIBBON:
        SET8(substruct3->victoryRibbon);
        break;
    case MON_DATA_ARTIST_RIBBON:
        SET8(substruct3->artistRibbon);
        break;
    case MON_DATA_EFFORT_RIBBON:
        SET8(substruct3->effortRibbon);
        break;
    case MON_DATA_MARINE_RIBBON:
        SET8(substruct3->marineRibbon);
        break;
    case MON_DATA_LAND_RIBBON:
        SET8(substruct3->landRibbon);
        break;
    case MON_DATA_SKY_RIBBON:
        SET8(substruct3->skyRibbon);
        break;
    case MON_DATA_COUNTRY_RIBBON:
        SET8(substruct3->countryRibbon);
        break;
    case MON_DATA_NATIONAL_RIBBON:
        SET8(substruct3->nationalRibbon);
        break;
    case MON_DATA_EARTH_RIBBON:
        SET8(substruct3->earthRibbon);
        break;
    case MON_DATA_WORLD_RIBBON:
        SET8(substruct3->worldRibbon);
        break;
    case MON_DATA_UNUSED_RIBBONS:
        SET8(substruct3->unusedRibbons);
        break;
    case MON_DATA_EVENT_LEGAL:
        SET8(substruct3->eventLegal);
        break;
    case MON_DATA_IVS:
    {
        u32 ivs = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        substruct3->hpIV = ivs & MAX_IV_MASK;
        substruct3->attackIV = (ivs >> 5) & MAX_IV_MASK;
        substruct3->defenseIV = (ivs >> 10) & MAX_IV_MASK;
        substruct3->speedIV = (ivs >> 15) & MAX_IV_MASK;
        substruct3->spAttackIV = (ivs >> 20) & MAX_IV_MASK;
        substruct3->spDefenseIV = (ivs >> 25) & MAX_IV_MASK;
        break;
    }
    default:
        break;
    }

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
    {
        boxMon->checksum = CalculateBoxMonChecksum(boxMon);
        EncryptBoxMon(boxMon);
    }
}

void CopyMon(void *dest, void *src, size_t size)
{
    memcpy(dest, src, size);
}

u8 GiveMonToPlayer(struct Pokemon *mon)
{
    s32 i;

    SetMonData(mon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetMonData(mon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);
    SetMonData(mon, MON_DATA_OT_ID, gSaveBlock2Ptr->playerTrainerId);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            break;
    }

    if (i >= PARTY_SIZE)
        return SendMonToPC(mon);

    CopyMon(&gPlayerParty[i], mon, sizeof(*mon));
    gPlayerPartyCount = i + 1;
    return MON_GIVEN_TO_PARTY;
}

u8 SendMonToPC(struct Pokemon* mon)
{
    s32 boxNo, boxPos;

    SetPCBoxToSendMon(VarGet(VAR_PC_BOX_TO_SEND_MON));

    boxNo = StorageGetCurrentBox();

    do
    {
        for (boxPos = 0; boxPos < IN_BOX_COUNT; boxPos++)
        {
            struct BoxPokemon* checkingMon = GetBoxedMonPtr(boxNo, boxPos);
            if (GetBoxMonData(checkingMon, MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            {
                MonRestorePP(mon);
                CopyMon(checkingMon, &mon->box, sizeof(mon->box));
                gSpecialVar_MonBoxId = boxNo;
                gSpecialVar_MonBoxPos = boxPos;
                if (GetPCBoxToSendMon() != boxNo)
                    FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
                VarSet(VAR_PC_BOX_TO_SEND_MON, boxNo);
                return MON_GIVEN_TO_PC;
            }
        }

        boxNo++;
        if (boxNo == TOTAL_BOXES_COUNT)
            boxNo = 0;
    } while (boxNo != StorageGetCurrentBox());

    return MON_CANT_GIVE;
}

u8 CalculatePlayerPartyCount(void)
{
    gPlayerPartyCount = 0;

    while (gPlayerPartyCount < PARTY_SIZE
        && GetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
    {
        gPlayerPartyCount++;
    }

    return gPlayerPartyCount;
}

u8 CalculateEnemyPartyCount(void)
{
    gEnemyPartyCount = 0;

    while (gEnemyPartyCount < PARTY_SIZE
        && GetMonData(&gEnemyParty[gEnemyPartyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
    {
        gEnemyPartyCount++;
    }

    return gEnemyPartyCount;
}

u8 GetMonsStateToDoubles(void)
{
    s32 aliveCount = 0;
    s32 i;
    CalculatePlayerPartyCount();

    if (gPlayerPartyCount == 1)
        return gPlayerPartyCount; // PLAYER_HAS_ONE_MON

    for (i = 0; i < gPlayerPartyCount; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_EGG
         && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0
         && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE)
            aliveCount++;
    }

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

u8 GetMonsStateToDoubles_2(void)
{
    s32 aliveCount = 0;
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
        if (species != SPECIES_EGG && species != SPECIES_NONE
         && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0)
            aliveCount++;
    }

    if (aliveCount == 1)
        return PLAYER_HAS_ONE_MON; // may have more than one, but only one is alive

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

u16 GetAbilityBySpecies(u16 species, u8 abilityNum)
{
    if (abilityNum == 2)
        gLastUsedAbility = gBaseStats[species].abilityHidden;
    else if (abilityNum == 1)
        gLastUsedAbility = gBaseStats[species].abilities[1];
    else
        gLastUsedAbility = gBaseStats[species].abilities[0];

    return gLastUsedAbility;
}

u16 GetMonAbility(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    return GetAbilityBySpecies(species, abilityNum);
}

void CreateSecretBaseEnemyParty(struct SecretBase *secretBaseRecord)
{
    s32 i, j;

    ZeroEnemyPartyMons();
    *gBattleResources->secretBase = *secretBaseRecord;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (gBattleResources->secretBase->party.species[i])
        {
            CreateMon(&gEnemyParty[i],
                gBattleResources->secretBase->party.species[i],
                gBattleResources->secretBase->party.levels[i],
                15,
                1,
                gBattleResources->secretBase->party.personality[i],
                OT_ID_RANDOM_NO_SHINY,
                0);

            SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBattleResources->secretBase->party.heldItems[i]);

            for (j = 0; j < NUM_STATS; j++)
                SetMonData(&gEnemyParty[i], MON_DATA_HP_EV + j, &gBattleResources->secretBase->party.EVs[i]);

            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                SetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j, &gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]);
                SetMonData(&gEnemyParty[i], MON_DATA_PP1 + j, &gBattleMoves[gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]].pp);
            }
        }
    }
}

u8 GetSecretBaseTrainerPicIndex(void)
{
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % 5];
    return gFacilityClassToPicIndex[facilityClass];
}

u8 GetSecretBaseTrainerClass(void)
{
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % 5];
    return gFacilityClassToTrainerClass[facilityClass];
}

bool8 IsPlayerPartyAndPokemonStorageFull(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            return FALSE;

    return IsPokemonStorageFull();
}

bool8 IsPokemonStorageFull(void)
{
    s32 i, j;

    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        for (j = 0; j < IN_BOX_COUNT; j++)
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == SPECIES_NONE)
                return FALSE;

    return TRUE;
}

void GetSpeciesName(u8 *name, u16 species)
{
    s32 i;

    for (i = 0; i <= POKEMON_NAME_LENGTH; i++)
    {
        if (species > NUM_SPECIES)
            name[i] = gSpeciesNames[0][i];
        else
            name[i] = gSpeciesNames[species][i];

        if (name[i] == EOS)
            break;
    }

    name[i] = EOS;
}

u8 CalculatePPWithBonus(u16 move, u8 ppBonuses, u8 moveIndex)
{
    u8 basePP = gBattleMoves[move].pp;
    return basePP + ((basePP * 20 * ((gPPUpGetMask[moveIndex] & ppBonuses) >> (2 * moveIndex))) / 100);
}

void RemoveMonPPBonus(struct Pokemon *mon, u8 moveIndex)
{
    u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses &= gPPUpSetMask[moveIndex];
    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void RemoveBattleMonPPBonus(struct BattlePokemon *mon, u8 moveIndex)
{
    mon->ppBonuses &= gPPUpSetMask[moveIndex];
}

void PokemonToBattleMon(struct Pokemon *src, struct BattlePokemon *dst)
{
    s32 i;
    u8 nickname[POKEMON_NAME_LENGTH * 2];

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        dst->moves[i] = GetMonData(src, MON_DATA_MOVE1 + i, NULL);
        dst->pp[i] = GetMonData(src, MON_DATA_PP1 + i, NULL);
    }

    dst->species = GetMonData(src, MON_DATA_SPECIES, NULL);
    dst->item = GetMonData(src, MON_DATA_HELD_ITEM, NULL);
    dst->ppBonuses = GetMonData(src, MON_DATA_PP_BONUSES, NULL);
    dst->friendship = GetMonData(src, MON_DATA_FRIENDSHIP, NULL);
    dst->experience = GetMonData(src, MON_DATA_EXP, NULL);
    dst->hpIV = GetMonData(src, MON_DATA_HP_IV, NULL);
    dst->attackIV = GetMonData(src, MON_DATA_ATK_IV, NULL);
    dst->defenseIV = GetMonData(src, MON_DATA_DEF_IV, NULL);
    dst->speedIV = GetMonData(src, MON_DATA_SPEED_IV, NULL);
    dst->spAttackIV = GetMonData(src, MON_DATA_SPATK_IV, NULL);
    dst->spDefenseIV = GetMonData(src, MON_DATA_SPDEF_IV, NULL);
    dst->personality = GetMonData(src, MON_DATA_PERSONALITY, NULL);
    dst->status1 = GetMonData(src, MON_DATA_STATUS, NULL);
    dst->level = GetMonData(src, MON_DATA_LEVEL, NULL);
    dst->hp = GetMonData(src, MON_DATA_HP, NULL);
    dst->maxHP = GetMonData(src, MON_DATA_MAX_HP, NULL);
    dst->attack = GetMonData(src, MON_DATA_ATK, NULL);
    dst->defense = GetMonData(src, MON_DATA_DEF, NULL);
    dst->speed = GetMonData(src, MON_DATA_SPEED, NULL);
    dst->spAttack = GetMonData(src, MON_DATA_SPATK, NULL);
    dst->spDefense = GetMonData(src, MON_DATA_SPDEF, NULL);
    dst->abilityNum = GetMonData(src, MON_DATA_ABILITY_NUM, NULL);
    dst->otId = GetMonData(src, MON_DATA_OT_ID, NULL);
    dst->type1 = gBaseStats[dst->species].type1;
    dst->type2 = gBaseStats[dst->species].type2;
    dst->type3 = TYPE_MYSTERY;
    dst->ability = GetAbilityBySpecies(dst->species, dst->abilityNum);
    GetMonData(src, MON_DATA_NICKNAME, nickname);
    StringCopy10(dst->nickname, nickname);
    GetMonData(src, MON_DATA_OT_NAME, dst->otName);

    for (i = 0; i < NUM_BATTLE_STATS; i++)
        dst->statStages[i] = 6;

    dst->status2 = 0;
}

void CopyPlayerPartyMonToBattleData(u8 battlerId, u8 partyIndex)
{
    PokemonToBattleMon(&gPlayerParty[partyIndex], &gBattleMons[battlerId]);
    gBattleStruct->hpOnSwitchout[GetBattlerSide(battlerId)] = gBattleMons[battlerId].hp;
    UpdateSentPokesToOpponentValue(battlerId);
    ClearTemporarySpeciesSpriteData(battlerId, FALSE);
}

bool8 ExecuteTableBasedItemEffect(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex)
{
    return PokemonUseItemEffects(mon, item, partyIndex, moveIndex, FALSE);
}

#define UPDATE_FRIENDSHIP_FROM_ITEM                                                                     \
{                                                                                                       \
    if ((retVal == 0 || friendshipOnly) && !ShouldSkipFriendshipChange() && friendshipChange == 0)      \
    {                                                                                                   \
        friendshipChange = itemEffect[itemEffectParam];                                                 \
        friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);                                        \
        if (friendshipChange > 0 && holdEffect == HOLD_EFFECT_FRIENDSHIP_UP)                            \
            friendship += 150 * friendshipChange / 100;                                                 \
        else                                                                                            \
            friendship += friendshipChange;                                                             \
        if (friendshipChange > 0)                                                                       \
        {                                                                                               \
            if (GetMonData(mon, MON_DATA_POKEBALL, NULL) == ITEM_LUXURY_BALL)                           \
                friendship++;                                                                           \
            if (GetMonData(mon, MON_DATA_MET_LOCATION, NULL) == GetCurrentRegionMapSectionId())         \
                friendship++;                                                                           \
        }                                                                                               \
        if (friendship < 0)                                                                             \
            friendship = 0;                                                                             \
        if (friendship > MAX_FRIENDSHIP)                                                                \
            friendship = MAX_FRIENDSHIP;                                                                \
        SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);                                              \
        retVal = FALSE;                                                                                 \
    }                                                                                                   \
}

// Returns TRUE if the item has no effect on the Pokémon, FALSE otherwise
bool8 PokemonUseItemEffects(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex, bool8 usedByAI)
{
    u32 dataUnsigned;
    s32 dataSigned, evCap;
    s32 friendship;
    s32 i;
    bool8 retVal = TRUE;
    const u8 *itemEffect;
    u8 itemEffectParam = ITEM_EFFECT_ARG_START;
    u32 temp1, temp2;
    s8 friendshipChange = 0;
    u8 holdEffect;
    u8 battlerId = MAX_BATTLERS_COUNT;
    u32 friendshipOnly = FALSE;
    u16 heldItem;
    u8 effectFlags;
    s8 evChange;
    u16 evCount;

    // Get item hold effect
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
    if (heldItem == ITEM_ENIGMA_BERRY)
    {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[gBattlerInMenuId].holdEffect;
        else
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(heldItem);
    }

    // Get battler id (if relevant)
    gPotentialItemEffectBattler = gBattlerInMenuId;
    if (gMain.inBattle)
    {
        gActiveBattler = gBattlerInMenuId;
        i = (GetBattlerSide(gActiveBattler) != B_SIDE_PLAYER);
        while (i < gBattlersCount)
        {
            if (gBattlerPartyIndexes[i] == partyIndex)
            {
                battlerId = i;
                break;
            }
            i += 2;
        }
    }
    else
    {
        gActiveBattler = 0;
        battlerId = MAX_BATTLERS_COUNT;
    }

    // Skip using the item if it won't do anything
    if (!ITEM_HAS_EFFECT(item))
        return TRUE;
    if (gItemEffectTable[item - ITEM_POTION] == NULL && item != ITEM_ENIGMA_BERRY)
        return TRUE;

    // Get item effect
    if (item == ITEM_ENIGMA_BERRY)
    {
        if (gMain.inBattle)
            itemEffect = gEnigmaBerries[gActiveBattler].itemEffect;
        else
            itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
    }
    else
    {
        itemEffect = gItemEffectTable[item - ITEM_POTION];
    }

    // Do item effect
    for (i = 0; i < ITEM_EFFECT_ARG_START; i++)
    {
        switch (i)
        {

        // Handle ITEM0 effects (infatuation, Dire Hit, X Attack). ITEM0_SACRED_ASH is handled in party_menu.c
        case 0:
            // Cure infatuation
            if ((itemEffect[i] & ITEM0_INFATUATION)
             && gMain.inBattle && battlerId != MAX_BATTLERS_COUNT && (gBattleMons[battlerId].status2 & STATUS2_INFATUATION))
            {
                gBattleMons[battlerId].status2 &= ~STATUS2_INFATUATION;
                retVal = FALSE;
            }

            // Dire Hit
            if ((itemEffect[i] & ITEM0_DIRE_HIT)
             && !(gBattleMons[gActiveBattler].status2 & STATUS2_FOCUS_ENERGY))
            {
                gBattleMons[gActiveBattler].status2 |= STATUS2_FOCUS_ENERGY;
                retVal = FALSE;
            }
        #ifndef ITEM_EXPANSION
            // X Attack
            if ((itemEffect[i] & ITEM0_X_ATTACK)
             && gBattleMons[gActiveBattler].statStages[STAT_ATK] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] += itemEffect[i] & ITEM0_X_ATTACK;
                if (gBattleMons[gActiveBattler].statStages[STAT_ATK] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] = MAX_STAT_STAGE;
                retVal = FALSE;
            }
        #endif
            break;

        // Handle ITEM1 effects (in-battle stat boosting effects)
        #ifndef ITEM_EXPANSION
        case 1:
            // X Defend
            if ((itemEffect[i] & ITEM1_X_DEFEND)
             && gBattleMons[gActiveBattler].statStages[STAT_DEF] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] += (itemEffect[i] & ITEM1_X_DEFEND) >> 4;
                if (gBattleMons[gActiveBattler].statStages[STAT_DEF] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Speed
            if ((itemEffect[i] & ITEM1_X_SPEED)
             && gBattleMons[gActiveBattler].statStages[STAT_SPEED] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] += itemEffect[i] & ITEM1_X_SPEED;
                if (gBattleMons[gActiveBattler].statStages[STAT_SPEED] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] = MAX_STAT_STAGE;
                retVal = FALSE;
            }
            break;
        // Handle ITEM2 effects (more stat boosting effects)
        case 2:
            // X Accuracy
            if ((itemEffect[i] & ITEM2_X_ACCURACY)
             && gBattleMons[gActiveBattler].statStages[STAT_ACC] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] += (itemEffect[i] & ITEM2_X_ACCURACY) >> 4;
                if (gBattleMons[gActiveBattler].statStages[STAT_ACC] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Sp Attack
            if ((itemEffect[i] & ITEM2_X_SPATK)
             && gBattleMons[gActiveBattler].statStages[STAT_SPATK] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] += itemEffect[i] & ITEM2_X_SPATK;
                if (gBattleMons[gActiveBattler].statStages[STAT_SPATK] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] = MAX_STAT_STAGE;
                retVal = FALSE;
            }
            break;
        #else
        // Handle ITEM1 effects (in-battle stat boosting effects)
        case 1:
            // X Attack
            if ((itemEffect[i] & ITEM1_X_ATTACK)
             && gBattleMons[gActiveBattler].statStages[STAT_ATK] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_ATK] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_ATK] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Defense
            if ((itemEffect[i] & ITEM1_X_DEFENSE)
             && gBattleMons[gActiveBattler].statStages[STAT_DEF] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_DEF] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_DEF] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Speed
            if ((itemEffect[i] & ITEM1_X_SPEED)
             && gBattleMons[gActiveBattler].statStages[STAT_SPEED] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_SPEED] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_SPEED] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Sp Attack
            if ((itemEffect[i] & ITEM1_X_SPATK)
             && gBattleMons[gActiveBattler].statStages[STAT_SPATK] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_SPATK] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_SPATK] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Sp Defense
            if ((itemEffect[i] & ITEM1_X_SPDEF)
             && gBattleMons[gActiveBattler].statStages[STAT_SPDEF] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_SPDEF] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_SPDEF] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_SPDEF] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_SPDEF] = MAX_STAT_STAGE;
                retVal = FALSE;
            }

            // X Accuracy
            if ((itemEffect[i] & ITEM1_X_ACCURACY)
             && gBattleMons[gActiveBattler].statStages[STAT_ACC] < MAX_STAT_STAGE)
            {
                if (B_X_ITEMS_BUFF == GEN_7)
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] += 2;
                else
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] += 1;
                if (gBattleMons[gActiveBattler].statStages[STAT_ACC] > MAX_STAT_STAGE)
                    gBattleMons[gActiveBattler].statStages[STAT_ACC] = MAX_STAT_STAGE;
                retVal = FALSE;
            }
            break;
        // Formerly used by the item effects of the X Sp. Atk and the X Accuracy
        case 2:
            break;
        #endif
        // Handle ITEM3 effects (Guard Spec, Rare Candy, cure status)
        case 3:
            // Guard Spec
            if ((itemEffect[i] & ITEM3_GUARD_SPEC)
             && gSideTimers[GetBattlerSide(gActiveBattler)].mistTimer == 0)
            {
                gSideTimers[GetBattlerSide(gActiveBattler)].mistTimer = 5;
                retVal = FALSE;
            }

            // Rare Candy
            if ((itemEffect[i] & ITEM3_LEVEL_UP)
             && GetMonData(mon, MON_DATA_LEVEL, NULL) != MAX_LEVEL)
            {
                dataUnsigned = gExperienceTables[gBaseStats[GetMonData(mon, MON_DATA_SPECIES, NULL)].growthRate][GetMonData(mon, MON_DATA_LEVEL, NULL) + 1];
                SetMonData(mon, MON_DATA_EXP, &dataUnsigned);
                CalculateMonStats(mon);
                retVal = FALSE;
            }

            // Cure status
            if ((itemEffect[i] & ITEM3_SLEEP)
             && HealStatusConditions(mon, partyIndex, STATUS1_SLEEP, battlerId) == 0)
            {
                if (battlerId != MAX_BATTLERS_COUNT)
                    gBattleMons[battlerId].status2 &= ~STATUS2_NIGHTMARE;
                retVal = FALSE;
            }
            if ((itemEffect[i] & ITEM3_POISON) && HealStatusConditions(mon, partyIndex, STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER, battlerId) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_BURN) && HealStatusConditions(mon, partyIndex, STATUS1_BURN, battlerId) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_FREEZE) && HealStatusConditions(mon, partyIndex, STATUS1_FREEZE, battlerId) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_PARALYSIS) && HealStatusConditions(mon, partyIndex, STATUS1_PARALYSIS, battlerId) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_CONFUSION)  // heal confusion
             && gMain.inBattle && battlerId != MAX_BATTLERS_COUNT && (gBattleMons[battlerId].status2 & STATUS2_CONFUSION))
            {
                gBattleMons[battlerId].status2 &= ~STATUS2_CONFUSION;
                retVal = FALSE;
            }
            break;

        // Handle ITEM4 effects (Change HP/Atk EVs, HP heal, PP heal, PP up, Revive, and evolution stones)
        case 4:
            effectFlags = itemEffect[i];

            // PP Up
            if (effectFlags & ITEM4_PP_UP)
            {
                effectFlags &= ~ITEM4_PP_UP;
                dataUnsigned = (GetMonData(mon, MON_DATA_PP_BONUSES, NULL) & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                temp1 = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                if (dataUnsigned <= 2 && temp1 > 4)
                {
                    dataUnsigned = GetMonData(mon, MON_DATA_PP_BONUSES, NULL) + gPPUpAddMask[moveIndex];
                    SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);

                    dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp1;
                    dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                    SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                    retVal = FALSE;
                }
            }
            temp1 = 0;

            // Loop through and try each of the remaining ITEM4 effects
            while (effectFlags != 0)
            {
                if (effectFlags & 1)
                {
                    switch (temp1)
                    {
                    case 0: // ITEM4_EV_HP
                    case 1: // ITEM4_EV_ATK
                        evCount = GetMonEVCount(mon);
                        temp2 = itemEffect[itemEffectParam];
                        dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1], NULL);
                        evChange = temp2;
     
                        if (evChange > 0) // Increasing EV (HP or Atk)
                        {
                            // Has EV increase limit already been reached?
                            if (evCount >= MAX_TOTAL_EVS)
                                return TRUE;

                            if (itemEffect[10] & ITEM10_IS_VITAMIN)
                                evCap = EV_ITEM_RAISE_LIMIT;
                            else
                                evCap = 252;

                            if (dataSigned >= evCap)
                                break;

                            // Limit the increase
                            if (dataSigned + evChange > evCap)
                                temp2 = evCap - (dataSigned + evChange) + evChange;
                            else
                                temp2 = evChange;

                            if (evCount + temp2 > MAX_TOTAL_EVS)
                                temp2 += MAX_TOTAL_EVS - (evCount + temp2);

                            dataSigned += temp2;
                        }
                        else // Decreasing EV (HP or Atk)
                        {
                            if (dataSigned == 0)
                            {
                                // No EVs to lose, but make sure friendship updates anyway
                                friendshipOnly = TRUE;
                                itemEffectParam++;
                                break;
                            }
                            dataSigned += evChange;
                            if (dataSigned < 0)
                                dataSigned = 0;
                        }

                        // Update EVs and stats
                        SetMonData(mon, sGetMonDataEVConstants[temp1], &dataSigned);
                        CalculateMonStats(mon);
                        itemEffectParam++;
                        retVal = FALSE;
                        break;

                    case 2: // ITEM4_HEAL_HP
                        // If Revive, update number of times revive has been used
                        if (effectFlags & (ITEM4_REVIVE >> 2))
                        {
                            if (GetMonData(mon, MON_DATA_HP, NULL) != 0)
                            {
                                itemEffectParam++;
                                break;
                            }
                            if (gMain.inBattle)
                            {
                                if (battlerId != MAX_BATTLERS_COUNT)
                                {
                                    gAbsentBattlerFlags &= ~gBitTable[battlerId];
                                    CopyPlayerPartyMonToBattleData(battlerId, GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[battlerId]));
                                    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleResults.numRevivesUsed < 255)
                                        gBattleResults.numRevivesUsed++;
                                }
                                else
                                {
                                    gAbsentBattlerFlags &= ~gBitTable[gActiveBattler ^ 2];
                                    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleResults.numRevivesUsed < 255)
                                        gBattleResults.numRevivesUsed++;
                                }
                            }
                        }
                        else
                        {
                            if (GetMonData(mon, MON_DATA_HP, NULL) == 0)
                            {
                                itemEffectParam++;
                                break;
                            }
                        }

                        // Get amount of HP to restore
                        dataUnsigned = itemEffect[itemEffectParam++];
                        switch (dataUnsigned)
                        {
                        case ITEM6_HEAL_HP_FULL:
                            dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL) - GetMonData(mon, MON_DATA_HP, NULL);
                            break;
                        case ITEM6_HEAL_HP_HALF:
                            dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL) / 2;
                            if (dataUnsigned == 0)
                                dataUnsigned = 1;
                            break;
                        case ITEM6_HEAL_HP_LVL_UP:
                            dataUnsigned = gBattleScripting.levelUpHP;
                            break;
                        }

                        // Only restore HP if not at max health
                        if (GetMonData(mon, MON_DATA_MAX_HP, NULL) != GetMonData(mon, MON_DATA_HP, NULL))
                        {
                            if (!usedByAI)
                            {
                                // Restore HP 
                                dataUnsigned = GetMonData(mon, MON_DATA_HP, NULL) + dataUnsigned;
                                if (dataUnsigned > GetMonData(mon, MON_DATA_MAX_HP, NULL))
                                    dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL);
                                SetMonData(mon, MON_DATA_HP, &dataUnsigned);

                                // Update battler (if applicable)
                                if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT)
                                {
                                    gBattleMons[battlerId].hp = dataUnsigned;
                                    if (!(effectFlags & (ITEM4_REVIVE >> 2)) && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
                                    {
                                        if (gBattleResults.numHealingItemsUsed < 255)
                                            gBattleResults.numHealingItemsUsed++;

                                        temp2 = gActiveBattler;
                                        gActiveBattler = battlerId;
                                        BtlController_EmitGetMonData(0, REQUEST_ALL_BATTLE, 0);
                                        MarkBattlerForControllerExec(gActiveBattler);
                                        gActiveBattler = temp2;
                                    }
                                }
                            }
                            else
                            {
                                gBattleMoveDamage = -dataUnsigned;
                            }
                            retVal = FALSE;
                        }
                        effectFlags &= ~(ITEM4_REVIVE >> 2);
                        break;

                    case 3: // ITEM4_HEAL_PP
                        if (!(effectFlags & (ITEM4_HEAL_PP_ONE >> 3)))
                        {
                            // Heal PP for all moves
                            for (temp2 = 0; (signed)(temp2) < (signed)(MAX_MON_MOVES); temp2++)
                            {
                                u16 moveId;
                                dataUnsigned = GetMonData(mon, MON_DATA_PP1 + temp2, NULL);
                                moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL);
                                if (dataUnsigned != CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2))
                                {
                                    dataUnsigned += itemEffect[itemEffectParam];
                                    moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL); // Redundant
                                    if (dataUnsigned > CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2))
                                    {
                                        moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL); // Redundant
                                        dataUnsigned = CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2);
                                    }
                                    SetMonData(mon, MON_DATA_PP1 + temp2, &dataUnsigned);

                                    // Heal battler PP too (if applicable)
                                    if (gMain.inBattle
                                     && battlerId != MAX_BATTLERS_COUNT && !(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED)
                                     && !(gDisableStructs[battlerId].mimickedMoves & gBitTable[temp2]))
                                        gBattleMons[battlerId].pp[temp2] = dataUnsigned;

                                    retVal = FALSE;
                                }
                            }
                            itemEffectParam++;
                        }
                        else
                        {
                            // Heal PP for one move
                            u16 moveId;
                            dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL);
                            moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL);
                            if (dataUnsigned != CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex))
                            {
                                dataUnsigned += itemEffect[itemEffectParam++];
                                moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL); // Redundant
                                if (dataUnsigned > CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex))
                                {
                                    moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL); // Redundant
                                    dataUnsigned = CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                                }
                                SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                                
                                // Heal battler PP too (if applicable)
                                if (gMain.inBattle
                                 && battlerId != MAX_BATTLERS_COUNT && !(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED)
                                 && !(gDisableStructs[battlerId].mimickedMoves & gBitTable[moveIndex]))
                                    gBattleMons[battlerId].pp[moveIndex] = dataUnsigned;

                                retVal = FALSE;
                            }
                        }
                        break;

                    // cases 4-6 are ITEM4_HEAL_PP_ONE, ITEM4_PP_UP, and ITEM4_REVIVE, which
                    // are already handled above by other cases or before the loop

                    case 7: // ITEM4_EVO_STONE
                        {
                            u16 targetSpecies = GetEvolutionTargetSpecies(mon, EVO_MODE_ITEM_USE, item, SPECIES_NONE);

                            if (targetSpecies != SPECIES_NONE)
                            {
                                BeginEvolutionScene(mon, targetSpecies, FALSE, partyIndex);
                                return FALSE;
                            }
                        }
                        break;
                    }
                }
                temp1++;
                effectFlags >>= 1;
            }
            break;

        // Handle ITEM5 effects (Change Def/SpDef/SpAtk/Speed EVs, PP Max, and friendship changes)
        case 5:
            effectFlags = itemEffect[i];
            temp1 = 0;
            
            // Loop through and try each of the ITEM5 effects
            while (effectFlags != 0)
            {
                if (effectFlags & 1)
                {
                    switch (temp1)
                    {
                    case 0: // ITEM5_EV_DEF
                    case 1: // ITEM5_EV_SPEED
                    case 2: // ITEM5_EV_SPDEF
                    case 3: // ITEM5_EV_SPATK
                        evCount = GetMonEVCount(mon);
                        temp2 = itemEffect[itemEffectParam];
                        dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1 + 2], NULL);
                        evChange = temp2;
                        if (evChange > 0) // Increasing EV
                        {
                            // Has EV increase limit already been reached?
                            if (evCount >= MAX_TOTAL_EVS)
                                return TRUE;

                            if (itemEffect[10] & ITEM10_IS_VITAMIN)
                                evCap = EV_ITEM_RAISE_LIMIT;
                            else
                                evCap = 252;

                            if (dataSigned >= evCap)
                                break;

                            // Limit the increase
                            if (dataSigned + evChange > evCap)
                                temp2 = evCap - (dataSigned + evChange) + evChange;
                            else
                                temp2 = evChange;

                            if (evCount + temp2 > MAX_TOTAL_EVS)
                                temp2 += MAX_TOTAL_EVS - (evCount + temp2);

                            dataSigned += temp2;
                        }
                        else // Decreasing EV
                        {
                            if (dataSigned == 0)
                            {
                                // No EVs to lose, but make sure friendship updates anyway
                                friendshipOnly = TRUE;
                                itemEffectParam++;
                                break;
                            }
                            dataSigned += evChange;
                            if (dataSigned < 0)
                                dataSigned = 0;
                        }

                        // Update EVs and stats
                        SetMonData(mon, sGetMonDataEVConstants[temp1 + 2], &dataSigned);
                        CalculateMonStats(mon);
                        retVal = FALSE;
                        itemEffectParam++;
                        break;

                    case 4: // ITEM5_PP_MAX
                        dataUnsigned = (GetMonData(mon, MON_DATA_PP_BONUSES, NULL) & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                        temp2 = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                        if (dataUnsigned < 3 && temp2 > 4)
                        {
                            dataUnsigned = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
                            dataUnsigned &= gPPUpSetMask[moveIndex];
                            dataUnsigned += gPPUpAddMask[moveIndex] * 3;

                            SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);
                            dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp2;
                            dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                            SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                            retVal = FALSE;
                        }
                        break;

                    case 5: // ITEM5_FRIENDSHIP_LOW
                        // Changes to friendship are given differently depending on
                        // how much friendship the Pokémon already has.
                        // In general, Pokémon with lower friendship receive more,
                        // and Pokémon with higher friendship receive less.
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 100)
                        {
                            UPDATE_FRIENDSHIP_FROM_ITEM;
                        }
                        itemEffectParam++;
                        break;

                    case 6: // ITEM5_FRIENDSHIP_MID
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 100 && GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 200)
                        {
                            UPDATE_FRIENDSHIP_FROM_ITEM;
                        }
                        itemEffectParam++;
                        break;

                    case 7: // ITEM5_FRIENDSHIP_HIGH
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 200)
                        {
                            UPDATE_FRIENDSHIP_FROM_ITEM;
                        }
                        itemEffectParam++;
                        break;
                    }
                }
                temp1++;
                effectFlags >>= 1;
            }
            break;
        }
    }
    return retVal;
}

bool8 HealStatusConditions(struct Pokemon *mon, u32 battlePartyId, u32 healMask, u8 battlerId)
{
    u32 status = GetMonData(mon, MON_DATA_STATUS, 0);

    if (status & healMask)
    {
        status &= ~healMask;
        SetMonData(mon, MON_DATA_STATUS, &status);
        if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT)
            gBattleMons[battlerId].status1 &= ~healMask;
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

u8 GetItemEffectParamOffset(u16 itemId, u8 effectByte, u8 effectBit)
{
    const u8 *temp;
    const u8 *itemEffect;
    u8 offset;
    int i;
    u8 j;
    u8 effectFlags;

    offset = ITEM_EFFECT_ARG_START;

    temp = gItemEffectTable[itemId - ITEM_POTION];

    if (!temp && itemId != ITEM_ENIGMA_BERRY)
        return 0;

    if (itemId == ITEM_ENIGMA_BERRY)
    {
        temp = gEnigmaBerries[gActiveBattler].itemEffect;
    }

    itemEffect = temp;

    for (i = 0; i < ITEM_EFFECT_ARG_START; i++)
    {
        switch (i)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            if (i == effectByte)
                return 0;
            break;
        case 4:
            effectFlags = itemEffect[4];
            if (effectFlags & ITEM4_PP_UP)
                effectFlags &= ~(ITEM4_PP_UP);
            j = 0;
            while (effectFlags)
            {
                if (effectFlags & 1)
                {
                    switch (j)
                    {
                    case 2: // ITEM4_HEAL_HP
                        if (effectFlags & (ITEM4_REVIVE >> 2))
                            effectFlags &= ~(ITEM4_REVIVE >> 2);
                        // fallthrough
                    case 0: // ITEM4_EV_HP
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 1: // ITEM4_EV_ATK
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 3: // ITEM4_HEAL_PP
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 7: // ITEM4_EVO_STONE
                        if (i == effectByte)
                            return 0;
                        break;
                    }
                }
                j++;
                effectFlags >>= 1;
                if (i == effectByte)
                    effectBit >>= 1;
            }
            break;
        case 5:
            effectFlags = itemEffect[5];
            j = 0;
            while (effectFlags)
            {
                if (effectFlags & 1)
                {
                    switch (j)
                    {
                    case 0: // ITEM5_EV_DEF
                    case 1: // ITEM5_EV_SPEED
                    case 2: // ITEM5_EV_SPDEF
                    case 3: // ITEM5_EV_SPATK
                    case 4: // ITEM5_PP_MAX
                    case 5: // ITEM5_FRIENDSHIP_LOW
                    case 6: // ITEM5_FRIENDSHIP_MID
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 7: // ITEM5_FRIENDSHIP_HIGH
                        if (i == effectByte)
                            return 0;
                        break;
                    }
                }
                j++;
                effectFlags >>= 1;
                if (i == effectByte)
                    effectBit >>= 1;
            }
            break;
        }
    }

    return offset;
}

static void BufferStatRoseMessage(s32 arg0)
{
    gBattlerTarget = gBattlerInMenuId;
    StringCopy(gBattleTextBuff1, gStatNamesTable[sStatsToRaise[arg0]]);
    if (B_X_ITEMS_BUFF == GEN_7)
    {
        StringCopy(gBattleTextBuff2, gText_StatSharply);
        StringAppend(gBattleTextBuff2, gText_StatRose);
    }
    else
    {
        StringCopy(gBattleTextBuff2, gText_StatRose);
    }
    BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnsStatChanged2);
}

u8 *UseStatIncreaseItem(u16 itemId)
{
    int i;
    const u8 *itemEffect;

    if (itemId == ITEM_ENIGMA_BERRY)
    {
        if (gMain.inBattle)
            itemEffect = gEnigmaBerries[gBattlerInMenuId].itemEffect;
        else
            itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
    }
    else
    {
        itemEffect = gItemEffectTable[itemId - ITEM_POTION];
    }

    gPotentialItemEffectBattler = gBattlerInMenuId;

#ifndef ITEM_EXPANSION
    for (i = 0; i < 3; i++)
    {
        if (itemEffect[i] & (ITEM0_X_ATTACK | ITEM1_X_SPEED | ITEM2_X_SPATK))
            BufferStatRoseMessage(i * 2);

        if (itemEffect[i] & (ITEM0_DIRE_HIT | ITEM1_X_DEFEND | ITEM2_X_ACCURACY))
        {
            if (i != 0) // Dire Hit is the only ITEM0 above
            {
                BufferStatRoseMessage(i * 2 + 1);
            }
            else
            {
                gBattlerAttacker = gBattlerInMenuId;
                BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnGettingPumped);
            }
        }
    }

    if (itemEffect[3] & ITEM3_GUARD_SPEC)
    {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnShroudedInMist);
    }
#else
    if (itemEffect[0] & ITEM0_DIRE_HIT)
    {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnGettingPumped);
    }

    switch (itemEffect[1])
    {
        case ITEM1_X_ATTACK:
            BufferStatRoseMessage(STAT_ATK);
            break;
        case ITEM1_X_DEFENSE:
            BufferStatRoseMessage(STAT_DEF);
            break;
        case ITEM1_X_SPEED:
            BufferStatRoseMessage(STAT_SPEED);
            break;
        case ITEM1_X_SPATK:
            BufferStatRoseMessage(STAT_SPATK);
            break;
        case ITEM1_X_SPDEF:
            BufferStatRoseMessage(STAT_SPDEF);
            break;
        case ITEM1_X_ACCURACY:
            BufferStatRoseMessage(STAT_ACC);
            break;
    }

    if (itemEffect[3] & ITEM3_GUARD_SPEC)
    {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnShroudedInMist);
    }
#endif

    return gDisplayedStringBattle;
}

u8 GetNature(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_PERSONALITY, 0) % NUM_NATURES;
}

u8 GetNatureFromPersonality(u32 personality)
{
    return personality % NUM_NATURES;
}

u16 GetEvolutionTargetSpecies(struct Pokemon *mon, u8 mode, u16 evolutionItem, u16 tradePartnerSpecies)
{
    int i, j;
    u16 targetSpecies = 0;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u8 level;
    u16 friendship;
    u8 beauty = GetMonData(mon, MON_DATA_BEAUTY, 0);
    u16 upperPersonality = personality >> 16;
    u8 holdEffect;
    u16 currentMap;

    if (heldItem == ITEM_ENIGMA_BERRY)
        holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    else
        holdEffect = ItemId_GetHoldEffect(heldItem);

    // Prevent evolution with Everstone, unless we're just viewing the party menu with an evolution item
    if (holdEffect == HOLD_EFFECT_PREVENT_EVOLVE && mode != EVO_MODE_ITEM_CHECK)
        return SPECIES_NONE;

    switch (mode)
    {
    case EVO_MODE_NORMAL:
        level = GetMonData(mon, MON_DATA_LEVEL, 0);
        friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);

        for (i = 0; i < EVOS_PER_MON; i++)
        {
            switch (gEvolutionTable[species][i].method)
            {
            case EVO_FRIENDSHIP:
                if (friendship >= 220)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_FRIENDSHIP_DAY:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 12 && gLocalTime.hours < 24 && friendship >= 220)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_DAY:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 12 && gLocalTime.hours < 24 && gEvolutionTable[species][i].param <= level)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_FRIENDSHIP_NIGHT:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 0 && gLocalTime.hours < 12 && friendship >= 220)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_NIGHT:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 0 && gLocalTime.hours < 12 && gEvolutionTable[species][i].param <= level)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_ITEM_HOLD_NIGHT:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 0 && gLocalTime.hours < 12 && heldItem == gEvolutionTable[species][i].param)
                {
                    heldItem = 0;
                    SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                }
                break;
            case EVO_ITEM_HOLD_DAY:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 12 && gLocalTime.hours < 24 && heldItem == gEvolutionTable[species][i].param)
                {
                    heldItem = 0;
                    SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                }
                break;
            case EVO_LEVEL_DUSK:
                RtcCalcLocalTime();
                if (gLocalTime.hours >= 17 && gLocalTime.hours < 18 && gEvolutionTable[species][i].param <= level)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL:
                if (gEvolutionTable[species][i].param <= level)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_FEMALE:
                if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_FEMALE)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_MALE:
                if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_MALE)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_ATK_GT_DEF:
                if (gEvolutionTable[species][i].param <= level)
                    if (GetMonData(mon, MON_DATA_ATK, 0) > GetMonData(mon, MON_DATA_DEF, 0))
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_ATK_EQ_DEF:
                if (gEvolutionTable[species][i].param <= level)
                    if (GetMonData(mon, MON_DATA_ATK, 0) == GetMonData(mon, MON_DATA_DEF, 0))
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_ATK_LT_DEF:
                if (gEvolutionTable[species][i].param <= level)
                    if (GetMonData(mon, MON_DATA_ATK, 0) < GetMonData(mon, MON_DATA_DEF, 0))
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_SILCOON:
                if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) <= 4)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_CASCOON:
                if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) > 4)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_LEVEL_NINJASK:
                if (gEvolutionTable[species][i].param <= level)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_BEAUTY:
                if (gEvolutionTable[species][i].param <= beauty)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_MOVE:
                if (MonKnowsMove(mon, gEvolutionTable[species][i].param))
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_MOVE_TYPE:
                for (j = 0; j < 4; j++)
                {
                    if (gBattleMoves[GetMonData(mon, MON_DATA_MOVE1 + j, NULL)].type == gEvolutionTable[species][i].param)
                    {
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    }
                }
                break;
            case EVO_SPECIFIC_MON_IN_PARTY:
                for (j = 0; j < PARTY_SIZE; j++)
                {
                    if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) == gEvolutionTable[species][i].param)
                    {
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    }
                }
                break;
            case EVO_LEVEL_DARK_TYPE_MON_IN_PARTY:
                if (gEvolutionTable[species][i].param <= level)
                {
                    for (j = 0; j < PARTY_SIZE; j++)
                    {
                        u16 species = GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL);
                        if (gBaseStats[species].type1 == TYPE_DARK
                            || gBaseStats[species].type2 == TYPE_DARK)
                        {
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                            break;
                        }
                    }
                }
                break;
            case EVO_LEVEL_RAIN:
                j = GetCurrentWeather();
                if (j == WEATHER_RAIN || j == WEATHER_RAIN_THUNDERSTORM || j == WEATHER_DOWNPOUR)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_MAPSEC:
                if (gMapHeader.regionMapSectionId == gEvolutionTable[species][i].param)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_SPECIFIC_MAP:
                currentMap = ((gSaveBlock1Ptr->location.mapGroup) << 8 | gSaveBlock1Ptr->location.mapNum);
                if (currentMap == gEvolutionTable[species][i].param)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            }
        }
        break;
    case EVO_MODE_TRADE:
        for (i = 0; i < EVOS_PER_MON; i++)
        {
            switch (gEvolutionTable[species][i].method)
            {
            case EVO_TRADE:
                targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_TRADE_ITEM:
                if (gEvolutionTable[species][i].param == heldItem)
                {
                    heldItem = 0;
                    SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                }
                break;
            case EVO_TRADE_SPECIFIC_MON:
                if (gEvolutionTable[species][i].param == tradePartnerSpecies)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            }
        }
        break;
    case EVO_MODE_ITEM_USE:
    case EVO_MODE_ITEM_CHECK:
        for (i = 0; i < EVOS_PER_MON; i++)
        {
            switch (gEvolutionTable[species][i].method)
            {
            case EVO_ITEM:
                if (gEvolutionTable[species][i].param == evolutionItem)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_ITEM_FEMALE:
                if (GetMonGender(mon) == MON_FEMALE && gEvolutionTable[species][i].param == evolutionItem)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            case EVO_ITEM_MALE:
                if (GetMonGender(mon) == MON_MALE && gEvolutionTable[species][i].param == evolutionItem)
                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                break;
            }
        }
        break;
    }

    return targetSpecies;
}

u16 HoennPokedexNumToSpecies(u16 hoennNum)
{
    u16 species;

    if (!hoennNum)
        return 0;

    species = 0;

    while (species < (NUM_SPECIES - 1) && gSpeciesToHoennPokedexNum[species] != hoennNum)
        species++;

    if (species == NUM_SPECIES - 1)
        return 0;

    return species + 1;
}

u16 NationalPokedexNumToSpecies(u16 nationalNum)
{
    u16 species;

    if (!nationalNum)
        return 0;

    species = 0;

    while (species < (NUM_SPECIES - 1) && gSpeciesToNationalPokedexNum[species] != nationalNum)
        species++;

    if (species == NUM_SPECIES - 1)
        return 0;

    return species + 1;
}

u16 NationalToHoennOrder(u16 nationalNum)
{
    u16 hoennNum;

    if (!nationalNum)
        return 0;

    hoennNum = 0;

    while (hoennNum < (HOENN_DEX_COUNT - 1) && gHoennToNationalOrder[hoennNum] != nationalNum)
        hoennNum++;

    if (hoennNum >= HOENN_DEX_COUNT - 1)
        return 0;

    return hoennNum + 1;
}

u16 SpeciesToNationalPokedexNum(u16 species)
{
    if (!species)
        return 0;

    return gSpeciesToNationalPokedexNum[species - 1];
}

u16 SpeciesToHoennPokedexNum(u16 species)
{
    if (!species)
        return 0;

    return gSpeciesToHoennPokedexNum[species - 1];
}

u16 HoennToNationalOrder(u16 hoennNum)
{
    if (!hoennNum || hoennNum >= HOENN_DEX_COUNT)
        return 0;

    return gHoennToNationalOrder[hoennNum - 1];
}

#define DRAW_SPINDA_SPOTS                                                       \
{                                                                               \
    int i;                                                                      \
    for (i = 0; i < 4; i++)                                                     \
    {                                                                           \
        int j;                                                                  \
        u8 x = gSpindaSpotGraphics[i].x + ((personality & 0x0F) - 8);           \
        u8 y = gSpindaSpotGraphics[i].y + (((personality & 0xF0) >> 4) - 8);    \
                                                                                \
        for (j = 0; j < 16; j++)                                                \
        {                                                                       \
            int k;                                                              \
            s32 row = gSpindaSpotGraphics[i].image[j];                          \
                                                                                \
            for (k = x; k < x + 16; k++)                                        \
            {                                                                   \
                u8 *val = dest + ((k / 8) * 32) +                               \
                                 ((k % 8) / 2) +                                \
                                 ((y >> 3) << 8) +                              \
                                 ((y & 7) << 2);                                \
                                                                                \
                if (row & 1)                                                    \
                {                                                               \
                    if (k & 1)                                                  \
                    {                                                           \
                        if ((u8)((*val & 0xF0) - 0x10) <= 0x20)                 \
                            *val += 0x40;                                       \
                    }                                                           \
                    else                                                        \
                    {                                                           \
                        if ((u8)((*val & 0xF) - 0x01) <= 0x02)                  \
                            *val += 0x04;                                       \
                    }                                                           \
                }                                                               \
                                                                                \
                row >>= 1;                                                      \
            }                                                                   \
                                                                                \
            y++;                                                                \
        }                                                                       \
                                                                                \
        personality >>= 8;                                                      \
    }                                                                           \
}

static void DrawSpindaSpotsUnused(u16 species, u32 personality, u8 *dest)
{
    if (species == SPECIES_SPINDA
        && dest != gMonSpritesGfxPtr->sprites.ptr[0]
        && dest != gMonSpritesGfxPtr->sprites.ptr[2])
        DRAW_SPINDA_SPOTS;
}

void DrawSpindaSpots(u16 species, u32 personality, u8 *dest, bool8 isFrontPic)
{
    if (species == SPECIES_SPINDA && isFrontPic)
        DRAW_SPINDA_SPOTS;
}

void EvolutionRenameMon(struct Pokemon *mon, u16 oldSpecies, u16 newSpecies)
{
    u8 language;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    language = GetMonData(mon, MON_DATA_LANGUAGE, &language);
    if (language == GAME_LANGUAGE && !StringCompare(gSpeciesNames[oldSpecies], gStringVar1))
        SetMonData(mon, MON_DATA_NICKNAME, gSpeciesNames[newSpecies]);
}

// The below two functions determine which side of a multi battle the trainer battles on
// 0 is the left (top in  party menu), 1 is right (bottom in party menu)
u8 GetPlayerFlankId(void)
{
    u8 flankId = 0;
    switch (gLinkPlayers[GetMultiplayerId()].id)
    {
    case 0:
    case 3:
        flankId = 0;
        break;
    case 1:
    case 2:
        flankId = 1;
        break;
    }
    return flankId;
}

u16 GetLinkTrainerFlankId(u8 linkPlayerId)
{
    u16 flankId = 0;
    switch (gLinkPlayers[linkPlayerId].id)
    {
    case 0:
    case 3:
        flankId = 0;
        break;
    case 1:
    case 2:
        flankId = 1;
        break;
    }
    return flankId;
}

s32 GetBattlerMultiplayerId(u16 a1)
{
    s32 id;
    for (id = 0; id < MAX_LINK_PLAYERS; id++)
        if (gLinkPlayers[id].id == a1)
            break;
    return id;
}

u8 GetTrainerEncounterMusicId(u16 trainerOpponentId)
{
    if (InBattlePyramid())
        return GetBattlePyramindTrainerEncounterMusicId(trainerOpponentId);
    else if (InTrainerHillChallenge())
        return GetTrainerEncounterMusicIdInTrainerHill(trainerOpponentId);
    else
        return TRAINER_ENCOUNTER_MUSIC(trainerOpponentId);
}

u16 ModifyStatByNature(u8 nature, u16 n, u8 statIndex)
{
    u16 retVal;
    // Don't modify HP, Accuracy, or Evasion by nature
    if (statIndex <= STAT_HP || statIndex > NUM_NATURE_STATS)
    {
        return n;
    }

    switch (gNatureStatTable[nature][statIndex - 1])
    {
    case 1:
        retVal = n * 110;
        retVal /= 100;
        break;
    case -1:
        retVal = n * 90;
        retVal /= 100;
        break;
    default:
        retVal = n;
        break;
    }

    return retVal;
}

#define IS_LEAGUE_BATTLE                                                                \
    ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)                                           \
    && (gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_ELITE_FOUR    \
     || gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_LEADER        \
     || gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_CHAMPION))    \

void AdjustFriendship(struct Pokemon *mon, u8 event)
{
    u16 species, heldItem;
    u8 holdEffect;

    if (ShouldSkipFriendshipChange())
        return;

    species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);

    if (heldItem == ITEM_ENIGMA_BERRY)
    {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[0].holdEffect;
        else
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(heldItem);
    }

    if (species && species != SPECIES_EGG)
    {
        u8 friendshipLevel = 0;
        s16 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);

        if (friendship > 99)
            friendshipLevel++;
        if (friendship > 199)
            friendshipLevel++;

        if ((event != FRIENDSHIP_EVENT_WALKING || !(Random() & 1))
         && (event != FRIENDSHIP_EVENT_LEAGUE_BATTLE || IS_LEAGUE_BATTLE))
        {
            s8 mod = sFriendshipEventModifiers[event][friendshipLevel];
            if (mod > 0 && holdEffect == HOLD_EFFECT_FRIENDSHIP_UP)
                mod = (150 * mod) / 100;
            friendship += mod;
            if (mod > 0)
            {
                if (GetMonData(mon, MON_DATA_POKEBALL, 0) == ITEM_LUXURY_BALL)
                    friendship++;
                if (GetMonData(mon, MON_DATA_MET_LOCATION, 0) == GetCurrentRegionMapSectionId())
                    friendship++;
            }
            if (friendship < 0)
                friendship = 0;
            if (friendship > MAX_FRIENDSHIP)
                friendship = MAX_FRIENDSHIP;
            SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
        }
    }
}

void MonGainEVs(struct Pokemon *mon, u16 defeatedSpecies)
{
    u8 evs[NUM_STATS];
    u16 evIncrease = 0;
    u16 totalEVs = 0;
    u16 heldItem;
    u8 holdEffect;
    int i, multiplier;
    u8 stat;
    u8 bonus;

    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);
    if (heldItem == ITEM_ENIGMA_BERRY)
    {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[0].holdEffect;
        else
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(heldItem);
    }

    stat = ItemId_GetSecondaryId(heldItem);
    bonus = ItemId_GetHoldEffectParam(heldItem);

    for (i = 0; i < NUM_STATS; i++)
    {
        evs[i] = GetMonData(mon, MON_DATA_HP_EV + i, 0);
        totalEVs += evs[i];
    }

    for (i = 0; i < NUM_STATS; i++)
    {
        if (totalEVs >= MAX_TOTAL_EVS)
            break;

        if (CheckPartyHasHadPokerus(mon, 0))
            multiplier = 2;
        else
            multiplier = 1;

        switch (i)
        {
        case STAT_HP:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_HP)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_HP + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_HP * multiplier;
            break;
        case STAT_ATK:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_ATK)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_Attack + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_Attack * multiplier;
            break;
        case STAT_DEF:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_DEF)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_Defense + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_Defense * multiplier;
            break;
        case STAT_SPEED:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPEED)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_Speed + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_Speed * multiplier;
            break;
        case STAT_SPATK:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPATK)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_SpAttack + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_SpAttack * multiplier;
            break;
        case STAT_SPDEF:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPDEF)
                evIncrease = (gBaseStats[defeatedSpecies].evYield_SpDefense + bonus) * multiplier;
            else
                evIncrease = gBaseStats[defeatedSpecies].evYield_SpDefense * multiplier;
            break;
        }

        if (holdEffect == HOLD_EFFECT_MACHO_BRACE)
            evIncrease *= 2;

        if (totalEVs + (s16)evIncrease > MAX_TOTAL_EVS)
            evIncrease = ((s16)evIncrease + MAX_TOTAL_EVS) - (totalEVs + evIncrease);

        if (evs[i] + (s16)evIncrease > MAX_PER_STAT_EVS)
        {
            int val1 = (s16)evIncrease + MAX_PER_STAT_EVS;
            int val2 = evs[i] + evIncrease;
            evIncrease = val1 - val2;
        }

        evs[i] += evIncrease;
        totalEVs += evIncrease;
        SetMonData(mon, MON_DATA_HP_EV + i, &evs[i]);
    }
}

u16 GetMonEVCount(struct Pokemon *mon)
{
    int i;
    u16 count = 0;

    for (i = 0; i < NUM_STATS; i++)
        count += GetMonData(mon, MON_DATA_HP_EV + i, 0);

    return count;
}

void RandomlyGivePartyPokerus(struct Pokemon *party)
{
    u16 rnd = Random();
    if (rnd == 0x4000 || rnd == 0x8000 || rnd == 0xC000)
    {
        struct Pokemon *mon;

        do
        {
            do
            {
                rnd = Random() % PARTY_SIZE;
                mon = &party[rnd];
            }
            while (!GetMonData(mon, MON_DATA_SPECIES, 0));
        }
        while (GetMonData(mon, MON_DATA_IS_EGG, 0));

        if (!(CheckPartyHasHadPokerus(party, gBitTable[rnd])))
        {
            u8 rnd2;

            do
            {
                rnd2 = Random();
            }
            while ((rnd2 & 0x7) == 0);

            if (rnd2 & 0xF0)
                rnd2 &= 0x7;

            rnd2 |= (rnd2 << 4);
            rnd2 &= 0xF3;
            rnd2++;

            SetMonData(&party[rnd], MON_DATA_POKERUS, &rnd2);
        }
    }
}

u8 CheckPartyPokerus(struct Pokemon *party, u8 selection)
{
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection)
    {
        do
        {
            if ((selection & 1) && (GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0) & 0xF))
                retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        }
        while (selection);
    }
    else if (GetMonData(&party[0], MON_DATA_POKERUS, 0) & 0xF)
    {
        retVal = 1;
    }

    return retVal;
}

u8 CheckPartyHasHadPokerus(struct Pokemon *party, u8 selection)
{
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection)
    {
        do
        {
            if ((selection & 1) && GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0))
                retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        }
        while (selection);
    }
    else if (GetMonData(&party[0], MON_DATA_POKERUS, 0))
    {
        retVal = 1;
    }

    return retVal;
}

void UpdatePartyPokerusTime(u16 days)
{
    int i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, 0))
        {
            u8 pokerus = GetMonData(&gPlayerParty[i], MON_DATA_POKERUS, 0);
            if (pokerus & 0xF)
            {
                if ((pokerus & 0xF) < days || days > 4)
                    pokerus &= 0xF0;
                else
                    pokerus -= days;

                if (pokerus == 0)
                    pokerus = 0x10;

                SetMonData(&gPlayerParty[i], MON_DATA_POKERUS, &pokerus);
            }
        }
    }
}

void PartySpreadPokerus(struct Pokemon *party)
{
    if ((Random() % 3) == 0)
    {
        int i;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES, 0))
            {
                u8 pokerus = GetMonData(&party[i], MON_DATA_POKERUS, 0);
                u8 curPokerus = pokerus;
                if (pokerus)
                {
                    if (pokerus & 0xF)
                    {
                        // Spread to adjacent party members.
                        if (i != 0 && !(GetMonData(&party[i - 1], MON_DATA_POKERUS, 0) & 0xF0))
                            SetMonData(&party[i - 1], MON_DATA_POKERUS, &curPokerus);
                        if (i != (PARTY_SIZE - 1) && !(GetMonData(&party[i + 1], MON_DATA_POKERUS, 0) & 0xF0))
                        {
                            SetMonData(&party[i + 1], MON_DATA_POKERUS, &curPokerus);
                            i++;
                        }
                    }
                }
            }
        }
    }
}

bool8 TryIncrementMonLevel(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 nextLevel = GetMonData(mon, MON_DATA_LEVEL, 0) + 1;
    u32 expPoints = GetMonData(mon, MON_DATA_EXP, 0);
    if (expPoints > gExperienceTables[gBaseStats[species].growthRate][MAX_LEVEL])
    {
        expPoints = gExperienceTables[gBaseStats[species].growthRate][MAX_LEVEL];
        SetMonData(mon, MON_DATA_EXP, &expPoints);
    }
    if (nextLevel > MAX_LEVEL || expPoints < gExperienceTables[gBaseStats[species].growthRate][nextLevel])
    {
        return FALSE;
    }
    else
    {
        SetMonData(mon, MON_DATA_LEVEL, &nextLevel);
        return TRUE;
    }
}

u32 CanMonLearnTMHM(struct Pokemon *mon, u8 tm)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    if (species == SPECIES_EGG)
    {
        return 0;
    }
    else if (tm < 32)
    {
        u32 mask = 1 << tm;
        return gTMHMLearnsets[species][0] & mask;
    }
    else
    {
        u32 mask = 1 << (tm - 32);
        return gTMHMLearnsets[species][1] & mask;
    }
}

u32 CanSpeciesLearnTMHM(u16 species, u8 tm)
{
    if (species == SPECIES_EGG)
    {
        return 0;
    }
    else if (tm < 32)
    {
        u32 mask = 1 << tm;
        return gTMHMLearnsets[species][0] & mask;
    }
    else
    {
        u32 mask = 1 << (tm - 32);
        return gTMHMLearnsets[species][1] & mask;
    }
}

u8 GetMoveRelearnerMoves(struct Pokemon *mon, u16 *moves)
{
    u16 learnedMoves[4];
    u8 numMoves = 0;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    int i, j, k;

    for (i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (i = 0; i < MAX_LEVEL_UP_MOVES; i++)
    {
        u16 moveLevel;

        if (gLevelUpLearnsets[species][i].move == LEVEL_UP_END)
            break;

        moveLevel = gLevelUpLearnsets[species][i].level;

        if (moveLevel <= level)
        {
            for (j = 0; j < MAX_MON_MOVES && learnedMoves[j] != gLevelUpLearnsets[species][i].move; j++)
                ;

            if (j == MAX_MON_MOVES)
            {
                for (k = 0; k < numMoves && moves[k] != gLevelUpLearnsets[species][i].move; k++)
                    ;

                if (k == numMoves)
                    moves[numMoves++] = gLevelUpLearnsets[species][i].move;
            }
        }
    }

    return numMoves;
}

u8 GetLevelUpMovesBySpecies(u16 species, u16 *moves)
{
    u8 numMoves = 0;
    int i;

    for (i = 0; i < MAX_LEVEL_UP_MOVES && gLevelUpLearnsets[species][i].move != LEVEL_UP_END; i++)
         moves[numMoves++] = gLevelUpLearnsets[species][i].move;

     return numMoves;
}

u8 GetNumberOfRelearnableMoves(struct Pokemon *mon)
{
    u16 learnedMoves[MAX_MON_MOVES];
    u16 moves[MAX_LEVEL_UP_MOVES];
    u8 numMoves = 0;
    u16 species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    int i, j, k;

    if (species == SPECIES_EGG)
        return 0;

    for (i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (i = 0; i < MAX_LEVEL_UP_MOVES; i++)
    {
        u16 moveLevel;

        if (gLevelUpLearnsets[species][i].move == LEVEL_UP_END)
            break;

        moveLevel = gLevelUpLearnsets[species][i].level;

        if (moveLevel <= level)
        {
            for (j = 0; j < MAX_MON_MOVES && learnedMoves[j] != gLevelUpLearnsets[species][i].move; j++)
                ;

            if (j == MAX_MON_MOVES)
            {
                for (k = 0; k < numMoves && moves[k] != gLevelUpLearnsets[species][i].move; k++)
                    ;

                if (k == numMoves)
                    moves[numMoves++] = gLevelUpLearnsets[species][i].move;
            }
        }
    }

    return numMoves;
}

u16 SpeciesToPokedexNum(u16 species)
{
    if (IsNationalPokedexEnabled())
    {
        return SpeciesToNationalPokedexNum(species);
    }
    else
    {
        species = SpeciesToHoennPokedexNum(species);
        if (species <= HOENN_DEX_COUNT)
            return species;
        return 0xFFFF;
    }
}

bool32 IsSpeciesInHoennDex(u16 species)
{
    if (SpeciesToHoennPokedexNum(species) > HOENN_DEX_COUNT)
        return FALSE;
    else
        return TRUE;
}

void ClearBattleMonForms(void)
{
    int i;
    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        gBattleMonForms[i] = 0;
}

u16 GetBattleBGM(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_KYOGRE_GROUDON)
        return MUS_VS_KYOGRE_GROUDON;
    else if (gBattleTypeFlags & BATTLE_TYPE_REGI)
        return MUS_VS_REGI;
    else if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        return MUS_VS_TRAINER;
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        u8 trainerClass;

        if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
            trainerClass = GetFrontierOpponentClass(gTrainerBattleOpponent_A);
        else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
            trainerClass = TRAINER_CLASS_EXPERT;
        else
            trainerClass = gTrainers[gTrainerBattleOpponent_A].trainerClass;

        switch (trainerClass)
        {
        case TRAINER_CLASS_AQUA_LEADER:
        case TRAINER_CLASS_MAGMA_LEADER:
            return MUS_VS_AQUA_MAGMA_LEADER;
        case TRAINER_CLASS_TEAM_AQUA:
        case TRAINER_CLASS_TEAM_MAGMA:
        case TRAINER_CLASS_AQUA_ADMIN:
        case TRAINER_CLASS_MAGMA_ADMIN:
            return MUS_VS_AQUA_MAGMA;
        case TRAINER_CLASS_LEADER:
            return MUS_VS_GYM_LEADER;
        case TRAINER_CLASS_CHAMPION:
            return MUS_VS_CHAMPION;
        case TRAINER_CLASS_PKMN_TRAINER_3:
            if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                return MUS_VS_RIVAL;
            if (!StringCompare(gTrainers[gTrainerBattleOpponent_A].trainerName, gText_BattleWallyName))
                return MUS_VS_TRAINER;
            return MUS_VS_RIVAL;
        case TRAINER_CLASS_ELITE_FOUR:
            return MUS_VS_ELITE_FOUR;
        case TRAINER_CLASS_SALON_MAIDEN:
        case TRAINER_CLASS_DOME_ACE:
        case TRAINER_CLASS_PALACE_MAVEN:
        case TRAINER_CLASS_ARENA_TYCOON:
        case TRAINER_CLASS_FACTORY_HEAD:
        case TRAINER_CLASS_PIKE_QUEEN:
        case TRAINER_CLASS_PYRAMID_KING:
            return MUS_VS_FRONTIER_BRAIN;
        default:
            return MUS_VS_TRAINER;
        }
    }
    else
        return MUS_VS_WILD;
}

void PlayBattleBGM(void)
{
    ResetMapMusic();
    m4aMPlayAllStop();
    PlayBGM(GetBattleBGM());
}

void PlayMapChosenOrBattleBGM(u16 songId)
{
    ResetMapMusic();
    m4aMPlayAllStop();
    if (songId)
        PlayNewMapMusic(songId);
    else
        PlayNewMapMusic(GetBattleBGM());
}

// Identical to PlayMapChosenOrBattleBGM, but uses a task instead
// Only used by Battle Dome
#define tSongId data[0]
void CreateTask_PlayMapChosenOrBattleBGM(u16 songId)
{
    u8 taskId;

    ResetMapMusic();
    m4aMPlayAllStop();

    taskId = CreateTask(Task_PlayMapChosenOrBattleBGM, 0);
    gTasks[taskId].tSongId = songId;
}

static void Task_PlayMapChosenOrBattleBGM(u8 taskId)
{
    if (gTasks[taskId].tSongId)
        PlayNewMapMusic(gTasks[taskId].tSongId);
    else
        PlayNewMapMusic(GetBattleBGM());
    DestroyTask(taskId);
}

#undef tSongId

const u32 *GetMonFrontSpritePal(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u32 otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    return GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality);
}

const u32 *GetMonSpritePalFromSpeciesAndPersonality(u16 species, u32 otId, u32 personality)
{
    u32 shinyValue;

    if (species > NUM_SPECIES)
        return gMonPaletteTable[0].data;

    shinyValue = HIHALF(otId) ^ LOHALF(otId) ^ HIHALF(personality) ^ LOHALF(personality);
    if (shinyValue < SHINY_ODDS)
    {
        if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonShinyPaletteTableFemale[species].data;
        else
            return gMonShinyPaletteTable[species].data;
    }
    else
    {
        if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonPaletteTableFemale[species].data;
        else
            return gMonPaletteTable[species].data;
    }
}

const struct CompressedSpritePalette *GetMonSpritePalStruct(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u32 otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    return GetMonSpritePalStructFromOtIdPersonality(species, otId, personality);
}

const struct CompressedSpritePalette *GetMonSpritePalStructFromOtIdPersonality(u16 species, u32 otId , u32 personality)
{
    u32 shinyValue;

    shinyValue = HIHALF(otId) ^ LOHALF(otId) ^ HIHALF(personality) ^ LOHALF(personality);
    if (shinyValue < SHINY_ODDS)
    {
        if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return &gMonShinyPaletteTableFemale[species];
        else
            return &gMonShinyPaletteTable[species];
    }
    else
    {
        if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return &gMonPaletteTableFemale[species];
        else
            return &gMonPaletteTable[species];
    }
}

bool32 IsHMMove2(u16 move)
{
    int i = 0;
    while (sHMMoves[i] != 0xFFFF)
    {
        if (sHMMoves[i++] == move)
            return TRUE;
    }
    return FALSE;
}

bool8 IsMonSpriteNotFlipped(u16 species)
{
    return gBaseStats[species].noFlip;
}

s8 GetMonFlavorRelation(struct Pokemon *mon, u8 flavor)
{
    u8 nature = GetNature(mon);
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

s8 GetFlavorRelationByPersonality(u32 personality, u8 flavor)
{
    u8 nature = GetNatureFromPersonality(personality);
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

bool8 IsTradedMon(struct Pokemon *mon)
{
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 otId;
    GetMonData(mon, MON_DATA_OT_NAME, otName);
    otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    return IsOtherTrainer(otId, otName);
}

bool8 IsOtherTrainer(u32 otId, u8 *otName)
{
    if (otId ==
        (gSaveBlock2Ptr->playerTrainerId[0]
         | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
         | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
         | (gSaveBlock2Ptr->playerTrainerId[3] << 24)))
    {
        int i;

        for (i = 0; otName[i] != EOS; i++)
            if (otName[i] != gSaveBlock2Ptr->playerName[i])
                return TRUE;
        return FALSE;
    }

    return TRUE;
}

void MonRestorePP(struct Pokemon *mon)
{
    BoxMonRestorePP(&mon->box);
}

void BoxMonRestorePP(struct BoxPokemon *boxMon)
{
    int i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, 0))
        {
            u16 move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, 0);
            u16 bonus = GetBoxMonData(boxMon, MON_DATA_PP_BONUSES, 0);
            u8 pp = CalculatePPWithBonus(move, bonus, i);
            SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp);
        }
    }
}

void SetMonPreventsSwitchingString(void)
{
    gLastUsedAbility = gBattleStruct->abilityPreventingSwitchout;

    gBattleTextBuff1[0] = B_BUFF_PLACEHOLDER_BEGIN;
    gBattleTextBuff1[1] = B_BUFF_MON_NICK_WITH_PREFIX;
    gBattleTextBuff1[2] = gBattleStruct->battlerPreventingSwitchout;
    gBattleTextBuff1[4] = B_BUFF_EOS;

    if (GetBattlerSide(gBattleStruct->battlerPreventingSwitchout) == B_SIDE_PLAYER)
        gBattleTextBuff1[3] = GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout]);
    else
        gBattleTextBuff1[3] = gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout];

    PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff2, gBattlerInMenuId, GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattlerInMenuId]))

    BattleStringExpandPlaceholders(gText_PkmnsXPreventsSwitching, gStringVar4);
}

static s32 GetWildMonTableIdInAlteringCave(u16 species)
{
    s32 i;
    for (i = 0; i < (s32) ARRAY_COUNT(sAlteringCaveWildMonHeldItems); i++)
        if (sAlteringCaveWildMonHeldItems[i].species == species)
            return i;
    return 0;
}

void SetWildMonHeldItem(void)
{
    u16 rnd, species, var1, var2, i, count;
    if (gBattleTypeFlags & (BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_TRAINER | BATTLE_TYPE_PYRAMID | BATTLE_TYPE_PIKE))
        return;

    count = (WILD_DOUBLE_BATTLE) ? 2 : 1;
    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG, 0)
        && (GetMonAbility(&gPlayerParty[0]) == ABILITY_COMPOUND_EYES
            || GetMonAbility(&gPlayerParty[0]) == ABILITY_SUPER_LUCK))
    {
        var1 = 20;
        var2 = 80;
    }
    else
    {
        var1 = 45;
        var2 = 95;
    }

    for (i = 0; i < count; i++)
    {
        rnd = Random() % 100;
        species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, 0);
        if (gMapHeader.mapLayoutId == LAYOUT_ALTERING_CAVE)
        {
            s32 alteringCaveId = GetWildMonTableIdInAlteringCave(species);
            if (alteringCaveId != 0)
            {
                if (rnd < var2)
                    continue;
                SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &sAlteringCaveWildMonHeldItems[alteringCaveId].item);
            }
            else
            {
                if (rnd < var1)
                    continue;
                if (rnd < var2)
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
                else
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item2);
            }
        }
        else
        {
            if (gBaseStats[species].item1 == gBaseStats[species].item2 && gBaseStats[species].item1 != 0)
            {
                SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
            }
            else
            {
                if (rnd < var1)
                    continue;
                if (rnd < var2)
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
                else
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item2);
            }
        }
    }
}

bool8 IsMonShiny(struct Pokemon *mon)
{
    u32 otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    return IsShinyOtIdPersonality(otId, personality);
}

bool8 IsShinyOtIdPersonality(u32 otId, u32 personality)
{
    bool8 retVal = FALSE;
    u32 shinyValue = HIHALF(otId) ^ LOHALF(otId) ^ HIHALF(personality) ^ LOHALF(personality);
    if (shinyValue < SHINY_ODDS)
        retVal = TRUE;
    return retVal;
}

const u8 *GetTrainerPartnerName(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        if (gPartnerTrainerId == TRAINER_STEVEN_PARTNER)
        {
            return gTrainers[TRAINER_STEVEN].trainerName;
        }
        else
        {
            GetFrontierTrainerName(gStringVar1, gPartnerTrainerId);
            return gStringVar1;
        }
    }
    else
    {
        u8 id = GetMultiplayerId();
        return gLinkPlayers[GetBattlerMultiplayerId(gLinkPlayers[id].id ^ 2)].name;
    }
}

#define READ_PTR_FROM_TASK(taskId, dataId)                      \
    (void*)(                                                    \
    ((u16)(gTasks[taskId].data[dataId]) |                       \
    ((u16)(gTasks[taskId].data[dataId + 1]) << 16)))

#define STORE_PTR_IN_TASK(ptr, taskId, dataId)                 \
{                                                              \
    gTasks[taskId].data[dataId] = (u32)(ptr);                  \
    gTasks[taskId].data[dataId + 1] = (u32)(ptr) >> 16;        \
}

static void Task_AnimateAfterDelay(u8 taskId)
{
    if (--gTasks[taskId].data[3] == 0)
    {
        LaunchAnimationTaskForFrontSprite(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].data[2]);
        DestroyTask(taskId);
    }
}

static void Task_PokemonSummaryAnimateAfterDelay(u8 taskId)
{
    if (--gTasks[taskId].data[3] == 0)
    {
        StartMonSummaryAnimation(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].data[2]);
        SummaryScreen_SetUnknownTaskId(0xFF);
        DestroyTask(taskId);
    }
}

void BattleAnimateFrontSprite(struct Sprite* sprite, u16 species, bool8 noCry, u8 arg3)
{
    if (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
        DoMonFrontSpriteAnimation(sprite, species, noCry, arg3 | 0x80);
    else
        DoMonFrontSpriteAnimation(sprite, species, noCry, arg3);
}

void DoMonFrontSpriteAnimation(struct Sprite* sprite, u16 species, bool8 noCry, u8 arg3)
{
    s8 pan;
    switch (arg3 & 0x7F)
    {
    case 0:
        pan = -25;
        break;
    case 1:
        pan = 25;
        break;
    default:
        pan = 0;
        break;
    }
    if (arg3 & 0x80)
    {
        if (!noCry)
            PlayCry1(species, pan);
        sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (!noCry)
        {
            PlayCry1(species, pan);
            if (HasTwoFramesAnimation(species))
                StartSpriteAnim(sprite, 1);
        }
        if (gMonAnimationDelayTable[species - 1] != 0)
        {
            u8 taskId = CreateTask(Task_AnimateAfterDelay, 0);
            STORE_PTR_IN_TASK(sprite, taskId, 0);
            gTasks[taskId].data[2] = gMonFrontAnimIdsTable[species - 1];
            gTasks[taskId].data[3] = gMonAnimationDelayTable[species - 1];
        }
        else
        {
            LaunchAnimationTaskForFrontSprite(sprite, gMonFrontAnimIdsTable[species - 1]);
        }
        sprite->callback = SpriteCallbackDummy_2;
    }
}

void PokemonSummaryDoMonAnimation(struct Sprite* sprite, u16 species, bool8 oneFrame)
{
    if (!oneFrame && HasTwoFramesAnimation(species))
        StartSpriteAnim(sprite, 1);
    if (gMonAnimationDelayTable[species - 1] != 0)
    {
        u8 taskId = CreateTask(Task_PokemonSummaryAnimateAfterDelay, 0);
        STORE_PTR_IN_TASK(sprite, taskId, 0);
        gTasks[taskId].data[2] = gMonFrontAnimIdsTable[species - 1];
        gTasks[taskId].data[3] = gMonAnimationDelayTable[species - 1];
        SummaryScreen_SetUnknownTaskId(taskId);
        SetSpriteCB_MonAnimDummy(sprite);
    }
    else
    {
        StartMonSummaryAnimation(sprite, gMonFrontAnimIdsTable[species - 1]);
    }
}

void StopPokemonAnimationDelayTask(void)
{
    u8 delayTaskId = FindTaskIdByFunc(Task_PokemonSummaryAnimateAfterDelay);
    if (delayTaskId != TASK_NONE)
        DestroyTask(delayTaskId);
}

void BattleAnimateBackSprite(struct Sprite* sprite, u16 species)
{
    if (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
    {
        sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        LaunchAnimationTaskForBackSprite(sprite, GetSpeciesBackAnimSet(species));
        sprite->callback = SpriteCallbackDummy_2;
    }
}

u8 sub_806EF08(u8 arg0)
{
    s32 i;
    s32 var = 0;
    u8 multiplayerId = GetMultiplayerId();
    switch (gLinkPlayers[multiplayerId].id)
    {
    case 0:
    case 2:
        var = (arg0 != 0) ? 1 : 3;
        break;
    case 1:
    case 3:
        var = (arg0 != 0) ? 2 : 0;
        break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == (s16)(var))
            break;
    }
    return i;
}

u8 sub_806EF84(u8 arg0, u8 arg1)
{
    s32 i;
    s32 var = 0;
    switch (gLinkPlayers[arg1].id)
    {
    case 0:
    case 2:
        var = (arg0 != 0) ? 1 : 3;
        break;
    case 1:
    case 3:
        var = (arg0 != 0) ? 2 : 0;
        break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == (s16)(var))
            break;
    }
    return i;
}

u16 FacilityClassToPicIndex(u16 facilityClass)
{
    return gFacilityClassToPicIndex[facilityClass];
}

u16 PlayerGenderToFrontTrainerPicId(u8 playerGender)
{
    if (playerGender != MALE)
        return FacilityClassToPicIndex(FACILITY_CLASS_MAY);
    else
        return FacilityClassToPicIndex(FACILITY_CLASS_BRENDAN);
}

void HandleSetPokedexFlag(u16 nationalNum, u8 caseId, u32 personality)
{
    u8 getFlagCaseId = (caseId == FLAG_SET_SEEN) ? FLAG_GET_SEEN : FLAG_GET_CAUGHT;
    if (!GetSetPokedexFlag(nationalNum, getFlagCaseId)) // don't set if it's already set
    {
        GetSetPokedexFlag(nationalNum, caseId);
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_UNOWN)
            gSaveBlock2Ptr->pokedex.unownPersonality = personality;
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_SPINDA)
            gSaveBlock2Ptr->pokedex.spindaPersonality = personality;
    }
}

const u8 *GetTrainerClassNameFromId(u16 trainerId)
{
    if (trainerId >= TRAINERS_COUNT)
        trainerId = TRAINER_NONE;
    return gTrainerClassNames[gTrainers[trainerId].trainerClass];
}

const u8 *GetTrainerNameFromId(u16 trainerId)
{
    if (trainerId >= TRAINERS_COUNT)
        trainerId = TRAINER_NONE;
    return gTrainers[trainerId].trainerName;
}

bool8 HasTwoFramesAnimation(u16 species)
{
    return (species != SPECIES_CASTFORM
            && species != SPECIES_SPINDA
            && species != SPECIES_UNOWN
            && species != SPECIES_CHERRIM);
}

static bool8 ShouldSkipFriendshipChange(void)
{
    if (gMain.inBattle && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER))
        return TRUE;
    if (!gMain.inBattle && (InBattlePike() || InBattlePyramid()))
        return TRUE;
    return FALSE;
}

static void sub_806F160(struct Unknown_806F160_Struct* structPtr)
{
    u16 i, j;
    for (i = 0; i < structPtr->field_0_0; i++)
    {
        structPtr->templates[i] = gBattlerSpriteTemplates[i];
        for (j = 0; j < structPtr->field_1; j++)
        {
            structPtr->frameImages[i * structPtr->field_1 + j].data = &structPtr->byteArrays[i][j * 0x800];
        }
        structPtr->templates[i].images = &structPtr->frameImages[i * structPtr->field_1];
    }
}

static void sub_806F1FC(struct Unknown_806F160_Struct* structPtr)
{
    u16 i, j;
    for (i = 0; i < structPtr->field_0_0; i++)
    {
        structPtr->templates[i] = gUnknown_08329F28;
        for (j = 0; j < structPtr->field_1; j++)
        {
            structPtr->frameImages[i * structPtr->field_0_0 + j].data = &structPtr->byteArrays[i][j * 0x800];
        }
        structPtr->templates[i].images = &structPtr->frameImages[i * structPtr->field_0_0];
        structPtr->templates[i].anims = gUnknown_082FF70C;
        structPtr->templates[i].paletteTag = i;
    }
}

struct Unknown_806F160_Struct *sub_806F2AC(u8 id, u8 arg1)
{
    u8 i;
    u8 flags;
    struct Unknown_806F160_Struct *structPtr;

    flags = 0;
    id %= 2;
    structPtr = AllocZeroed(sizeof(*structPtr));
    if (structPtr == NULL)
        return NULL;

    switch (arg1)
    {
    case 2:
        structPtr->field_0_0 = 7;
        structPtr->field_0_1 = 7;
        structPtr->field_1 = 4;
        structPtr->field_3_0 = 1;
        structPtr->field_3_1 = 2;
        break;
    case 0:
    default:
        structPtr->field_0_0 = 4;
        structPtr->field_0_1 = 4;
        structPtr->field_1 = 4;
        structPtr->field_3_0 = 1;
        structPtr->field_3_1 = 0;
        break;
    }

    structPtr->bytes = AllocZeroed(structPtr->field_3_0 * 0x800 * 4 * structPtr->field_0_0);
    structPtr->byteArrays = AllocZeroed(structPtr->field_0_0 * 32);
    if (structPtr->bytes == NULL || structPtr->byteArrays == NULL)
    {
        flags |= 1;
    }
    else
    {
        for (i = 0; i < structPtr->field_0_0; i++)
            structPtr->byteArrays[i] = structPtr->bytes + (structPtr->field_3_0 * (i << 13));
    }

    structPtr->templates = AllocZeroed(sizeof(struct SpriteTemplate) * structPtr->field_0_0);
    structPtr->frameImages = AllocZeroed(sizeof(struct SpriteFrameImage) * structPtr->field_0_0 * structPtr->field_1);
    if (structPtr->templates == NULL || structPtr->frameImages == NULL)
    {
        flags |= 2;
    }
    else
    {
        for (i = 0; i < structPtr->field_1 * structPtr->field_0_0; i++)
            structPtr->frameImages[i].size = 0x800;

        switch (structPtr->field_3_1)
        {
        case 2:
            sub_806F1FC(structPtr);
            break;
        case 0:
        case 1:
        default:
            sub_806F160(structPtr);
            break;
        }
    }

    if (flags & 2)
    {
        if (structPtr->frameImages != NULL)
            FREE_AND_SET_NULL(structPtr->frameImages);
        if (structPtr->templates != NULL)
            FREE_AND_SET_NULL(structPtr->templates);
    }
    if (flags & 1)
    {
        if (structPtr->byteArrays != NULL)
            FREE_AND_SET_NULL(structPtr->byteArrays);
        if (structPtr->bytes != NULL)
            FREE_AND_SET_NULL(structPtr->bytes);
    }

    if (flags)
    {
        memset(structPtr, 0, sizeof(*structPtr));
        Free(structPtr);
    }
    else
    {
        structPtr->magic = 0xA3;
        gUnknown_020249B4[id] = structPtr;
    }

    return gUnknown_020249B4[id];
}

void sub_806F47C(u8 id)
{
    struct Unknown_806F160_Struct *structPtr;

    id %= 2;
    structPtr = gUnknown_020249B4[id];
    if (structPtr == NULL)
        return;

    if (structPtr->magic != 0xA3)
    {
        memset(structPtr, 0, sizeof(struct Unknown_806F160_Struct));
    }
    else
    {

        if (structPtr->frameImages != NULL)
            FREE_AND_SET_NULL(structPtr->frameImages);
        if (structPtr->templates != NULL)
            FREE_AND_SET_NULL(structPtr->templates);
        if (structPtr->byteArrays != NULL)
            FREE_AND_SET_NULL(structPtr->byteArrays);
        if (structPtr->bytes != NULL)
            FREE_AND_SET_NULL(structPtr->bytes);

        memset(structPtr, 0, sizeof(struct Unknown_806F160_Struct));
        Free(structPtr);
    }
}

u8 *sub_806F4F8(u8 id, u8 arg1)
{
    struct Unknown_806F160_Struct *structPtr = gUnknown_020249B4[id % 2];
    if (structPtr->magic != 0xA3)
    {
        return NULL;
    }
    else
    {
        if (arg1 >= structPtr->field_0_0)
            arg1 = 0;

        return structPtr->byteArrays[arg1];
    }
}

u16 GetFormSpeciesId(u16 speciesId, u8 formId)
{
    if (gFormSpeciesIdTables[speciesId] != NULL)
        return gFormSpeciesIdTables[speciesId][formId];
    else
        return speciesId;
}

u8 GetFormIdFromFormSpeciesId(u16 formSpeciesId)
{
    u8 targetFormId = 0;

    if (gFormSpeciesIdTables[formSpeciesId] != NULL)
    {
        for (targetFormId = 0; gFormSpeciesIdTables[formSpeciesId][targetFormId] != FORM_SPECIES_END; targetFormId++)
        {
            if (formSpeciesId == gFormSpeciesIdTables[formSpeciesId][targetFormId])
                break;
        }
    }
    return targetFormId;
}
