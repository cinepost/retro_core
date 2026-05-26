#ifndef __RETRO_CORE_FRAMEWORK_APU_APU_H
#define __RETRO_CORE_FRAMEWORK_APU_APU_H

#include <array>
#include <vector>
#include <tuple>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <functional>

namespace RetroCore {

namespace APU {

// ==================== ENUMS & BASE TYPES ====================
enum class ApuComponent : uint8_t {
    NONE = 0, 
    NES, 
    SMS, 
    YM2612, 
    SNES, 
    MSX, 
    WAV, 
    PSG, 
    BEEPPER, 
    COVOX,
    MAX
};

constexpr uint8_t toTupleIndex(ApuComponent component) {
    return static_cast<uint8_t>(component);
}

struct IAudioComponent {
public:
    virtual ~IAudioComponent() = default;
    virtual void reset() = 0;
    virtual void process(float* out, std::size_t frames) = 0;
    virtual double getBaseClock() const = 0;
    virtual void writeRegister(uint16_t addr, uint8_t val) = 0;
    static constexpr uint16_t regStart = 0;
    static constexpr uint16_t regEnd = 0;
};

// ==================== HYBRID APu MIXER ====================
template <typename... Components>
class HybridApu : public IAudioComponent {
public:
    template <typename... Args>
    explicit HybridApu(Args&&... args) : mComponents(std::forward<Args>(args)...) {}

    void reset() override {
        std::apply([](auto&... comps) { (comps.reset(), ...); }, mComponents);
    }

    double getBaseClock() const override { return 44100.0; }

    void process(float* out, std::size_t frames) override {
        static constexpr float inv_weight = 1.0f / (float)sizeof...(Components);
        static std::vector<float> temp_buffer;
        
        std::fill(out, out + frames * 2, 0.0f);
        temp_buffer.resize(frames * 2);
        std::fill(temp_buffer.begin(), temp_buffer.end(), 0.0f);

        float* temp_ptr = temp_buffer.data();

        std::apply([temp_ptr, out, frames](auto&... comps) {(
            (
                comps.process(temp_ptr, frames),
                std::transform(temp_ptr, temp_ptr + frames * 2, out, out, [](float temp, float res) { return res + temp; })
            ), 
            ... 
        ); }, mComponents);
    }

    void writeRegister(uint16_t addr, uint8_t val) override {
        std::apply([addr, val](auto&... comps) { (comps.writeRegister(addr, val), ...); }, mComponents);
    }

    // Expose components for route registration
    std::tuple<Components...>& getComponents() { return mComponents; }

private:
    std::tuple<Components...> mComponents;
};

// ==================== APU IMPLEMENTATIONS ====================

class Beeper : public IAudioComponent {
public:
    void reset() override { state = false; }
    double getBaseClock() const override { return 0.0; }
    void writeRegister(uint16_t addr, uint8_t val) override {
        state = (val & 0x01) != 0;
    }
    void process(float* out, std::size_t frames) override {
        float val = state ? 1.0f : -1.0f;
        for (std::size_t i = 0; i < frames; ++i) {
            out[i*2] = val;
            out[i*2+1] = val;
        }
    }
private:
    bool state = false;
};

class Covox : public IAudioComponent {
public:
    void reset() override { dacValue = 128; }
    double getBaseClock() const override { return 0.0; }
    void writeRegister(uint16_t addr, uint8_t val) override {
        dacValue = val;
    }
    void process(float* out, std::size_t frames) override {
        float val = (static_cast<float>(dacValue) / 127.5f) - 1.0f;
        for (std::size_t i = 0; i < frames; ++i) {
            out[i*2] = val;
            out[i*2+1] = val;
        }
    }
private:
    uint8_t dacValue = 128;
};

}  // namespace APU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_APU_H