#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

#include "level_01.tmx.hpp"
#include "level_01_tiles.png.hpp"

#include <random> 

using namespace KnightGame;
using namespace RetroCore::PPU;

static_assert(LevelBase::check_extras_layer(level_01_extras), "All level 1 extras positions must be divisible by 16 and size 16x16 !");

static const std::string sMainMusicThemeFilename = "music/suno_level_01_bgm_01.mp3";

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

    // Test sprites
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int16_t> distr_x(-7, 512);
    std::uniform_int_distribution<int16_t> distr_y(-7, 288);
    std::bernoulli_distribution d(0.5); // Bernoulli distribution (50% chance of true/false)

    uint16_t pattern = 0;
    for(uint16_t i = 0; i < 1024; ++i) {
        mSprites.push_back({distr_x(gen), distr_y(gen), d(gen) ? 1 : -1, d(gen) ? 1 : -1, pattern, 0});
        pattern += 4;
        if(pattern > 16) pattern = 0;
    }

    // Background music
    if(mAssetManager.hasFile(sMainMusicThemeFilename)) {
        const Asset bgm_asset = mAssetManager.getAsset(sMainMusicThemeFilename);
        auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(bgm_asset.pData, bgm_asset.sizeInBytes, true /* loop */);
        mSoundEngine.playBGM(std::move(pBgmTrack));
    } else {
        std::cerr << "Error loading file " << sMainMusicThemeFilename << std::endl;
    }
}

void Level_1::enterBossZone() {

}

void Level_1::exit() {
    mPPU.setBlankingBit(true);
}

void Level_1::handleInput(retro_input_state_t input_cb) {

}

void Level_1::update(double dt) {
    LevelBase::update(dt);
}

void Level_1::render() {
    uint16_t sprite_id = 0;
    uint8_t color = 0;
    for(const auto& sprite: mSprites) {
        MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
        hw_sprite.x = sprite.x;
        hw_sprite.y = sprite.y;
        hw_sprite.index = sprite.patternIndex;
        hw_sprite.attribs.color = color++;

        if(color == 16) color = 0;
    }
}
