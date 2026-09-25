#include "game.h"

#include "framework/ppu/ppu_utils.h"
#include "framework/ppu/ppu_msx_utils.h"
#include "framework/game_engine/mp3_stream.h"

static const std::string sAmikonLogoFileName = "images/amikon_logo_01.png";
static const std::string sBgmFileName = "sfx/amikon_logo_sound.mp3";

namespace KnightGame {

void Boot::enter() {
    using PATTERN_8D = PPU::MsxPPU_BASE::PATTERN_8D;
    using PATTERN_8D_8C = PPU::MsxPPU_BASE::PATTERN_8D_8C;
    
    mSWDrawn = false;
    mScrollY = 16;

    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_2);
    mPPU.disableSprites();
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(4);

    std::vector<PATTERN_8D_8C> logo_tiles;

    if(mAssetManager.hasFile(sAmikonLogoFileName)) {
        const Asset logo_image_asset = mAssetManager.getAsset(sAmikonLogoFileName);
        logo_tiles = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_8D_8C>(logo_image_asset.pData, logo_image_asset.sizeInBytes, nullptr /* ref palette */, true /* skip empty tiles */);
    }

    if(logo_tiles.empty()) {
        std::cerr << "Error loading logo tiles from " << sAmikonLogoFileName << std::endl;
        return;
    }

    mPPU.setColorTableAddress(0x00000000);
    mPPU.setPatternTableAddress(0x0000000 + mPPU.getVramPageSize());
    mPPU.setNameTableAddress(0x0000000 + mPPU.getVramPageSize() * 2);

    uint16_t tile_index = 1; // reserve index 0 for empty tile(pattern)
    for(const auto& pattern: logo_tiles) {
        mPPU.pushTile(tile_index++, pattern); 
    }

    static const uint16_t tiles_offst = mPPU.getPatternsCountPerScreen() + 1;

    mPPU.writeTileIndex(tiles_offst + 90, 1);
    mPPU.writeTileIndex(tiles_offst + 91, 2);
    mPPU.writeTileIndex(tiles_offst + 94, 3);
    mPPU.writeTileIndex(tiles_offst + 95, 4);

    mPPU.writeTileIndex(tiles_offst + 153, 5);
    mPPU.writeTileIndex(tiles_offst + 154, 6);
    mPPU.writeTileIndex(tiles_offst + 155, 7);
    mPPU.writeTileIndex(tiles_offst + 156, 8);
    mPPU.writeTileIndex(tiles_offst + 157, 9);
    mPPU.writeTileIndex(tiles_offst + 158, 10);
    mPPU.writeTileIndex(tiles_offst + 159, 11);
    mPPU.writeTileIndex(tiles_offst + 160, 12);
    mPPU.writeTileIndex(tiles_offst + 161, 13);
    mPPU.writeTileIndex(tiles_offst + 162, 14);
    mPPU.writeTileIndex(tiles_offst + 163, 15);
    mPPU.writeTileIndex(tiles_offst + 164, 16);

    mPPU.writeTileIndex(tiles_offst + 217, 17);
    mPPU.writeTileIndex(tiles_offst + 218, 18);
    mPPU.writeTileIndex(tiles_offst + 219, 19);
    mPPU.writeTileIndex(tiles_offst + 220, 20);
    mPPU.writeTileIndex(tiles_offst + 221, 21);
    mPPU.writeTileIndex(tiles_offst + 222, 22);
    mPPU.writeTileIndex(tiles_offst + 223, 23);
    mPPU.writeTileIndex(tiles_offst + 224, 24);
    mPPU.writeTileIndex(tiles_offst + 225, 25);
    mPPU.writeTileIndex(tiles_offst + 226, 26);
    mPPU.writeTileIndex(tiles_offst + 227, 27);
    mPPU.writeTileIndex(tiles_offst + 228, 28);
    

    if(!mAssetManager.hasFile(sBgmFileName)){
        std::cerr << "Error loading file " << sBgmFileName << std::endl;
    }

    mPPU.setBlankingBit(true);
}

void Boot::exit() {
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(15);
    mPPU.clearVRAM();
}

void Boot::handleInput(retro_input_state_t input_cb) {
    unsigned port = 0;
    if(isAnyKeyPressed(input_cb, port)) {
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage*/));
    }
}

void Boot::update(double dt) {

    if(mScrollY < 168) {
        mScrollY+=4;
    }

    if(getTimeElapsed() > 2) {
        mStateManager.pushState(std::make_unique<Intro>(mStateManager, mPPU, mSoundEngine, mAssetManager));
    }
}

void Boot::render() {
    if(mScrollY == 168 && !mSWDrawn) {

        const Asset& bgm_asset = mAssetManager.getAsset(sBgmFileName);
        auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(bgm_asset.pData, bgm_asset.sizeInBytes, false /* dont loop sound */);
        mSoundEngine.playSFX(std::move(pBgmTrack));

        static const uint16_t tiles_offst = mPPU.getPatternsCountPerScreen() + 1;

        mPPU.writeTileIndex(tiles_offst + 283, 29);
        mPPU.writeTileIndex(tiles_offst + 284, 30);
        mPPU.writeTileIndex(tiles_offst + 285, 31);
        mPPU.writeTileIndex(tiles_offst + 286, 32);
        mPPU.writeTileIndex(tiles_offst + 287, 33);
        mPPU.writeTileIndex(tiles_offst + 288, 34);
        mPPU.writeTileIndex(tiles_offst + 289, 35);
        mPPU.writeTileIndex(tiles_offst + 290, 36);

        mSWDrawn = true;
    }

    mPPU.setScrollY(mScrollY);
}

}  // namespace KnightGame