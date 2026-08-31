
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "Palette.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
namespace {

// -----------------------------------------------------------------------------
// bound
//
// Clamps a value to the inclusive range defined by the supplied lower and upper
// bounds.
//
// Parameters:
//   lower - Minimum permitted value.
//   value - Value to clamp.
//   upper - Maximum permitted value.
//
// Returns:
//   Clamped value within the requested range.
// -----------------------------------------------------------------------------
template <class T>
constexpr T bound(T lower, T value, T upper) {
	return std::max(lower, std::min(value, upper));
}


// -----------------------------------------------------------------------------
// wave
//
// Computes the NES NTSC square-wave state for a given pixel phase and color
// index.
//
// Parameters:
//   p     - Current phase position within the 12-cycle color waveform.
//   color - NES color index used to select the waveform phase.
//
// Returns:
//   1 when the waveform is in its high state, otherwise 0.
// -----------------------------------------------------------------------------
constexpr int wave(int p, int color) {
	return (color + p + 8) % 12 < 6;
}


// -----------------------------------------------------------------------------
// gamma_fix
//
// Applies gamma correction to a normalized color component.
//
// Negative input values are clamped to zero before the gamma curve is applied.
//
// Parameters:
//   f     - Normalized color-component value.
//   gamma - Target gamma value.
//
// Returns:
//   Gamma-corrected component value.
// -----------------------------------------------------------------------------
constexpr float gamma_fix(float f, float gamma) {
	return f < 0.f ? 0.f : std::pow(f, 2.2f / gamma);
}


// -----------------------------------------------------------------------------
// make_rgb_color
//
// Converts a NES palette entry into an RGB color.
//
// The NES color index, including emphasis bits, is converted into an emulated
// NTSC waveform.  The waveform is demodulated into YIQ, adjusted for saturation,
// hue, contrast, brightness, and gamma, and finally converted to RGB.
//
// Parameters:
//   pixel      - NES palette index including color-emphasis bits.
//   saturation - Chroma saturation adjustment.
//   hue        - Hue phase adjustment.
//   contrast   - Contrast adjustment.
//   brightness - Brightness adjustment.
//   gamma      - Output gamma correction value.
//
// Returns:
//   RGB color corresponding to the supplied NES palette entry.
// -----------------------------------------------------------------------------
rgb_color_t 
make_rgb_color (uint16_t pixel, float saturation, float hue, float contrast, float brightness, float gamma) 
{

	// The input value is a NES color index (with de-emphasis bits).
	// We need RGB values. Convert the index into RGB.
	// For most part, this process is described at:
	//    http://wiki.nesdev.com/w/index.php/NTSC_video

	// Decode the color index
	const uint8_t color = (pixel & 0x0f);
	const uint8_t level = color < 0x0e ? (pixel >> 4) & 3 : 1;

	// Voltage levels, relative to synch voltage
	static constexpr float black       = 0.518f;
	static constexpr float white       = 1.962f;
	static constexpr float attenuation = 0.746f;

	static const float levels[8] = {
		0.350f, 0.518f, 0.962f, 1.550f, // Signal low
		1.094f, 1.506f, 1.962f, 1.962f  // Signal high
	};

	const float lo_and_hi[2] = {
		levels[level + 4 * (color == 0x00)],
		levels[level + 4 * (color < 0x0d)],
	};

	// Calculate the luma and chroma by emulating the relevant circuits:
	float y = 0.f;
	float i = 0.f;
	float q = 0.f;

	// 12 clock cycles per pixel.
	for (int p = 0; p < 12; ++p) {

		// NES NTSC modulator (square wave between two voltage levels):
		float spot = lo_and_hi[wave(p, color)];

		// De-emphasis bits attenuate a part of the signal:
		if (((pixel & 0x40) && wave(p, 12)) || ((pixel & 0x80) && wave(p, 4)) || ((pixel & 0x100) && wave(p, 8))) {
			spot *= attenuation;
		}

		// Normalize:
		float v = (spot - black) / (white - black);

		// Ideal TV NTSC demodulator:
		// Apply contrast/brightness
		v = (v - .5f) * contrast + .5f;
		v *= brightness / 12.f;

		y += v;
		i += v * std::cos((M_PI / 6.) * (p + hue));
		q += v * std::sin((M_PI / 6.) * (p  + hue));
	}

	i *= saturation;
	q *= saturation;

	// Convert YIQ into RGB according to FCC-sanctioned conversion matrix.
	rgb_color_t rgb;
	rgb.r = bound(0x00, static_cast<int>(255 * gamma_fix(y + 0.946882f * i + 0.623557f * q, gamma)), 0xff);
	rgb.g = bound(0x00, static_cast<int>(255 * gamma_fix(y + -0.274788f * i + -0.635691f * q, gamma)), 0xff);
	rgb.b = bound(0x00, static_cast<int>(255 * gamma_fix(y + -1.108545f * i + 1.709007f * q, gamma)), 0xff);
	return rgb;
}

}


// -----------------------------------------------------------------------------
// Palette::Generate
//
// Generates the emulator's 64-color NES RGB palette.
//
// Each NES palette index is converted through the NTSC color model using the
// supplied saturation, hue, contrast, brightness, and gamma adjustments.  The
// generated colors are stored in a persistent static palette array.
//
// Parameters:
//   saturation - Chroma saturation adjustment.
//   hue        - Hue phase adjustment.
//   contrast   - Contrast adjustment.
//   brightness - Brightness adjustment.
//   gamma      - Output gamma correction value.
//
// Returns:
//   Pointer to the generated 64-entry RGB color palette.
// -----------------------------------------------------------------------------
const rgb_color_t *Palette::Generate (float saturation, float hue, float contrast, float brightness, float gamma)
{

	std::cout << "Creating Palette: (" << saturation << "," << hue << "," << contrast << "," << brightness << "," << gamma << ")" << std::endl;
	
	static rgb_color_t color_list[64];

	for (int i = 0; i < 64; ++i) {
		color_list[i] = make_rgb_color(i, saturation, hue, contrast, brightness, gamma);
	}

	return color_list;
}
