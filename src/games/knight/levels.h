#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H

#include "framework/ppu/ppu_msx.h"
#include "framework/game_engine/engine_core.h"
#include "framework/game_engine/game_state.h"
#include "framework/game_engine/mp3_stream.h"

#include "game.h"

using namespace RetroCore;

class LevelBase : public BaseState {
    public:
        LevelBase(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am) {}
        
    protected:
        void enter() override;
        void update(double dt) override;

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
        struct Sprite {
            int16_t x = 0;
            int16_t y = 0;

            int16_t dir_x = 0;
            int16_t dir_y = 0;

            uint16_t    patternIndex = 0; // sprite pattern
            uint8_t     attributes = 0;
        };

        std::vector<Sprite> mSprites;

        KnightGame::GameWorld mWorld;

        uint16_t mScrollY;
        double   mScrollY_F;

    private:
        // level flags
        bool     mBossZoneEntered = false;
        bool     mBossReached = false;
        //
        uint16_t mVerticalMapOffset = 0;
        bool     mUpdateTileSet;
};

class Level_1 : public LevelBase {
    public:
        Level_1(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): LevelBase(sm, ppu, se, am) {}

    protected:
        void enter() override final;
        void update(double dt);
        void exit() override final;
        void handleInput(retro_input_state_t input_cb) override final;
        void render() override final;

        virtual void enterBossZone() override final;
};

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_LEVELS_H

