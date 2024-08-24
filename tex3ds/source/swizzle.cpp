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
/** @file swizzle.cpp
 *  @brief Swizzle routines
 */

#include "swizzle.h"
#include "magick_compat.h"

namespace
{
/** @brief Swizzle an 8x8 tile (Morton order)
 *  @param[in] p       Tile to swizzle
 *  @param[in] reverse Whether to unswizzle
 */
void swizzle (PixelPacket p, bool reverse)
{
	// swizzle foursome table
	static const unsigned char table[][4] = {
	    /* clang-format off */
		{  2,  8, 16,  4, },
		{  3,  9, 17,  5, },
		{  6, 10, 24, 20, },
		{  7, 11, 25, 21, },
		{ 14, 26, 28, 22, },
		{ 15, 27, 29, 23, },
		{ 34, 40, 48, 36, },
		{ 35, 41, 49, 37, },
		{ 38, 42, 56, 52, },
		{ 39, 43, 57, 53, },
		{ 46, 58, 60, 54, },
		{ 47, 59, 61, 55, },
	    /* clang-format on */
	};

	if (!reverse)
	{
		// swizzle each foursome
		for (const auto &entry : table)
		{
			Magick::Color tmp = p[entry[0]];
			p[entry[0]]       = p[entry[1]];
			p[entry[1]]       = p[entry[2]];
			p[entry[2]]       = p[entry[3]];
			p[entry[3]]       = tmp;
		}
	}
	else
	{
		// unswizzle each foursome
		for (const auto &entry : table)
		{
			Magick::Color tmp = p[entry[3]];
			p[entry[3]]       = p[entry[2]];
			p[entry[2]]       = p[entry[1]];
			p[entry[1]]       = p[entry[0]];
			p[entry[0]]       = tmp;
		}
	}

	// (un)swizzle each pair
	swapPixel (p[12], p[18]);
	swapPixel (p[13], p[19]);
	swapPixel (p[44], p[50]);
	swapPixel (p[45], p[51]);
}
}

/** @brief Swizzle an image (Morton order)
 *  @param[in] img     Image to swizzle
 *  @param[in] reverse Whether to unswizzle
 */
void swizzle (Magick::Image &img, bool reverse)
{
	Pixels cache (img);
	size_t height = img.rows ();
	size_t width  = img.columns ();

	// (un)swizzle each tile
	for (size_t j = 0; j < height; j += 8)
	{
		for (size_t i = 0; i < width; i += 8)
		{
			PixelPacket p = cache.get (i, j, 8, 8);
			swizzle (p, reverse);
			cache.sync ();
		}
	}
}
