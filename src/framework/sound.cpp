#include "framework/sound.h"

#include <MidiFile.h>


namespace RetroCore {

namespace Sound {

static double getInitialTempoUsqn(smf::MidiFile& midi) {
    for (int track = 0; track < midi.getTrackCount(); track++) {
        for (int i = 0; i < midi.getEventCount(track); i++) {
            
            if (midi[track][i].isTempo()) {
                // This library method returns a double [1]
                return midi[track][i].getTempoMicroseconds(); 
            }
        }
    }
    
    // Default MIDI specification value (120 BPM)
    return 500000.0; 
}

static void quantizeMidiFile(smf::MidiFile& midi, double noteFraction = 0.25) {
    // 1. Get the resolution of the MIDI file (Ticks Per Quarter Note)
    int tpq = midi.getTicksPerQuarterNote();

    // 2. Calculate how many ticks are in your target grid block
    // Examples: 
    // noteFraction = 1.0  -> Quarter Note grid (tpq * 1)
    // noteFraction = 0.5  -> Eighth Note grid  (tpq * 0.5)
    // noteFraction = 0.25 -> Sixteenth Note grid (tpq * 0.25)
    int gridTicks = static_cast<int>(tpq * noteFraction);
    
    if (gridTicks <= 0) return;

    // 3. Loop through all tracks and all events
    for (int track = 0; track < midi.getTrackCount(); track++) {
        for (int i = 0; i < midi.getEventCount(track); i++) {
            smf::MidiEvent& event = midi[track][i];

            // Quantize only Note-On and Note-Off events to preserve structural messages
            if (event.isNoteOn() || event.isNoteOff()) {
                int currentTick = event.tick;

                // Round to the nearest grid step
                int quantizedTick = std::round(static_cast<double>(currentTick) / gridTicks) * gridTicks;

                // Update the event time
                event.tick = quantizedTick;
            }
        }
    }

    // 4. Critical: Sort the events because quantization can shift order
    midi.sortTracks();
}

bool MidiConverter::convert(const std::string& midiPath, MusicTrackData& out, const MidiToMusicTrackConfig& config) {
    smf::MidiFile midi(midiPath);
    if (!midi.status()) {
    	// TODO: log here
    	printf("Unable to open file: %s\n", midiPath.c_str());
    	return false;
    }

    quantizeMidiFile(midi, 0.001); // Normalize timing to avoid floating point drift
    double ticksPerBeat = midi.getTicksPerQuarterNote();
    if (ticksPerBeat <= 0) return false;

    // --- Tempo Handling ---
    double tempoBPM = config.baseTempo;
    if (config.autoMapTempo) {
        double usqn = getInitialTempoUsqn(midi); // Microseconds per quarter note
        if (usqn > 0) tempoBPM = 60000000.0 / usqn;
    }
    out.tempo = static_cast<uint16_t>(tempoBPM * 100); // BPM * 100
    out.loop = false;
    out.channels.clear();
    out.events.clear();

    // --- Process Tracks & Events ---
    midi.sortTracks(); // Ensure events are time-sorted
    for (int track = 0; track < midi.getNumTracks(); ++track) {
        for (int i = 0; i < midi[track].size(); ++i) {
            auto& ev = midi[track][i];
            if (!ev.isNoteOn() && !ev.isNoteOff()) continue;

            int ch = ev.getChannel();
            int note = ev.getP1();
            int vel = ev.getP2();
            double timeSec = midi.getTimeInSeconds(ev.tick);

            // Map to config
            auto& map = config.channelRouting[ch];
            if (out.channelCount < 8) {
                if (config.deduplicateChannels) {
                    // Avoid duplicate APU channel mappings
                    bool exists = false;
                    for (uint8_t c = 0; c < out.channelCount; ++c) {
                        if (out.channels[c].component == map.component && out.channels[c].channelIdx == map.channelIdx) {
                            exists = true; break;
                        }
                    }
                    if (!exists) {
                        out.channels.push_back(map);
                        out.channelCount++;
                    }
                } else {
                    out.channels.push_back(map);
                    out.channelCount++;
                }
            }

            if (ev.isNoteOn()) {
                // Find corresponding note-off for duration
                double dur = 0.0;
                for (int j = i + 1; j < midi[track].size(); ++j) {
                    auto& next = midi[track][j];
                    if (next.isNoteOff() && next.getChannel() == ch && next.getP1() == note) {
                        dur = midi.getTimeInSeconds(next.tick) - timeSec;
                        break;
                    }
                }
                if (dur < 0.01) dur = 0.1; // Minimum 10ms duration

                // NOTE_ON event
                MusicTrackData::Event noteOn;
                noteOn.time = static_cast<uint16_t>(timeSec * 100.0f); // Centiseconds
                noteOn.type = 0; // NOTE_ON
                noteOn.note = note;
                noteOn.param = vel; // Velocity in param
                out.events.push_back(noteOn);

                // NOTE_OFF event
                MusicTrackData::Event noteOff;
                noteOff.time = static_cast<uint16_t>((timeSec + dur) * 100.0f);
                noteOff.type = 1; // NOTE_OFF
                noteOff.note = note;
                noteOff.param = 0;
                out.events.push_back(noteOff);
            }
        }
    }

    // Sort events by time (centiseconds)
    std::sort(out.events.begin(), out.events.end(), [](const MusicTrackData::Event& a, const MusicTrackData::Event& b) {
        return a.time < b.time;
    });

    return true;
}

}  // namespace Sound

}  // namespace RetroCore