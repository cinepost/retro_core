#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

#include <random> 

using namespace RetroCore::PPU;

namespace KnightGame {

static const std::string sWorldTilesImageFilename = "images/level_01_tiles.png";
static const std::string sMainMusicThemeFilename = "music/suno_level_01_bgm_01.mp3";
static const std::string sBossMusicThemeFilename = "music/suno_level_01_boss_01.mp3";

Level_1::Level_1(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): LevelBase(sm, ppu, se, am) {
    // Background music
    mMainBgmAsset = mAssetManager.getAsset(sMainMusicThemeFilename);
    mBossBgmAsset = mAssetManager.getAsset(sBossMusicThemeFilename);
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
    if(mAssetManager.hasFile(sWorldTilesImageFilename)) {
        std::cout << "Load " << sWorldTilesImageFilename << std::endl;

        const Asset sprites_asset = mAssetManager.getAsset(sWorldTilesImageFilename);
        tile_patterns = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_8D_8C>(sprites_asset.pData, sprites_asset.sizeInBytes, &mPPU.getPalette() /* ref palette */, false /* dont skip empty tiles */);
    
        for(uint16_t i = 0; i < static_cast<uint16_t>(tile_patterns.size()); ++i) {
            mPPU.pushTile(i, tile_patterns[i]);
        }
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