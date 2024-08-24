/*------------------------------------------------------------------------------
 * Copyright (c) 2019
 *     Michael Theall (mtheall)
 *     piepie62
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
/** @file bcfnt.h
 *  @brief BCFNT definitions
 */
#pragma once

#include "freetype.h"
#include "magick_compat.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace bcfnt
{
/** @brief Character width information. */
struct CharWidthInfo
{
	std::int8_t left;        ///< Horizontal offset to draw the glyph with.
	std::uint8_t glyphWidth; ///< Width of the glyph.
	std::uint8_t charWidth;  ///< Width of the character, that is, horizontal distance to advance.
};

class CMAPData
{
public:
	/** @brief Font character map methods. */
	enum Type : std::uint8_t
	{
		CMAP_TYPE_DIRECT = 0, ///< Identity mapping.
		CMAP_TYPE_TABLE  = 1, ///< Mapping using a table.
		CMAP_TYPE_SCAN   = 2, ///< Mapping using a list of mapped characters.
	};

	virtual Type type () const = 0;

	virtual ~CMAPData() = default;
};

class CMAPDirect : public CMAPData
{
public:
	CMAPDirect (std::uint16_t offset) : offset (offset)
	{
	}

	Type type () const override
	{
		return CMAP_TYPE_DIRECT;
	}

	const std::uint16_t offset;
};

class CMAPTable : public CMAPData
{
public:
	Type type () const override
	{
		return CMAP_TYPE_TABLE;
	}

	std::vector<std::uint16_t> table;
};

class CMAPScan : public CMAPData
{
public:
	Type type () const override
	{
		return CMAP_TYPE_SCAN;
	}

	std::map<std::uint16_t, std::uint16_t> entries;
};

/** @brief Font character map structure. */
struct CMAP
{
	std::uint16_t codeBegin;     ///< First Unicode codepoint the block applies to.
	std::uint16_t codeEnd;       ///< Last Unicode codepoint the block applies to.
	std::uint16_t mappingMethod; ///< Mapping method.
	std::uint16_t reserved;
	std::uint32_t next; ///< Pointer to the next map.

	std::unique_ptr<CMAPData> data; ///< Character map data.

	std::uint16_t codePointFromIndex (
	    std::uint16_t index) const; ///< Gets the codepoint that corresponds to the current index,
	                                ///< or 0xFFFF if it's not valid
};

struct Glyph
{
	Magick::Image img;
	CharWidthInfo info;
	int ascent;
};

class BCFNT
{
public:
	BCFNT ()
	{
	}

	BCFNT (const std::vector<std::uint8_t> &data);

	bool serialize (const std::string &path);

	void addFont (std::shared_ptr<freetype::Face> face,
	    std::vector<std::uint16_t> &list,
	    bool isBlacklist);
	void addFont (BCFNT &font, std::vector<std::uint16_t> &list, bool isBlacklist);

private:
	void readGlyphImages (std::vector<std::uint8_t>::const_iterator &bcfnt, int sheetNum);
	std::vector<Magick::Image> sheetify ();
	std::uint16_t codepoint (std::uint16_t index) const;
	void refreshCMAPs ();

	std::vector<CMAP> cmaps;
	// character code and image
	std::map<std::uint16_t, Glyph> glyphs;

	std::uint16_t numSheets = 0;
	std::uint16_t altIndex  = 0;
	CharWidthInfo defaultWidth;
	std::uint8_t lineFeed = 0;
	std::uint8_t height   = 0;
	std::uint8_t width    = 0;
	std::uint8_t maxWidth = 0;
	std::uint8_t ascent   = 0;

	std::uint8_t cellWidth  = 0;
	std::uint8_t cellHeight = 0;

	std::uint16_t SHEET_WIDTH  = 1024;
	std::uint16_t SHEET_HEIGHT = 1024;
	std::uint32_t SHEET_SIZE   = SHEET_WIDTH * SHEET_HEIGHT / 2;

	std::uint16_t glyphWidth     = 0;
	std::uint16_t glyphHeight    = 0;
	std::uint16_t glyphsPerRow   = 0;
	std::uint16_t glyphsPerCol   = 0;
	std::uint16_t glyphsPerSheet = 0;
};
}
