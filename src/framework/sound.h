#ifndef __RETRO_CORE_FRAMEWORK_SOUND_H
#define __RETRO_CORE_FRAMEWORK_SOUND_H

#include "framework/apu/apu.h"


namespace RetroCore {

namespace Sound {

enum class WaveType { Sine, Square, Saw, Triangle, Noise };

struct WaveTable {
    WaveType waveType = WaveType::Sine;
    uint32_t size = 512;
    float data[512] = {};
};

class WaveTablePool {
public:
    uint32_t create(WaveType type, uint32_t size = 512) {
        if (count >= maxTables) return 0;
        size = (size <= 1) ? 256 : size;
        while ((size & (size - 1)) != 0) size++;
        tables[count].waveType = type;
        tables[count].size = size;
        uint32_t mask = size - 1;
        for (uint32_t i = 0; i < size; ++i) {
            double t = static_cast<double>(i) / size;
            switch (type) {
                case WaveType::Sine:      tables[count].data[i] = std::sin(2.0 * M_PI * t); break;
                case WaveType::Square:    tables[count].data[i] = (t < 0.5f) ? 1.0f : -1.0f; break;
                case WaveType::Saw:       tables[count].data[i] = 2.0f * (t - 0.5f); break;
                case WaveType::Triangle:  tables[count].data[i] = 4.0f * std::abs(t - 0.5f) - 1.0f; break;
                case WaveType::Noise:     tables[count].data[i] = (nextNoiseSample() >> 16) * 0.00006103515625f; break;
                default:                  tables[count].data[i] = 0.0f;
            }
        }
        return count++;
    }
    const float* getData(uint32_t id) const { return id < count ? tables[id].data : nullptr; }
    uint32_t getSize(uint32_t id) const { return id < count ? tables[id].size : 0; }
private:
    static constexpr uint32_t maxTables = 16;
    WaveTable tables[maxTables] = {};
    uint32_t count = 0;
    uint32_t lfsr = 0xACE1u;
    uint32_t nextNoiseSample() {
        lfsr ^= (lfsr << 13);
        lfsr ^= (lfsr >> 17);
        lfsr ^= (lfsr << 5);
        return lfsr;
    }
};

struct Lfo {
    bool active = false;
    uint32_t phase = 0;
    uint32_t rate = 0;
    uint32_t tableId = 0;
    float depth = 0.0f;
    float getOutput(const WaveTablePool& pool, float dt, float sr) {
        if (!active) return 0.0f;
        uint32_t inc = static_cast<uint32_t>((static_cast<double>(rate) * pool.getSize(tableId)) / sr);
        phase += inc;
        const float* tbl = pool.getData(tableId);
        uint32_t size = pool.getSize(tableId);
        uint32_t idx = (phase >> 12) & (size - 1);
        return tbl[idx];
    }
    void reset() { active = false; phase = 0; rate = 0; depth = 0.0f; }
};

struct LfoArp {
    bool active = false;
    float multipliers[3] = {1.0f, 1.0f, 1.0f};
    float speed = 0.0f;
    uint32_t stepTime = 0;
    uint32_t timeAcc = 0;
    uint8_t currentStep = 0;
    void reset() { active = false; speed = 0.0f; timeAcc = 0; currentStep = 0; }
    float getMultiplier() const { return active ? multipliers[currentStep] : 1.0f; }
    void update(float dt) {
        if (!active) return;
        timeAcc += static_cast<uint32_t>(dt * speed * 4096.0f);
        if (timeAcc >= stepTime) {
            timeAcc -= stepTime;
            currentStep = (currentStep + 1) % 3;
        }
    }
};

struct WahPreset {
    float speed = 3.0f;
    float depthHz = 15.0f;
    uint32_t modTableId = 2;
    float baseFreqShift = 1.0f;
    float tremoloDepth = 0.0f;
    uint32_t tremTableId = 1;
    static WahPreset getDeep() { return {2.5f, 25.0f, 2, 0.8f, 0.6f, 1}; }
    static WahPreset getFast() { return {8.0f, 10.0f, 2, 1.0f, 0.3f, 1}; }
    static WahPreset getVocal() { return {3.0f, 20.0f, 2, 1.2f, 0.4f, 1}; }
    static WahPreset getSciFi() { return {4.0f, 35.0f, 2, 0.5f, 0.0f, 1}; }
};

struct SfxGenerator {
    bool active = false;
    uint32_t tableId = 0;
    uint32_t tableSize = 512;
    uint32_t phase = 0;
    uint32_t phaseInc = 0;
    double freqStart = 440.0;
    double freqEnd = 440.0;
    double currentFreq = 440.0;
    float duration = 1.0f;
    float elapsed = 0.0f;
    float adsr[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float envelope = 1.0f;
    float pan = 0.0f;
    float volume = 1.0f;
    float sampleRate = 44100.0f;
    Lfo lfoPhase;
    Lfo lfoTrem;
    LfoArp arpLfo;
    float phaseModDepth = 0.0f;

    void reset() {
        active = false; phase = 0; elapsed = 0.0f; envelope = 1.0f;
        currentFreq = freqStart; lfoPhase.reset(); lfoTrem.reset(); arpLfo.reset(); phaseModDepth = 0.0f;
    }
    void init(uint32_t tblId, double startFreq, double endFreq, float dur, const float* adsr, float pan, float vol, float sr) {
        active = true; tableId = tblId; freqStart = startFreq; freqEnd = endFreq; duration = dur; elapsed = 0.0f;
        if (adsr) std::copy(adsr, adsr + 4, this->adsr);
        pan = std::max(-1.0f, std::min(1.0f, pan)); volume = std::max(0.0f, std::min(1.0f, vol)); sampleRate = sr;
        envelope = 1.0f; currentFreq = startFreq;
    }
    void update(float dt, const WaveTablePool& pool) {
        if (!active) return;
        elapsed += dt;
        if (elapsed >= duration) { active = false; return; }
        float t = elapsed / duration;
        currentFreq = freqStart + (freqEnd - freqStart) * t;
        float envTime = 0.0f;
        if (elapsed < adsr[0]) { envTime = elapsed / adsr[0]; envelope = envTime; }
        else if (elapsed < adsr[0] + adsr[1]) { envTime = (elapsed - adsr[0]) / adsr[1]; envelope = 1.0f - (1.0f - adsr[2]) * envTime; }
        else { envelope = adsr[2]; }
        float lfoOut = lfoPhase.getOutput(pool, dt, sampleRate);
        if (lfoPhase.active) { /* phase mod applied in generate */ }
        if (lfoTrem.active) {
            float tremOut = lfoTrem.getOutput(pool, dt, sampleRate);
            envelope *= (1.0f + (tremOut * lfoTrem.depth));
            envelope = std::clamp(envelope, 0.0f, 1.0f);
        }
        arpLfo.update(dt);
    }
    void generate(float* out, std::size_t frames, const WaveTablePool& pool) {
        if (!active) return;
        const float* tbl = pool.getData(tableId);
        uint32_t tblSize = pool.getSize(tableId);
        if (!tbl || tblSize == 0) return;
        uint32_t basePhaseInc = static_cast<uint32_t>(currentFreq * tblSize * 4096.0f / sampleRate);
        uint32_t phaseModInc = 0;
        if (lfoPhase.active) {
            float lfoOut = lfoPhase.getOutput(pool, 0.0f, sampleRate);
            phaseModInc = static_cast<uint32_t>(lfoOut * phaseModDepth * tblSize * 4096.0f / sampleRate);
        }
        float lGain = volume * std::cos(pan * M_PI * 0.5f);
        float rGain = volume * std::sin(pan * M_PI * 0.5f);
        uint32_t mask = tblSize - 1;
        for (std::size_t i = 0; i < frames; ++i) {
            uint32_t idx = phase >> 12;
            float sample = tbl[idx] * envelope;
            out[i*2] += sample * lGain;
            out[i*2+1] += sample * rGain;
            phase += (basePhaseInc + phaseModInc);
        }
    }
    void setArpeggio(const float* notes, float speed) {
        arpLfo.active = true; arpLfo.speed = speed; arpLfo.stepTime = static_cast<uint32_t>(4096.0f / speed);
        for (int i = 0; i < 3; ++i) arpLfo.multipliers[i] = notes[i];
    }
    void applyWah(const WahPreset& preset, const WaveTablePool& pool) {
        lfoPhase.active = true; lfoPhase.tableId = preset.modTableId;
        lfoPhase.rate = static_cast<uint32_t>(preset.speed * 4096.0f); lfoPhase.depth = preset.depthHz; phaseModDepth = preset.depthHz;
        if (preset.tremoloDepth > 0.0f) {
            lfoTrem.active = true; lfoTrem.tableId = preset.tremTableId;
            lfoTrem.rate = static_cast<uint32_t>(preset.speed * 4096.0f); lfoTrem.depth = preset.tremoloDepth;
        } else { lfoTrem.active = false; }
        currentFreq *= preset.baseFreqShift;
    }
    bool isActive() const { return active; }
};

struct SfxSlot {
    uint32_t id = 0;
    bool active = false;
    uint8_t priority = 0;
    SfxGenerator generator;
};

// ==================== MUSIC DATA ====================
struct ChannelMap {
    APU::ApuComponent component = APU::ApuComponent::NONE;
    uint8_t channelIdx = 0;
    uint8_t defaultVol = 127;
    int8_t defaultPan = 0;
};

struct Event {
    uint16_t time = 0;
    uint8_t type = 0;
    uint8_t note = 0;
    uint8_t param = 0;
};

struct MusicTrackData {
    using Event = RetroCore::Sound::Event;
    uint16_t tempo = 12000;
    uint8_t channelCount = 0;
    bool loop = false;
    std::vector<ChannelMap> channels;
    std::vector<Event> events;
};

// ==================== MIDI CONVERTER ====================

struct MidiToMusicTrackConfig {
    std::array<ChannelMap, 16> channelRouting;
    float baseTempo = 120.0f;
    bool autoMapTempo = true;
    bool deduplicateChannels = true;
};

class MidiConverter {
public:
    static bool convert(const std::string& midiPath, MusicTrackData& out, const MidiToMusicTrackConfig& config);
};

}  // namespace Sound

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_SOUND_H