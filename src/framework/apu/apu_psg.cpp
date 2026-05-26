#include "framework/apu/apu_psg.h"

namespace RetroCore {

namespace APU {

void PsgApu::reset() { 
    cycleAcc = 0;
    for (auto& ch : channels) { 
        ch.period = 0; 
        ch.phase = 0; 
        ch.enabled = false; 
        ch.output = 0.0f; 
    }
    
    envPeriod = 0; 
    envShape = 0; 
    envPhase = 0; 
    envLevel = 1.0f;
}

void PsgApu::writeRegister(uint16_t addr, uint8_t val) {
    uint8_t reg = addr & 0x0F;
    switch (reg) {
        case 0x00: 
            channels[0].period = (channels[0].period & 0xFF00) | val; 
            break;
        case 0x01: 
            channels[0].period = (channels[0].period & 0x00FF) | (val << 8); 
            break;
        case 0x02: 
            channels[0].duty = val & 0x0F; 
            break;
        case 0x03: 
            channels[1].period = (channels[1].period & 0xFF00) | val; 
            break;
        case 0x04: 
            channels[1].period = (channels[1].period & 0x00FF) | (val << 8); 
            break;
        case 0x05: 
            channels[1].duty = val & 0x0F; 
            break;
        case 0x06: 
            channels[2].period = (channels[2].period & 0xFF00) | val; 
            break;
        case 0x07: 
            channels[2].period = (channels[2].period & 0x00FF) | (val << 8); 
            break;
        case 0x08: 
            channels[2].duty = val & 0x0F; 
            break;
        case 0x09: 
            envPeriod = (envPeriod & 0xFF00) | val; 
            break;
        case 0x0A: 
            envPeriod = (envPeriod & 0x00FF) | (val << 8); 
            break;
        case 0x0B: 
            envShape = val & 0x0F; 
            break;
        case 0x0C: 
            channels[0].enabled = (val & 0x01) != 0;
            channels[1].enabled = (val & 0x02) != 0;
            channels[2].enabled = (val & 0x04) != 0;
            break;
        default: 
            break;
    }
}

void PsgApu::process(float* out, std::size_t frames) {
    double cyclesPerSample = getBaseClock() / sampleRate;
    for (std::size_t i = 0; i < frames; ++i) {
        cycleAcc += cyclesPerSample;
        while (cycleAcc >= 1.0) {
            cycleAcc -= 1.0;
            advanceEnv();
            for (auto& ch : channels) {
                if (ch.period > 0) {
                    ch.phase += (1.0 / 44100.0) * (1789772.5 / (ch.period * 4));
                    if (ch.phase >= 1.0) {
                        ch.phase -= 1.0;
                    }
                }
                float thresh = (ch.duty + 1) / 16.0f;
                ch.output = (ch.enabled && ch.period > 0) ? ((ch.phase < thresh) ? 1.0f : -1.0f) * envLevel : 0.0f;
            }
        }
        float sample = channels[0].output + channels[1].output + channels[2].output;
        out[i*2] = sample * 0.333f;
        out[i*2+1] = sample * 0.333f;
    }
}

}  // namespace APU

}  // namespace RetroCore