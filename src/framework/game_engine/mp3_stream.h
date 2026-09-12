#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H

#include "minimp3/minimp3_ex.h"

#include "sound_engine.h"

#include <cstdint>
#include <cstddef>

namespace RetroCore {

namespace GameEngine {

class MP3Stream : public AudioSource {
    public:
        MP3Stream(const uint8_t* mp3Data, size_t dataSize, bool loop = true): mIsValid(false), mLoop(loop) {
            if (mp3dec_ex_open_buf(&mDecoder, mp3Data, dataSize, 0) == 0) {
                mIsValid = true;
            }
        }

        ~MP3Stream() override {
            if (mIsValid) mp3dec_ex_close(&mDecoder);
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
             if (!mIsValid) {
                std::memset(targetBuffer, 0, sampleCount * sizeof(int16_t));
                return 0;
            }

            size_t samplesRead = mp3dec_ex_read(&mDecoder, targetBuffer, sampleCount);

            if (samplesRead < sampleCount) {
                if (mLoop) {
                    mp3dec_ex_seek(&mDecoder, 0);
                    size_t remaining = sampleCount - samplesRead;
                    mp3dec_ex_read(&mDecoder, targetBuffer + samplesRead, remaining);
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

        bool isFinished() const override { return !mIsValid; }

    private:
        mp3dec_ex_t mDecoder;
        bool mIsValid = false;
        bool mLoop = true;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MP3_STREAM_H