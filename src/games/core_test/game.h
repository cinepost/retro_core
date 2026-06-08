#ifndef __RETRO_CORE_TEST_GAME_H
#define __RETRO_CORE_TEST_GAME_H

#include "framework/ppu/ppu_msx.h"
#include "framework/game_engine/engine_core.h"
#include "framework/game_engine/game_state.h"
#include "framework/game_engine/game_audio.h"

#define FRAMEBUFFER_WIDTH 512
#define FRAMEBUFFER_HEIGHT 288

using namespace RetroCore;

using V99x8 = PPU::MsxPPU<{FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT}>;

// UI & Sequence States
class IntroState : public GameEngine::GameState {
    public:
        IntroState(V99x8& ppu, GameEngine::StateManager& sm): GameEngine::GameState(sm), mPPU(ppu) {}

    protected:
        void enter();
        void update(double dt) override;
        void exit() override;
        void handleInput(retro_input_state_t input_cb) override;
        void render() override; // Draws splash art
        void renderAudio(int16_t* pSamplesData, size_t samples_per_frame) override;

    private:
        V99x8& mPPU;

        uint16_t x_scroll = 0;
        uint16_t y_scroll = 0;

        double mScrollY_F;

        std::unique_ptr<GameEngine::MP3Stream> mBgmMp3Stream;

        GameEngine::AssetManager mAssetManager; // TODO: Source from game engine
};

class Level1State : public GameEngine::GameState {
    public:
        Level1State(V99x8& ppu, GameEngine::StateManager& sm): GameEngine::GameState(sm), mPPU(ppu) {}

    protected:
        void enter();
        void update(double dt) override;
        void exit() override;
        void handleInput(retro_input_state_t input_cb) override;
        void render() override;
        void renderAudio(int16_t* pSamplesData, size_t samples_per_frame) override {};

    private:
        V99x8& mPPU;

        uint16_t mScrollY;
};

class BootState : public GameEngine::GameState {
    public:
        BootState(V99x8& ppu, GameEngine::StateManager& sm): GameEngine::GameState(sm), mPPU(ppu) {}

    protected:
        void enter();
        void update(double dt) override;
        void exit() override;
        void handleInput(retro_input_state_t input_cb) override;
        void render() override; // Draws fake boot screen
        void renderAudio(int16_t* pSamplesData, size_t samples_per_frame) override;

    private:
        V99x8& mPPU;

        std::unique_ptr<GameEngine::MP3Stream> mBgmMp3Stream;

        uint16_t mScrollY;
        bool     mSWDrawn;

        GameEngine::AssetManager mAssetManager; // TODO: Source from game engine
};

class TestMsxGame : public GameEngine::EngineCore<V99x8> {
    public:
        TestMsxGame(double target_fps = 60.0): GameEngine::EngineCore<V99x8>(target_fps) {

        }

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

#endif  // __RETRO_CORE_TEST_GAME_H

