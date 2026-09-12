#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_ENGINE_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_ENGINE_H

#include "asset_manager.h"

#include <vector>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cstring>
#include <limits>

namespace RetroCore {

namespace GameEngine {

class AudioSource {
    public:
        virtual ~AudioSource() = default;

        // Fills the target buffer with interleaved 16-bit stereo samples.
        // Returns the actual number of samples written.
        virtual size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) = 0;

        virtual bool isFinished() const = 0;

        // Returns the calculated length in seconds
        // Returns maximum float value if the length is variable, streaming, or unknown.
        virtual float getLength() const { return std::numeric_limits<float>::max(); }

        // Returns current playback timestamp position in seconds. 
        // Defaults to 0.0f if not trackable or stream-driven.
        virtual float getPosition() const { return 0.0f; }
};  

class SoundEngine {
    public:
        SoundEngine() = default;

        void playBGM(std::unique_ptr<AudioSource> bgmTrack) const {
            assert(bgmTrack);
            mpBgmTrack = std::move(bgmTrack);
        }

        void stopBGM() const { 
            mpBgmTrack.reset();
        }

        bool isBGMPlaying() const { return mpBgmTrack && !mpBgmTrack->isFinished(); }

        float getBGMLength() const {
            return (mpBgmTrack) ? mpBgmTrack->getLength() : 0.0f;
        }

        float getBGMPosition() const {
            return (mpBgmTrack) ? mpBgmTrack->getPosition() : 0.0f;
        }

        void playSFX(std::unique_ptr<AudioSource> sfxTrack) const {
            if (sfxTrack) {
                mpActiveSFX.push_back(std::move(sfxTrack));
            }
        }

        void setMasterVolume(float volume) const {
            volume = std::min(0.0f, std::max(1.0f, volume));
            mMasterVolume = static_cast<uint32_t>(volume * 65536.0f);
        }

        void mixFrameAudio(int16_t* outPcmBuffer, size_t sampleCount) {
            // drop dead sound components smoothly
            mpActiveSFX.erase(
                std::remove_if(mpActiveSFX.begin(), mpActiveSFX.end(),
                    [](const std::unique_ptr<AudioSource>& sfx) { return sfx->isFinished(); }),
                mpActiveSFX.end()
            );

            mIntMixBuffer.resize(sampleCount);
            mTempReadBuffer.resize(sampleCount);

            std::fill(mIntMixBuffer.begin(), mIntMixBuffer.end(), 0);

            // Mix bgm music channel
            if (mpBgmTrack && !mpBgmTrack->isFinished()) {
                mpBgmTrack->renderPCM(mTempReadBuffer.data(), sampleCount);
                for (size_t i = 0; i < sampleCount; ++i) {
                    mIntMixBuffer[i] += mTempReadBuffer[i];
                }
            }

            // mix all concurrent SFX channels (MP3, WAV, VGM tracking nodes alike!)
            for (auto& sfx : mpActiveSFX) {
                std::memset(mTempReadBuffer.data(), 0, sampleCount * sizeof(int16_t));
                sfx->renderPCM(mTempReadBuffer.data(), sampleCount);
                for (size_t i = 0; i < sampleCount; ++i) {
                    mIntMixBuffer[i] += mTempReadBuffer[i];
                }
            }

            // clip safeguard clamp & downsample back down to 16-bit PCM bounds
            for (size_t i = 0; i < sampleCount; ++i) {
                int32_t mixedSample = mIntMixBuffer[i];
                mixedSample = (mixedSample * static_cast<int32_t>(mMasterVolume)) >> 16;

                // Strict hard clamping safeguards to prevent nasty numerical wrapping errors
                if (mixedSample > 32767)  mixedSample = 32767;
                if (mixedSample < -32768) mixedSample = -32768;
                outPcmBuffer[i] = static_cast<int16_t>(mixedSample);
            }
        }

    private:
        mutable std::unique_ptr<GameEngine::AudioSource> mpBgmTrack;
        mutable std::vector<std::unique_ptr<AudioSource>> mpActiveSFX;

        // Internal fixed-point representation of master volume (65536 = 100% volume)
        mutable uint32_t mMasterVolume = 65536; 

        std::vector<int32_t> mIntMixBuffer;
        std::vector<int16_t> mTempReadBuffer;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_ENGINE_H