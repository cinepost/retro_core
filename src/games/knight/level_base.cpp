#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

#include <random> 
#include <cmath>
#include <cctype>

using namespace RetroCore::PPU;

namespace KnightGame {

static_assert(GameWorld::kMapTilesCount == GameWorld::kMapExtrasCount * 4);

static const float kCameraScrollSpeed = 0.2f;
static const std::string sCommonSpritesFilename = "images/common_sprites.png";
static const std::string sPlayerDyingBGMFilename = "music/player_dying.mp3";

void LevelBase::enter() {
    mPlayerDyingBgmAsset = mAssetManager.getAsset(sPlayerDyingBGMFilename);

    auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(mMainBgmAsset.pData, mMainBgmAsset.sizeInBytes, true /* loop music */);
    mSoundEngine.playBGM(std::move(pBgmTrack));

    mScrollY = 0;
    mBossZoneEntered = false;
    mBossReached = false;

    // PPU state
    mPPU.setScreenMode(RetroCore::PPU::MsxPPU_BASE::ScreenMode::VSCREEN_2); // SHOULD ALWAYS BE FIRST !!!

    mPPU.createDefaultMemoryLayout();

    mPPU.clearAllSpriteAttributes();
    mPPU.clearNameTable();
    mPPU.setBlankingBit(false);
    mPPU.setSpriteSize(RetroCore::PPU::MsxPPU_BASE::SpriteSize::SPRITE_16);
    mPPU.setBorderBackgroundColor(0);

    mPPU.setScrollX(0);
    mPPU.setScrollY(0);

    auto scanline_cb = [&](uint16_t line) {
        if(line >= 272) {
            // status bar
            mPPU.setCurrentVramPageIndex(1);
            mPPU.setScrollY(16);
            mPPU.setScrollX(0);
            mPPU.disableSprites();
        } else {
            mPPU.setCurrentVramPageIndex(0);
            mPPU.setScrollY(mScrollY);
            mPPU.setScrollX(0);
            mPPU.enableSprites();
        }
    };

    mPPU.setScanlineCallback(scanline_cb);

    mPPU.setDefaultPalette();

    // Load sprite patterns
    using PATTERN_32D = PPU::MsxPPU_BASE::PATTERN_32D;
    
    std::vector<PATTERN_32D> sprite_patterns;
    if(mAssetManager.hasFile(sCommonSpritesFilename)) {
        const Asset sprites_asset = mAssetManager.getAsset(sCommonSpritesFilename);
        sprite_patterns = RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG<PATTERN_32D>(sprites_asset.pData, sprites_asset.sizeInBytes, nullptr /* ref palette */, false /* dont skip empty tiles */);
    
        for(uint16_t i = 0; i < static_cast<uint16_t>(sprite_patterns.size()); ++i) {
            const PATTERN_32D& pattern = sprite_patterns[i];
            mPPU.pushSpritePattern(i * 4, pattern.tile);
        }
    }

    mPPU.enableSprites();
    mPPU.setBlankingBit(true);
}

void LevelBase::update(double dt) {
    mPPU.clearDebugDrawables();

    mWorld.update(1.0); // frame based update

    if(mWorld.isGameOver()) {
        mSoundEngine.stopBGM();
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage*/));
        return;
    }

    if(mWorld.isOnlyOrBothPlayersDying() && !mIsOnlyOrBothPlayersDying) {
        mIsOnlyOrBothPlayersDying = true;
        auto pPlayerDyingBgmTrack = std::make_unique<GameEngine::MP3Stream>(mPlayerDyingBgmAsset.pData, mPlayerDyingBgmAsset.sizeInBytes, false /* no loop */);
        mSoundEngine.playBGM(std::move(pPlayerDyingBgmTrack));
        return;
    }
    
    if(!mBossReached) {
        if(mWorld.getCamera().getPosY() == 0) {
            mWorld.getCamera().stop();
            mBossReached = true;
        }
    }

    // check boss zone entered. zone start 36 tiles from top
    if(!mBossZoneEntered && mWorld.getCamera().getPosY() <= 240) {
        mBossZoneEntered = true;

        auto pBossBgmTrack = std::make_unique<GameEngine::MP3Stream>(mBossBgmAsset.pData, mBossBgmAsset.sizeInBytes, true /* loop music */);
        mSoundEngine.crossfadeBGM(std::move(pBossBgmTrack), 1.0f);
    }
}

void LevelBase::handleInput(retro_input_state_t input_cb) {
    if (!input_cb) return;

    // Poll the Libretro gamepad state keys
    for(uint i = 0; i < 2; ++i) {
        bool pressUp    = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
        bool pressDown  = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN); 
        bool pressLeft  = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
        bool pressRight = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
        bool pressFire  = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

        // Move the player directly based on the button readings
        if (pressUp)    mWorld.movePlayer(i,  0.0f, -1.0f);
        if (pressDown)  mWorld.movePlayer(i,  0.0f,  1.0f);
        if (pressLeft)  mWorld.movePlayer(i, -1.0f,  0.0f);
        if (pressRight) mWorld.movePlayer(i,  1.0f,  0.0f);
        if (pressFire)  mWorld.firePlayer(i);
    }
}

void LevelBase::render() {
    const uint16_t camSubtilePosY = mWorld.getCamera().getSubtilePosY();
    mScrollY = camSubtilePosY;

    // Update PPU nametable
    uint32_t current_camera_tiles_offset = mWorld.getCurrentCameraTilesOffset();

    #pragma unroll
    for(uint16_t i = 0; i < (mScrollY == 0 ? kVisibleTilesCount : kVisibleTilesCount + 64 /* extra tiles line */) ; ++i) {
        mPPU.writeTileIndex(i, mWorld.getTilePatternIndex(current_camera_tiles_offset++));
    }

    // Update PPU SAT
    mPPU.clearAllSpriteAttributes();

    const uint16_t vertical_camera_offset_px = mWorld.getCamera().getPosY();

    mSpriteList.clear();
    for(const auto& pObject: mWorld.getGameObjects()) {
        assert(pObject);
        pObject->draw(mSpriteList);

        // draw debug
        const auto& obstacle_aabb = pObject->getObstacleAABB();
        if(!obstacle_aabb.isSingular()) {
            mPPU.drawDebugRect(
                obstacle_aabb.x + pObject->getPosX<int16_t>(), 
                obstacle_aabb.y + pObject->getPosY<int16_t>() - vertical_camera_offset_px, 
                obstacle_aabb.w, obstacle_aabb.h, true /* outline */
            ); 
        }

        const auto& collision_aabb = pObject->getCollisionAABB();
        if(!collision_aabb.isSingular()) {
            mPPU.drawDebugRect(
                collision_aabb.x + pObject->getPosX<int16_t>(), 
                collision_aabb.y + pObject->getPosY<int16_t>() - vertical_camera_offset_px, 
                collision_aabb.w, collision_aabb.h, true /* outline */
            ); 
        }
    }

    uint16_t sprite_id = 0;
    for(const auto& sprite: mSpriteList.getSprites()) {
        MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
        hw_sprite.x = sprite.x;
        hw_sprite.y = sprite.y - vertical_camera_offset_px;
        hw_sprite.index = sprite.pattern;
        hw_sprite.attribs = sprite.attribs;
    }

    // render top player(s) status bar
    const Player* pPlayer = mWorld.getPlayer(0);
    if(pPlayer) {
        for(int i = 0; i < pPlayer->getLivesCount(); ++i) {
            {
            MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
            hw_sprite.x = 8 + i * 12;
            hw_sprite.y = 8;
            hw_sprite.index = 120;
            hw_sprite.attribs.color = 1;
            }
            {
            MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
            hw_sprite.x = 8 + i * 12;
            hw_sprite.y = 8;
            hw_sprite.index = 116;
            hw_sprite.attribs.color = 7;
            }
            {
            MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
            hw_sprite.x = 8 + i * 12;
            hw_sprite.y = 8;
            hw_sprite.index = 124;
            hw_sprite.attribs.color = 15;
            }
        }

        float state_time_left = std::floor(pPlayer->getStateTimeLeft());
        if(state_time_left > 0.01f) {
            const std::string int_part_str = std::to_string(static_cast<uint8_t>(state_time_left));
            std::cout << "State time left " << int_part_str << std::endl;

            static const uint16_t sDigistsSpritesStartIndex = 192;
            uint8_t color = pPlayer->getState() == Player::State::INVALNURABLE ? 6 : 15;
            uint16_t i = 0;    
            for (char c : int_part_str) {
                uint16_t digit_sprite_index = sDigistsSpritesStartIndex + (static_cast<uint16_t>(c) - 48) * 4;
                uint16_t x_pos = 50 + 8 * i++;
                static const uint16_t y_pos = 9;
                {
                    MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
                    hw_sprite.x = x_pos + 1;
                    hw_sprite.y = y_pos + 1;
                    hw_sprite.index = digit_sprite_index;
                    hw_sprite.attribs.color = 1;
                }

                {
                    MsxPPU_BASE::Sprite& hw_sprite = mPPU.getSpriteAttribute(sprite_id++);
                    hw_sprite.x = x_pos;
                    hw_sprite.y = y_pos;
                    hw_sprite.index = digit_sprite_index;
                    hw_sprite.attribs.color = color;
                }
            }
        }
    }

    // bottom status lines
    renderStatusLines();
}

void LevelBase::renderStatusLines() {
    static const uint16_t status_line_start_tile_index = 2304;
    static const uint16_t status_line_tiles_count = 128;

    static uint16_t t = 0;

    // clear status line
    for(uint16_t i = status_line_start_tile_index; i < (status_line_start_tile_index + status_line_tiles_count); ++i) mPPU.writeTileIndex(i, 1);

    auto printLine = [&](uint16_t x, uint16_t y, const std::string& str) {
        uint16_t tile_index = status_line_start_tile_index + x + y * 64;   
        for (char c : str) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            int ascii_code = static_cast<int>(c);
            if(ascii_code >= 48 && ascii_code < 58) {
                mPPU.writeTileIndex(tile_index, static_cast<uint16_t>(ascii_code) - 31);
            } else if (ascii_code >= 65 && ascii_code < 90) {
                mPPU.writeTileIndex(tile_index, static_cast<uint16_t>(ascii_code) - 33);
            } else {
                mPPU.writeTileIndex(tile_index, 1);
            }
            tile_index++;
        }
    };

    const Player* pPlayer0 = mWorld.getPlayer(0);

    if(pPlayer0) {
        printLine(0, 1, "SCORE " + std::to_string(pPlayer0->getScore()));
    }
}

}  // KnightGame