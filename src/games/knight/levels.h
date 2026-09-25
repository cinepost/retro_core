#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H

#include "framework/ppu/ppu_msx.h"
#include "framework/game_engine/engine_core.h"
#include "framework/game_engine/game_state.h"
#include "framework/game_engine/mp3_stream.h"

#include "game.h"
#include "game_world.h"

using namespace RetroCore;

namespace KnightGame {

class LevelBase : public BaseState {
    protected:
        static constexpr uint16_t kVerticalTilesCount = 272 / 8; // 34 tile lines - 2 status lines
        static constexpr uint16_t kVisibleTilesCount = GameWorld::kMapWidth * kVerticalTilesCount; // One bottom tile lines are reserved for status bar minus one for scrolling

    public:
        LevelBase(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am), mWorld(ppu, am, se), mSpriteList(ppu) {}
        
    protected:
        void enter() override;
        void update(double dt) override;
        void render() override final;
        void handleInput(retro_input_state_t input_cb) override final;

        virtual void enterBossZone() = 0;
                
    protected:
        void renderStatusLines();

        KnightGame::GameWorld mWorld;

        Asset mMainBgmAsset;  // main background music
        Asset mBossBgmAsset;  // boss background music
        Asset mPlayerDyingBgmAsset; // last or only player is dying

        uint16_t  mScrollY;   // VDP scroll register
    private:
        SpriteList mSpriteList;

        // level flags
        bool     mBossZoneEntered = false;
        bool     mBossReached = false;
        bool     mIsOnlyOrBothPlayersDying = false; // flag for bgm change
};

class Level_1 : public LevelBase {
    public:
        Level_1(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am);

    protected:
        void enter() override final;
        void update(double dt);
        void exit() override final;

        virtual void enterBossZone() override final;
};

}  // KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H

