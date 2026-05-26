#ifndef __RETRO_CORE_FRAMEWORK_APU_APU_NES_H
#define __RETRO_CORE_FRAMEWORK_APU_APU_NES_H

#include "framework/apu/apu.h"

namespace RetroCore {

namespace APU {

class NesApu : public IAudioComponent {
public:
    void reset() override;
    double getBaseClock() const override { return 1789772.5; }
    void writeRegister(uint16_t addr, uint8_t val) override;
    void process(float* out, std::size_t frames) override;

private:
    struct PulseCh {
        uint8_t duty, periodLo, ctrl, envCtrl;
        uint8_t lengthCnt, envCnt;
        uint16_t period;
        uint8_t phase, vol, sweepTimer, sweepReload;
        bool sweepBit7, sweepEnabled, enabled;
        
        void reset() { 
            duty = periodLo = ctrl = envCtrl = 0; 
            lengthCnt = envCnt = phase = vol = sweepTimer = sweepReload = 0; 
            period = 0; 
            sweepBit7 = sweepEnabled = enabled = false; 
        }

        void updateDuty() { /* handled in sample */ }
        
        uint8_t sample() const {
            if (!enabled || lengthCnt == 0) return 0;
            static constexpr uint8_t lut[4][4] = {{1,0,0,0},{1,1,0,0},{1,1,1,0},{1,1,1,1}};
            return (lut[duty][phase % 4] * vol) / 15;
        }
    } pulse[2];

    struct TriCh {
        uint8_t periodLo, ctrl; 
        uint8_t lengthCnt, linCnt; 
        uint16_t period; 
        uint8_t phase; 
        bool enabled;
        
        void reset() { 
            periodLo = ctrl = 0; 
            lengthCnt = linCnt = phase = 0; 
            period = 0; 
            enabled = false; 
        }
        
        uint8_t sample() const { 
            return (enabled && lengthCnt && linCnt) ? ((phase < 8) ? phase : 15 - phase) : 0; 
        }
    } triangle;

    struct NoiseCh {
        uint8_t periodLo, ctrl, envCtrl; 
        uint8_t lengthCnt, envCnt; 
        uint16_t period; 
        mutable uint32_t lfsr; 
        uint16_t lfsrPeriod; 
        uint8_t vol; 
        bool enabled;
        
        void reset() { 
            periodLo = ctrl = envCtrl = 0; 
            lengthCnt = envCnt = lfsrPeriod = 0; 
            period = 0; 
            lfsr = 0x7FFF; 
            vol = 0; 
            enabled = false; 
        }

        void updateLfsrPeriod() { 
            static constexpr uint16_t s[8] = {4,8,16,32,64,96,128,192}; 
            static constexpr uint16_t l[8] = {160,320,640,1280,2560,3200,3840,4480}; 
            uint8_t i = periodLo & 0x0F; 
            lfsrPeriod = (ctrl & 0x20) ? l[i] : s[i]; 
        }
        
        uint8_t sample() const {
            if (!enabled || lengthCnt == 0) return 0;
            bool b = ((lfsr >> 14) ^ (lfsr >> 15)) & 1;
            lfsr = (lfsr >> 1) | (b << 14);
            return ((lfsr >> 14) * vol) / 15;
        }
    } noise;

    struct DmcCh {
        uint8_t ctrl, irqCtrl; 
        uint16_t addr, length; 
        uint8_t sampleData; 
        int8_t currentSample; 
        bool enabled, irqFlag, bitsLeft;
        
        void reset() { 
            ctrl = irqCtrl = 0; 
            addr = length = 0; 
            sampleData = 0; 
            currentSample = 0; 
            enabled = irqFlag = bitsLeft = 0; 
        }
    
        uint8_t sample() const { 
            return (enabled && length > 0) ? static_cast<uint8_t>(currentSample + 8) : 8; 
        }
    } dmc;

    void advanceCycles(double delta) {
        cycleAcc += delta;
        while (cycleAcc >= 1.0) {
            cycleAcc -= 1.0;
            updateSweep();
            updateLfsr();
            updateEnvelopes();
            updateLengths();
            updatePhase();
        }
    }
    void updateSweep() { /* simplified sweep logic */ }
    void updateLfsr() { /* simplified lfsr logic */ }
    void updateEnvelopes() { /* simplified env logic */ }
    void updateLengths() { /* simplified length logic */ }
    void updatePhase() { /* simplified phase logic */ }

private:
    double cycleAcc = 0.0;
    double cyclesPerSample = 0.0;
    double sampleRate = 44100.0;
    static constexpr float dacTable[16] = {-1.0f,-0.9375f,-0.875f,-0.8125f,-0.75f,-0.6875f,-0.625f,-0.5625f,-0.5f,-0.4375f,-0.375f,-0.3125f,-0.25f,-0.1875f,-0.125f,-0.0625f};
};

}  // namespace APU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_APU_NES_H