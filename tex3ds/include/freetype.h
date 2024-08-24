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
/** @file freetype.h
 *  @brief FreeType wrappers
 */
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace freetype
{
class Library : public std::enable_shared_from_this<Library>
{
public:
	~Library ();

	static std::shared_ptr<Library> makeLibrary ();

	FT_Library library () const;

	std::unique_lock<std::mutex> lock ();

private:
	Library ();

	std::mutex m_mutex;
	FT_Library m_library;
};

class Face : public std::enable_shared_from_this<Face>
{
public:
	~Face ();

	static std::shared_ptr<Face>
	    makeFace (std::shared_ptr<Library> library, const std::string &path, double ptSize);

	FT_Face getFace ();

private:
	Face (const std::string &path, double ptSize);

	const std::string m_path;
	const double m_ptSize;

	std::shared_ptr<Library> m_library;

	std::mutex m_mutex;
	std::map<std::thread::id, FT_Face> m_face;
};

const char *strerror (FT_Error error);
}
