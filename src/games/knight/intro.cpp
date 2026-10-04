#include "framework/game_engine/mp3_stream.h"
#include "framework/ppu/ppu_msx_utils.h"

#include "sound_files.h"
#include "game.h"

using namespace RetroCore::PPU;

namespace KnightGame {

static const std::string kIntroImageFilename = "images/title_image.png";
static const std::string kIntroLogoFilename = "images/title_logo.png";
static const std::string kSysFontFilename = "images/ascii_font.png";

Intro::Intro(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am, bool skip_to_menu): BaseState(sm, ppu, se, am), mFlashingOscillator(30, 0, 1, 0.5f) {
    Menu game_mode_menu("START");

    game_mode_menu.addEntry(std::make_unique<MenuEntry>("1 PLAYER", [this] { this->startLevel(); }));
    game_mode_menu.addEntry(std::make_unique<MenuEntry>("2 PLAYERS"));
    game_mode_menu.addEntry(std::make_unique<MenuEntry>("AI CO-OP"));

    const uint32_t game_mode_menu_id = mMenu.addEntry(std::move(game_mode_menu));
    MenuEntry& entry = mMenu.getEntry(game_mode_menu_id);
    entry.setAction([this, &entry] {
        this->mpCurrentMenu = reinterpret_cast<Menu*>(&entry); 
    });

    mMenu.addEntry(std::make_unique<MenuEntry>("OPTIONS", [] {
        std::cout << "OPTIONS toggled!\n";
    }));

    mMenu.addEntry(std::make_unique<MenuEntry>("HI-SCORES", [] {
        std::cout << "HI-SCORES toggled!\n";
    }));

    mMenu.addEntry(std::make_unique<MenuEntry>("ABOUT", [] {
        std::cout << "ABOUT toggled!\n";
    }));

    mpCurrentMenu = &mMenu;

    if(skip_to_menu) {
        mState = State::MENU;
        mTearTimer.start(10.0f); // tear animation
    } else {
        mState = State::SCROLLING;
    }
}


void Intro::enter() {
    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_5);
    mPPU.setSpriteSize(RetroCore::PPU::MsxPPU_BASE::SpriteSize::SPRITE_8);
    mPPU.createDefaultMemoryLayout();
    mPPU.clearAllSpriteAttributes();
    mPPU.setBlankingBit(false);
    mPPU.disableSprites();
    mPPU.setBorderBackgroundColor(0);

    RetroCore::Palette<16> img_palette;

    // Load font
    using PATTERN_8D = PPU::MsxPPU_BASE::PATTERN_8D;
    std::vector<PATTERN_8D> sprite_patterns;
    if(mAssetManager.hasFile(kSysFontFilename)) {
        const Asset sprites_asset = mAssetManager.getAsset(kSysFontFilename);
        sprite_patterns = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_8D>(sprites_asset.pData, sprites_asset.sizeInBytes, nullptr /* ref palette */, false /* dont skip empty tiles */);
        
        std::cout << "Sys font sprites loaded " << sprite_patterns.size() << std::endl;

        for(uint16_t i = 0; i < static_cast<uint16_t>(sprite_patterns.size()); ++i) {
            const PATTERN_8D& pattern = sprite_patterns[i];
            mPPU.pushSpritePattern(i ,pattern.tile);
        }
    } else {
        std::cerr << "Error loading sys_font " << std::endl;
    }

    // Load main 512x288 image
    if(mAssetManager.hasFile(kIntroImageFilename)) {
        const Asset& image_asset = mAssetManager.getAsset(kIntroImageFilename);
        if(mPPU.loadIndexedImagePNG(image_asset.pData, image_asset.sizeInBytes, mPPU.getVramPageAddress(1), 512, 288, &img_palette)) {
            mPPU.setPalette(img_palette);
        }
    } else {
        std::cerr << "Error loading asset file " << kIntroImageFilename << std::endl;
    }

    // Load 160x80 logo
    if(mAssetManager.hasFile(kIntroLogoFilename)) {
        const Asset& logo_asset = mAssetManager.getAsset(kIntroLogoFilename);
        if(mPPU.loadIndexedImagePNG(logo_asset.pData, logo_asset.sizeInBytes, mPPU.getVramPageAddress(2), 160, 80)) {
        }
    } else {
        std::cerr << "Error loading asset file " << kIntroLogoFilename << std::endl;
    }

    if(mState == State::SCROLLING){
        auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(mAssetManager.getAsset(kIntroMusicFileName), false /* dont loop sound */);
        mSoundEngine.playBGM(std::move(pBgmTrack));
    } else {
        // start from menu
        auto pMenuBgmTrack = std::make_unique<GameEngine::MP3Stream>(mAssetManager.getAsset(kIntroMenuMusicFileName), true /* loop music */);
        mSoundEngine.crossfadeBGM(std::move(pMenuBgmTrack), 0.2f);
    }

    mPPU.enableSprites();
    mPPU.setBlankingBit(true);

    mScrollTimer.start(384.0f /* 6.4 seconds at 60 fps */);
}

void Intro::exit() {
    mSoundEngine.stopBGM();
    mPPU.disableSprites();
    mPPU.clearVRAM();
}

void Intro::handleInput(const Input& input) {
    static const MP3Stream sfxMenuSelectTrack(mAssetManager.getAsset(kMenuSelectSfxFileName), false /* no loop */);
    static const MP3Stream sfxMenuHiliteTrack(mAssetManager.getAsset(kMenuHiliteSfxFileName), false /* no loop */);

    switch(mState) {
        case State::SCROLLING:
        case State::LOGO:
            {
                unsigned port = 0;
                if(input.isAnyKeyPressed()) {
                    mTearTimer.start(10.0f); // tear animation
                    mFlashingOscillator.reset();
                    mState = State::MENU;

                    auto pMenuBgmTrack = std::make_unique<GameEngine::MP3Stream>(mAssetManager.getAsset(kIntroMenuMusicFileName), true /* loop music */);
                    mSoundEngine.crossfadeBGM(std::move(pMenuBgmTrack), 0.2f);
                }
            }
            break;
        case State::MENU:
            if(input.isPressed(0, RETRO_DEVICE_ID_JOYPAD_UP)) {
                if(mpCurrentMenu->moveCursorUp()) {
                    mSoundEngine.playSFX(std::make_unique<MP3Stream>(sfxMenuHiliteTrack));
                    mFlashingOscillator.reset();
                }
            } else if (input.isPressed(0, RETRO_DEVICE_ID_JOYPAD_DOWN)) {
                if(mpCurrentMenu->moveCursorDown()) {
                    mSoundEngine.playSFX(std::make_unique<MP3Stream>(sfxMenuHiliteTrack));
                    mFlashingOscillator.reset();
                }
            } else if (input.isPressed(0, RETRO_DEVICE_ID_JOYPAD_SELECT) || input.isPressed(0, RETRO_DEVICE_ID_JOYPAD_A) || input.isPressed(0, RETRO_DEVICE_ID_JOYPAD_B)) {
                mpCurrentMenu->executeCurrentEntry();
                //mSoundEngine.playSFX(std::make_unique<MP3Stream>(sfxMenuSelectTrack));
                mFlashingOscillator.reset();
            }
            break;
        default:
            break;
    }
}

void Intro::startLevel(bool short_display_time) {
    mSoundEngine.stopAll();
    mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage */, short_display_time ? 2.0f /* shorter display time */ : -1.0f));
}

void Intro::update(double dt) {
    mScrollTimer.update(1.0f /* frame based */);
    mPushSpaceTimer.update(1.0f);
    mTearTimer.update(1.0f /* frame based */);
    mFlashingOscillator.update(1.0f /* flame based */);

    if(getTimeElapsed() > 57 && mState != State::MENU) {
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage */));
    }

    if(mState == State::SCROLLING) {
        if(mScrollY >= 288) {
            mState = State::LOGO;
        } else {
            mScrollY = static_cast<uint16_t>(std::floor(mScrollTimer.getElapsedPercentage() * 288.0f));
        }
    }
}

void Intro::render() {
    static uint16_t s_sprite_id;

    s_sprite_id = 0;
    mPPU.clearAllSpriteAttributes();

    auto printLine = [&](uint16_t x, uint16_t y, const std::string& str, uint8_t color = 15) {
        uint16_t i = 0;
        for (char c : str) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(s_sprite_id++);
            hw_sprite.x = x + 8 * i++;
            hw_sprite.y = y;
            hw_sprite.index = static_cast<uint16_t>(c);
            hw_sprite.attribs.color = color;
        }
    };

    switch(mState) {
        case State::SCROLLING:
            mPPU.setScrollY(mScrollY);
            break;
        case State::LOGO:
            mPPU.setScrollY(0);
            if(!mLogoShown) {
                mPPU.cmdHMMM(0, 288, 0, 0, 512, 288, false); // blit blackground to first screen
                mPPU.cmdHMMM(0, 576, 176, 180, 160, 80, true /* index 0 color key */); // blit logo
                mPushSpaceTimer.start(60.0f); // start "push space button" reveal timer. one sec delay at 60fps
                mLogoShown = true; 
            }
            if(mPushSpaceTimer.hasExpired() && mFlashingOscillator.getValue()) printLine(212, 272, "PUSH BUTTON");
            break;
        case State::MENU:
            mPPU.setScrollY(0);
            if(!mTearTimer.hasExpired()) {
                // slide background off center 
                uint16_t src_offset = static_cast<uint16_t>(std::floor(mTearTimer.getTimeElapsed()) * 5.0f);
                mPPU.cmdHMMM(5 + src_offset, 288, 0, 0, 256, 288, false); // copy image part from screen 1
                mPPU.cmdHMMM(256 - src_offset, 288, 256, 0, 256, 288, false); // copy image part from screen 1

                SineWaveOscillator<uint16_t, uint16_t> sineOscillator0(52, 0, 3);
                SineWaveOscillator<uint16_t, uint16_t> sineOscillator1(131, 0, 3);
                SawOscillator<uint16_t, uint16_t>      sawOscillator0(75, 0, 5);
                TriangleOscillator<uint16_t, uint16_t> triOscillator0(97, 0, 5);

                uint32_t addr = mPPU.getVramPageAddress(0) + 256;
                for(uint16_t i = 0; i < 288; ++i) {

                    sineOscillator0.update(1);
                    sineOscillator1.update(1);
                    sawOscillator0.update(1);
                    triOscillator0.update(1);
                    uint16_t torn_edge_offset = (uint16_t)(sineOscillator0.getValue() + sineOscillator1.getValue() + triOscillator0.getValue() + sawOscillator0.getValue());

                    mPPU.vramBlockSet(addr - src_offset - torn_edge_offset, (uint8_t)0, (uint16_t)(src_offset * 2 + 16));
                    addr += 512;
                }
                mPPU.cmdHMMM(0, 576, 176, 180 + (src_offset >> 1), 160, 80, true /* index 0 color key */); //logo
                break;
            } 

            {
                assert(mpCurrentMenu);
                uint16_t start_y = static_cast<uint16_t>(144 - (mpCurrentMenu->getEntriesCount() * 16) / 2);

                uint32_t i = 0;
                for(MenuEntry::UniquePtr& pEntry: mpCurrentMenu->getEntries()) {
                    uint8_t color = mpCurrentMenu->getCurrentEntryID() == i && mFlashingOscillator.getValue() ? 15 : 7; 
                    const std::string& title = pEntry->getTitle();
                    printLine(256 - (uint16_t)(((float)title.size() * 8.0f) / 2.f), start_y, title, color);
                    start_y += 16;
                    i++;
                }
            }

            break;
        default:
            break;
    }

//    if(y_scroll < (FRAMEBUFFER_HEIGHT)) {
//        y_scroll = mScrollY_F;
//        mPPU.setScrollY(y_scroll);
//    } else if(y_scroll >= FRAMEBUFFER_HEIGHT && !mLogoShown) {
//        // blit logo
//        mPPU.cmdHMMM(0, 576, 176, 468, 160, 80, true /* index 0 color key */);
//        // start "push space button" reveal timer
//        mPushSpaceTimer.start(5.0); 
//        mLogoShown = true;
//    }
}

}  // KnightGame