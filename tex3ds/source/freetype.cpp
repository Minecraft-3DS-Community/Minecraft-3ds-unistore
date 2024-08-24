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
/** @file freetype.cpp
 *  @brief FreeType wrappers
 */

#include "freetype.h"
using namespace freetype;

///////////////////////////////////////////////////////////////////////////
Library::~Library ()
{
	if (m_library)
		FT_Done_FreeType (m_library);
}

Library::Library () : m_library ()
{
}

std::shared_ptr<Library> Library::makeLibrary ()
{
	auto library = std::shared_ptr<Library> ();
	library.reset (new Library ());

	FT_Error error = FT_Init_FreeType (&library->m_library);
	if (error)
	{
		std::fprintf (stderr, "FT_Init_FreeType: %s\n", freetype::strerror (error));
		return nullptr;
	}

	return library;
}

FT_Library Library::library () const
{
	return m_library;
}

std::unique_lock<std::mutex> Library::lock ()
{
	return std::unique_lock<std::mutex> (m_mutex);
}

///////////////////////////////////////////////////////////////////////////
Face::~Face ()
{
	for (auto &pair : m_face)
	{
		auto &face = pair.second;
		if (face)
			FT_Done_Face (face);
	}
}

Face::Face (const std::string &path, double ptSize)
    : m_path (path), m_ptSize (ptSize), m_library (), m_face ()
{
}

std::shared_ptr<Face>
    Face::makeFace (std::shared_ptr<Library> library, const std::string &path, double ptSize)
{
	auto face = std::shared_ptr<Face> ();
	face.reset (new Face (path, ptSize));

	face->m_library = library;
	if (!face->getFace ())
		return nullptr;

	return face;
}

FT_Face Face::getFace ()
{
	auto id = std::this_thread::get_id ();
	std::unique_lock<std::mutex> lock (m_mutex);
	auto &face = m_face[id];
	lock.unlock ();

	if (face)
		return face;

	FT_Error error;
	{
		auto libraryLock = m_library->lock ();
		error            = FT_New_Face (m_library->library (), m_path.c_str (), 0, &face);
	}

	if (error)
	{
		std::fprintf (stderr, "FT_New_Face: %s\n", freetype::strerror (error));
		return nullptr;
	}

	error = FT_Select_Charmap (face, FT_ENCODING_UNICODE);
	if (error)
	{
		std::fprintf (stderr, "FT_Select_Charmap: %s\n", freetype::strerror (error));
		face = nullptr;
		return nullptr;
	}

	error = FT_Set_Char_Size (face, m_ptSize * (1 << 6), 0, 96, 0);
	if (error)
	{
		std::fprintf (stderr, "FT_Set_Char_Size: %s\n", freetype::strerror (error));
		face = nullptr;
		return nullptr;
	}

	return face;
}

///////////////////////////////////////////////////////////////////////////
const char *freetype::strerror (FT_Error error)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s)                                                                       \
	case e:                                                                                        \
		return s;
#define FT_ERROR_START_LIST                                                                        \
	switch (error)                                                                                 \
	{
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H

	return "(Unknown error)";
}
