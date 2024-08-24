/*------------------------------------------------------------------------------
 * Copyright (c) 2017-2019
 *     Michael Theall (mtheall)
 *
 * This file is part of tex3ds.
 *
 * tex3ds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tex3ds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
/** @file quantum.h
 *  @brief Magick::Quantum conversions
 */
#pragma once

#include "magick_compat.h"

#include <algorithm>
#include <cmath>

namespace
{
/** @brief Convert Magick::Quantum to an n-bit value
 *  @tparam    bits Number of bits for output value
 *  @param[in] v    Quantum to convert
 *  @returns n-bit quantum
 */
template <int bits>
inline uint8_t quantum_to_bits (Magick::Quantum v)
{
	using Magick::Quantum;
	return (1 << bits) * v / (QuantumRange + 1);
}

/** @brief Convert an n-bit value to a Magick::Quantum
 *  @tparam    bits Number of bits for input value
 *  @param[in] v    Input n-bit value
 *  @returns Magick::Quantum
 */
template <int bits>
inline Magick::Quantum bits_to_quantum (uint8_t v)
{
	using Magick::Quantum;
	return v * QuantumRange / ((1 << bits) - 1);
}

/** @brief Quantize a Magick::Quantum to its n-bit equivalent
 *  @tparam    bits Number of significant bits
 *  @param[in] v    Quantum to quantize
 *  @returns quantized Magick::Quantum
 */
template <int bits>
inline Magick::Quantum quantize (Magick::Quantum v)
{
	return bits_to_quantum<bits> (quantum_to_bits<bits> (v));
}

/** @brief sRGB Gamma inverse
 *  @param[in] v Value to get inverse gamma
 *  @return inverse gamma
 */
inline double gamma_inverse (double v)
{
	if (v <= 0.04045)
		return v / 12.92;
	return std::pow ((v + 0.055) / 1.055, 2.4);
}

/** @brief sRGB Gamma
 *  @param[in] v Value to get gamma
 *  @return gamma
 */
inline double gamma (double v)
{
	if (v <= 0.0031308)
		return v * 12.92;
	return 1.055 * std::pow (v, 1.0 / 2.4) - 0.055;
}

/** @brief Get luminance from RGB with gamma correction
 *  @param[in] c Color to get luminance
 *  @return luminance
 */
inline Magick::Quantum luminance (const Magick::Color &c)
{
	// ITU Recommendation BT.709
	const double r = 0.212655;
	const double g = 0.715158;
	const double b = 0.072187;

	using Magick::Quantum;

	// Gamma correction
	double v = gamma (r * gamma_inverse (static_cast<double> (quantumRed (c)) / QuantumRange) +
	                  g * gamma_inverse (static_cast<double> (quantumGreen (c)) / QuantumRange) +
	                  b * gamma_inverse (static_cast<double> (quantumBlue (c)) / QuantumRange));

	// clamp
	return std::max (0.0, std::min (1.0, v)) * QuantumRange;
}
}
