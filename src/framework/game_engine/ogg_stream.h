#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_OGG_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_OGG_STREAM_H

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c" 

#include "sound_engine.h"

#include <cstdint>
#include <cstddef>

namespace RetroCore {

namespace GameEngine {

class OGGStream : public AudioSource {
    public:
        OGGStream(const uint8_t* pOGGData, size_t dataSize, bool loop = true):mpStream(nullptr), mLoop(loop) {
            if (!pOGGData || dataSize == 0) {
                std::cerr << "Invalid data pointer or size passed to OGGStream." << std::endl;
                return;
            }

            int error = 0;
            // Open the OGG stream directly from allocated RAM
            mpStream = stb_vorbis_open_memory(pOGGData, static_cast<int>(dataSize), &error, nullptr);
            
            if (!mpStream) {
                std::cerr << "Failed to open OGG memory stream. Error code: " << error << std::endl;
                return;
            }

            stb_vorbis_info info = stb_vorbis_get_info(mpStream);
            mChannels = info.channels;
            mSampleRate = info.sample_rate;
        }

        ~OGGStream() override {
            if (mpStream) {
                stb_vorbis_close(mpStream);
            }
        }

        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) override {
            if (!mpStream || !targetBuffer || sampleCount == 0 || mChannels == 0) {
                return 0;
            }

            size_t totalElementsRendered = 0;
        
            // Track our remaining buffer requirements in flat int16_t slots
            size_t elementsRemaining = sampleCount;
            int16_t* currentWritePtr = targetBuffer;


            while (elementsRemaining > 0) {
                int samplesPerChannelRequested = static_cast<int>(elementsRemaining / mChannels);
                if (samplesPerChannelRequested == 0) break;

                // Fetch PCM data from decoding state
                int samplesDecodedPerChannel = stb_vorbis_get_samples_short_interleaved(
                    mpStream, 
                    mChannels, 
                    currentWritePtr, 
                    samplesPerChannelRequested
                );

                size_t elementsRenderedThisPass = static_cast<size_t>(samplesDecodedPerChannel * mChannels);

                if (elementsRenderedThisPass > 0) {
                    totalElementsRendered += elementsRenderedThisPass;
                    elementsRemaining -= elementsRenderedThisPass;
                    currentWritePtr += elementsRenderedThisPass;
                }

                // EOF handling: if the stream didn't fill the requested window
                if (elementsRenderedThisPass < (static_cast<size_t>(samplesPerChannelRequested) * mChannels)) {
                    if (mLoop) {
                        // Reset read cursor back to structural file beginning
                        stb_vorbis_seek_start(mpStream);
                        
                        // If nothing was decoded and we can't seek, break out to prevent infinite loops
                        if (elementsRenderedThisPass == 0 && samplesDecodedPerChannel == 0) {
                            break; 
                        }
                    } else {
                        // Loop is disabled, zero out any remaining buffer space and exit
                        std::fill_n(currentWritePtr, elementsRemaining, 0);
                        break;
                    }
                }
            }

            return totalElementsRendered;
        }

        bool isFinished() const override { return !mIsValid; }

    private:
        stb_vorbis* mpStream;
        int mChannels = 0;
        int mSampleRate = 0;
        bool mLoop = true;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_OGG_STREAM_H