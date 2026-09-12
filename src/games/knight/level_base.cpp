#include "levels.h"
#include "common_sprites.png.hpp"

#include <random> 

using namespace KnightGame;
using namespace RetroCore::PPU;

static_assert(GameWorld::kMapTilesCount == GameWorld::kMapExtrasCount * 4);

void LevelBase::enter() {
    // Clear level map and set common state
    mWorld.clear();
    mUpdateTileSet = true;

    mBossZoneEntered = false;
    mBossReached = false;
    mScrollY_F = 8.0f;

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

    auto scln_cb = [&](uint16_t line) {
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

    mPPU.setScanlineCallback(scln_cb);

    mPPU.setDefaultPalette();

    // Common sprite patterns
    for(uint16_t i = 0; i < COMMON_SPRITES_SPRITE_COUNT; ++i) {
        mPPU.pushSpritePattern(i * 4, &COMMON_SPRITES_DATA[i][0], (uint8_t)COMMON_SPRITES_BYTES_PER_SPRITE);
    }

    mPPU.enableSprites();
    mPPU.setBlankingBit(true);
}

void LevelBase::update(double dt) {
    static constexpr uint16_t vertical_tiles_count = 288 / 8; // 18 tiles
    static constexpr uint16_t visible_tiles_count = GameWorld::kMapWidth * (vertical_tiles_count - 1); // 2 bottom tile lines are reserved for status bar

    if(!mBossReached) {
        mScrollY_F -= 0.2f;

        if(mScrollY_F <= 0.0f) {
            mScrollY_F = 8.0f;

            if(mVerticalMapOffset < (GameWorld::kMapHeight - vertical_tiles_count)) {
                mVerticalMapOffset++;
                mUpdateTileSet = true;
            } else if( mScrollY == 0){
                // Stop camera movement.
                mScrollY_F = 0.0f;
                mBossReached = true;
            }
        }

        mScrollY = mScrollY_F;
    }

    // update test sprites
    for(auto& sprite: mSprites) {
        if(sprite.x <= -15 || sprite.x >= 512) {
            sprite.dir_x = -sprite.dir_x;
        }

        if(sprite.y <= -15 || sprite.y >= 288) {
            sprite.dir_y = -sprite.dir_y;
        }

        sprite.x += sprite.dir_x;
        sprite.y += sprite.dir_y;    
    }

    // update tile set
     if(mUpdateTileSet) {
        uint32_t current_camera_tiles_offset = (GameWorld::kMapHeight - (vertical_tiles_count + mVerticalMapOffset)) * GameWorld::kMapWidth;
        for(uint16_t i = 0; i < visible_tiles_count; ++i) {
            mPPU.writeTileIndex(i, mWorld.getTileIndex(current_camera_tiles_offset++));
        }

        mUpdateTileSet = false;
    }
}
