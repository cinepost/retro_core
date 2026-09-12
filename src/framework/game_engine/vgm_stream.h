#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_VGM_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_VGM_STREAM_H

#include "sound_engine.h"

#include <cstdint>
#include <cstddef>

namespace RetroCore {

namespace GameEngine {

class VGMChipStream : public AudioSource {
    public:
        VGMChipStream(const uint8_t* vgmData, size_t dataSize): mIsEnabled(false) {

        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
            if (!mIsEnabled) return 0;

            return samplesRead;
        }

        bool IsFinished() const override { return !mIsValid; }

    private:
        bool mIsEnabled = false;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_VGM_STREAM_H