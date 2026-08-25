#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ENGINE_CORE_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ENGINE_CORE_H

#include "asset_manager.h"

#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <string>
#include <cassert>
#include <iostream>

#include "libretro.h"


namespace RetroCore {

namespace GameEngine {

// Forward declarations
class GameState;

class StateManager {
    public:
        StateManager() {
        }

        ~StateManager() {
        }

        void changeState(std::unique_ptr<GameState> pState);
        void pushState(std::unique_ptr<GameState> pState);
        void popState();
        void handleInput(retro_input_state_t input_cb);
        void update(double dt);
        void render();
        void renderAudio(int16_t* pSamplesData, size_t samples_per_frame);
        void reset();
        void clearAllAndChangeState(std::unique_ptr<GameState> pState);

    private:
        std::vector<std::unique_ptr<GameState>> mStates;

        friend class GameState;
};


// High-precision clock definitions for framerate locking
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double>;

template<typename PPU>
class EngineCore {
    public:
        EngineCore(double targetFps = 60.0): mPPU() {
            assert(targetFps > 0.0f);

            mTargetFps = targetFps;
            mTargetFrameDuration = 1.0 / mTargetFps;

            mSamplesPerFrame = (44100.0 / mTargetFps) * 2; 
            mPCMMixBuffer.resize(mSamplesPerFrame);
        }

        virtual ~EngineCore() = default;

        const AssetManager& getAssetManager() const;

        // Call this inside retro_load_game to ingest the static frontend pointers
        void bindLibretroEnvironmentCallback(retro_environment_t cb) {
            m_environ_cb = cb;
        }

        void bindLibretroVideoCallback(retro_video_refresh_t cb) {
            m_video_cb = cb;
        }

        void bindLibretroAudioCallback(retro_audio_sample_t cb) {
            m_audio_cb = cb;
        }

        void bindLibretroAudioBatchCallback(retro_audio_sample_batch_t cb) {
            m_audio_batch_cb = cb;
        }

        void bindLibretroInputPollCallback(retro_input_poll_t cb) {
            m_input_poll_cb = cb;
        }

        void bindLibretroInputStateCallback(retro_input_state_t cb) {
            m_input_state_cb = cb;
        }

        [[nodiscard]] bool init() {
            if(mPPU.init()) {
                return initImpl();
            }
            std::cerr << "Error initalizing PPU.\n";
            return false;
        }

        void processAudio() {
            if (!m_audio_batch_cb) return;

            std::memset(mPCMMixBuffer.data(), 0, mPCMMixBuffer.size() * sizeof(uint16_t));

            // If a GameState has attached an audio source, pull samples from it
            mStateManager.renderAudio(mPCMMixBuffer.data(), mSamplesPerFrame);
            
            // Deliver raw stereo PCM blocks to the active Libretro frontend
            m_audio_batch_cb(mPCMMixBuffer.data(), mPCMMixBuffer.size() / 2);
        }


        void renderFrame() {
            // Poll input via libretro callback
            if (m_input_poll_cb) {
                m_input_poll_cb();
            }

            // Drive engine updates (fixed delta time provided by target frame rate)
            mStateManager.handleInput(m_input_state_cb);
            mStateManager.update(mTargetFrameDuration);

            // Render state layout into virtual PPU structures
            mStateManager.render();

            // Rasterize VRAM + OAM to raw 32-bit pixel array
            const uint8_t *buf = nullptr;
            uint32_t stride_bytes = getFramebufferStride();
            struct retro_framebuffer fb = {0};
            fb.width = PPU::getScreenWidth();
            fb.height = PPU::getScreenHeight();
            fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;

            if (m_environ_cb && m_environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_XRGB8888) {
                // Direct to libretro rendering
                stride_bytes = fb.pitch;
                mPPU.render(static_cast<uint8_t*>(fb.data), stride_bytes);
                buf = static_cast<const uint8_t*>(fb.data);
            } else {
                // Rendering into intermediate buffer
               // static std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT * 4> sFramebuffer; 
                mPPU.render(mFramebuffer.data(), stride_bytes);
                buf = mFramebuffer.data();
            }

            // Send frame data to the frontend window via Libretro callback
            if (m_video_cb) {
                assert(buf);
                m_video_cb(buf, fb.width, fb.height, stride_bytes);
            }

            processAudio();
        }

        bool shutdown() {
            if(!shutdownImpl()) {
                return false;
            }

            mStateManager.reset();
            mPPU.deinit();
            return true;
        }

        [[nodiscard]] const PPU& getPPU() const noexcept { return mPPU; }

        [[nodiscard]] double getTargetFPS() const { return mTargetFps; }
        [[nodiscard]] double getSoundSamplingRate() const { return mSoundSamplingRate; }

        PPU& getPPU() { return mPPU; }

        virtual constexpr uint32_t getFramebufferStride() const = 0;
        virtual constexpr uint16_t getFramebufferWidth() const = 0;
        virtual constexpr uint16_t getFramebufferHeight() const = 0;
        virtual constexpr float getFramebufferAspect() const = 0;

    protected:
        [[nodiscard]] virtual bool initImpl() = 0;
        [[nodiscard]] virtual bool shutdownImpl() = 0;

        // Expose the manager so derived custom games can load states
        [[nodiscard]] inline StateManager& getStateManager() noexcept { return mStateManager; }

    private:
        PPU mPPU;
        
    private:
        double  mTargetFps;
        double  mTargetFrameDuration;
        size_t  mSamplesPerFrame;
        double  mSoundSamplingRate = 44100.0;

        std::array<uint8_t, PPU::getScreenWidth() * PPU::getScreenHeight() * 4> mFramebuffer;
        std::vector<int16_t> mPCMMixBuffer;

        StateManager mStateManager;
        AssetManager mAssetManager; // Central container initialized once 

        // Libretro core callbacks storage
        retro_environment_t          m_environ_cb = nullptr;
        retro_video_refresh_t        m_video_cb = nullptr;
        retro_audio_sample_t         m_audio_cb = nullptr;
        retro_audio_sample_batch_t   m_audio_batch_cb = nullptr;
        retro_input_poll_t           m_input_poll_cb = nullptr;
        retro_input_state_t          m_input_state_cb = nullptr;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ENGINE_CORE_H