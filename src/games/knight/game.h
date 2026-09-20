#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_GAME_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_GAME_H

#include "framework/ppu/ppu_msx.h"
#include "framework/game_engine/engine_core.h"
#include "framework/game_engine/game_state.h"
#include "framework/game_engine/mp3_stream.h"

#include <cmath>
#include <cstdint>

#define FRAMEBUFFER_WIDTH 512
#define FRAMEBUFFER_HEIGHT 288

using namespace RetroCore;

using V99x8 = PPU::MsxPPU<{FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT}>;
using Asset = GameEngine::AssetManager::Asset;
using AssetManager = GameEngine::AssetManager;
using SoundEngine  = GameEngine::SoundEngine;
using StateManager = GameEngine::StateManager;

namespace KnightGame {

class GameObject;

class ObjectSpawner {
    public:
        virtual ~ObjectSpawner() = default;
        virtual void spawnObject(std::unique_ptr<GameObject> newObject) = 0;
};

class SpriteList {
    public:

        SpriteList(V99x8& ppu): mPPU(ppu) {
            mSprites.reserve(1024);
        }

        struct Sprite {
            Sprite(): x(0), y(PPU::MsxPPU_BASE::kVerticalTerminatorCode), pattern(0), attribs(0) {}
            Sprite(int16_t _x, int16_t _y, uint16_t _pattern, uint8_t _attr): x(_x), y(_y), pattern(_pattern), attribs(_attr) {}
            Sprite(float _x, float _y, uint16_t _pattern, uint8_t _attr): x(static_cast<int16_t>(std::floor(_x))), y(static_cast<int16_t>(std::floor(_y))), pattern(_pattern), attribs(_attr) {}
            int16_t x = 0;
            int16_t y = 0;

            uint16_t pattern = 0; // sprite pattern
            PPU::MsxPPU_BASE::Sprite::Attributes attribs;
        };

        void clear() noexcept {
            mSprites.clear();
        }

        void push(const Sprite& sprite) const {
            assert(mSprites.size() < PPU::MsxPPU_BASE::kMaximumSpritesCount);
            mSprites.push_back(sprite); 
        }

        [[nodiscard]] const std::vector<Sprite>& getSprites() const noexcept { return mSprites; }

    private:
        V99x8& mPPU;
        mutable std::vector<Sprite> mSprites;
};

// UI & Sequence States
class BaseState : public GameEngine::GameState {
    public:
        BaseState(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): GameEngine::GameState(sm), mPPU(ppu), mSoundEngine(se), mAssetManager(am) {}

    protected:
        V99x8&              mPPU;
        SoundEngine&        mSoundEngine;
        const AssetManager& mAssetManager;
};


class Intro : public BaseState {
    public:
        Intro(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am) {}

    protected:
        void enter() override ;
        void update(double dt) override;
        void exit() override;
        void handleInput(retro_input_state_t input_cb) override;
        void render() override; // Draws splash art

    private:
        uint16_t x_scroll = 0;
        uint16_t y_scroll = 0;

        Timer  mPushSpaceTimer;

        double mScrollY_F;
        bool   mLogoShown = false;
};

// Fake boot screen
class Boot : public BaseState {
    public:
        Boot(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am) {}

    protected:
        void enter() override final;
        void update(double dt) override final;
        void exit() override final;
        void handleInput(retro_input_state_t input_cb) override final;
        void render() override final;

    private:
        uint16_t mScrollY;
        bool     mSWDrawn;
};

class LevelSummary: public BaseState {
    public:
        LevelSummary(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am, uint32_t stageLevel, float timeToShow = 6.5f): 
            BaseState(sm, ppu, se, am), mStateLevel(stageLevel), mTimeToShow(static_cast<double>(timeToShow)) {}

    protected:
        void enter() override final;
        void update(double dt) override final;
        void exit() override final;
        void handleInput(retro_input_state_t input_cb) override final;
        void render() override final;

    protected:
        void finish();

    private:
        uint32_t mStateLevel;
        double   mTimeToShow;
};

class Game : public GameEngine::EngineCore<V99x8> {
    public:
        Game(double target_fps = 60.0);

        virtual constexpr uint16_t getFramebufferWidth() const { return FRAMEBUFFER_WIDTH; }
        virtual constexpr uint16_t getFramebufferHeight() const { return FRAMEBUFFER_HEIGHT; }
        virtual constexpr float getFramebufferAspect() const { return (float)FRAMEBUFFER_WIDTH / (float)FRAMEBUFFER_HEIGHT; }

        virtual constexpr  uint32_t getFramebufferStride() const { 
            return FRAMEBUFFER_WIDTH * 4 /* RGBA8888 */;
        }

    protected:
        [[nodiscard]] virtual bool initImpl();
        [[nodiscard]] virtual bool shutdownImpl();
};

}  // namespace KnightGame

#endif  // __RETRO_CORE_GAMES_KNIGHTMARE_GAME_H

