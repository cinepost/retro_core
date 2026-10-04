#include "levels.h"
#include "bosses.h"
#include "sound_files.h"
#include "framework/ppu/ppu_msx_utils.h"

#include <random> 

using namespace RetroCore::PPU;

namespace KnightGame {

static const std::string kLevelTilesFileName = "images/level_01_tiles.png";
static const std::string kBossSpritesFilename = "images/boss_01_sprites.png";

Level_1::Level_1(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): LevelBase(sm, ppu, se, am) {
    // Background music
    mMainBgmAsset = mAssetManager.getAsset(kMainMusicThemeFileName);
    mBossBgmAsset = mAssetManager.getAsset(kBossMusicThemeFileName);
}

void Level_1::enter() {
    LevelBase::enter();

    // Clear level map and set common state
    if(!mWorld.init("maps/level_01.tmx")) {
        return;
    }

    mWorld.setCameraHeight(34);

    // Load level tiles
    using PATTERN_8D_8C = PPU::MsxPPU_BASE::PATTERN_8D_8C;

    std::vector<PATTERN_8D_8C> tile_patterns;
    if(mAssetManager.hasFile(kLevelTilesFileName)) {
        std::cout << "Load " << kLevelTilesFileName << std::endl;

        const Asset sprites_asset = mAssetManager.getAsset(kLevelTilesFileName);
        tile_patterns = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_8D_8C>(sprites_asset.pData, sprites_asset.sizeInBytes, &mPPU.getPalette() /* ref palette */, false /* dont skip empty tiles */);
    
        for(uint16_t i = 0; i < static_cast<uint16_t>(tile_patterns.size()); ++i) {
            mPPU.pushTile(i, tile_patterns[i]);
        }
    }

    // Load level boss sprites
    using PATTERN_32D = PPU::MsxPPU_BASE::PATTERN_32D;

    std::vector<PATTERN_32D> sprite_patterns;
    if(mAssetManager.hasFile(kBossSpritesFilename)) {
        const Asset sprites_asset = mAssetManager.getAsset(kBossSpritesFilename);
        sprite_patterns = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_32D>(sprites_asset.pData, sprites_asset.sizeInBytes, nullptr /* ref palette */, false /* dont skip empty tiles */);
    
        constexpr uint16_t boss_start_sprite_index = 8 * 16 * 4; // TODO: instead of magic 8 * 16 get common sprites loaded count from LevelBase::enter() somehow

        for(uint16_t i = 0; i < static_cast<uint16_t>(sprite_patterns.size()); ++i) {
            const PATTERN_32D& pattern = sprite_patterns[i];
            mPPU.pushSpritePattern(boss_start_sprite_index + i * 4, pattern.tile);
        }
    
        mWorld.addBoss(std::make_unique<Medusa_Boss>(232, 32, boss_start_sprite_index, mWorld.getPlayers()));
    }
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