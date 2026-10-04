#ifndef __RETRO_CORE_GAMES_KNIGHTMARE_GAME_H
#define __RETRO_CORE_GAMES_KNIGHTMARE_GAME_H

#include "framework/ppu/ppu_msx.h"
#include "framework/oscillators.h"
#include "framework/game_engine/engine_core.h"
#include "framework/game_engine/game_state.h"
#include "framework/game_engine/mp3_stream.h"
#include "framework/game_engine/menu.h"

#include <cmath>
#include <cstdint>

#define FRAMEBUFFER_WIDTH 512
#define FRAMEBUFFER_HEIGHT 288

using namespace RetroCore;

using V99x8 = PPU::MsxPPU<{FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT}>;
using Asset = GameEngine::AssetManager::Asset;
using AssetManager = GameEngine::AssetManager;
using SoundEngine  = GameEngine::SoundEngine;
using MP3Stream    = GameEngine::MP3Stream;
using StateManager = GameEngine::StateManager;
using Menu = GameEngine::Menu;
using MenuEntry = GameEngine::MenuEntry;
using Input = GameEngine::Input;

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
            Sprite(int _x, int _y, uint16_t _pattern, uint8_t _attr): x(static_cast<int16_t>(_x)), y(static_cast<int16_t>(_y)), pattern(_pattern), attribs(_attr) {}
            Sprite(int _x, int _y, uint16_t _pattern, int _attr): x(static_cast<int16_t>(_x)), y(static_cast<int16_t>(_y)), pattern(_pattern), attribs(static_cast<uint8_t>(_attr & 0x00FF)) {
                assert(_attr <= 256);
            }

            Sprite(int _x, int _y, int _pattern, uint8_t _attr): x(static_cast<int16_t>(_x)), y(static_cast<int16_t>(_y)), pattern(static_cast<uint16_t>(_pattern & 0x0000FFFF)), attribs(_attr) {
                assert(_pattern < 65536);
            }
            Sprite(int _x, int _y, int _pattern, int _attr): x(static_cast<int16_t>(_x)), y(static_cast<int16_t>(_y)), pattern(static_cast<uint16_t>(_pattern & 0x0000FFFF)), attribs(static_cast<uint8_t>(_attr & 0x00FF)) {
                assert(_pattern < 65536);
                assert(_attr < 256);
            }
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
        Intro(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am, bool skip_to_menu = false);

    protected:
        void enter() override ;
        void update(double dt) override;
        void exit() override;
        void handleInput(const Input& input) override;
        void render() override; // Draws splash art

    private:
        enum class State: uint8_t {
            SCROLLING,
            LOGO,
            MENU,
            COUNT
        };

        void startLevel(bool short_display_time = false);

    private:
        Timer   mPushSpaceTimer;
        Timer   mScrollTimer;

        uint16_t mScrollY = 0;

        State  mState = State::SCROLLING;
        SquareWaveOscillator<float, uint> mFlashingOscillator; // text flashing
        Timer mTearTimer;

        Menu mMenu;
        Menu* mpCurrentMenu = nullptr;

        bool mLogoShown = false;
};

// Fake boot screen
class Boot : public BaseState {
    public:
        Boot(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am): BaseState(sm, ppu, se, am) {}

    protected:
        void enter() override final;
        void update(double dt) override final;
        void exit() override final;
        void handleInput(const Input& input) override final;
        void render() override final;

    private:
        uint16_t mScrollY;
        bool     mSWDrawn;
};

class LevelSummary: public BaseState {
    public:
        LevelSummary(StateManager& sm, V99x8& ppu, SoundEngine& se, const AssetManager& am, uint32_t stageLevel, float time_to_show = -1.0f): 
            BaseState(sm, ppu, se, am), mStateLevel(stageLevel), mTimeToShow(static_cast<double>(time_to_show <= 0.0f ? 6.5f : time_to_show )) {}

    protected:
        void enter() override final;
        void update(double dt) override final;
        void exit() override final;
        void handleInput(const Input& input) override final;
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

