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
/** @file magick_compat.h
 *  @brief ImageMagick compatibility routines
 *
 *  @details
 *  This file hides the differences between ImageMagick 6 and ImageMagick 7.
 *
 *  Changes from ImageMagick 6 to ImageMagick 7:
 *  - Magick::FilterTypes renamed to Magick::FilterType
 *  - Magick::Color::redQuantum renamed to Magick::Color::quantumRed
 *  - Magick::Color::blueQuantum renamed to Magick::Color::quantumBlue
 *  - Magick::Color::greenQuantum renamed to Magick::Color::quantumGreen
 *  - Magick::Color::alphaQuantum renamed to Magick::Color::quantumAlpha
 *  - Alpha channel is now alpha instead of opacity
 *  - Magick::PixelPacket removed
 */
#pragma once

#include <cassert> // must precede Magick++.h

#include <Magick++.h>

#include <algorithm>

// ImageMagick compatibility
#if MagickLibVersion >= 0x700

/** @brief Resize filter type */
typedef Magick::FilterType FilterType;

namespace
{
/** @brief Get the red value from a color
 *  @param[in] c Color to read
 *  @returns Red quantum
 */
inline Magick::Quantum quantumRed (const Magick::Color &c)
{
	return c.quantumRed ();
}

/** @brief Set the red value for a color
 *  @param[in] c Color to modify
 *  @param[in] v Green quantum to set
 */
inline void quantumRed (Magick::Color &c, Magick::Quantum v)
{
	c.quantumRed (v);
}

/** @brief Get the green value from a color
 *  @param[in] c Color to read
 *  @returns Green quantum
 */
inline Magick::Quantum quantumGreen (const Magick::Color &c)
{
	return c.quantumGreen ();
}

/** @brief Set the green value for a color
 *  @param[in] c Color to modify
 *  @param[in] v Green quantum to set
 */
inline void quantumGreen (Magick::Color &c, Magick::Quantum v)
{
	c.quantumGreen (v);
}

/** @brief Get the blue value from a color
 *  @param[in] c Color to read
 *  @returns Blue quantum
 */
inline Magick::Quantum quantumBlue (const Magick::Color &c)
{
	return c.quantumBlue ();
}

/** @brief Set the blue value for a color
 *  @param[in] c Color to modify
 *  @param[in] v Blue quantum to set
 */
inline void quantumBlue (Magick::Color &c, Magick::Quantum v)
{
	c.quantumBlue (v);
}

/** @brief Get the alpha value from a color
 *  @param[in] c Color to read
 *  @returns Alpha quantum
 */
inline Magick::Quantum quantumAlpha (const Magick::Color &c)
{
	return c.quantumAlpha ();
}

/** @brief Set the alpha value for a color
 *  @param[in] c Color to modify
 *  @param[in] v Alpha quantum to set
 */
inline void quantumAlpha (Magick::Color &c, Magick::Quantum v)
{
	c.quantumAlpha (v);
}
}

class Pixels;

/** @brief Emulator for Magick::PixelPacket* */
class PixelPacket
{
private:
	const Pixels *cache;     ///< Pixel cache
	Magick::Quantum *pixels; ///< Pixel data

public:
	/** @brief Emulator for Magick::PixelPacket */
	class Reference
	{
	private:
		const Pixels *cache;    ///< Pixel cache
		Magick::Quantum *pixel; ///< Pixel referenced

		/** @brief Default constructor */
		Reference () = delete;

		/** @brief Constructor
		 *  @param[in] cache Pixel cache
		 *  @param[in] pixel Pixel referenced
		 */
		Reference (const Pixels *cache, Magick::Quantum *pixel);

		friend class PixelPacket;

	public:
		/** @brief Copy constructor
		 *  @param[in] other Reference to copy
		 */
		Reference (const Reference &other) = default;

		/** @brief Copy constructor
		 *  @param[in] other Reference to copy
		 */
		Reference (Reference &&other) = default;

		/** @brief Assignment operator
		 *  @param[in] other Reference to assign
		 *  @returns reference to self
		 */
		Reference &operator= (const Reference &other);

		/** @brief Assignment operator
		 *  @param[in] other Reference to assign
		 *  @returns reference to self
		 */
		Reference &operator= (Reference &&other);

		/** @brief Assignment operator
		 *  @param[in] c Color to assign
		 *  @returns reference to self
		 */
		Reference &operator= (const Magick::Color &c);

		/** @brief Cast operator to Magick::Color
		 *  @returns Magick::Color
		 */
		operator Magick::Color () const;
	};

	/** @brief Constructor
	 *  @param[in] cache  Pixel cache
	 *  @param[in] pixels Pixel data
	 */
	PixelPacket (const Pixels *cache, Magick::Quantum *pixels);

	/** @brief Copy constructor
	 *  @param[in] other PixelPacket to copy
	 */
	PixelPacket (const PixelPacket &other);

	/** @brief Assignment operator
	 *  @param[in] other PixelPacket to assign
	 *  @returns reference to self
	 */
	PixelPacket &operator= (const PixelPacket &other);

	/** @brief Copy constructor
	 *  @param[in] other PixelPacket to copy
	 */
	PixelPacket (PixelPacket &&other);

	/** @brief Assignment operator
	 *  @param[in] other PixelPacket to assign
	 *  @returns reference to self
	 */
	PixelPacket &operator= (PixelPacket &&other);

	/** @brief Index operator
	 *  @param[in] index Pixel index
	 *  @returns pixel reference at index
	 */
	Reference operator[] (size_t index) const;

	/** @brief Dereference operator
	 *  @returns pixel reference at index 0
	 */
	Reference operator* () const;

	/** @brief Addition operator
	 *  @param[in] index Pixel index
	 *  @returns PixelPacket at index
	 */
	PixelPacket operator+ (size_t index);

	/** @brief Prefix increment operator
	 *  @returns reference to self after pointing to next pixel
	 */
	PixelPacket &operator++ ();

	/** @brief Postfix increment operator
	 *  @returns next PixelPacket
	 */
	PixelPacket operator++ (int);
};

/** @brief Emulator for Magick::Pixels */
class Pixels
{
private:
	Magick::Image &img;   ///< Image
	Magick::Pixels cache; ///< Pixel cache
	const ssize_t red;    ///< Red index
	const ssize_t green;  ///< Green index
	const ssize_t blue;   ///< Blue index
	const ssize_t alpha;  ///< Alpha index
	const size_t stride;  ///< Pixel stride

	friend class PixelPacket;
	friend class PixelPacket::Reference;

public:
	/** @brief Constructor
	 *  @param[in] img Image to cache
	 */
	Pixels (Magick::Image &img);

	/** @brief Get PixelPacket that represents the given portion of the image
	 *  @param[in] x X coordinate
	 *  @param[in] y Y coordinate
	 *  @param[in] w Width
	 *  @param[in] h Height
	 *  @returns PixelPacket
	 */
	PixelPacket get (ssize_t x, ssize_t y, size_t w, size_t h);

	/** @brief Flush cache to image */
	void sync ();
};

namespace
{
/** @brief Swap pixel data
 *  @param[in] p1 First pixel
 *  @param[in] p2 Second pixel
 */
inline void swapPixel (PixelPacket::Reference p1, PixelPacket::Reference p2)
{
	Magick::Color tmp = p1;
	p1                = p2;
	p2                = tmp;
}

/** @brief Get transparent color
 *  @returns transparent Magick::Color
 */
inline Magick::Color transparent ()
{
	// transparent has an 'alpha' value of 0
	return Magick::Color (0, 0, 0, 0);
}

/** @brief Check if image has all three RGB channels
 *  @returns whether image has all three RGB channels
 */
inline bool has_rgb (Magick::Image &img)
{
	if (img.hasChannel (Magick::RedPixelChannel) && img.hasChannel (Magick::GreenPixelChannel) &&
	    img.hasChannel (Magick::BluePixelChannel))
		return true;

	return false;
}
}
#else /* MagickLibVersion < 0x700 */
typedef Magick::FilterTypes FilterType;
typedef Magick::Pixels Pixels;
typedef Magick::PixelPacket *PixelPacket;

namespace
{
inline Magick::Quantum quantumRed (const Magick::Color &c)
{
	return c.redQuantum ();
}

inline void quantumRed (Magick::Color &c, Magick::Quantum v)
{
	c.redQuantum (v);
}

inline Magick::Quantum quantumGreen (const Magick::Color &c)
{
	return c.greenQuantum ();
}

inline void quantumGreen (Magick::Color &c, Magick::Quantum v)
{
	c.greenQuantum (v);
}

inline Magick::Quantum quantumBlue (const Magick::Color &c)
{
	return c.blueQuantum ();
}

inline void quantumBlue (Magick::Color &c, Magick::Quantum v)
{
	c.blueQuantum (v);
}

inline Magick::Quantum quantumAlpha (const Magick::Color &c)
{
	// get alpha instead of opacity
	using Magick::Quantum;
	return QuantumRange - c.alphaQuantum ();
}

inline void quantumAlpha (Magick::Color &c, Magick::Quantum v)
{
	// set alpha instead of opacity
	using Magick::Quantum;
	c.alphaQuantum (QuantumRange - v);
}

inline void swapPixel (Magick::PixelPacket &p1, Magick::PixelPacket &p2)
{
	std::swap (p1, p2);
}

inline Magick::Color transparent ()
{
	// transparent has an 'alpha' value of QuantumRange
	using Magick::Quantum;
	return Magick::Color (0, 0, 0, QuantumRange);
}

inline bool has_rgb (Magick::Image &img)
{
	return true;
}
}
#endif
