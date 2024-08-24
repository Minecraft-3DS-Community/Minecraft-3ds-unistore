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
/** @file atlas.cpp
 *  @brief Atlas interface.
 */

#include "atlas.h"
#include "subimage.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

namespace
{
inline size_t calcPOT (double x)
{
	if (x < 8)
		return 8;

	return std::pow (2.0, std::ceil (std::log2 (x)));
}

typedef std::pair<size_t, size_t> XY;

struct Block
{
	size_t index;
	Magick::Image img;
	XY xy;
	size_t x, y, w, h;
	bool flipped;

	Block ()                   = delete;
	Block (const Block &other) = default;
	Block (Block &&other)      = default;
	Block &operator= (const Block &other) = delete;
	Block &operator= (Block &&other) = delete;

	Block (size_t index,
	    const Magick::Image &img,
	    size_t x,
	    size_t y,
	    size_t w,
	    size_t h,
	    bool flipped)
	    : index (index), img (img), x (x), y (y), w (w), h (h), flipped (false)
	{
	}

	Block (size_t index, const Magick::Image &img)
	    : index (index),
	      img (img),
	      x (0),
	      y (0),
	      w (img.columns ()),
	      h (img.rows ()),
	      flipped (false)
	{
	}

	SubImage subImage (const Magick::Image &atlas) const
	{
		float left   = static_cast<float> (x) / atlas.columns ();
		float top    = 1.0f - (static_cast<float> (y) / atlas.rows ());
		float right  = static_cast<float> (x + w) / atlas.columns ();
		float bottom = 1.0f - (static_cast<float> (y + h) / atlas.rows ());

		if (img.columns () == w && img.rows () == h)
			return SubImage (index, img.fileName (), left, top, right, bottom);

		// rotated
		return SubImage (index, img.fileName (), bottom, left, top, right);
	}

	bool operator< (const Block &other) const
	{
		if (x != other.x)
			return x < other.x;
		return y < other.y;
	}

	bool operator< (const XY &xy) const
	{
		if (x != xy.first)
			return x < xy.first;
		return y < xy.second;
	}
};

struct Packer
{
	std::set<Block> placed;
	std::vector<Block> next;
	std::set<XY> free;

	size_t width, height;
	bool border;

	Packer ()                    = delete;
	Packer (const Packer &other) = delete;
	Packer (Packer &&other)      = default;
	Packer &operator= (const Packer &other) = delete;
	Packer &operator= (Packer &&other) = default;

	Packer (const std::vector<Magick::Image> &images, size_t width, size_t height, bool border);

	Magick::Image composite () const
	{
		Magick::Image img (Magick::Geometry (width, height), transparent ());

		for (const auto &block : placed)
		{
			if (!block.flipped)
				img.composite (
				    block.img, Magick::Geometry (0, 0, block.x, block.y), Magick::OverCompositeOp);
			else
			{
				Magick::Image copy = block.img;
				copy.rotate (-90);
				img.composite (
				    copy, Magick::Geometry (0, 0, block.x, block.y), Magick::OverCompositeOp);
			}
		}

		return img;
	}

	void draw () const
	{
		Magick::Image img = composite ();
	}

	void pack (size_t &x, size_t &y, size_t w, size_t h);
	size_t calc_score (size_t x, size_t y, size_t w, size_t h);
	bool solve ();

	bool intersects_placed (size_t x, size_t y) const
	{
		for (const auto &block : placed)
		{
			if (x >= block.x && x < block.x + block.w && y >= block.y && y < block.y + block.h)
				return true;
		}

		return false;
	}

	bool intersects_placed (const XY &xy) const
	{
		return intersects_placed (xy.first, xy.second);
	}

	void add_free (size_t x, size_t y, bool vert)
	{
		if (x >= width || y >= height)
			return;

		if (intersects_placed (x, y))
			return;

		free.insert (XY (x, y));
	}

	void fixup ()
	{
		auto it = std::begin (free);
		while (it != std::end (free))
		{
			if (intersects_placed (*it))
				it = free.erase (it);
			else
				++it;
		}
	}
};

Packer::Packer (const std::vector<Magick::Image> &images, size_t width, size_t height, bool border)
    : placed (), next (), free (), width (width), height (height), border (border)
{
	for (const auto &img : images)
		next.push_back (Block (std::stoul (img.attribute ("index")), img));

	free.insert (XY (0, 0));
}

bool Packer::solve ()
{
	while (!next.empty ())
	{
		Block block = next.back ();
		next.pop_back ();

		XY best;
		size_t best_score = 0;
		bool best_flipped = false;
		for (const auto &it : free)
		{
			block.x = it.first;
			block.y = it.second;

			size_t score = calc_score (it.first, it.second, block.w, block.h);
			if (score > best_score)
			{
				best         = it;
				best_score   = score;
				best_flipped = false;
			}

			if (block.w != block.h)
			{
				size_t score = calc_score (it.first, it.second, block.h, block.w);
				if (score > best_score)
				{
					best         = it;
					best_score   = score;
					best_flipped = true;
				}
			}
		}

		if (best_score == 0)
			return false;

		block.x = best.first;
		block.y = best.second;
		if (best_flipped)
		{
			std::swap (block.w, block.h);
			block.flipped = true;
		}

		if (border)
		{
			block.w++;
			block.h++;
		}

		pack (block.x, block.y, block.w, block.h);
		placed.insert (block);
		free.erase (best);

		add_free (block.x + block.w, block.y, true);
		add_free (block.x, block.y + block.h, false);

		fixup ();

		draw ();
	}

	return true;
}

void Packer::pack (size_t &x, size_t &y, size_t w, size_t h)
{
	bool intersects_left = (x == 0) || intersects_placed (x - 1, y);
	bool intersects_up   = (y == 0) || intersects_placed (x, y - 1);

	if (!intersects_left && !intersects_up)
		std::abort (); // should be adjacent to a placed block

	if (intersects_left && intersects_up)
		return;

	if (intersects_left)
	{
		// move up as far as possible
		--y;
		while (y > 0 && !intersects_placed (x, y - 1))
			--y;
	}
	else
	{
		// move left as far as possible
		--x;
		while (x > 0 && !intersects_placed (x - 1, y))
			--x;
	}
}

size_t Packer::calc_score (size_t x, size_t y, size_t w, size_t h)
{
	size_t score = 0;

	pack (x, y, w, h);

	if (x + w > width)
		return 0;
	if (y + h > height)
		return 0;

	for (const auto &block : placed)
	{
		if (x + w < block.x)
			break;

		if (x < block.x + block.w && x + w > block.x && y < block.y + block.h && y + h > block.y)
			return 0;

		if (x == block.x + block.w || x + w == block.x)
		{
			size_t start = std::max (y, block.y);
			size_t end   = std::min (y + h, block.y + block.h);
			if (end > start)
				score += end - start;
		}

		if (y == block.y + block.h || y + h == block.y)
		{
			size_t start = std::max (x, block.x);
			size_t end   = std::min (x + w, block.x + block.w);
			if (end > start)
				score += end - start;
		}
	}

	if (x == 0)
		score += h;
	if (x + w == width)
		score += h;

	if (y == 0)
		score += w;
	if (y + h == height)
		score += w;

	return score;
}

struct AreaSizeComparator
{
	bool compare (size_t w1, size_t h1, size_t w2, size_t h2) const
	{
		size_t area1 = w1 * h1;
		size_t area2 = w2 * h2;

		if (area1 != area2)
			return area1 < area2;

		if (std::max (w1, h1) == std::max (w2, h2))
			return w1 < w2;

		return std::max (w1, h1) < std::max (w2, h2);
	}

	bool operator() (const Magick::Image &lhs, const Magick::Image &rhs) const
	{
		return compare (lhs.columns (), lhs.rows (), rhs.columns (), rhs.rows ());
	}

	bool operator() (const Packer &lhs, const Packer &rhs) const
	{
		return compare (lhs.width, lhs.height, rhs.width, rhs.height);
	}
};
}

Atlas Atlas::build (const std::vector<std::string> &paths, bool trim, bool border)
{
	std::vector<Magick::Image> images;

	size_t i = 0;
	for (const auto &path : paths)
	{
		Magick::Image img (path);

		if (trim)
		{
			img.trim ();
			img.page (Magick::Geometry (img.columns (), img.rows ()));
		}

		img.attribute ("index", std::to_string (i++));
		images.push_back (img);
	}

	std::sort (std::begin (images), std::end (images), AreaSizeComparator ());

	size_t totalArea = 0;
	for (const auto &img : images)
		totalArea += img.rows () * img.columns ();

	std::vector<Packer> packers;
	for (size_t h = calcPOT (std::min (images.back ().columns (), images.back ().rows ()));
	     h <= 1024;
	     h *= 2)
	{
		for (size_t w = calcPOT (std::min (images.back ().columns (), images.back ().rows ()));
		     w <= 1024;
		     w *= 2)
		{
			size_t allowed_height = h;
			size_t allowed_width  = w;
			if (border)
			{
				allowed_height -= 2;
				allowed_width -= 2;
			}
			if (allowed_width * allowed_height >= totalArea)
				packers.push_back (Packer (images, allowed_width, allowed_height, border));
		}
	}

	std::sort (std::begin (packers), std::end (packers), AreaSizeComparator ());

	for (auto &packer : packers)
	{
		if (packer.solve ())
		{
			Atlas atlas;

			atlas.img = packer.composite ();
			for (auto &block : packer.placed)
				atlas.subs.push_back (block.subImage (atlas.img));

			std::sort (std::begin (atlas.subs), std::end (atlas.subs));
			return atlas;
		}
	}

	throw std::runtime_error ("No atlas solution found.\n");
}
