#ifndef __RETRO_CORE_TEST_GAME_H
#define __RETRO_CORE_TEST_GAME_H

#include "framework/ppu/ppu_raw.h"
#include "framework/game_engine/engine_core.h"

#define FRAMEBUFFER_WIDTH 320
#define FRAMEBUFFER_HEIGHT 224

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

    protected:
        void finish();

    private:
        V99x8& mPPU;

        uint16_t x_scroll = 0;
        uint16_t y_scroll = 0;

        double mScrollY_F;
        bool   mLogoShown = false;

        std::unique_ptr<GameEngine::MP3Stream> mBgmMp3Stream;

        GameEngine::AssetManager mAssetManager; // TODO: Source from game engine
};

class LevelBaseState : public GameEngine::GameState {
    public:
        LevelBaseState(V99x8& ppu, GameEngine::StateManager& sm): GameEngine::GameState(sm), mPPU(ppu) {}

    protected:
        void enter();
        void update(double dt) override;

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
            uint16_t pattern = 0;
            int16_t pos_x = 0;
            int16_t pos_y = 0;

            int16_t dir_x = 1;
            int16_t dir_y = 1;
        };

        V99x8& mPPU;

        std::unique_ptr<GameEngine::MP3Stream> mBgmMp3Stream;

        GameEngine::AssetManager mAssetManager; // TODO: Source from game engine
        std::vector<Sprite> mSprites;

        KnightGame::GameWorld mWorld;

        uint16_t mScrollY;
        double mScrollY_F;

    private:
        uint16_t mVerticalMapOffset = 0;
        bool     mUpdateTileSet;
        bool     mBossReached = false;
};

class Level1State : public LevelBaseState {
    public:
        Level1State(V99x8& ppu, GameEngine::StateManager& sm): LevelBaseState(ppu, sm) {}

    protected:
        void enter();
        void update(double dt);
        void exit() override;
        void handleInput(retro_input_state_t input_cb) override;
        void render() override;
        void renderAudio(int16_t* pSamplesData, size_t samples_per_frame) override;
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

    protected:
        void finish();

    private:
        V99x8& mPPU;

        std::unique_ptr<GameEngine::MP3Stream> mBgmMp3Stream;

        uint16_t mScrollY;
        bool     mSWDrawn;

        GameEngine::AssetManager mAssetManager; // TODO: Source from game engine
};

class TestMsxGame : public GameEngine::EngineCore<V99x8> {
    public:
        TestMsxGame(double target_fps = 60.0);

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

