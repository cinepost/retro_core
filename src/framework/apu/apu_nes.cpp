#include "framework/apu/apu_nes.h"

namespace RetroCore {

namespace APU {

void NesApu::reset() { 
    cycleAcc = 0.0;
    for (auto& ch : pulse) { 
        ch.reset(); 
    }
    triangle.reset();
    noise.reset();
    dmc.reset();
}


void NesApu::writeRegister(uint16_t addr, uint8_t val) {
    addr &= 0x1F;
    switch (addr) {
        case 0x4000: 
            pulse[0].ctrl = val; 
            pulse[0].updateDuty(); 
            break;
        case 0x4001: 
            pulse[0].periodLo = val; 
            pulse[0].period = (pulse[0].period & 0xFF00) | val; 
            break;
        case 0x4002: 
            pulse[1].periodLo = val; 
            pulse[1].period = (pulse[1].period & 0xFF00) | val; 
            break;
        case 0x4003: 
            pulse[1].ctrl = val; 
            pulse[1].updateDuty(); 
            break;
        case 0x4004: 
            triangle.periodLo = val; 
            triangle.period = (triangle.period & 0xFF00) | val; 
            break;
        case 0x4005: 
            noise.periodLo = val; 
            noise.updateLfsrPeriod(); 
            break;
        case 0x4006: 
            dmc.addr = (dmc.addr & 0xFF00) | val; 
            break;
        case 0x4007: 
            dmc.length = (dmc.length & 0xFF00) | val; 
            break;
        case 0x4008: 
            triangle.ctrl = val; 
            break;
        case 0x4009: 
            dmc.ctrl = val; 
            break;
        case 0x400A: 
            dmc.addr = (dmc.addr & 0x00FF) | (val << 8); 
            break;
        case 0x400B: 
            dmc.length = (dmc.length & 0x00FF) | (val << 8); 
            break;
        case 0x400C: 
            pulse[0].duty = val >> 6; 
            break;
        case 0x400D: 
            pulse[1].duty = val >> 6; 
            break;
        case 0x400E: 
            break; // Sweep timer
        case 0x400F: 
            dmc.irqFlag = false; 
            break;
        default: 
            break;
    }
}

void NesApu::process(float* out, std::size_t frames) {
    advanceCycles(cyclesPerSample * frames);
    for (std::size_t i = 0; i < frames; ++i) {
        uint8_t s1 = pulse[0].sample();
        uint8_t s2 = pulse[1].sample();
        uint8_t s3 = triangle.sample();
        uint8_t s4 = noise.sample();
        uint8_t s5 = dmc.sample();
        float sample = (dacTable[s1] + dacTable[s2] + dacTable[s3] + dacTable[s4] + dacTable[s5]) / 5.0f;
        out[i*2]   = sample;
        out[i*2+1] = sample;
    }
}

}  // namespace APU

}  // namespace RetroCore