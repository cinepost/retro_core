#include "framework/game_engine/mp3_stream.h"
#include "game.h"

using namespace RetroCore::PPU;

static const std::string sIntroImageFilename = "images/title_image.png";
static const std::string sIntroLogoFilename = "images/title_logo.png";
static const std::string sIntroMusicMp3Filename = "music/suno_game_intro_01.mp3";
    

void Intro::enter() {
    mScrollY_F = 0.0;
    mLogoShown = false;

    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_5);
    mPPU.createDefaultMemoryLayout();
    mPPU.clearAllSpriteAttributes();
    mPPU.setBlankingBit(false);
    mPPU.setBorderBackgroundColor(0);

    RetroCore::Palette<16> img_palette;

    if(mAssetManager.hasFile(sIntroImageFilename)) {
        const Asset& image_asset = mAssetManager.getAsset(sIntroImageFilename);
        if(mPPU.loadIndexedImagePNG(image_asset.pData, image_asset.sizeInBytes, mPPU.getVramPageAddress(1), 512, 288, &img_palette)) {
            mPPU.setPalette(img_palette);
        }
    } else {
        std::cerr << "Error loading asset file " << sIntroImageFilename << std::endl;
    }

    if(mAssetManager.hasFile(sIntroLogoFilename)) {
        const Asset& logo_asset = mAssetManager.getAsset(sIntroLogoFilename);
        if(mPPU.loadIndexedImagePNG(logo_asset.pData, logo_asset.sizeInBytes, mPPU.getVramPageAddress(2), 160, 80)) {
        }
    } else {
        std::cerr << "Error loading asset file " << sIntroImageFilename << std::endl;
    }

    if(mAssetManager.hasFile(sIntroMusicMp3Filename)){
        const Asset& bgm_asset = mAssetManager.getAsset(sIntroMusicMp3Filename);
        auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(bgm_asset.pData, bgm_asset.sizeInBytes, false /* dont loop sound */);
        mSoundEngine.playBGM(std::move(pBgmTrack));
    } else {
        std::cerr << "Error loading asset file " << sIntroMusicMp3Filename << std::endl;
    }

    mPPU.disableSprites();
    mPPU.setBlankingBit(true);
}

void Intro::exit() {
    mSoundEngine.stopBGM();
    mPPU.disableSprites();
    mPPU.clearVRAM();
}

void Intro::handleInput(retro_input_state_t input_cb) {
    unsigned port = 0;
    if(isAnyKeyPressed(input_cb, port)) {
        mSoundEngine.stopBGM();
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage */, 2.0f /* shorter display time */));
    }
}

void Intro::update(double dt) {
    mPushSpaceTimer.update(dt);

    if(getTimeElapsed() > 57) {
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage */));
    }

    mScrollY_F += 0.75;
}

void Intro::render() {
    if(y_scroll < (FRAMEBUFFER_HEIGHT)) {
        y_scroll = mScrollY_F;
        mPPU.setScrollY(y_scroll);
    }

    if(y_scroll >= 288 && !mLogoShown) {
        // blit logo
        mPPU.cmdHMMM(0, 576, 176, 468, 160, 80, true /* index 0 color key */);

        // start "push space button" reveal timer
        mPushSpaceTimer.start(5.0); 
        mLogoShown = true;
    }
}
