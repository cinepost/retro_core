#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H

#include "minimp3/minimp3_ex.h"

#include "asset_manager.h"
#include "sound_engine.h"

#include <cstdint>
#include <cstddef>
#include <cmath>

namespace RetroCore {

namespace GameEngine {

class MP3Stream : public AudioSource {
    public:
        MP3Stream(const AssetManager::Asset& asset, bool loop = true): MP3Stream(asset.pData, asset.sizeInBytes, loop) { }

        MP3Stream(const uint8_t* pMP3Data, size_t dataSize, bool loop = true): mIsValid(false), mLoop(loop) {
            assert(pMP3Data);

            if (pMP3Data && dataSize > 0 && mp3dec_ex_open_buf(&mDecoder, pMP3Data, dataSize, 0) == 0) {
                mIsValid = true;
                mSourceHz = mDecoder.info.hz;
                mSourceChannelCount = mDecoder.info.channels; 
                assert(mSourceChannelCount == 1 || mSourceChannelCount == 2);
                setTargetSampleRate(mSourceHz); // default target is equal to source
                mSourceBufferCache.reserve(2048); // default for 48000 stereo resampling
            }
        }

        ~MP3Stream() override {
            if (mIsValid) mp3dec_ex_close(&mDecoder);
        }

        void setTargetSampleRate(uint32_t targetHz) override {
            mTargetHz = targetHz;
            mStepRatio = static_cast<double>(mSourceHz) / static_cast<double>(mTargetHz);
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
            std::memset(targetBuffer, 0, sampleCount * sizeof(int16_t));

            if (!mIsValid || mTargetHz == 0) return 0;

            auto mp3dec_ex_read_stereo = [&](int16_t* pTargetBuffer, size_t sampleCount) {
                size_t samplesRead = mp3dec_ex_read(&mDecoder, pTargetBuffer, mSourceChannelCount == 2 ? sampleCount : (sampleCount / 2));

                // mono to stero if needed
                if (mSourceChannelCount == 1) {
                    mTempStereoBuffer.resize(sampleCount);

                    for (size_t i = 0; i < samplesRead; ++i) {
                        mTempStereoBuffer[i * 2]     = pTargetBuffer[i]; // Left
                        mTempStereoBuffer[i * 2 + 1] = pTargetBuffer[i]; // Right
                    }
                    samplesRead *= 2;
                    std::memcpy(pTargetBuffer, mTempStereoBuffer.data(), samplesRead * sizeof(int16_t));
                }

                return samplesRead;
            };

            // Source and Target sample rates match 
            if (mSourceHz == mTargetHz) {
                // Decode directly into the target mixer buffer, completely bypassing interpolation math

                //size_t samplesRead = mp3dec_ex_read(&mDecoder, targetBuffer, mSourceChannelCount == 2 ? sampleCount : (sampleCount / 2));
                size_t samplesRead = mp3dec_ex_read_stereo(targetBuffer, sampleCount);

                // Handle track termination or looping
                if (samplesRead < sampleCount) {
                    if (mLoop) {
                        mp3dec_ex_seek(&mDecoder, 0);
                        size_t remaining = sampleCount - samplesRead;
                        //mp3dec_ex_read(&mDecoder, targetBuffer + samplesRead, remaining);
                        mp3dec_ex_read_stereo(targetBuffer + samplesRead, remaining);
                        samplesRead = sampleCount;
                    } else {
                        size_t remaining = sampleCount - samplesRead;
                        std::memset(targetBuffer + samplesRead, 0, remaining * sizeof(int16_t));
                        mp3dec_ex_close(&mDecoder);
                        mIsValid = false;
                    }
                }
                return samplesRead;
            }

            // Frequencies mismatch, run high-fidelity cubic resampling
            size_t outputStereoFrames = sampleCount / 2;
            double totalSourceFramesNeeded = (outputStereoFrames * mStepRatio) + 4.0;
            size_t sourceFramesToFetch = static_cast<size_t>(std::ceil(totalSourceFramesNeeded));
            size_t totalSamplesToFetch = sourceFramesToFetch * 2;

            mSourceBufferCache.resize(totalSamplesToFetch);
            std::fill(mSourceBufferCache.begin(), mSourceBufferCache.end(), 0);


            size_t samplesRead = mp3dec_ex_read(&mDecoder, mSourceBufferCache.data(), totalSamplesToFetch);
            size_t framesRead = samplesRead / 2;

            if (framesRead == 0) {
                if (mLoop) {
                    mp3dec_ex_seek(&mDecoder, 0);
                    samplesRead = mp3dec_ex_read(&mDecoder, mSourceBufferCache.data(), totalSamplesToFetch);
                    framesRead = samplesRead / 2;
                } else {
                    mIsValid = false;
                    return 0;
                }
            }

            // If we hit EOF mid-buffer and loop is active, patch fill the remainder
            if (framesRead < sourceFramesToFetch && mLoop) {
                mp3dec_ex_seek(&mDecoder, 0);
                size_t samplesLeftToFill = totalSamplesToFetch - samplesRead;
                mp3dec_ex_read(&mDecoder, mSourceBufferCache.data() + samplesRead, samplesLeftToFill);
                framesRead = sourceFramesToFetch; // Force boundary evaluation limits
            }

            size_t targetIdx = 0;
            double localPlayhead = mPlaybackPosition; // Read from persistent tracker class context

            for (size_t f = 0; f < outputStereoFrames; ++f) {
                size_t baseFrame = static_cast<size_t>(localPlayhead);
                float alpha = static_cast<float>(localPlayhead - baseFrame);

                size_t idx0 = (baseFrame > 0) ? baseFrame - 1 : 0;
                size_t idx1 = baseFrame;
                size_t idx2 = (baseFrame + 1 < framesRead) ? baseFrame + 1 : framesRead - 1;
                size_t idx3 = (baseFrame + 2 < framesRead) ? baseFrame + 2 : framesRead - 1;

                // Verify array ranges to completely avoid OOB reads
                if (idx3 * 2 + 1 >= mSourceBufferCache.size()) {
                    idx0 = idx1 = idx2 = idx3 = 0; // Emergency safe fall-back to zero amplitude
                }

                for (int channel = 0; channel < 2; ++channel) {
                    float y0 = static_cast<float>(mSourceBufferCache[idx0 * 2 + channel]);
                    float y1 = static_cast<float>(mSourceBufferCache[idx1 * 2 + channel]);
                    float y2 = static_cast<float>(mSourceBufferCache[idx2 * 2 + channel]);
                    float y3 = static_cast<float>(mSourceBufferCache[idx3 * 2 + channel]);

                    float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
                    float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                    float c = -0.5f * y0 + 0.5f * y2;
                    float d = y1;

                    float finalSample = ((a * alpha + b) * alpha + c) * alpha + d;

                    // Hard clamp output bounds to protect integer formats from overflow clicks
                    if (finalSample > 32767.0f)  finalSample = 32767.0f;
                    if (finalSample < -32768.0f) finalSample = -32768.0f;

                    targetBuffer[targetIdx++] = static_cast<int16_t>(finalSample);
                }

                localPlayhead += mStepRatio;
            }

            // Update the global position offset, relative strictly to the frames we actually processed
            mPlaybackPosition = localPlayhead - framesRead;
            if (mPlaybackPosition < 0.0) {
                mPlaybackPosition = 0.0; // Prevent rounding drifts from pulling indexes negative
            }

            return sampleCount;
        }

        bool isFinished() const override { return !mIsValid; }

    private:
        mp3dec_ex_t mDecoder;
        bool mIsValid = false;
        bool mLoop = true;

        // defaults
        uint32_t mSourceHz = 44100;
        uint32_t mTargetHz = 44100;
        int      mSourceChannelCount = 2;

        double mStepRatio = 1.0;
        double mPlaybackPosition = 0.0;

        std::vector<int16_t> mSourceBufferCache;
        std::vector<int16_t> mTempStereoBuffer;

};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H