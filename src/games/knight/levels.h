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
        static constexpr uint16_t kVerticalTilesCount = 280 / 8; // 35 tiles - 1 status line for 8p xscrolling
        static constexpr uint16_t kVisibleTilesCount = GameWorld::kMapWidth * kVerticalTilesCount; // One bottom tile lines are reserved for status bar minus one for scrolling

    public:
        LevelBase(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am), mWorld(se), mSpriteList(ppu) {}
        
    protected:
        void enter() override;
        void update(double dt) override;
        void render() override final;
        void handleInput(retro_input_state_t input_cb) override final;

        virtual void enterBossZone() = 0;

    public:
        // Helper function to check extras layer placement
        template <typename T, std::size_t N>
        static constexpr bool check_extras_layer(const std::array<T, N>& arr) {
            for (std::size_t i = 0; i < arr.size(); ++i) {
                if (arr[i].x % 16 != 0) {
                    return false;
                }
                if (arr[i].y % 16 != 0) {
                    return false;
                }
                if (arr[i].width != 16 || arr[i].height != 16) {
                    return false;
                }
            }
            return true;
        }
                
    protected:
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
        bool     mIsLastOrOnlyPlayerDying = false;
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

