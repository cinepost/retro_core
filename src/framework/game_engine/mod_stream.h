#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MOD_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MOD_STREAM_H

#ifndef POCKETMOD_IMPLEMENTATION
#define POCKETMOD_IMPLEMENTATION
#endif
#include "pocketmod/pocketmod.h"

#include "sound_engine.h"

#include <cstdint>
#include <cstddef>

namespace RetroCore {

namespace GameEngine {

class MODStream : public AudioSource {
    public:
        MODStream(const uint8_t* modData, size_t dataSize, bool loop = true): mIsValid(false), mLoop(loop) {
            if (pocketmod_init(&mContext, modData, static_cast<int>(dataSize), 44100)) {
                mIsValid = true;
                CalculateLength();
            }
        }

        ~MODStream() override {
            if (mIsValid) mp3dec_ex_close(&mDecoder);
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
             if (!mIsValid) {
                std::memset(targetBuffer, 0, sampleCount * sizeof(int16_t));
                return 0;
            }

            // Pocketmod renders interleaved stereo samples. 
            // 1 sample frame = 2 array slots (Left + Right channels).
            int totalFramesNeeded = static_cast<int>(sampleCount / 2);
            
            // pocketmod_render returns the number of bytes rendered.
            // Size of 1 stereo frame = 2 channels * sizeof(int16_t) = 4 bytes.
            int bytesRendered = pocketmod_render(&mContext, targetBuffer, totalFramesNeeded * 4);
            int framesRendered = bytesRendered / 4;
            size_t samplesRendered = static_cast<size_t>(framesRendered * 2);

            // If the tracker hits the end of the song arrangement sequence
            if (samplesRendered < sampleCount) {
                if (mLoop) {
                    // Loop: Rewind pocketmod internals back to pattern index 0
                    pocketmod_init(&mContext, mContext.source, mContext.source_size, mContext.samples_per_second);
                    
                    size_t remainingSamples = sampleCount - samplesRendered;
                    int16_t* nextBufferSlot = targetBuffer + samplesRendered;
                    
                    int extraBytes = pocketmod_render(&mContext, nextBufferSlot, static_cast<int>(remainingSamples / 2) * 4);
                    samplesRendered += static_cast<size_t>((extraBytes / 4) * 2);
                } else {
                    // Play once: Fill the remainder of the buffer window with absolute silence
                    size_t remainingSamples = sampleCount - samplesRendered;
                    std::memset(targetBuffer + samplesRendered, 0, remainingSamples * sizeof(int16_t));
                    mIsValid = false; // Flag track completed
                }
            }

            return samplesRendered;
        }

        bool isFinished() const override { return !mIsValid; }

    private:
        pocketmod_context mContext;
        bool mIsValid = false;
        bool mLoop = true;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MOD_STREAM_H