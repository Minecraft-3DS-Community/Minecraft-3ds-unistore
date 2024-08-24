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
/** @file atlas.h
 *  @brief Atlas interface.
 */
#pragma once

#include "magick_compat.h"
#include "subimage.h"

#include <string>
#include <vector>

struct Atlas
{
	Magick::Image img;
	std::vector<SubImage> subs;

	Atlas ()
	{
	}

	Atlas (const Atlas &other) = delete;
	Atlas (Atlas &&other)      = default;
	Atlas &operator= (const Atlas &other) = delete;
	Atlas &operator= (Atlas &&other) = delete;

	static Atlas build (const std::vector<std::string> &paths, bool trim, bool border);
};
