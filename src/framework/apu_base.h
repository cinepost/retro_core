#ifndef __RETRO_CORE_FRAMEWORK_APU_BASE_H
#define __RETRO_CORE_FRAMEWORK_APU_BASE_H

#include "framework/common.h"
#include "framework/types.h"

#include <array>


namespace RetroCore {

template<Platform APU>
struct ChipTraits {
    static constexpr size_t channel_count = 0;
    static constexpr bool uses_samples = false;
    static constexpr size_t wave_ram_size = 0; 
};

template<>
struct ChipTraits<Platform::NES> {
    static constexpr size_t channel_count = 5; // 2 Pulse, 1 Triangle, 1 Noise, 1 DMC
    static constexpr bool uses_samples = true;  // Enabled for DMC / custom sequencing
    static constexpr bool has_nes_quirks = true;
    static constexpr size_t wave_ram_size = 32; // Stores triangle step sequencer tables
};

template<>
struct ChipTraits<Platform::TG16> {
    static constexpr size_t channel_count = 6;
    static constexpr bool uses_samples = true;
    static constexpr size_t wave_ram_size = 32; // 32-byte custom waveforms
};

template<>
struct ChipTraits<Platform::SNES> {
    static constexpr size_t channel_count = 8;
    static constexpr bool uses_samples = true;
    static constexpr size_t wave_ram_size = 256; // Dynamic sample buffer target
};

class AudioChannelBase {
    public:
        enum class Type { 
            None, 
            Pulse, 
            Triangle, 
            Noise, 
            DMC 
        };
};

template<Platform APU>
class AudioChannel: public AudioChannelBase {
    using Traits = ChipTraits<APU>;

    uint32_t phase_accumulator = 0;
    uint32_t phase_step = 0;
    int16_t volume = 255;
    bool enabled = true;

    // NES-Specific Register Fields
    Type nes_type = Type::None;
    uint8_t pulse_duty_selection = 2;  // Default to 50%
    uint16_t lfsr_register = 1;        // Must be non-zero to seed noise generator [1]
    bool noise_short_mode = false;     // Pseudo-random flag selector

public:
    std::array<int16_t, Traits::wave_ram_size> wave_ram{};

    constexpr AudioChannel() {
        wave_ram.fill(0);
        if constexpr (APU == Platform::NES) {
            // Pre-seed triangle configuration tables directly into internal RAM
            for(size_t i = 0; i < 32; ++i) {
                // Amplify 4-bit range (0-15) to a standard 16-bit sound field
                wave_ram[i] = static_cast<int16_t>((NESTables::triangle_sequence[i] - 7) * 4000);
            }
        } else {
            static_assert(false, "Unimplemented!");
        }
    }

    constexpr void configure_nes_type(AudioChannelBase::Type type) {
        nes_type = type;
    }

    constexpr void set_nes_pulse_duty(uint8_t duty_index) {
        pulse_duty_selection = duty_index & 3;
    }

    constexpr void set_nes_noise_mode(bool short_mode) {
        noise_short_mode = short_mode;
    }

    constexpr void set_frequency(uint32_t raw_period, uint32_t host_sample_rate) {
        if constexpr (std::is_same_v<APU, Platform::NES>) {
            uint32_t target_hz = 0;
            if (nes_type == AudioChannelBase::Type::Pulse) {
                // NES NTSC formula: CPU_Clock / (16 * (Period + 1))
                target_hz = 1789773 / (16 * (raw_period + 1));
                phase_step = (static_cast<uint64_t>(target_hz) << 16) / host_sample_rate;
            } else if (nes_type == AudioChannelBase::Type::Triangle) {
                // NES NTSC Triangle formula: CPU_Clock / (32 * (Period + 1))
                target_hz = 1789773 / (32 * (raw_period + 1));
                // Stepping over a 32-sample lookup array configuration
                phase_step = (static_cast<uint64_t>(target_hz) << 16) / host_sample_rate;
            } else if (nes_type == AudioChannelBase::Type::Noise) {
                // Map NES noise speed register indices directly to frequencies
                static constexpr std::array<uint32_t, 16> noise_dividers = {
                    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
                };
                target_hz = 1789773 / noise_dividers[raw_period & 0xF];
                phase_step = (static_cast<uint64_t>(target_hz) << 16) / host_sample_rate;
            }
        } else {
            phase_step = (static_cast<uint64_t>(raw_period) << 16) / host_sample_rate;
        }
    }

    [[nodiscard]] constexpr int16_t tick() {
        if (!enabled) return 0;

        if constexpr (Traits::has_nes_quirks) {
            // --- BRANCH A: NES HARDWARE LOGIC ---
            switch (nes_type) {
                case AudioChannelBase::Type::Pulse: {
                    phase_accumulator += phase_step;
                    // Scale phase into 8 discrete step phases
                    uint8_t current_step = (phase_accumulator >> 16) & 7;
                    bool active = NESTables::pulse_duties[pulse_duty_selection][current_step];
                    return active ? static_cast<int16_t>(volume * 32) : static_cast<int16_t>(-volume * 32);
                }
                case AudioChannelBase::Type::Triangle: {
                    // Triangle maps to a continuous linear-interpolated lookup index 
                    uint32_t index = phase_accumulator >> 16;
                    uint32_t next_index = (index + 1) % 32;
                    uint16_t fraction = phase_accumulator & 0xFFFF;

                    int32_t s1 = wave_ram[index];
                    int32_t s2 = wave_ram[next_index];
                    int32_t interpolated = s1 + (((s2 - s1) * fraction) >> 16);

                    phase_accumulator += phase_step;
                    if ((phase_accumulator >> 16) >= 32) {
                        phase_accumulator -= (32 << 16);
                    }
                    return static_cast<int16_t>(interpolated);
                }
                case AudioChannelBase::Type::Noise: {
                    phase_accumulator += phase_step;
                    // Clock the LFSR logic whenever the phase counter rolls over
                    if (phase_accumulator >= (1 << 16)) {
                        phase_accumulator -= (1 << 16);
                        
                        // NES feedback selection shifts [1]
                        uint16_t shift_amount = noise_short_mode ? 6 : 1;
                        uint16_t feedback = (lfsr_register ^ (lfsr_register >> shift_amount)) & 1;
                        lfsr_register = (lfsr_register >> 1) | (feedback << 14);
                    }
                    // Read the inverted output bit to get a clean noise value
                    return (lfsr_register & 1) ? static_cast<int16_t>(volume * 16) : static_cast<int16_t>(-volume * 16);
                }
                default: return 0;
            }
        } else {
            // --- BRANCH B: SYSTEM WAVETABLE INTERPOLATION FALLBACK ---
            if constexpr (Traits::uses_samples) {
                uint32_t index = phase_accumulator >> 16;
                uint32_t next_index = (index + 1) % Traits::wave_ram_size;
                uint16_t fraction = phase_accumulator & 0xFFFF;

                int32_t sample1 = wave_ram[index];
                int32_t sample2 = wave_ram[next_index];
                int32_t interpolated = sample1 + (((sample2 - sample1) * fraction) >> 16);

                phase_accumulator += phase_step;
                if ((phase_accumulator >> 16) >= Traits::wave_ram_size) {
                    phase_accumulator -= (Traits::wave_ram_size << 16);
                }
                return static_cast<int16_t>((interpolated * volume) >> 8);
            } else {
                phase_accumulator += phase_step;
                return ((phase_accumulator >> 16) & 1) ? 4096 : -4096;
            }
        }
    }
};

template<Platform APU>
class AudioProcessor {
    public:
        using Traits = ChipTraits<APU>;

        constexpr AudioProcessor(uint32_t target_output_rate) 
            : mHostSampleRate(target_output_rate) {
                static_assert(Traits::channel_count > 0);
            
            // If target platform is NES, assign identity rules to individual channel slots
            if constexpr (APU == Platform::NES) {
                mChannels[0].configure_nes_type(AudioChannelBase::Type::Pulse);     // Pulse 1
                mChannels[1].configure_nes_type(AudioChannelBase::Type::Pulse);     // Pulse 2
                mChannels[2].configure_nes_type(AudioChannelBase::Type::Triangle);  // Triangle
                mChannels[3].configure_nes_type(AudioChannelBase::Type::Noise);     // Noise
                mChannels[4].configure_nes_type(AudioChannelBase::Type::DMC);       // DMC Sample
            }
        }

        consteval uint32_t getHostSampleRate() const { return mHostSampleRate; }

        constexpr void write_pitch(size_t channel_idx, uint16_t period_val) {
            if (channel_idx < Traits::channel_count) {
                mChannels[channel_idx].set_frequency(period_val, getHostSampleRate());
            }
        }

        constexpr void modify_nes_pulse(size_t channel_idx, uint8_t duty_mode) {
            if constexpr (std::is_same_v<APU, Platform::NES>) {
                if (channel_idx < 2) {
                    mChannels[channel_idx].set_nes_pulse_duty(duty_mode);
                }
            }
        }

        constexpr void modify_nes_noise(bool short_mode) {
            if constexpr (std::is_same_v<APU, Platform::NES>) {
                mChannels[3].set_nes_noise_mode(short_mode);
            }
        }

        void process_audio(int16_t* out_buffer, size_t sample_count) {
            for (size_t i = 0; i < sample_count; ++i) {
                int32_t master_mix = 0;

                for (size_t c = 0; c < Traits::channel_count; ++c) {
                    master_mix += mChannels[c].tick();
                }

                if (master_mix > 32767) master_mix = 32767;
                else if (master_mix < -32768) master_mix = -32768;

                out_buffer[i] = static_cast<int16_t>(master_mix);
            }
        }

    protected:
        std::array<AudioChannel<APU>, Traits::channel_count> mChannels;

    private:
        uint32_t mHostSampleRate;
};


}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_BASE_H