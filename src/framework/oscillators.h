#ifndef __RETRO_CORE_FRAMEWORK_OSCILLATORS_H
#define __RETRO_CORE_FRAMEWORK_OSCILLATORS_H

#include <cmath>
#include <type_traits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace RetroCore {

// T: Time tracking type (e.g., float, double, int)
// V: Output value type (e.g., uint8_t, int, float, vec3)
template<typename T, typename V>
class OscillatorBase {
	public:
		OscillatorBase(T period, V minValue, V maxValue)
        	: mPeriod(period > static_cast<T>(0) ? period : static_cast<T>(1))
        	, mTimer(static_cast<T>(0))
        	, mMinValue(minValue)
        	, mMaxValue(maxValue) {}

	    virtual ~OscillatorBase() = default;
	    virtual V getValue() const = 0;

	    virtual void update(T dt) {
	        mTimer += dt;

	        // Clean boundary wrap
	        if (mTimer >= mPeriod) {
	            mTimer -= mPeriod;
	        } else if (mTimer < static_cast<T>(0)) {
	            mTimer += mPeriod;
	        }
	    }

	    //Convenience wrapper
	    V updateValue(T dt) {
        	update(dt);
        	return getValue();
    	}

    	// Shared Modifiers
    	void setMinValue(V minValue) { mMinValue = minValue; }
    	void setMaxValue(V maxValue) { mMaxValue = maxValue; }
	    void setPeriod(T period) { mPeriod = period > static_cast<T>(0) ? period : static_cast<T>(1); }
	    void setRange(V minValue, V maxValue) { mMinValue = minValue; mMaxValue = maxValue; }
	    void reset() { mTimer = static_cast<T>(0); }

    protected:
		T mPeriod;            // Total duration of one cycle
    	T mTimer;             // Tracks raw elapsed time [0, m_period)
    	V mMinValue;          // Output value boundary min
    	V mMaxValue;          // Output value boundary max
};

template<typename T, typename V>
class SquareWaveOscillator: public OscillatorBase<T, V> {
	public:
		SquareWaveOscillator(T period, V minValue, V maxValue, double dutyCycle = 0.5)
	        : OscillatorBase<T, V>(period, minValue, maxValue)
	        , mDutyCycle(dutyCycle) 
        {
        	updateHighDuration(dutyCycle);
        }

		V getValue() const override final {
	        if (this->mTimer < mHighDuration) {
	            return this->mMaxValue;
	        }
	        return this->mMinValue;
	    }

	    void setPeriod(T period, double dutyCycle = 0.5) {
        	OscillatorBase<T, V>::setPeriod(period);
        	mDutyCycle = dutyCycle;
        	updateHighDuration();
    	}

	private:
		// Internal helper to calculate the duration of the high phase
	    void updateHighDuration(double dutyCycle) {
	        if constexpr (std::is_floating_point_v<T>) {
	            mHighDuration = this->mPeriod * static_cast<T>(dutyCycle);
	        } else {
	            mHighDuration = static_cast<T>(static_cast<double>(this->mPeriod) * dutyCycle);
	        }
	    }

	private:
		T mHighDuration;      // Cached threshold splitting high/low phases
		double mDutyCycle;
};

template <typename T, typename V>
class SineWaveOscillator : public OscillatorBase<T, V> {
	private:
	    // Fallback engine lerp for basic mathematical types (float, double, int)
	    // If V is a complex class/struct, it must override or support operators.
	    V lerp(const V& minVal, const V& maxVal, double t) const {
	        if constexpr (std::is_arithmetic_v<V>) {
	            return static_cast<V>(minVal + (maxVal - minVal) * t);
	        } else {
	            // If driving a class like Vector3 or Color, defer to its internal lerp method
	            return V::lerp(minVal, maxVal, t); 
	        }
	    }

	public:
	    using OscillatorBase<T, V>::OscillatorBase; // Inherit base constructors

	    // Override to read smooth wave translation
	    V getValue() const override {
	        // Convert raw time context to radians [0, 2 * PI]
	        double progress = static_cast<double>(this->mTimer) / static_cast<double>(this->mPeriod);
	        double radians = progress * 2.0 * M_PI;
	        
	        // Standard sin outputs [-1.0, 1.0]. Remap it to a smooth [0.0, 1.0] factor
	        double normalFactor = (std::sin(radians) + 1.0) * 0.5;
	        
	        // Return interpolated position safely bounded by range parameters
	        return lerp(this->mMinValue, this->mMaxValue, normalFactor);
	    }
};

template <typename T, typename V>
class SawOscillator : public OscillatorBase<T, V> {
	private:
	    // Uses the same safe engine lerp for basic arithmetic types,
	    // or defers to custom class/struct static lerp methods.
	    V lerp(const V& minVal, const V& maxVal, double t) const {
	        if constexpr (std::is_arithmetic_v<V>) {
	            return static_cast<V>(minVal + (maxVal - minVal) * t);
	        } else {
	            return V::lerp(minVal, maxVal, t); 
	        }
	    }

	public:
	    using OscillatorBase<T, V>::OscillatorBase; // Inherit base constructors

	    // Override to read linear ramp translation
	    V getValue() const override {
	        // Calculate raw progress through the cycle as a pure factor from [0.0, 1.0)
	        double progress = static_cast<double>(this->mTimer) / static_cast<double>(this->mPeriod);
	        
	        // Return linearly interpolated position from min to max across the timeline
	        return lerp(this->mMinValue, this->mMaxValue, progress);
	    }
};

template <typename T, typename V>
class BounceOscillator : public OscillatorBase<T, V> {
	private:
	    // Safe engine lerp strategy
	    V lerp(const V& minVal, const V& maxVal, double t) const {
	        if constexpr (std::is_arithmetic_v<V>) {
	            return static_cast<V>(minVal + (maxVal - minVal) * t);
	        } else {
	            return V::lerp(minVal, maxVal, t); 
	        }
	    }

	public:
	    using OscillatorBase<T, V>::OscillatorBase; // Inherit base constructors

	    // Override to return a smooth bouncing arch shape
	    V getValue() const override {
	        // Calculate raw progress through the cycle from [0.0, 1.0)
	        double progress = static_cast<double>(this->mTimer) / static_cast<double>(this->mPeriod);
	        
	        // Map progress across a half-sine arc [0, PI]
	        // This ensures the value starts at minVal, peaks at maxVal, and returns to minVal exactly at the end of the period
	        double bounceFactor = std::sin(progress * M_PI);
	        
	        return lerp(this->mMinValue, this->mMaxValue, bounceFactor);
	    }
};

template <typename T, typename V>
class TriangleOscillator : public OscillatorBase<T, V> {
	private:
	    V lerp(const V& minVal, const V& maxVal, double t) const {
	        if constexpr (std::is_arithmetic_v<V>) {
	            return static_cast<V>(minVal + (maxVal - minVal) * t);
	        } else {
	            return V::lerp(minVal, maxVal, t); 
	        }
	    }

	public:
	    using OscillatorBase<T, V>::OscillatorBase; // Inherit base constructors

	    // Override to return a linear ping-pong ramp shape
	    V getValue() const override {
	        double progress = static_cast<double>(this->mTimer) / static_cast<double>(this->mPeriod);
	        
	        // Convert [0.0, 1.0) progress into a symmetrical triangle wave factor [0.0, 1.0, 0.0]
	        double triangleFactor = 1.0 - std::abs(2.0 * progress - 1.0);
	        
	        return lerp(this->mMinValue, this->mMaxValue, triangleFactor);
	    }
};

template <typename T, typename V>
class PulseOscillator : public OscillatorBase<T, V> {
	public:
	    // Initializes with a strict pulseDuration and a starting position offset inside the timeline
	    PulseOscillator(T period, V minValue, V maxValue, T pulseDuration, T pulsePosition = static_cast<T>(0))
	        : OscillatorBase<T, V>(period, minValue, maxValue)
	        , mPulseDuration(pulseDuration)
	        , mPulseStart(pulsePosition) 
	    {
	        updatePulseWindow();
	    }

	    // Evaluates if the current timeline position falls inside our explicit window
	    V getValue() const override {
	        if (this->mTimer >= mPulseStart && this->mTimer < mPulseEnd) {
	            return this->mMaxValue;
	        }
	        return this->mMinValue;
	    }

	    // Shadow core base methods to ensure structural adjustments trigger dynamic safety updates
	    void setPeriod(T period) {
	        OscillatorBase<T, V>::setPeriod(period);
	        updatePulseWindow();
	    }

	    // Adjust the exact width/duration of your active state trigger
	    void setPulseDuration(T duration) {
	        mPulseDuration = duration;
	        updatePulseWindow();
	    }

	    // Slide the starting window position of your active state backward or forward within the period
	    void setPulsePosition(T position) {
	        mPulseStart = position;
	        updatePulseWindow();
	    }

	private:
	    T mPulseDuration;  // Exact width/duration of the pulse (independent of period)
	    T mPulseStart;     // Timestamp inside the period where the pulse starts [0, period)
	    T mPulseEnd;       // Cached timestamp where the pulse finishes

	    // Internal safety helper to update bounds and clamp them smoothly within the period
	    void updatePulseWindow() {
	        // Clamp start point so it fits within the timeline loop
	        if (mPulseStart >= this->mPeriod) mPulseStart = this->mPeriod;
	        if (mPulseStart < static_cast<T>(0)) mPulseStart = static_cast<T>(0);

	        // Prevent the pulse from running past the end of the total period duration
	        T maxAvailableDuration = this->mPeriod - mPulseStart;
	        if (mPulseDuration > maxAvailableDuration) {
	            mPulseDuration = maxAvailableDuration;
	        }
	        if (mPulseDuration < static_cast<T>(0)) mPulseDuration = static_cast<T>(0);

	        mPulseEnd = mPulseStart + mPulseDuration;
	    }
};


}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_OSCILLATORS_H