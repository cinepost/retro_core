#include "levels.h"
#include "framework/ppu/ppu_msx_utils.h"

//#include "common_sprites.png.hpp"

#include <random> 
#include <cmath>

using namespace RetroCore::PPU;

namespace KnightGame {

static_assert(GameWorld::kMapTilesCount == GameWorld::kMapExtrasCount * 4);

static const float kCameraScrollSpeed = 0.2f;
static const std::string sCommonSpritesFilename = "images/common_sprites.png";
static const std::string sPlayerDyingBGMFilename = "music/player_dying.mp3";

void LevelBase::enter() {
    using PATTERN_32D = PPU::MsxPPU_BASE::PATTERN_32D;

    mPlayerDyingBgmAsset = mAssetManager.getAsset(sPlayerDyingBGMFilename);

    auto pBgmTrack = std::make_unique<GameEngine::MP3Stream>(mMainBgmAsset.pData, mMainBgmAsset.sizeInBytes, true /* loop music */);
    mSoundEngine.playBGM(std::move(pBgmTrack));

    // Clear level map and set common state
    mWorld.clear();
    mWorld.setCameraHeight(280 / 8);

    mScrollY = 8;
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
    mWorld.update(1.0); // frame based update

    if(mWorld.isPlayerDead()) {
        mSoundEngine.stopBGM();
        mStateManager.pushState(std::make_unique<LevelSummary>(mStateManager, mPPU, mSoundEngine, mAssetManager, 1 /* first stage*/));
        return;
    }

    if(mWorld.isLastOrOnlyPlayerDying() && !mIsLastOrOnlyPlayerDying) {
        mIsLastOrOnlyPlayerDying = true;
        auto pPlayerDyingBgmTrack = std::make_unique<GameEngine::MP3Stream>(mPlayerDyingBgmAsset.pData, mPlayerDyingBgmAsset.sizeInBytes, false /* no loop */);
        mSoundEngine.playBGM(std::move(pPlayerDyingBgmTrack));
        return;
    }

    const uint16_t camSubtilePosY = mWorld.getCamera().getSubtilePosY();
    mScrollY = (camSubtilePosY == 0) ? 8 : camSubtilePosY;

    if(!mBossReached) {
        if(mWorld.getCamera().getPosY() == 0) {
            mWorld.getCamera().stop();
            mBossReached = true;
        }
    }

    // check boss zone entered. zone start 36 tiles from top
    if(!mBossZoneEntered && mWorld.getCamera().getPosY() == 288) {
        mBossZoneEntered = true;

        auto pBossBgmTrack = std::make_unique<GameEngine::MP3Stream>(mBossBgmAsset.pData, mBossBgmAsset.sizeInBytes, true /* loop music */);
        mSoundEngine.playBGM(std::move(pBossBgmTrack));
    }
}

void LevelBase::handleInput(retro_input_state_t input_cb) {
    if (!input_cb) return;

    // Poll the Libretro gamepad state keys
    bool pressUp    = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
    bool pressDown  = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN); 
    bool pressLeft  = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
    bool pressRight = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    bool pressFire  = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

    // Move the player directly based on the button readings
    if (pressUp)    mWorld.movePlayer(0.0f, -1.0f);
    if (pressDown)  mWorld.movePlayer(0.0f, 1.0f);
    if (pressLeft)  mWorld.movePlayer(-1.0f, 0.0f);
    if (pressRight) mWorld.movePlayer(1.0f, 0.0f);
    if (pressFire)  mWorld.firePlayer();
}

void LevelBase::render() {
    // cleat debug drawables
    mPPU.clearDebugDrawables();

    // Update PPU nametable
    uint32_t current_camera_tiles_offset = mWorld.getCurrentCameraTilesOffset();

    #pragma unroll
    for(uint16_t i = 0; i < kVisibleTilesCount; ++i) {
        mPPU.writeTileIndex(i, mWorld.getTilePatternIndex(current_camera_tiles_offset++));
    }

    // Update PPU SAT
    mPPU.clearAllSpriteAttributes();

    const uint16_t vertical_camera_offset_px = mWorld.getCamera().getPosY() + 8; // 8px scroll padding

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
}

}  // KnightGame