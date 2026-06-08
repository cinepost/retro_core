#include "game.h"

void IntroState::enter() {
    mScrollY_F = 0.0;

    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_5);
    mPPU.setBorderBackgroundColor(0);

    RetroCore::Palette<16> img_palette;
    static const std::string test_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/knightmare_title_02-dithered.png";
    
    if(mPPU.loadIndexedImagePNG(test_filename, mPPU.getVramPageAddress(1), 512, 288, img_palette)) {
        mPPU.setPalette(img_palette);
    }

    static const std::string test_status_line_filename = "/home/max/mnt/misc_hdd/dev/retro_core/games/virt_msx/KnightmareW/test_status_line.png";
    if(!mPPU.loadIndexedImagePNG(test_status_line_filename, mPPU.getVramPageAddress(2), 512, 16, img_palette)) {
      assert(false && "Error loading test_status_line.png");
    }

    auto scln_cb = [&](uint16_t line) {
        if(line >= 272) {
            mPPU.setCurrentVramPageIndex(1);
            mPPU.setScrollY(16);
            mPPU.setScrollX(0);
        } else {
            mPPU.setCurrentVramPageIndex(0);
            mPPU.setScrollY(y_scroll);
            mPPU.setScrollX(x_scroll);
        }
    };

    mPPU.setScanlineCallback(scln_cb);

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
}

void IntroState::exit() {
    if(mBgmMp3Stream) mBgmMp3Stream.reset(); // Deallocates decoder memory cleanly
}

void IntroState::handleInput(retro_input_state_t input_cb) {

}

void IntroState::update(double dt) {
    if(getTimeElapsed() > 9) {
        mStateManager.pushState(std::make_unique<Level1State>(mPPU, mStateManager));
    }

    mScrollY_F += 0.75;
}

void IntroState::render() {
    if(y_scroll < (FRAMEBUFFER_HEIGHT-1)) {
        y_scroll = mScrollY_F;
        mPPU.setScrollY(y_scroll);
    } else {
        y_scroll = FRAMEBUFFER_HEIGHT;
        if(x_scroll < (FRAMEBUFFER_WIDTH - 1)) {
            mPPU.setScrollX(x_scroll++);
        } else {
            x_scroll = 0;
            mPPU.setScrollX(0);
        }
    }
}

void IntroState::renderAudio(int16_t* pSamplesData, size_t samples_per_frame)  {
    if(mBgmMp3Stream) mBgmMp3Stream->renderPCM(pSamplesData, samples_per_frame);
}