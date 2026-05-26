#ifndef __RETRO_CORE_FRAMEWORK_SOUND_ENGINE_H
#define __RETRO_CORE_FRAMEWORK_SOUND_ENGINE_H

#include "framework/apu/apu.h"
#include "framework/sound.h"

namespace RetroCore {

namespace Sound {

class SoundEngine {
public:
    struct Config {
        uint32_t maxTracks = 16;
        uint32_t maxSfx = 32;
        uint32_t maxChannels = 64;
    };

    SoundEngine() = default;
    ~SoundEngine() = default;
    SoundEngine(const SoundEngine&) = delete;
    SoundEngine& operator=(const SoundEngine&) = delete;

    void init(APU::IAudioComponent* apu, const Config& cfg) {
        this->apu = apu;
        this->cfg = cfg;
        nextTrackId = 1;
        nextSfxId = 1;
        masterVol = 1.0f;
        wavePool.create(WaveType::Sine, 512);
        wavePool.create(WaveType::Square, 256);
        wavePool.create(WaveType::Saw, 512);
        wavePool.create(WaveType::Triangle, 256);
        wavePool.create(WaveType::Noise, 128);
        routeCount = 0;
    }

    void reset() {
        for (auto& track : tracks) { track.active = false; track.eventHead = 0; track.eventTail = 0; }
        for (auto& sfx : sfxPool) { sfx.active = false; sfx.generator.reset(); }
        for (auto& route : channelRoutes) { route.trackId = 0; route.vol = 1.0f; route.pan = 0.0f; }
        masterVol = 1.0f;
    }

    void addRoute(uint16_t start, uint16_t end, APU::IAudioComponent* comp) {
        if (routeCount < routes.size()) {
            routes[routeCount++] = {start, end, comp};
            std::sort(routes.begin(), routes.begin() + routeCount, [](const Route& a, const Route& b) { return a.start < b.start; });
        }
    }

    uint32_t createTrack(const MusicTrackData& data) {
        for (uint32_t i = 0; i < cfg.maxTracks; ++i) {
            if (!tracks[i].active) {
                auto& track = tracks[i];
                track.active = true; track.id = nextTrackId++; track.tempo = data.tempo; track.loop = data.loop;
                track.volume = 1.0f; track.pan = 0.0f; track.pitch = 1.0f; track.channelCount = 0;
                track.eventHead = 0; track.eventTail = 0;
                for (uint32_t c = 0; c < data.channels.size() && c < 8; ++c) {
                    auto& ch = data.channels[c];
                    track.physicalChannels[track.channelCount] = {toTupleIndex(ch.component), ch.channelIdx};
                    for (uint32_t p = 0; p < cfg.maxChannels; ++p) {
                        if (channelRoutes[p].trackId == 0) {
                            channelRoutes[p] = {toTupleIndex(ch.component), ch.channelIdx, track.id, ch.defaultVol / 127.0f, ch.defaultPan / 64.0f};
                            track.channelCount++;
                            break;
                        }
                    }
                }
                for (uint32_t e = 0; e < data.events.size() && e < 256; ++e) {
                    track.events[e] = data.events[e];
                    track.eventTail++;
                }
                return track.id;
            }
        }
        return 0;
    }

    void playTrack(uint32_t id) {
        for (auto& track : tracks) if (track.active && track.id == id) { track.active = true; track.currentTime = 0.0f; track.eventHead = 0; }
    }
    void pauseTrack(uint32_t id) {
        for (auto& track : tracks) if (track.active && track.id == id) track.active = false;
    }
    void setTrackVolume(uint32_t id, float vol) {
        for (auto& track : tracks) if (track.active && track.id == id) track.volume = std::clamp(vol, 0.0f, 1.0f);
    }
    void setTrackPan(uint32_t id, float pan) {
        for (auto& track : tracks) if (track.active && track.id == id) track.pan = std::clamp(pan, -1.0f, 1.0f);
    }
    void setTrackPitch(uint32_t id, float pitch) {
        for (auto& track : tracks) if (track.active && track.id == id) track.pitch = std::clamp(pitch, 0.5f, 2.0f);
    }

    uint32_t playWaveSfx(uint32_t tableId, double startFreq, double endFreq, float duration, const float* adsr, float pan, float vol, uint8_t priority = 0) {
        for (uint32_t i = 0; i < cfg.maxSfx; ++i) {
            if (!sfxPool[i].active) {
                sfxPool[i].active = true; sfxPool[i].id = nextSfxId++; sfxPool[i].priority = priority;
                sfxPool[i].generator.init(tableId, startFreq, endFreq, duration, adsr, pan, vol, 44100.0f);
                return sfxPool[i].id;
            }
        }
        uint32_t stealIdx = 0xFFFFFFFF;
        uint8_t minPri = 255;
        for (uint32_t i = 0; i < cfg.maxSfx; ++i) {
            if (sfxPool[i].active && sfxPool[i].priority < minPri) { minPri = sfxPool[i].priority; stealIdx = i; }
        }
        if (stealIdx != 0xFFFFFFFF && priority > minPri) {
            sfxPool[stealIdx].active = false; sfxPool[stealIdx].id = nextSfxId++; sfxPool[stealIdx].priority = priority;
            sfxPool[stealIdx].generator.init(tableId, startFreq, endFreq, duration, adsr, pan, vol, 44100.0f);
            return sfxPool[stealIdx].id;
        }
        return 0;
    }

    uint32_t playWahSfx(uint32_t tableId, double baseFreq, float duration, const float* adsr, float pan, float vol, const WahPreset& preset = {}, uint8_t priority = 0) {
        for (uint32_t i = 0; i < cfg.maxSfx; ++i) {
            if (!sfxPool[i].active) {
                sfxPool[i].active = true; sfxPool[i].id = nextSfxId++; sfxPool[i].priority = priority;
                sfxPool[i].generator.init(tableId, baseFreq, baseFreq, duration, adsr, pan, vol, 44100.0f);
                sfxPool[i].generator.applyWah(preset, wavePool);
                return sfxPool[i].id;
            }
        }
        uint32_t stealIdx = 0xFFFFFFFF;
        uint8_t minPri = 255;
        for (uint32_t i = 0; i < cfg.maxSfx; ++i) {
            if (sfxPool[i].active && sfxPool[i].priority < minPri) { minPri = sfxPool[i].priority; stealIdx = i; }
        }
        if (stealIdx != 0xFFFFFFFF && priority > minPri) {
            sfxPool[stealIdx].active = false; sfxPool[stealIdx].id = nextSfxId++; sfxPool[stealIdx].priority = priority;
            sfxPool[stealIdx].generator.init(tableId, baseFreq, baseFreq, duration, adsr, pan, vol, 44100.0f);
            sfxPool[stealIdx].generator.applyWah(preset, wavePool);
            return sfxPool[stealIdx].id;
        }
        return 0;
    }

    void stopSfx(uint32_t sfxId) {
        for (auto& sfx : sfxPool) if (sfx.active && sfx.id == sfxId) { sfx.active = false; sfx.generator.reset(); }
    }

    void writeRegister(uint16_t addr, uint8_t val) {
        if (!apu) return;
        for (const auto& route : routes) {
            if (addr >= route.start && addr <= route.end) { route.comp->writeRegister(addr, val); return; }
        }
    }

    void update(float dt) {
        for (auto& track : tracks) {
            if (!track.active) continue;
            track.currentTime += dt;
            while (track.eventHead < track.eventTail && track.events[track.eventHead].time <= static_cast<uint16_t>(track.currentTime * 100.0f)) {
                processEvent(track, track.events[track.eventHead]);
                track.eventHead++;
            }
            if (track.loop && track.currentTime > 10.0f) { track.currentTime -= 10.0f; track.eventHead = 0; }
        }
        for (auto& sfx : sfxPool) if (sfx.active) sfx.generator.update(dt, wavePool);
    }

    void finalizeAudio(float* out, std::size_t frames) {
        std::fill(out, out + frames * 2, 0.0f);
        if (apu) apu->process(out, frames);
        for (auto& sfx : sfxPool) if (sfx.active) sfx.generator.generate(out, frames, wavePool);
        for (auto& route : channelRoutes) {
            if (route.trackId == 0) continue;
            float lGain = route.vol * std::cos(route.pan * M_PI * 0.5f) * masterVol;
            float rGain = route.vol * std::sin(route.pan * M_PI * 0.5f) * masterVol;
            for (std::size_t i = 0; i < frames; ++i) {
                out[i*2] *= lGain;
                out[i*2+1] *= rGain;
            }
        }
    }

    void setMasterVolume(float vol) { masterVol = std::clamp(vol, 0.0f, 1.0f); }

private:
    struct Track {
        uint32_t id = 0;
        bool active = false;
        float currentTime = 0.0f;
        float tempo = 12000.0f;
        bool loop = false;
        float volume = 1.0f;
        float pan = 0.0f;
        float pitch = 1.0f;
        std::array<Event, 256> events;
        uint32_t eventHead = 0;
        uint32_t eventTail = 0;
        uint8_t channelCount = 0;
        struct { uint8_t compIdx; uint8_t chIdx; } physicalChannels[8];
    };

    struct ChannelRoute {
        uint8_t compIdx = 0;
        uint8_t chIdx = 0;
        uint32_t trackId = 0;
        float vol = 1.0f;
        float pan = 0.0f;
    };

    struct Route {
        uint16_t start = 0;
        uint16_t end = 0;
        APU::IAudioComponent* comp = nullptr;
    };

    APU::IAudioComponent* apu = nullptr;
    Config cfg;
    WaveTablePool wavePool;
    std::array<Track, 16> tracks;
    std::array<SfxSlot, 32> sfxPool;
    std::array<ChannelRoute, 64> channelRoutes;
    std::array<Route, 16> routes;
    uint8_t routeCount = 0;
    float masterVol = 1.0f;
    uint32_t nextTrackId = 1;
    uint32_t nextSfxId = 1;

    void processEvent(Track& track, const Event& event) {
        switch (event.type) {
            case 0: /* NOTE_ON: map pitch */ break;
            case 1: /* NOTE_OFF: fade out */ break;
            case 2: track.volume = event.note / 127.0f; break;
            case 3: track.pan = (event.note - 64) / 64.0f; break;
            case 4: track.pitch = 1.0f + (event.note / 128.0f - 1.0f); break;
            case 5: /* ARP: set speed */ break;
            case 6: /* WAH: set preset */ break;
            case 7: /* LFO: set rate/depth */ break;
        }
    }
};

}  // namespace Sound

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_SOUND_ENGINE_H