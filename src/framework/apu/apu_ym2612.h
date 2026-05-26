#ifndef __RETRO_CORE_FRAMEWORK_APU_APU_YM2612_H
#define __RETRO_CORE_FRAMEWORK_APU_APU_YM2612_H

#include "framework/apu/apu.h"

#include <atomic>

namespace RetroCore {

namespace APU {

class Ym2612Apu : public IAudioComponent {
public:
    void reset() override;
    double getBaseClock() const override { return 7670400.0; }
    void writeRegister(uint16_t addr, uint8_t val) override;
    void process(float* out, std::size_t frames) override;

private:
    struct Operator {
        uint32_t phase = 0;
        float envLevel = 1.0f;
        uint8_t freqLo, freqHi;
        float output = 0.0f;

        void reset() {
            phase = 0; 
            envLevel = 1.0f; 
            output = 0.0f; 
        }

        void tick(double dt, double sr, uint8_t ar, uint8_t dr, uint8_t sr_env, uint8_t sl, uint8_t rr, uint8_t tl) {
            float rate = 0.0f;
            if (ar > 0) rate = ar;
            else if (dr > 0) rate = -dr;
            else if (sr_env > 0) rate = -sr_env;
            else if (rr > 0) rate = -rr;
            envLevel += rate * (dt / sr);
            envLevel = std::clamp(envLevel, 0.0f, 1.0f);
            output = std::sin(6.283185307179586 * (phase / 4294967296.0)) * envLevel * (1.0f - tl / 127.0f);
        }
    };

    struct Channel {
        Operator ops[4];
        uint16_t freq = 0;
        uint8_t feedback = 0, algorithm = 0, tl = 0, ks = 0;
        uint8_t am = 0, pm = 0, ms = 0;
        uint8_t ar = 0, dr = 0, srRate = 0, rr = 0;
        float output = 0.0f;
        
        void reset() { 
            for (auto& op : ops) { 
                op.reset();
            } 
            output = 0.0f; freq = 0;
            feedback = 0; algorithm = 0; tl = 0; ks = 0;
            am = 0; pm = 0; ms = 0;
            ar = 0; dr = 0; srRate = 0; rr = 0;
        }
        
        float process(double sr) {
            for (auto& op : ops) {
                op.tick(1.0, sr, ar, dr, srRate, srRate, rr, tl);
            }
            float fb = (feedback > 0) ? ops[0].output * (feedback / 7.0f) : 0.0f;
            ops[0].phase += static_cast<uint32_t>(fb * 100000.0);
            output = ops[0].output + ops[1].output + ops[2].output;
            return output * 0.5f;
        }

    } channels[6];

    struct PcmCh {
        private:
            static constexpr int stepTable[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
            static constexpr size_t BUFFER_SIZE = 4096; 

            std::array<uint8_t, BUFFER_SIZE> ringBuffer;

            std::atomic<size_t> writeHead;
            std::atomic<size_t> readHead;

            // Hardware State Registers
            bool dacEnabled;
            uint8_t dacLastSample;

        public:
            PcmCh() {
                reset();
            }

            bool isDacEnabled() const { return dacEnabled; }
    
            void reset() {
                writeHead.store(0);
                readHead.store(0);
                dacEnabled = false;
                dacLastSample = 0x80; // 8-bit unsigned midpoint
            }

            void write(uint8_t reg, uint8_t val) { 
                if (reg == 0x2B) {
                    // Bit 7 controls DAC activation
                    dacEnabled = (val & 0x80) != 0;
                } 
                else if (reg == 0x2A && dacEnabled) {
                    // Push sample into the ring buffer
                    size_t nextWrite = (writeHead.load(std::memory_order_relaxed) + 1) % BUFFER_SIZE;
                    
                    // Check for buffer overflow to avoid overwriting unplayed audio
                    if (nextWrite != readHead.load(std::memory_order_acquire)) {
                        ringBuffer[writeHead.load(std::memory_order_relaxed)] = val;
                        writeHead.store(nextWrite, std::memory_order_release);
                    }
                }
            }
            
            float decode(double sr) {
                // If DAC is off, Channel 6 would process FM instead (returns 0 here)
                if (!dacEnabled) {
                    return 0.f; 
                }

                size_t currentRead = readHead.load(std::memory_order_relaxed);
                
                // If buffer has data, fetch the next byte
                if (currentRead != writeHead.load(std::memory_order_acquire)) {
                    dacLastSample = ringBuffer[currentRead];
                    size_t nextRead = (currentRead + 1) % BUFFER_SIZE;
                    readHead.store(nextRead, std::memory_order_release);
                }
                // If buffer is empty, hold the last known voltage level (causes hardware dc-bias/buzzing)

                // Convert YM2612 8-bit unsigned PCM (0-255) to standard 16-bit signed audio (-32768 to 32767)
                int32_t sample16 = static_cast<int32_t>(dacLastSample) - 128; // Center around 0
                return static_cast<int16_t>(sample16 << 8);                 // Scale to 16-bit
            }
    } pcm;

    double cycleAcc = 0.0;
    double sampleRate = 44100.0;
};

}  // namespace APU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_APU_YM2612_H