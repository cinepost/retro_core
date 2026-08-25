#include "game.h"

using namespace RetroCore::PPU;

void IntroState::enter() {
    mScrollY_F = 0.0;
    mLogoShown = false;

    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_5);
    mPPU.createDefaultMemoryLayout();
    mPPU.clearAllSpriteAttributes();
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(0);

    RetroCore::Palette<16> img_palette;
    static const std::string test_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/knightmare_title_02-dithered.png";
    
    if(mPPU.loadIndexedImagePNG(test_filename, mPPU.getVramPageAddress(1), 512, 288, &img_palette)) {
        mPPU.setPalette(img_palette);
    }

    static const std::string logo_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/knightmare-logo-dithered-01.png";
    if(!mPPU.loadIndexedImagePNG(logo_filename, mPPU.getVramPageAddress(2), 160, 80)) {
        std::cerr << "Error loading " << logo_filename << std::endl;
    }

    // MP3 audio test playback

    static const std::string intro_mp3_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/music/suno_game_start_01.mp3";
    const uint8_t* mp3File = mAssetManager.getFile(intro_mp3_filename);

    if(mp3File) {
        size_t file_size = mAssetManager.getFileSize(intro_mp3_filename);

        // 1. Initialize the MP3 asset locally within this state context
        mBgmMp3Stream = std::make_unique<GameEngine::MP3Stream>(mp3File, file_size, true);
    } else {
        std::cerr << "Error loading file " << intro_mp3_filename << std::endl;
    }

    mPPU.disableSprites();
    mPPU.setBlankingBit(true);
}

void IntroState::finish() {
    mStateManager.pushState(std::make_unique<Level1State>(mPPU, mStateManager));
}

void IntroState::exit() {
    if(mBgmMp3Stream) mBgmMp3Stream.reset(); // Deallocates decoder memory cleanly

    mPPU.disableSprites();
    mPPU.clearVRAM();
}

void IntroState::handleInput(retro_input_state_t input_cb) {
    unsigned port = 0;
    if(isAnyKeyPressed(input_cb, port)) {
        finish();
    }
}

void IntroState::update(double dt) {
    if(getTimeElapsed() > 10) {
        finish();
    }

    mScrollY_F += 0.75;
}

void IntroState::render() {
    if(y_scroll < (FRAMEBUFFER_HEIGHT-1)) {
        y_scroll = mScrollY_F;
        mPPU.setScrollY(y_scroll);
    }

    if(y_scroll > 280 && !mLogoShown) {
        mPPU.cmdHMMM(0, 576, 176, 488, 160, 80, true /* index 0 color key */);
        mLogoShown = true;
    }
}

void IntroState::renderAudio(int16_t* pSamplesData, size_t samples_per_frame)  {
    if(mBgmMp3Stream) mBgmMp3Stream->renderPCM(pSamplesData, samples_per_frame);
}