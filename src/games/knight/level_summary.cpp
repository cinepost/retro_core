#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

#include "sound_files.h"
#include "level_01_tiles.png.hpp"

namespace KnightGame {

void LevelSummary::enter() {
    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_2);
    mPPU.createDefaultMemoryLayout();

    mPPU.disableSprites();
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(0);
    mPPU.clearAllSpriteAttributes();
    mPPU.clearNameTable();
    mPPU.setScrollY(0);
    mPPU.clearVRAM();

    // Load level 1 tiles for font rendering
    mPPU.pushTiles(level_1_tiles_tiles.data(), LEVEL_1_TILES_TILE_COUNT, 0 /* first tile offset */);

    uint32_t level_attr_offset = 1052;
    mPPU.writeTileIndex(level_attr_offset++, 50); // S
    mPPU.writeTileIndex(level_attr_offset++, 51); // T
    mPPU.writeTileIndex(level_attr_offset++, 32); // A
    mPPU.writeTileIndex(level_attr_offset++, 38); // G
    mPPU.writeTileIndex(level_attr_offset++, 36); // E
    level_attr_offset++;
    mPPU.writeTileIndex(level_attr_offset++, 17); // 0
    mPPU.writeTileIndex(level_attr_offset++, 18); // 1

    if(!mSoundEngine.isBGMPlaying() && mTimeToShow >= 6.0 /* enough time to play bgm track */) {
        const Asset& bgm_asset = mAssetManager.getAsset(kLevelStartBgmFileName);
        auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(bgm_asset.pData, bgm_asset.sizeInBytes, false /* dont loop sound */);
        mSoundEngine.playBGM(std::move(pBgmTrack));
    }
 
    mPPU.setBlankingBit(true);
}

void LevelSummary::exit() {
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(15);
    mPPU.clearVRAM();
}

void LevelSummary::finish() {
    switch(mStateLevel) {
        case 1:
        default:
            mStateManager.pushState(std::make_unique<Level_1>(mStateManager, mPPU, mSoundEngine, mAssetManager));
    }
}

void LevelSummary::handleInput(const Input& input) {
    if(input.isAnyKeyPressed()) {
        finish();
    }
}

void LevelSummary::update(double dt) {
    if(getTimeElapsed() > mTimeToShow) {
        finish();
    }
}

void LevelSummary::render() {
    
}

}  // KnightGame