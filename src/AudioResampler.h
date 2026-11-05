
#ifndef _AUDIO_RESAMPLER_H_
#define _AUDIO_RESAMPLER_H_

#include <array>
#include <cstddef>

namespace nes::apu {

// -----------------------------------------------------------------------------
// AudioResampler
//
// Converts CPU-rate mixed audio into band-limited host audio.
//
// The input is one instantaneous mixed-audio level per emulated CPU cycle.
//
// A windowed-sinc FIR filter removes high-frequency components before the
// signal is sampled at the host output rate.
//
// Fractional-phase filter coefficients are precomputed so that audio
// generation does not require trigonometric calculations.
//
// The resampler does not modify channel state, apply DC blocking, or convert
// samples to unsigned 8-bit PCM.
// -----------------------------------------------------------------------------
class AudioResampler
{
	public:
    static constexpr std::size_t kTapCount = 1025;
    static constexpr std::size_t kPhaseCount = 64;

	 public:
    AudioResampler (double inputRate, double outputRate,
        		   double cutoffFrequency = 20000.0);

    // Reset the sample history and fractional sampling phase.
    void reset (double initialLevel = 0.0);

    // Accept one CPU-cycle sample.
    // Returns true when a new host-rate sample has been generated.
    bool clock (double input, double &output);

	private:
    // Build normalized coefficients for each fractional phase.
    void build_filter();

    // Evaluate the FIR filter at the selected fractional phase.
    double filtered_sample(std::size_t phase) const;

	private:
    double input_rate_;
    double output_rate_;
    double cutoff_frequency_;

    // Host-sample scheduling accumulator.
    double sample_phase_ = 0.0;

    // Circular CPU-rate input history.
    std::array<double, kTapCount> history_{};
    std::size_t write_index_ = 0;

    // Precomputed polyphase FIR coefficients.
    std::array<std::array<double, kTapCount>, kPhaseCount> coefficients_{};
};

} // namespace nes::apu


#endif // _AUDIO_RESAMPLER_H_
