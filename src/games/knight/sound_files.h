#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_SOUND_FILES_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_SOUND_FILES_H

#include <string> 

namespace KnightGame {

// Background music files
static const std::string kIntroMenuMusicFileName    = "music/suno_menu_loop.mp3";
static const std::string kIntroMusicFileName        = "music/suno_game_intro_01.mp3";
static const std::string kPlayerDyingBgmFileName    = "music/player_dying.mp3";
static const std::string kLevelStartBgmFileName     = "music/level_start.mp3";

static const std::string kMainMusicThemeFileName    = "music/suno_level_01_bgm_01.mp3";
static const std::string kBossMusicThemeFileName    = "music/suno_level_01_boss_01.mp3";

// SFX sound files
static const std::string kAmikonLogoSfxFileName     = "sfx/amikon_logo_sound.mp3";
static const std::string kMenuSelectSfxFileName     = "sfx/menu_select.mp3";
static const std::string kMenuHiliteSfxFileName     = "sfx/menu_highlight.mp3";

static const std::string kShotArrowSfxFileName      = "sfx/shot_arrow.mp3";
static const std::string kShotBoomerangSfxFileName  = "sfx/shot_boomerang.mp3";
static const std::string kExtraBlockTakeSfxFileName = "sfx/extra_block_take.mp3";
static const std::string kExtraBlockHitSfxFileName  = "sfx/extra_block_hit.mp3";
static const std::string kPowerUpHitSfxFileName     = "sfx/power_up_hit.mp3";
static const std::string kEnemyKillSfxFileName      = "sfx/enemy_kill.mp3";

}  // KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_SOUND_FILES_H