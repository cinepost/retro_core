#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_AUDIO_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_AUDIO_H

#include "minimp3/minimp3_ex.h"

#include "libretro.h"

#include <cstdint>
#include <cstddef>

namespace RetroCore {

namespace GameEngine {

class AudioSource {
    public:
        virtual ~AudioSource() = default;

        // Fills the target buffer with interleaved 16-bit stereo samples.
        // Returns the actual number of samples written.
        virtual size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) = 0;
};


class MP3Stream : public AudioSource {
    public:
        MP3Stream(const uint8_t* mp3Data, size_t dataSize, bool loop = true) : mLoop(loop) {
            if (mp3dec_ex_open_buf(&mDecoder, mp3Data, dataSize, 0) == 0) {
                mIsValid = true;
            }
        }

        ~MP3Stream() override {
            if (mIsValid) {
                mp3dec_ex_close(&mDecoder);
            }
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
            if (!mIsValid) return 0;

            size_t samplesRead = mp3dec_ex_read(&mDecoder, targetBuffer, sampleCount);

            // Handle structural stream looping natively inside the state's memory tracker
            if (samplesRead < sampleCount && mLoop) {
                mp3dec_ex_seek(&mDecoder, 0);
                size_t remaining = sampleCount - samplesRead;
                mp3dec_ex_read(&mDecoder, targetBuffer + samplesRead, remaining);
                samplesRead = sampleCount;
            }

            return samplesRead;
        }

    private:
        mp3dec_ex_t mDecoder;
        bool mIsValid = false;
        bool mLoop = true;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_GAME_AUDIO_H