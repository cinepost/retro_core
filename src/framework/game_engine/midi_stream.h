#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MIDI_STREAM_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MIDI_STREAM_H

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <algorithm>

// Define implementation flags in ONE compilation unit before including
#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include "sound_engine.h"

namespace RetroCore {

namespace GameEngine {

class MIDIStream {
    public:
        // Constructor matching your OGGStream logic
        // midiData: pointer to raw .mid file payload loaded into memory
        // sf2Data: pointer to raw SoundFont (.sf2) instrument data loaded into memory
        MIDIStream(const uint8_t* midiData, size_t midiSize, 
                   const uint8_t* sf2Data, size_t sf2Size, 
                   bool loop = true) 
            : mLoop(loop) 
        {
            if (!midiData || midiSize == 0 || !sf2Data || sf2Size == 0) {
                std::cerr << "Invalid data buffers passed to MIDIStream." << std::endl;
                return;
            }

            // 1. Load the SoundFont synthesizer instrument bank from memory
            mSynth = tsf_load_memory(sf2Data, static_cast<int>(sf2Size));
            if (!mSynth) {
                std::cerr << "Failed to load SoundFont bank from memory." << std::endl;
                return;
            }

            // Configure the synth for standard interleaved stereo
            tsf_set_output(mSynth, TSF_STEREO_INTERLEAVED, mSampleRate, 0.0f);

            // 2. Parse the MIDI track commands from memory
            mMidiHead = tml_load_memory(midiData, static_cast<int>(midiSize));
            if (!mMidiHead) {
                std::cerr << "Failed to parse MIDI sequence data from memory." << std::endl;
                tsf_close(mSynth);
                mSynth = nullptr;
                return;
            }

            // Initialize play head position
            mCurrentMsg = mMidiHead;
        }

        ~MIDIStream() {
            if (mMidiHead) tml_free(mMidiHead);
            if (mSynth) tsf_close(mSynth);
        }

        // --- YOUR MATCHING INTEGRATED FUNCTION ---
        // targetBuffer: Destination array provided by your audio engine
        // sampleCount: The number of flat int16_t values requested (Samples * Channels)
        size_t renderPCM(int16_t* targetBuffer, size_t sampleCount) {
            if (!mSynth || !mMidiHead || !targetBuffer || sampleCount == 0) {
                return 0;
            }

            size_t totalElementsRendered = 0;
            size_t elementsRemaining = sampleCount;
            int16_t* currentWritePtr = targetBuffer;

            // Number of stereo frames requested (L/R pair = 1 frame)
            size_t totalFramesRequested = sampleCount / mChannels;
            size_t framesRendered = 0;

            // Process audio block by block based on timestamp requirements
            while (framesRendered < totalFramesRequested) {
                // Determine how many frames we can render before the next MIDI event triggers
                double sampleLengthMs = (1000.0 / mSampleRate);
                size_t framesUntilNextEvent = totalFramesRequested - framesRendered;

                if (mCurrentMsg) {
                    double timeToNextEventMs = mCurrentMsg->time - mElapsedTimeMs;
                    if (timeToNextEventMs > 0) {
                        size_t samplesNeeded = static_cast<size_t>(timeToNextEventMs / sampleLengthMs);
                        if (samplesNeeded < framesUntilNextEvent) {
                            framesUntilNextEvent = samplesNeeded;
                        }
                    }
                }

                // Synthesize the audio segment up until the event threshold
                if (framesUntilNextEvent > 0) {
                    tsf_render_short(mSynth, currentWritePtr, static_cast<int>(framesUntilNextEvent), 0);
                    
                    size_t elementsDone = framesUntilNextEvent * mChannels;
                    currentWritePtr += elementsDone;
                    framesRendered += framesUntilNextEvent;
                    mElapsedTimeMs += framesUntilNextEvent * sampleLengthMs;
                }

                // Fire off any MIDI event commands scheduled for this exact timestamp
                while (mCurrentMsg && mElapsedTimeMs >= mCurrentMsg->time) {
                    switch (mCurrentMsg->type) {
                        case TML_NOTE_ON:
                            tsf_note_on(mSynth, mCurrentMsg->channel, mCurrentMsg->key, mCurrentMsg->velocity / 127.0f); //
                            break;
                        case TML_NOTE_OFF:
                            tsf_note_off(mSynth, mCurrentMsg->channel, mCurrentMsg->key);
                            break;
                        case TML_PROGRAM_CHANGE:
                            tsf_channel_set_presetnumber(mSynth, mCurrentMsg->channel, mCurrentMsg->program, (mCurrentMsg->channel == 9));
                            break;
                        case TML_CONTROL_CHANGE:
                            tsf_channel_midi_control(mSynth, mCurrentMsg->channel, mCurrentMsg->control, mCurrentMsg->control_value);
                            break;
                        case TML_PITCH_BEND:
                            tsf_channel_set_pitchwheel(mSynth, mCurrentMsg->channel, mCurrentMsg->pitch_bend);
                            break;
                    }
                    mCurrentMsg = mCurrentMsg->next;
                }

                // EOF handling: If there are no more midi events left in the data stream
                if (!mCurrentMsg) {
                    if (mLoop) {
                        // Reset MIDI sequencer back to beginning
                        mCurrentMsg = mMidiHead;
                        mElapsedTimeMs = 0.0;
                        tsf_reset_voice_alloc(mSynth); // Clears hanging notes smoothly
                    } else {
                        // Loop is disabled, zero out remaining block and break
                        size_t framesLeft = totalFramesRequested - framesRendered;
                        std::fill_n(currentWritePtr, framesLeft * mChannels, 0);
                        framesRendered = totalFramesRequested;
                        break;
                    }
                }
            }

            return framesRendered * mChannels;
        }

        // Clean rule-of-three compliance deletions 
        MIDIStream(const MIDIStream&) = delete;
        MIDIStream& operator=(const MIDIStream&) = delete;

        // Getters
        int getChannels() const { return mChannels; }
        int getSampleRate() const { return mSampleRate; }
        bool isValid() const { return mSynth != nullptr; }

    private:
        tsf* mSynth = nullptr;
        tml_message* mMidiHead = nullptr; // Head of the linked list representing the MIDI track
        tml_message* mCurrentMsg = nullptr; // Iterator for playing
        
        double mElapsedTimeMs = 0.0;
        int mChannels = 2;       // Standard Stereo output
        int mSampleRate = 44100; // Standard 44.1kHz sample rate
        bool mLoop = true;
};

}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MIDI_STREAM_H