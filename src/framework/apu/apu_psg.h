#ifndef __RETRO_CORE_FRAMEWORK_APU_APU_PSG_H
#define __RETRO_CORE_FRAMEWORK_APU_APU_PSG_H

#include "framework/apu/apu.h"

namespace RetroCore {

namespace APU {

class PsgApu : public IAudioComponent {
public:
    void reset() override;
    double getBaseClock() const override { return 1789772.5; }
    void writeRegister(uint16_t addr, uint8_t val) override;
    void process(float* out, std::size_t frames) override;

private:
    struct ToneCh {
        uint16_t period = 0;
        uint8_t duty = 0;
        double phase = 0.0;
        bool enabled = false;
        float output = 0.0f;

        void reset() {
            period = 0; 
            phase = 0.0; 
            enabled = false; 
            output = 0.0f; 
        }
    } channels[3];

    uint16_t envPeriod = 0;
    uint8_t envShape = 0;
    double envPhase = 0.0;
    float envLevel = 1.0f;
    double cycleAcc = 0.0;
    double sampleRate = 44100.0;

    void advanceEnv() {
        if (envPeriod == 0) return;
        envPhase += 1.0;
        if (envPhase >= 1.0) {
            envPhase -= 1.0;
            if (envShape >= 2 && envShape <= 4) {
                envLevel += 0.01f;
                if (envLevel > 1.0f) envLevel = 1.0f;
            } else if (envShape >= 5 && envShape <= 7) {
                envLevel -= 0.01f;
                if (envLevel < 0.0f) envLevel = 0.0f;
            }
        }
    }
};

}  // namespace APU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_APU_PSG_H