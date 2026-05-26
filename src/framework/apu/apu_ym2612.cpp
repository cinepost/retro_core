#include "framework/apu/apu_ym2612.h"

namespace RetroCore {

namespace APU {

void Ym2612Apu::reset() { 
    cycleAcc = 0;
    for (auto& ch : channels) {
        ch.reset();
    }
    pcm.reset();
}

void Ym2612Apu::writeRegister(uint16_t addr, uint8_t val) {
    uint16_t reg = addr & 0x3F;
    if (reg >= 0x28 && reg <= 0x57) {
        int chIdx = (reg - 0x28) / 8;
        int opIdx = (reg & 0x07) / 1;
        int regOffset = reg % 8;
        
        switch (regOffset) {
            case 0x00: channels[chIdx].ops[opIdx].freqLo = val; break;
            case 0x01: channels[chIdx].ops[opIdx].freqHi = val; break;
            case 0x02: channels[chIdx].feedback = val & 0x07; break;
            case 0x03: channels[chIdx].algorithm = val & 0x07; break;
            case 0x04: channels[chIdx].tl = val; break;
            case 0x05: channels[chIdx].ks = val & 0x03; break;
            case 0x06: channels[chIdx].ar = val >> 4; channels[chIdx].dr = val & 0x0F; break;
            case 0x07: channels[chIdx].srRate = val >> 4; channels[chIdx].rr = val & 0x0F; break;
            case 0x08: channels[chIdx].am = (val >> 7) & 1; channels[chIdx].pm = val & 0x07; channels[chIdx].ms = val >> 4; break;
            default: break;
        }
    } else if (addr >= 0x100 && addr <= 0x10F) {
        pcm.write(addr & 0x0F, val);
    }
}

void Ym2612Apu::process(float* out, std::size_t frames) {
    double cyclesPerSample = getBaseClock() / sampleRate;
    for (std::size_t i = 0; i < frames; ++i) {
        cycleAcc += cyclesPerSample;
        while (cycleAcc >= 1.0) { 
            cycleAcc -= 1.0; 
        }
        float sample = 0.0f;
        for (int ch = 0; ch < 5; ++ch) {
            sample += channels[ch].process(sampleRate);
        }
        if(pcm.isDacEnabled()) {
            sample += pcm.decode(sampleRate) * 0.5f;
        } else {
            sample += channels[5].process(sampleRate);
        }
        out[i*2] = sample; out[i*2+1] = sample;
    }
}

}  // namespace APU

}  // namespace RetroCore