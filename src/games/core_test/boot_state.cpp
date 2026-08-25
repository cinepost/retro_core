#include "game.h"
#include "framework/ppu/ppu_utils.h"
#include "framework/ppu/ppu_msx_utils.h"


static RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C T_0(
    {0x00, 0x1C, 0x22, 0x63, 0x63, 0x63, 0x22, 0x1C}, //Pattern data
    {0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0}  //Color data
);

static RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C T_1(
    {0x00, 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E}, //Pattern data
    {0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0}  //Color data
);

static RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C T_2(
    {0x00, 0x3E, 0x63, 0x03, 0x0E, 0x3C, 0x70, 0x7F}, //Pattern data
    {0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0}  //Color data
);

void BootState::enter() {
    mSWDrawn = false;
    mScrollY = 16;

    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_2);
    mPPU.disableSprites();
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(4);

    static const std::string amikon_logo_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/amikon_logo_01.png";
    std::vector<PPU::MsxPPU_BASE::PATTERN_8D_8C> logo_tiles = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(amikon_logo_filename, nullptr /* ref palette */, true /* skip empty tiles */);
    if(logo_tiles.empty()) {
        std::cerr << "Error loading logo tiles from " << amikon_logo_filename << std::endl;
        return;
    }

    std::cout << logo_tiles.size() << " tiles loaded from " << amikon_logo_filename << std::endl;

    mPPU.setColorTableAddress(0x00000000);
    mPPU.setPatternTableAddress(0x0000000 + mPPU.getVramPageSize());
    mPPU.setNameTableAddress(0x0000000 + mPPU.getVramPageSize() * 2);

    uint16_t tile_index = 1; // reserve index 0 for empty tile(pattern)
    for(const auto& pattern: logo_tiles) {
        mPPU.pushTile(tile_index++, pattern); 
    }

    static const uint16_t tiles_offst = mPPU.getPatternsCountPerScreen() + 1;

    //mPPU.pushTile(1, T_1);
    //mPPU.pushTile(2, T_2);

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
    

    // MP3 audio test playback

    static const std::string intro_mp3_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/music/amikon_logo_sound.mp3";
    const uint8_t* mp3File = mAssetManager.getFile(intro_mp3_filename);

    if(mp3File) {
        size_t file_size = mAssetManager.getFileSize(intro_mp3_filename);

        // 1. Initialize the MP3 asset locally within this state context
        mBgmMp3Stream = std::make_unique<GameEngine::MP3Stream>(mp3File, file_size, false /* dont loop sound */);
    } else {
        std::cerr << "Error loading file " << intro_mp3_filename << std::endl;
    }

    mPPU.setBlankingBit(true);
}

void BootState::exit() {
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(15);
    mPPU.clearVRAM();
}

void BootState::finish() {
    mStateManager.pushState(std::make_unique<IntroState>(mPPU, mStateManager));
}


void BootState::handleInput(retro_input_state_t input_cb) {
    unsigned port = 0;
    if(isAnyKeyPressed(input_cb, port)) {
        finish();
    }
}

void BootState::update(double dt) {

    if(mScrollY < 168) {
        mScrollY+=4;
    }

    if(getTimeElapsed() > 2) {
        finish();
    }
}

void BootState::render() {
    if(mScrollY == 168 && !mSWDrawn) {
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

void BootState::renderAudio(int16_t* pSamplesData, size_t samples_per_frame)  {
    if(mBgmMp3Stream && mSWDrawn) mBgmMp3Stream->renderPCM(pSamplesData, samples_per_frame);
}