
#include "AudioResampler.h"

#include <algorithm>
#include <cmath>

namespace nes::apu {

namespace {

constexpr double kPi =  3.141592653589793238462643383279502884;

}


// -----------------------------------------------------------------------------
// AudioResampler::AudioResampler
//
// Constructs the band-limited audio resampler.
//
// Filter coefficients are calculated once and reused for every audio sample.
//
// Parameters:
//   inputRate       - Input sampling frequency in Hz.
//   outputRate      - Host output sampling frequency in Hz.
//   cutoffFrequency - Low-pass cutoff frequency in Hz.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
AudioResampler::AudioResampler(double inputRate, double outputRate, double cutoffFrequency)
    : input_rate_(inputRate),
      output_rate_(outputRate),
      cutoff_frequency_(cutoffFrequency)
{
    // Keep the cutoff below both Nyquist limits.
    cutoff_frequency_ = std::clamp(cutoff_frequency_, 1.0, 0.45 * std::min(input_rate_, output_rate_));

    build_filter();
    reset();
}


// -----------------------------------------------------------------------------
// AudioResampler::reset
//
// Clears the resampler's history and resets its sampling phase.
//
// The initial history is filled with the supplied signal level. This avoids
// treating a preexisting, steady DAC level as a newly generated transition.
//
// Parameters:
//   initialLevel - Initial raw mixed-audio amplitude.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioResampler::reset (double initialLevel)
{
    history_.fill(initialLevel);

    write_index_ = 0;
    sample_phase_ = 0.0;
}


// -----------------------------------------------------------------------------
// AudioResampler::build_filter
//
// Builds a bank of fractional-delay, windowed-sinc FIR filters.
//
// Each phase represents a slightly different sampling position between two
// consecutive CPU-cycle input samples.
//
// A Blackman window suppresses sidelobes and provides substantial rejection
// of frequencies above the desired audio passband.
//
// Every phase is normalized to unity DC gain, preserving constant input
// levels without introducing an amplitude change.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioResampler::build_filter()
{
    const double normalizedCutoff = cutoff_frequency_ / input_rate_;
    const double center = static_cast<double>(kTapCount - 1) * 0.5;

    for (std::size_t phase = 0; phase < kPhaseCount; ++phase) {

        const double fraction = static_cast<double>(phase) / static_cast<double>(kPhaseCount);
        double sum = 0.0;

        for (std::size_t tap = 0; tap < kTapCount; ++tap) {
        	const double position = static_cast<double>(tap);
        	const double distance = position - center - fraction;

            // Ideal low-pass impulse response.
            double coefficient;

            if (std::abs(distance) < 1.0e-12) {
                coefficient = 2.0 * normalizedCutoff;
            } else {
                coefficient =
                    std::sin(2.0 * kPi * normalizedCutoff * distance) / (kPi * distance);
            }

            // Blackman window.
            const double angle = 2.0 * kPi * position / static_cast<double>(kTapCount - 1);
            const double window = 0.42 - 0.50 * std::cos(angle) + 0.08 * std::cos(2.0 * angle);

            coefficient *= window;

            coefficients_[phase][tap] = coefficient;
            sum += coefficient;
        }

        // Normalize the DC gain of this phase.
        if (std::abs(sum) > 1.0e-15) {
            for (std::size_t tap = 0; tap < kTapCount; ++tap) {
            	coefficients_[phase][tap] /= sum;
            }
        }
    }
}


// -----------------------------------------------------------------------------
// AudioResampler::filtered_sample
//
// Evaluates the FIR filter against the circular CPU-rate input history.
//
// Tap zero corresponds to the newest input sample. Increasing tap indices
// reference progressively older samples.
//
// Parameters:
//   phase - Fractional-delay coefficient-bank index.
//
// Returns:
//   Band-limited raw audio amplitude.
// -----------------------------------------------------------------------------
double
AudioResampler::filtered_sample(std::size_t phase) const
{
    const auto &filter = coefficients_[phase];

    std::size_t index = (write_index_ == 0) ? kTapCount - 1 : write_index_ - 1;\
    double output = 0.0;

    for (std::size_t tap = 0; tap < kTapCount; ++tap) {
    	output += filter[tap] * history_[index];

        if (index == 0) {
            index = kTapCount - 1;
        } else {
            --index;
        }
    }

    return output;
}


// -----------------------------------------------------------------------------
// AudioResampler::clock
//
// Accepts one raw mixed-audio sample at the emulated CPU clock rate.
//
// Input samples are stored in a circular history buffer. A fractional
// accumulator schedules output samples at the requested host sample rate.
//
// When an output sample is due, the filter phase is selected according to
// the fractional position of that sample within the current CPU clock.
//
// Parameters:
//   input  - Current instantaneous raw mixed-audio level.
//   output - Receives a new filtered sample when the function returns true.
//
// Returns:
//   true  - A host-rate audio sample was generated.
//   false - No output sample is due for this CPU cycle.
// -----------------------------------------------------------------------------
bool
AudioResampler::clock(double input, double &output)
{
    history_[write_index_] = input;

    ++write_index_;

    if (write_index_ == kTapCount) {
        write_index_ = 0;
    }

    sample_phase_ += output_rate_;

    if (sample_phase_ < input_rate_) {
        return false;
    }

    sample_phase_ -= input_rate_;

    // Fractional age of the scheduled host sample relative to the newest
    // CPU-rate sample.
    const double fraction = sample_phase_ / output_rate_;
    std::size_t phase = static_cast<std::size_t>(fraction * static_cast<double>(kPhaseCount));

    if (phase >= kPhaseCount) {
        phase = kPhaseCount - 1;
    }

    output = filtered_sample(phase);

    return true;
}

} // namespace nes::apu
