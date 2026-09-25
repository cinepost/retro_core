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
        using UniquePtr = std::unique_ptr<AudioSource>;

        virtual ~AudioSource() = default;

        // Fills the target buffer with interleaved 16-bit stereo samples.
        // Returns the actual number of samples written.
        virtual size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) = 0;

        virtual void setTargetSampleRate(uint32_t targetHz) {}

        virtual bool isFinished() const = 0;

        // Returns the calculated length in seconds
        // Returns maximum float value if the length is variable, streaming, or unknown.
        virtual float getLength() const { return std::numeric_limits<float>::max(); }

        // Returns current playback timestamp position in seconds. 
        // Defaults to 0.0f if not trackable or stream-driven.
        virtual float getPosition() const { return 0.0f; }
};  

class CrossfadeMixer : public AudioSource {
    public:
        // Takes ownership of the source tracks and configures the duration window in seconds
        CrossfadeMixer(std::unique_ptr<AudioSource> pCurrentSource, std::unique_ptr<AudioSource> pTargetSource, float durationSeconds)
            : mpSourceA(std::move(pCurrentSource))
            , mpSourceB(std::move(pTargetSource))
            , mDuration(durationSeconds)
            , mElapsed(0.0f)
            , mIsFinished(false)
        {
            // Safety bounds check to avoid division-by-zero errors
            if (mDuration <= 0.0f) mDuration = 0.01f;
            mTmpBufferA.reserve(2048);
            mTmpBufferB.reserve(2048);
        }

        ~CrossfadeMixer() override = default;

        virtual void setTargetSampleRate(uint32_t targetHz) override {
            if(mpSourceA) mpSourceA->setTargetSampleRate(targetHz);
            if(mpSourceB) mpSourceB->setTargetSampleRate(targetHz);
        }

        // The mixer finishes when the target track completes or if the crossfade naturally drops out
        bool isFinished() const override { return mIsFinished; }

        // Returns the remaining duration profile of the target destination track
        float getLength() const override {
            return mpSourceB ? mpSourceB->getLength() : 0.0f;
        }

        float getPosition() const override {
            return mpSourceB ? mpSourceB->getPosition() : 0.0f;
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
            if (mIsFinished) return 0;

            // 1. Advance the internal crossfade transition timeline clock
            // 44100Hz Stereo means 1 second = 88200 total samples
            float secondsPerSample = 1.0f / 88200.0f;
            
            // Setup cache buffers to pull component waveforms
            // Instead of local allocations, real implementations recycle class arrays, 
            // but since stack buffers are temporary we clear them with stack limits
            mTmpBufferA.resize(sampleCount);
            mTmpBufferB.resize(sampleCount);

            std::memset(mTmpBufferA.data(), 0, sampleCount * sizeof(int16_t));
            std::memset(mTmpBufferB.data(), 0, sampleCount * sizeof(int16_t));

            if (mpSourceA && !mpSourceA->isFinished()) mpSourceA->renderPCM(mTmpBufferA.data(), sampleCount);
            if (mpSourceB && !mpSourceB->isFinished()) mpSourceB->renderPCM(mTmpBufferB.data(), sampleCount);

            // 2. Perform sample-by-sample fixed-point volume attenuation mixing
            for (size_t i = 0; i < sampleCount; ++i) {
                // Track completion progression scale: 0.0f to 1.0f
                float progress = mElapsed / mDuration;
                if (progress > 1.0f) progress = 1.0f;

                // Compute fixed-point Q16 gain parameters
                uint32_t gainB = static_cast<uint32_t>(progress * 65536.0f);
                uint32_t gainA = 65536 - gainB; // Inverse fade mapping

                // Apply fixed-point volume filters completely inside fast integer spaces
                int32_t sampleA = (static_cast<int32_t>(mTmpBufferA[i]) * static_cast<int32_t>(gainA)) >> 16;
                int32_t sampleB = (static_cast<int32_t>(mTmpBufferB[i]) * static_cast<int32_t>(gainB)) >> 16;

                int32_t mixed = sampleA + sampleB;

                // Clamping threshold safety guards
                if (mixed > 32767)  mixed = 32767;
                if (mixed < -32768) mixed = -32768;

                targetBuffer[i] = static_cast<int16_t>(mixed);

                // Increment timeline position incrementally per sub-sample block step
                mElapsed += secondsPerSample;
            }

            // 3. Lifecycle Termination Check: If fade completes, flag cleanup window cuts
            if (mElapsed >= mDuration) {
                mIsFinished = true;
            }

            return sampleCount;
        }

        // Transfers ownership of the finalized track out when the fade concludes
        std::unique_ptr<AudioSource> extractTargetSource() {
            return std::move(mpSourceB);
        }

    private:
        std::unique_ptr<AudioSource> mpSourceA; // Fading Out track
        std::unique_ptr<AudioSource> mpSourceB; // Fading In track
        float mDuration;
        float mElapsed;
        bool mIsFinished;

        std::vector<int16_t> mTmpBufferA;
        std::vector<int16_t> mTmpBufferB;
};


class SoundEngine {
    public:
        SoundEngine(uint32_t sampleRate = 44100): mMasterVolume(65536), mOutputSampleRate(sampleRate) {
            mIntMixBuffer.reserve(2048);
            mTempReadBuffer.reserve(2048);
        }

        void setOutputSampleRate(uint32_t outputHz) {
            mOutputSampleRate = outputHz;

            if(mpBgmTrack) mpBgmTrack->setTargetSampleRate(mOutputSampleRate);
            for(auto& pSFX: mpActiveSFX ) {
                pSFX->setTargetSampleRate(mOutputSampleRate);
            }
        }

        uint32_t getOutputSampleRate() const { return mOutputSampleRate; }

        void setMasterVolume(float volume) const {
            volume = std::min(0.0f, std::max(1.0f, volume));
            mMasterVolume = static_cast<uint32_t>(volume * 65536.0f);
        }

        void playBGM(std::unique_ptr<AudioSource> bgmTrack) const {
            if(bgmTrack) {
                bgmTrack->setTargetSampleRate(mOutputSampleRate);
            }
            mpBgmTrack = std::move(bgmTrack);
        }

        void stopBGM() const { 
            mpBgmTrack.reset();
        }

        void crossfadeBGM(std::unique_ptr<AudioSource> pNextTrack, float fadeDurationSeconds) {
            if (!mpBgmTrack) {
                playBGM(std::move(pNextTrack));
                return;
            }

            pNextTrack->setTargetSampleRate(mOutputSampleRate);

            // Wrap the old BGM and the new track into the crossfade manager object container
            auto mixer = std::make_unique<CrossfadeMixer>(std::move(mpBgmTrack), std::move(pNextTrack), fadeDurationSeconds);

            mpActiveBgmTrackCrossfade = mixer.get();

            // Direct the master loop pointer to read from the crossfade mixer node
            mpBgmTrack = std::move(mixer);
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
                sfxTrack->setTargetSampleRate(mOutputSampleRate);
                mpActiveSFX.push_back(std::move(sfxTrack));
            }
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

            // Crossfade
            if (mpActiveBgmTrackCrossfade && mpActiveBgmTrackCrossfade->isFinished()) {
                // Extract the target track out of the temporary wrapper, discarding the old dead song
                std::unique_ptr<AudioSource> finalTrack = mpActiveBgmTrackCrossfade->extractTargetSource();
                mpActiveBgmTrackCrossfade = nullptr;
            
                // Promote the target track to be the main permanent background loop tracker
                mpBgmTrack = std::move(finalTrack);
            }

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
        mutable std::unique_ptr<AudioSource> mpBgmTrack;
        mutable std::vector<std::unique_ptr<AudioSource>> mpActiveSFX;

        mutable CrossfadeMixer* mpActiveBgmTrackCrossfade;

        // Internal fixed-point representation of master volume (65536 = 100% volume)
        mutable uint32_t mMasterVolume = 65536; 
        mutable uint32_t mOutputSampleRate;

        std::vector<int32_t> mIntMixBuffer;
        std::vector<int16_t> mTempReadBuffer;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_SOUND_ENGINE_H