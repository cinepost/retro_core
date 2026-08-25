#include "game.h"

#include <random> 

static const std::array<uint8_t, 32> player_horns_pattern = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0xC0, 0xC4, 0xE8, 0x68, 0x08, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x03, 0x03, 0x07, 0x06, 0x00
};

static const std::array<uint8_t, 32> player_body_pattern = {
    0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x2F,
    0x70, 0x77, 0x2F, 0x00, 0x0F, 0x0F, 0x09, 0x06,
    0xE0, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4,
    0x0E, 0xEE, 0xF6, 0x06, 0xF0, 0x00, 0x00, 0x00
};

static const std::array<uint8_t, 32> player_shadow_pattern = {
    0x10, 0x0F, 0x08, 0x10, 0x0F, 0x1F, 0x3F, 0x3F,
    0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0xF0, 0x10, 0x08, 0xF0, 0xF8, 0xFC, 0xFC,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const std::array<uint8_t, 32> arrow_pattern = {
    0x20, 0x20, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70,
    0xA8, 0x70, 0xA8, 0x50, 0x88, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const std::array<uint8_t, 32> double_arrow_pattern = {
    0x20, 0x20, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70,
    0xA9, 0x70, 0xA9, 0x50, 0x89, 0x00, 0x00, 0x00,
    0x40, 0x40, 0xE0, 0x40, 0x40, 0x40, 0x40, 0xE0,
    0x50, 0xE0, 0x50, 0xA0, 0x10, 0x00, 0x00, 0x00
};

static const std::array<uint8_t, 32> fire_arrow_pattern = {
    0x10, 0x38, 0x3A, 0x78, 0x34, 0x54, 0x10, 0x91,
    0x38, 0x54, 0x38, 0x54, 0x28, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const std::array<uint8_t, 32> knife_pattern = {
    0x10, 0x10, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0xFC, 0x10, 0x30, 0x30, 0x30, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

using namespace KnightGame;
using namespace RetroCore::PPU;

static_assert(GameWorld::kMapTilesCount == GameWorld::kMapExtrasCount * 4);

void LevelBaseState::enter() {
    // Clear level map and set common state
    mWorld.clear();
    mUpdateTileSet = true;
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

    mPPU.pushSpritePattern(0, player_horns_pattern);
    mPPU.pushSpritePattern(4, player_body_pattern);
    mPPU.pushSpritePattern(8, player_shadow_pattern);
    mPPU.pushSpritePattern(12, arrow_pattern);
    mPPU.pushSpritePattern(16, double_arrow_pattern);
    mPPU.pushSpritePattern(20, fire_arrow_pattern);
    mPPU.pushSpritePattern(24, knife_pattern);

    mPPU.enableSprites();
    mPPU.setBlankingBit(true);
}

void LevelBaseState::update(double dt) {
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
        if(sprite.pos_x <= -15 || sprite.pos_x >= 512) {
            sprite.dir_x = -sprite.dir_x;
        }

        if(sprite.pos_y <= -15 || sprite.pos_y >= 288) {
            sprite.dir_y = -sprite.dir_y;
        }

        sprite.pos_x += sprite.dir_x;
        sprite.pos_y += sprite.dir_y;    
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
