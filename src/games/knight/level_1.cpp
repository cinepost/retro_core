#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

#include "level_01.tmx.hpp"
#include "level_01_tiles.png.hpp"

#include <random> 

using namespace RetroCore::PPU;

namespace KnightGame {

static_assert(LevelBase::check_extras_layer(level_01_extras), "All level 1 extras positions must be divisible by 16 and size 16x16 !");

static const std::string sMainMusicThemeFilename = "music/suno_level_01_bgm_01.mp3";
static const std::string sBossMusicThemeFilename = "music/suno_level_01_boss_01.mp3";

Level_1::Level_1(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): LevelBase(sm, ppu, se, am) {
    // Background music
    mMainBgmAsset = mAssetManager.getAsset(sMainMusicThemeFilename);
    mBossBgmAsset = mAssetManager.getAsset(sBossMusicThemeFilename);
}

void Level_1::enter() {
    LevelBase::enter();

    // Prepare level map
    mWorld.addLayer(level_01_ground_map, GameWorld::Tile::Type::Ground, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_columns_map, GameWorld::Tile::Type::Wall, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_rivers_map, GameWorld::Tile::Type::Water, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_bridges_map, GameWorld::Tile::Type::Bridge, GameWorld::Tile::Flags::None);
    mWorld.addLayer(level_01_end_map, GameWorld::Tile::Type::Wall, GameWorld::Tile::Flags::None);

    mWorld.addExtras(level_01_extras);

    // Load level tiles
    mPPU.pushTiles(level_1_tiles_tiles.data(), LEVEL_1_TILES_TILE_COUNT, 0 /* first tile offset */);
}

void Level_1::enterBossZone() {

}

void Level_1::exit() {
    mPPU.setBlankingBit(true);
}

void Level_1::update(double dt) {
    LevelBase::update(dt);
}

}  // KnightGame