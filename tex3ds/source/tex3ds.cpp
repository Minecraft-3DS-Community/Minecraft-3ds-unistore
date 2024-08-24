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
/** @file tex3ds.cpp
 *  @brief tex3ds program entry point
 */

#include "atlas.h"
#include "compress.h"
#include "encode.h"
#include "magick_compat.h"
#include "quantum.h"
#include "rg_etc1.h"
#include "subimage.h"
#include "swizzle.h"

#include <getopt.h>
#include <libgen.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
/** @brief Get power-of-2 ceiling
 *  @param[in] x Value to calculate
 *  @returns Power-of-2 ceiling
 */
inline size_t potCeil (size_t x)
{
	if (x < 8)
		return 8;

	return std::pow (2.0, std::ceil (std::log2 (x)));
}

/** @brief Case-insensitive string comparator */
template <typename T>
struct CaseInsensitiveComparator
{
	bool operator() (const std::pair<const char *, T> &lhs, const char *rhs) const
	{
		return strcasecmp (lhs.first, rhs) < 0;
	}
};

/** @brief Process format */
enum ProcessFormat
{
	RGBA8888 = 0x00, ///< RGBA8888 encoding
	RGB888   = 0x01, ///< RGB888 encoding
	RGBA5551 = 0x02, ///< RGBA5551 encoding
	RGB565   = 0x03, ///< RGB565 encoding
	RGBA4444 = 0x04, ///< RGBA4444 encoding
	LA88     = 0x05, ///< LA88 encoding
	HILO88   = 0x06, ///< HILO88 encoding
	L8       = 0x07, ///< L8 encoding
	A8       = 0x08, ///< A8 encoding
	LA44     = 0x09, ///< LA44 encoding
	L4       = 0x0A, ///< L4 encoding
	A4       = 0x0B, ///< A4 encoding
	ETC1     = 0x0C, ///< ETC1 encoding
	ETC1A4   = 0x0D, ///< ETC1A4 encoding
	AUTO_L8,         ///< L8/LA88 encoding
	AUTO_L4,         ///< L4/LA44 encoding
	AUTO_ETC1,       ///< ETC1/ETC1A4 encoding
};

typedef std::pair<const char *, ProcessFormat> ProcessFormatMap;
typedef CaseInsensitiveComparator<ProcessFormat> ProcessFormatComparator;

/** @brief Process format strings */
const ProcessFormatMap output_format_strings[] = {
    /* clang-format off */
	{ "a",         A8,        },
	{ "a4",        A4,        },
	{ "a8",        A8,        },
	{ "auto-etc1", AUTO_ETC1, },
	{ "auto-l4",   AUTO_L4,   },
	{ "auto-l8",   AUTO_L8,   },
	{ "etc1",      ETC1,      },
	{ "etc1a4",    ETC1A4,    },
	{ "hilo",      HILO88,    },
	{ "hilo8",     HILO88,    },
	{ "hilo88",    HILO88,    },
	{ "l",         L8,        },
	{ "l4",        L4,        },
	{ "l8",        L8,        },
	{ "la",        LA88,      },
	{ "la4",       LA44,      },
	{ "la44",      LA44,      },
	{ "la8",       LA88,      },
	{ "la88",      LA88,      },
	{ "rgb",       RGB888,    },
	{ "rgb565",    RGB565,    },
	{ "rgb8",      RGB888,    },
	{ "rgb888",    RGB888,    },
	{ "rgba",      RGBA8888,  },
	{ "rgba4",     RGBA4444,  },
	{ "rgba4444",  RGBA4444,  },
	{ "rgba5551",  RGBA5551,  },
	{ "rgba8",     RGBA8888,  },
	{ "rgba8888",  RGBA8888,  },
    /* clang-format on */
};

/** @brief Compression format */
enum CompressionFormat
{
	COMPRESSION_NONE, ///< No compression
	COMPRESSION_LZ10, ///< LZSS/LZ10 compression
	COMPRESSION_LZ11, ///< LZ11 compression
	COMPRESSION_RLE,  ///< Run-length encoding compression
	COMPRESSION_HUFF, ///< Huffman encoding
	COMPRESSION_AUTO, ///< Choose best compression
};

typedef std::pair<const char *, CompressionFormat> CompressionFormatMap;
typedef CaseInsensitiveComparator<CompressionFormat> CompressionFormatComparator;

/** @brief Compression format strings */
const CompressionFormatMap compression_format_strings[] = {
    /* clang-format off */
	{ "auto",     COMPRESSION_AUTO, },
	{ "huff",     COMPRESSION_HUFF, },
	{ "huffman",  COMPRESSION_HUFF, },
	{ "lz10",     COMPRESSION_LZ10, },
	{ "lz11",     COMPRESSION_LZ11, },
	{ "lzss",     COMPRESSION_LZ10, },
	{ "none",     COMPRESSION_NONE, },
	{ "rle",      COMPRESSION_RLE,  },
    /* clang-format on */
};

typedef std::pair<const char *, FilterType> FilterTypeMap;
typedef CaseInsensitiveComparator<FilterType> FilterTypeComparator;

/** @brief Filter type strings */
const FilterTypeMap filter_type_strings[] = {
    /* clang-format off */
	{ "bartlett",       Magick::BartlettFilter,      },
	{ "bessel",         Magick::BesselFilter,        },
	{ "blackman",       Magick::BlackmanFilter,      },
	{ "bohman",         Magick::BohmanFilter,        },
	{ "box",            Magick::BoxFilter,           },
	{ "catrom",         Magick::CatromFilter,        },
	{ "cosine",         Magick::CosineFilter,        },
	{ "cubic",          Magick::CubicFilter,         },
	{ "gaussian",       Magick::GaussianFilter,      },
	{ "hamming",        Magick::HammingFilter,       },
	{ "hanning",        Magick::HanningFilter,       },
	{ "hermite",        Magick::HermiteFilter,       },
	{ "jinc",           Magick::JincFilter,          },
	{ "kaiser",         Magick::KaiserFilter,        },
	{ "lagrange",       Magick::LagrangeFilter,      },
	{ "lanczos",        Magick::LanczosFilter,       },
	{ "lanczos-radius", Magick::LanczosRadiusFilter, },
	{ "lanczos-sharp",  Magick::LanczosSharpFilter,  },
	{ "lanczos2",       Magick::Lanczos2Filter,      },
	{ "lanczos2-sharp", Magick::Lanczos2SharpFilter, },
	{ "mitchell",       Magick::MitchellFilter,      },
	{ "parzen",         Magick::ParzenFilter,        },
	{ "point",          Magick::PointFilter,         },
	{ "quadratic",      Magick::QuadraticFilter,     },
	{ "robidoux",       Magick::RobidouxFilter,      },
	{ "robidoux-sharp", Magick::RobidouxSharpFilter, },
	{ "sinc",           Magick::SincFilter,          },
	{ "spline",         Magick::SplineFilter,        },
	{ "triangle",       Magick::TriangleFilter,      },
	{ "welsh",          Magick::WelshFilter,         },
    /* clang-format on */
};

/** @brief Processing mode */
enum ProcessingMode
{
	PROCESS_NORMAL,  ///< Normal
	PROCESS_ATLAS,   ///< Atlas
	PROCESS_CUBEMAP, ///< Cubemap
	PROCESS_SKYBOX,  ///< Skybox
};

/** @brief Include stack */
std::vector<std::string> include_stack (1);

/** @brief Dependency path option */
std::string depends_path;

/** @brief Dependencies list */
std::set<std::string> dependencies;

/** @brief Header path option */
std::string header_path;

/** @brief Output path option */
std::string output_path;

/** @brief Preview path option */
std::string preview_path;

/** @brief Process format option */
ProcessFormat process_format = RGBA8888;

/** @brief ETC1 quality option */
rg_etc1::etc1_quality etc1_quality = rg_etc1::cMediumQuality;

/** @brief Compression format option */
CompressionFormat compression_format = COMPRESSION_AUTO;

/** @brief Mipmap filter type option */
FilterType filter_type = Magick::UndefinedFilter;

/** @brief Processing mode option */
ProcessingMode process_mode = PROCESS_NORMAL;

/** @brief Trim input images */
bool trim = false;

/** @brief Output subimage data */
std::vector<SubImage> subimage_data;

/** @brief Output image data */
encode::Buffer image_data;

/** @brief Output width */
size_t output_width;

/** @brief Output raw image data */
bool output_raw = false;

/** @brief Output height */
size_t output_height;

/** @brief Load image
 *  @param[in] img Input image
 *  @returns vector of images to process
 */
std::vector<Magick::Image> load_image (Magick::Image &img)
{
	// check for RGB colorspace
	switch (img.colorSpace ())
	{
	case Magick::RGBColorspace:
	case Magick::sRGBColorspace:
		break;

	default:
		// convert to RGB colorspace
		img.colorSpace (Magick::RGBColorspace);
		break;
	}

	// double-check RGB channels
	if (!has_rgb (img))
		throw std::runtime_error ("No RGB information");

	double width  = img.columns ();
	double height = img.rows ();

	// get sub-image size for cubemap/skybox
	if (process_mode == PROCESS_CUBEMAP || process_mode == PROCESS_SKYBOX)
	{
		width /= 4.0;
		height /= 3.0;

		// check that sub-image width is integral
		if (width != static_cast<size_t> (width))
			throw std::runtime_error ("Invalid width");

		// check that sub-image height is integral
		if (height != static_cast<size_t> (height))
			throw std::runtime_error ("Invalid height");

		// check for correct texture width
		switch (static_cast<size_t> (width))
		{
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
		case 256:
		case 512:
		case 1024:
			break;

		default:
			throw std::runtime_error ("Invalid width");
		}

		// check for correct texture height
		switch (static_cast<size_t> (height))
		{
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
		case 256:
		case 512:
		case 1024:
			break;

		default:
			throw std::runtime_error ("Invalid height");
		}
	}
	else
	{
		// check for valid width
		if (width > 1024)
			throw std::runtime_error ("Invalid height");

		// check for valid height
		if (height > 1024)
			throw std::runtime_error ("Invalid width");
	}

	// Set page offsets to 0
	img.page (Magick::Geometry (img.columns (), img.rows ()));

	std::vector<Magick::Image> result;
	if (process_mode == PROCESS_NORMAL || process_mode == PROCESS_ATLAS)
	{
		// expand canvas if necessary
		if (img.columns () != potCeil (img.columns ()) || img.rows () != potCeil (img.rows ()))
		{
			Magick::Image copy = img;

			img = Magick::Image (
			    Magick::Geometry (potCeil (img.columns ()), potCeil (img.rows ())), transparent ());
			img.composite (copy, Magick::Geometry (0, 0), Magick::OverCompositeOp);

			// generate subimage info
			subimage_data.push_back (SubImage (0,
			    "",
			    0.0f,
			    1.0f,
			    static_cast<float> (copy.columns ()) / img.columns (),
			    1.0f - (static_cast<float> (copy.rows ()) / img.rows ())));
		}
		else if (process_mode != PROCESS_ATLAS)
		{
			subimage_data.push_back (SubImage (0, "", 0.0f, 1.0f, 1.0f, 0.0f));
		}

		output_width  = img.columns ();
		output_height = img.rows ();

		// push the source image
		result.push_back (img);
	}
	else
	{
		// extract the six faces from cubemap/skybox
		// PICA 200 cubemapping inverts texture vertical axis
		Magick::Image copy;
		output_width  = width;
		output_height = height;

		// +x
		copy = img;
		copy.crop (Magick::Geometry (width, height, 2 * width, height));
		if (process_mode == PROCESS_SKYBOX)
			copy.flop (); // flip horizontal
		copy.flip ();     // flip vertical
		copy.comment ("px_");
		result.push_back (copy);

		// -x
		copy = img;
		copy.crop (Magick::Geometry (width, height, 0, height));
		if (process_mode == PROCESS_SKYBOX)
			copy.flop (); // flip horizontal
		copy.flip ();     // flip vertical
		copy.comment ("nx_");
		result.push_back (copy);

		// +y
		copy = img;
		copy.crop (Magick::Geometry (width, height, width, 0));
		if (process_mode == PROCESS_CUBEMAP)
			copy.flip (); // flip vertical
		copy.comment ("py_");
		result.push_back (copy);

		// -y
		copy = img;
		copy.crop (Magick::Geometry (width, height, width, height * 2));
		if (process_mode == PROCESS_CUBEMAP)
			copy.flip (); // flip vertical
		copy.comment ("ny_");
		result.push_back (copy);

		// +z
		copy = img;
		if (process_mode == PROCESS_CUBEMAP)
			copy.crop (Magick::Geometry (width, height, width, height));
		else
		{
			copy.crop (Magick::Geometry (width, height, width * 3, height));
			copy.flop (); // flip horizontal
		}
		copy.flip (); // flip vertical
		copy.comment ("pz_");
		result.push_back (copy);

		// -z
		copy = img;
		if (process_mode == PROCESS_CUBEMAP)
			copy.crop (Magick::Geometry (width, height, width * 3, height));
		else
		{
			copy.crop (Magick::Geometry (width, height, width, height));
			copy.flop (); // flip horizontal
		}
		copy.flip (); // flip vertical
		copy.comment ("nz_");
		result.push_back (copy);
	}

	return result;
}

/** @brief Check if an image has any transparency
 *  @param[in] img Image to check
 *  @returns whether image has any transparency
 */
template <int bits>
bool has_alpha (Magick::Image &img)
{
	Pixels cache (img);
	PixelPacket p = cache.get (0, 0, img.columns (), img.rows ());

	size_t num = img.rows () * img.columns ();

	// check all pixels
	for (size_t i = 0; i < num; ++i)
	{
		Magick::Color c = *p++;

		// if the quantized pixel is not fully opaque, return true
		if (quantum_to_bits<bits> (quantumAlpha (c)))
			return true;
	}

	// no (partially) transparent pixels found
	return false;
}

/** @brief Add prefix to a file name
 *  @param[in] path   Path to prefix
 *  @param[in] prefix Prefix to add
 *  @returns Path with prefixed file name
 */
std::string add_prefix (std::string path, std::string prefix)
{
	// look for the file name
	size_t pos = path.rfind ('/');

	// add prefix to file name
	if (pos != std::string::npos)
		return path.substr (0, pos + 1) + prefix + path.substr (pos + 1);
	return prefix + path;
}

/** @brief Finalize process format
 *  @param[in] images Input images
 */
void finalize_process_format (std::vector<Magick::Image> &images)
{
	// check each sub-image for transparency
	if (process_format == AUTO_L8 &&
	    std::any_of (std::begin (images), std::end (images), has_alpha<8>))
		process_format = LA88;
	else if (process_format == AUTO_L4 &&
	         std::any_of (std::begin (images), std::end (images), has_alpha<4>))
		process_format = LA44;
	else if (process_format == AUTO_ETC1 &&
	         std::any_of (std::begin (images), std::end (images), has_alpha<4>))
		process_format = ETC1A4;

	// check if no transparency was found
	if (process_format == AUTO_L8)
		process_format = L8;
	else if (process_format == AUTO_L4)
		process_format = L4;
	else if (process_format == AUTO_ETC1)
		process_format = ETC1;
}

/** @brief Work queue */
std::queue<encode::WorkUnit> work_queue;

/** @brief Result queue */
std::vector<encode::WorkUnit> result_queue;

/** @brief Work queue condition variable */
std::condition_variable work_cond;

/** @brief Result queue condition variable */
std::condition_variable result_cond;

/** @brief Work queue mutex */
std::mutex work_mutex;

/** @brief Result queue mutex */
std::mutex result_mutex;

/** @brief Whether anymore work is coming */
bool work_done = false;

/** @brief Work thread
 *  @param[in] param Unused
 */
void work_thread (void *param)
{
	std::unique_lock<std::mutex> mutex (work_mutex);
	while (true)
	{
		// wait for work
		while (!work_done && work_queue.empty ())
			work_cond.wait (mutex);

		// if there's no more work, quit
		if (work_done && work_queue.empty ())
			return;

		// get a work unit
		encode::WorkUnit work = std::move (work_queue.front ());
		work_queue.pop ();
		mutex.unlock ();

		// process the work unit
		work.process (work);

		// put result on the result queue
		result_mutex.lock ();
		result_queue.push_back (std::move (work));
		std::push_heap (result_queue.begin (), result_queue.end ());
		result_cond.notify_one ();
		result_mutex.unlock ();

		mutex.lock ();
	}
}

/** @brief Process image
 *  @param[in] img Image to process
 */
void process_image (Magick::Image &img)
{
	// get the image prefix
	const std::string prefix = img.comment ();

	void (*process) (encode::WorkUnit &) = nullptr;

	// get the processing routine
	switch (process_format)
	{
	case RGBA8888:
		process = encode::rgba8888;
		break;

	case RGB888:
		process = encode::rgb888;
		break;

	case RGBA5551:
		process = encode::rgba5551;
		break;

	case RGB565:
		process = encode::rgb565;
		break;

	case RGBA4444:
		process = encode::rgba4444;
		break;

	case LA88:
		process = encode::la88;
		break;

	case HILO88:
		process = encode::hilo88;
		break;

	case L8:
		process = encode::l8;
		break;

	case A8:
		process = encode::a8;
		break;

	case LA44:
		process = encode::la44;
		break;

	case L4:
		process = encode::l4;
		break;

	case A4:
		process = encode::a4;
		break;

	case ETC1:
		process = encode::etc1;
		break;

	case ETC1A4:
		process = encode::etc1a4;
		break;

	case AUTO_L8:
	case AUTO_L4:
	case AUTO_ETC1:
		// should have been changed with finalize_process_format()
		std::abort ();
		break;
	}

	// mipmap queue
	std::queue<Magick::Image> img_queue;

	// add base level
	img_queue.push (img);

	// keep preview width/height
	size_t preview_width  = img.columns ();
	size_t preview_height = img.rows ();

	// generate mipmaps
	if (filter_type != Magick::UndefinedFilter && preview_width > 8 && preview_height > 8)
	{
		size_t width  = preview_width;
		size_t height = preview_height;

		// mipmaps will go on the right third of the preview image
		preview_width *= 1.5;

		// mipmaps must have both dimensions >= 8
		while (width > 8 && height > 8)
		{
			// copy image
			img = img_queue.front ();

			// set resize filter type
			img.filterType (filter_type);

			// half each dimension
			width  = width / 2;
			height = height / 2;

			// resize the image
			img.resize (Magick::Geometry (width, height));

			// add to mipmap queue
			img_queue.push (img);
		}
	}

	// create the preview image
	Magick::Image preview (Magick::Geometry (preview_width, preview_height), transparent ());

	// create worker threads
	std::vector<std::thread> workers;
	work_mutex.lock ();
	work_done = false;
	work_mutex.unlock ();
	for (size_t i = 0; i < std::thread::hardware_concurrency (); ++i)
		workers.push_back (std::thread (work_thread, nullptr));

	size_t voff = 0; // vertical offset for mipmap preview
	size_t hoff = 0; // horizontal offset for mipmap preview

	// process each image in the mipmap queue
	while (!img_queue.empty ())
	{
		// get the first image in the queue
		img = img_queue.front ();
		img_queue.pop ();

		// get the mipmap dimensions
		size_t width  = img.columns ();
		size_t height = img.rows ();

		// all formats are swizzled except ETC1/ETC1A4
		if (process_format != ETC1 && process_format != ETC1A4)
			swizzle (img, false);

		// get pixel cache
		Pixels cache (img);
		PixelPacket p = cache.get (0, 0, img.columns (), img.rows ());

		// process each 8x8 tile
		uint64_t num_work = 0;
		for (size_t j = 0; j < height; j += 8)
		{
			for (size_t i = 0; i < width; i += 8)
			{
				// create the work unit
				encode::WorkUnit work (num_work++,
				    p + (j * width + i),
				    width,
				    etc1_quality,
				    !output_path.empty (),
				    !preview_path.empty (),
				    process);

				// queue the work unit
				work_mutex.lock ();
				work_queue.push (std::move (work));
				work_cond.notify_one ();
				work_mutex.unlock ();
			}
		}

		if (img_queue.empty ())
		{
			// no more work is coming
			work_mutex.lock ();
			work_done = true;
			work_cond.notify_all ();
			work_mutex.unlock ();
		}

		// gather results
		for (uint64_t num_result = 0; num_result < num_work; ++num_result)
		{
			// wait for the next result
			std::unique_lock<std::mutex> mutex (result_mutex);
			while (result_queue.empty () || result_queue.front ().sequence != num_result)
			{
				result_cond.wait (mutex);
			}

			// get the result's output buffer
			encode::Buffer result;
			std::pop_heap (result_queue.begin (), result_queue.end ());
			result.swap (result_queue.back ().result);
			result_queue.pop_back ();
			mutex.unlock ();

			// append the result's output buffer
			image_data.insert (image_data.end (), result.begin (), result.end ());

			mutex.lock ();
		}

		// synchronize the pixel cache
		cache.sync ();

		if (!preview_path.empty ())
		{
			// unswizzle the mipmap image
			if (process_format != ETC1 && process_format != ETC1A4)
				swizzle (img, true);

			// composite the mipmap onto the preview
			preview.composite (img, Magick::Geometry (0, 0, hoff, voff), Magick::OverCompositeOp);

			// position for next mipmap
			voff += height;
			if (hoff == 0)
			{
				voff = 0;
				hoff = width;
			}
		}
	}

	// join all the worker threads
	while (!workers.empty ())
	{
		workers.back ().join ();
		workers.pop_back ();
	}

	if (!preview_path.empty ())
	{
		try
		{
			// output the preview image
			preview.write (add_prefix (preview_path, prefix));
		}
		catch (...)
		{
			try
			{
				// type couldn't be determined from file extension, so try png
				preview.magick ("PNG");
				preview.write (add_prefix (preview_path, prefix));
			}
			catch (...)
			{
				std::fprintf (stderr, "Failed to output preview\n");
			}
		}
	}
}

/** @brief Write buffer
 *  @param[in] fp File handle
 */
void write_buffer (FILE *fp, const void *buffer, size_t size)
{
	const uint8_t *buf = static_cast<const uint8_t *> (buffer);
	size_t pos         = 0;

	while (pos < size)
	{
		ssize_t rc = std::fwrite (buf + pos, 1, size - pos, fp);
		if (rc <= 0)
		{
			std::fclose (fp);
			throw std::runtime_error ("Failed to output data");
		}

		pos += rc;
	}
}

/** @brief Write Tex3DS header
 *  @param[in] fp File handle
 */
void write_tex3ds_header (FILE *fp)
{
	encode::Buffer buf;

	encode::encode<uint16_t> (subimage_data.size (), buf);

	uint8_t texture_params = 0;

	assert (output_width >= 8);
	assert (output_width <= 1024);
	assert (output_height >= 8);
	assert (output_height <= 1024);

	uint8_t w = std::log (static_cast<double> (output_width)) / std::log (2.0);
	uint8_t h = std::log (static_cast<double> (output_height)) / std::log (2.0);

	assert (w >= 3);
	assert (w <= 10);
	assert (h >= 3);
	assert (h <= 10);

	texture_params |= (w - 3) << 0;
	texture_params |= (h - 3) << 3;

	if (process_mode == PROCESS_CUBEMAP || process_mode == PROCESS_SKYBOX)
		texture_params |= 1 << 6;

	encode::encode<uint8_t> (texture_params, buf);
	encode::encode<uint8_t> (process_format, buf);

	uint8_t num_mipmaps = std::min (w, h) - 3;
	if (filter_type == Magick::UndefinedFilter)
		num_mipmaps = 0;
	encode::encode<uint8_t> (num_mipmaps, buf);

	// encode subimage info
	// for(size_t i = 0; i < subimage_data.size(); ++i)
	for (const auto &sub : subimage_data)
	{
		uint16_t width;
		uint16_t height;

		// check if subimage is rotated
		if (sub.top < sub.bottom)
		{
			height = (sub.bottom - sub.top) * output_width;
			width  = (sub.right - sub.left) * output_height;
		}
		else
		{
			width  = (sub.right - sub.left) * output_width;
			height = (sub.top - sub.bottom) * output_height;
		}

		encode::encode (sub, width, height, buf);
	}

	write_buffer (fp, buf.data (), buf.size ());
}

/** @brief Dummy compression
 *  @param[in]  src    Source buffer
 *  @param[in]  len    Source length
 *  @returns "Compressed" buffer
 */
std::vector<uint8_t> compressNone (const void *src, size_t len)
{
	const uint8_t *source = reinterpret_cast<const uint8_t *> (src);

	std::vector<uint8_t> result;

	// append compression header
	compressionHeader (result, 0x00, len);

	// add data
	result.insert (std::end (result), source, source + len);

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	return result;
}

/** @brief Auto-select compression
 *  @param[in]  src    Source buffer
 *  @param[in]  len    Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> compressAuto (const void *src, size_t len)
{
	std::vector<uint8_t> best;

	static std::vector<uint8_t> (*const compress_funcs[]) (const void *, size_t) = {
	    compressNone,
	    lzssEncode,
	    lz11Encode,
	    // huffEncode, // broken
	    rleEncode,
	};

	for (const auto &compress : compress_funcs)
	{
		std::vector<uint8_t> output = compress (src, len);

		if (best.empty () || (!output.empty () && output.size () < best.size ()))
			best.swap (output);
	}

	return best;
}

/** @brief Write image data
 *  @param[in] fp File handle
 */
void write_image_data (FILE *fp)
{
	std::vector<uint8_t> (*compress) (const void *, size_t) = nullptr;

	// get the compression routine
	switch (compression_format)
	{
	case COMPRESSION_NONE:
		compress = compressNone;
		break;

	case COMPRESSION_LZ10:
		compress = lzssEncode;
		break;

	case COMPRESSION_LZ11:
		compress = lz11Encode;
		break;

	case COMPRESSION_RLE:
		compress = rleEncode;
		break;

	case COMPRESSION_HUFF:
		compress = huffEncode;
		break;

	case COMPRESSION_AUTO:
		compress = compressAuto;
		break;

	default:
		// We should only get a valid type here
		std::abort ();
	}

	// compress data
	std::vector<uint8_t> buffer = compress (image_data.data (), image_data.size ());
	if (buffer.empty ())
	{
		std::fclose (fp);
		throw std::runtime_error ("Failed to compress data");
	}

	// output data
	write_buffer (fp, buffer.data (), buffer.size ());
}

/** @brief Write output data
 */
void write_output_data ()
{
	// check if we need to output the data
	if (output_path.empty ())
		return;

	FILE *fp = std::fopen (output_path.c_str (), "wb");
	if (!fp)
		throw std::runtime_error ("Failed to open output file");

	if (!output_raw)
		write_tex3ds_header (fp);

	write_image_data (fp);

	// close output file
	std::fclose (fp);
}

/** @brief Sanitize identifier
 */
void sanitize_identifier (std::string &id)
{
	if (!std::isalnum (id[0]) && id[0] != '_')
		id.insert (0, 1, '_');

	// for(size_t i = 0; i < id.size(); ++i)
	for (auto &c : id)
	{
		if (!std::isalnum (c) && c != '_')
			c = '_';
	}
}

/** @brief Write dependency file
 */
void write_dependency ()
{
	if (depends_path.empty ())
		return;

	FILE *fp = std::fopen (depends_path.c_str (), "w");
	if (!fp)
		throw std::runtime_error ("Failed to open output dependency file");

	std::fputs ("# Generated by tex3ds\n", fp);

	if (output_path.empty () && header_path.empty ())
	{
		std::fclose (fp);
		return;
	}

	std::string target;
	if (output_path.empty ())
		target = header_path;
	else if (header_path.empty ())
		target = output_path;
	else
		target = output_path + ' ' + header_path;

	std::fprintf (fp, "%s:", target.c_str ());
	for (const auto &dependency : dependencies)
		std::fprintf (fp, " %s", dependency.c_str ());
	std::fputc ('\n', fp);

	std::fclose (fp);
}

/** @brief Write header
 */
void write_header ()
{
	// check if we need to output the header
	if (header_path.empty ())
		return;

	FILE *fp = std::fopen (header_path.c_str (), "w");
	if (!fp)
		throw std::runtime_error ("Failed to open output header");

	std::fprintf (fp, "/* Generated by tex3ds */\n");
	std::fprintf (fp, "#pragma once\n\n");

	{
		std::vector<char> path (header_path.begin (), header_path.end ());
		path.push_back (0);
		header_path = ::basename (path.data ());
	}

	auto pos = header_path.rfind ('.');
	if (pos != std::string::npos)
		header_path.resize (pos);

	sanitize_identifier (header_path);

	size_t i = 0;
	for (const auto &sub : subimage_data)
	{
		std::string label = sub.name;

		pos = label.rfind ('.');
		if (pos != std::string::npos)
			label.resize (pos);

		sanitize_identifier (label);

		label += "_idx";

		if (label[0] != '_')
			label.insert (0, 1, '_');

		std::fprintf (fp, "#define %s%s %zu\n", header_path.c_str (), label.c_str (), i++);
	}

	// close output header
	std::fclose (fp);
}

/** @brief Print version information */
void print_version ()
{
	std::printf ("tex3ds v1.0.1\n"
	             "Copyright (c) 2017-2019 Michael Theall (mtheall)\n\n"

	             "tex3ds is free software: you can redistribute it and/or modify\n"
	             "it under the terms of the GNU General Public License as published by\n"
	             "the Free Software Foundation, either version 3 of the License, or\n"
	             "(at your option) any later version.\n\n"

	             "tex3ds is distributed in the hope that it will be useful,\n"
	             "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	             "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	             "GNU General Public License for more details.\n\n"

	             "You should have received a copy of the GNU General Public License\n"
	             "along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.\n");
}

/** @brief Print usage information
 *  @param[in] prog Program invocation
 */
void print_usage (const char *prog)
{
	std::printf ("Usage: %s [OPTIONS...] <input>\n", prog);

	std::printf (
	    "  Options:\n"
	    "    -d, --depends <file>         Output dependency file\n"
	    "    -f, --format <format>        See \"Format Options\"\n"
	    "    -H, --header <file>          Output C header to file\n"
	    "    -h, --help                   Show this help message\n"
	    "    -i, --include <file>         Include options from file\n"
	    "    -m, --mipmap <filter>        Generate mipmaps. See \"Mipmap Filter Options\"\n"
	    "    -o, --output <output>        Output file\n"
	    "    -p, --preview <preview>      Output preview file\n"
	    "    -q, --quality <etc1-quality> ETC1 quality. Valid options: low, medium (default), "
	    "high\n"
	    "    -r, --raw                    Output image data only\n"
	    "    -t, --trim                   Trim input image(s)\n"
	    "    -v, --version                Show version and copyright information\n"
	    "    -z, --compress <compression> Compress output. See \"Compression Options\"\n"
	    "    --atlas                      Generate texture atlas\n"
	    "    --cubemap                    Generate a cubemap. See \"Cubemap\"\n"
	    "    --skybox                     Generate a skybox. See \"Skybox\"\n"
	    "    <input>                      Input file\n\n"

	    "  Format Options:\n"
	    "    -f rgba, -f rgba8, -f rgba8888\n"
	    "      32-bit RGBA (8-bit components) (default)\n\n"

	    "    -f rgb, -f rgb8, -f rgb888\n"
	    "      24-bit RGB (8-bit components)\n\n"

	    "    -f rgba5551\n"
	    "      16-bit RGBA (5-bit RGB, 1-bit Alpha)\n\n"

	    "    -f rgb565\n"
	    "      16-bit RGB (5-bit Red/Blue, 6-bit Green)\n\n"

	    "    -f rgba4, -f rgba444\n"
	    "      16-bit RGBA (4-bit components)\n\n"

	    "    -f la, -f la8, -f la88\n"
	    "      16-bit Luminance/Alpha (8-bit components)\n\n"

	    "    -f hilo, -f hilo8, -f hilo88\n"
	    "      16-bit HILO (8-bit components)\n"
	    "      Note: HI comes from Red channel, LO comes from Green channel\n\n"

	    "    -f l, -f l8\n"
	    "      8-bit Luminance\n\n"

	    "    -f a, -f a8\n"
	    "      8-bit Alpha\n\n"

	    "    -f la4, -f la44\n"
	    "      8-bit Luminance/Alpha (4-bit components)\n\n"

	    "    -f l4\n"
	    "      4-bit Luminance\n\n"

	    "    -f a4\n"
	    "      4-bit Alpha\n\n"

	    "    -f etc1\n"
	    "      ETC1\n\n"

	    "    -f etc1a4\n"
	    "      ETC1 with 4-bit Alpha\n\n"

	    "    -f auto-l8\n"
	    "      L8 when input has no alpha, otherwise LA8\n\n"

	    "    -f auto-l4\n"
	    "      L4 when input has no alpha, otherwise LA4\n\n"

	    "    -f auto-etc1\n"
	    "      ETC1 when input has no alpha, otherwise ETC1A4\n\n"

	    "  Mipmap Filter Options:\n");
	for (const auto &type : filter_type_strings)
		std::printf ("    -m %s\n", type.first);

	std::printf (
	    "\n"
	    "  Compression Options:\n"
	    "    -z auto              Automatically select best compression (default)\n"
	    "    -z none              No compression\n"
	    "    -z huff, -z huffman  Huffman encoding (possible to produce garbage)\n"
	    "    -z lzss, -z lz10     LZSS compression\n"
	    "    -z lz11              LZ11 compression\n"
	    "    -z rle               Run-length encoding\n\n"

	    "    NOTE: All compression types use a compression header: a single byte which denotes the "
	    "compression type, followed by three bytes (little-endian) which specify the size of the "
	    "uncompressed data. If the compression type byte has the MSB (0x80) set, the size is "
	    "specified by four bytes (little-endian) plus three bytes of reserved (zero) padding.\n\n"

	    "    Types:\n"
	    "      0x00: Fake (uncompressed)\n"
	    "      0x10: LZSS\n"
	    "      0x11: LZ11\n"
	    "      0x28: Huffman encoding\n"
	    "      0x30: Run-length encoding\n\n"

	    "  Cubemap:\n"
	    "    A cubemap is generated from the input image in the following convention:\n"
	    "    +----+----+---------+\n"
	    "    |    | +Y |         |\n"
	    "    +----+----+----+----+\n"
	    "    | -X | +Z | +X | -Z |\n"
	    "    +----+----+----+----+\n"
	    "    |    | -Y |         |\n"
	    "    +----+----+---------+\n\n"

	    "  Skybox:\n"
	    "    A skybox is generated from the input image in the following convention:\n"
	    "    +----+----+---------+\n"
	    "    |    | +Y |         |\n"
	    "    +----+----+----+----+\n"
	    "    | -X | -Z | +X | +Z |\n"
	    "    +----+----+----+----+\n"
	    "    |    | -Y |         |\n"
	    "    +----+----+---------+\n\n");
}

/** @brief Program long options */
const struct option long_options[] = {
    /* clang-format off */
	{ "atlas",    no_argument,       nullptr, 'a', },
	{ "cubemap",  no_argument,       nullptr, 'c', },
	{ "depends",  required_argument, nullptr, 'd', },
	{ "format",   required_argument, nullptr, 'f', },
	{ "help",     no_argument,       nullptr, 'h', },
	{ "mipmap",   required_argument, nullptr, 'm', },
	{ "output",   required_argument, nullptr, 'o', },
	{ "preview",  required_argument, nullptr, 'p', },
	{ "quality",  required_argument, nullptr, 'q', },
	{ "raw",      no_argument,       nullptr, 'r', },
	{ "skybox",   no_argument,       nullptr, 's', },
	{ "trim",     no_argument,       nullptr, 't', },
	{ "version",  no_argument,       nullptr, 'v', },
	{ "compress", required_argument, nullptr, 'z', },
	{ nullptr,    no_argument,       nullptr,   0, },
	/* clang-format off */
};

/** @brief Parsing status */
enum ParseStatus
{
	PARSE_SUCCESS,
	PARSE_FAILURE,
	PARSE_EXIT,
};

const char *prog;
std::vector<std::string> input_files;

std::string getPath (std::string path)
{
#ifdef WIN32
	std::replace (path.begin (), path.end (), '\\', '/');
#endif

	std::string acc;
	if ((path.size () > 1 && path[0] == '/')
#ifdef WIN32
	    || (path.size () > 3 && path[1] == ':' && path[2] == '/')
#endif
	)
		acc = std::move (path); // absolute path
	else
	{
		// relative path
		acc = include_stack.back ();
#ifdef WIN32
		std::replace (acc.begin (), acc.end (), '\\', '/');
#endif
		if (!acc.empty () && acc.back () != '/')
			acc.push_back ('/');
		acc += path;
	}

	// remove leading ./
	if (acc.size () >= 2 && acc[0] == '.' && acc[1] == '/')
		acc = acc.substr (2);

	return acc;
}

std::vector<std::string> readOptions (const std::string &path)
{
	FILE *fp = std::fopen (path.c_str (), "r");
	if (!fp)
		throw std::runtime_error ("Failed to open options file");

	try
	{
		std::vector<std::string> options (1);
		int c;
		bool quoted = false;
		std::string opt;

		while ((c = std::fgetc (fp)) != EOF)
		{
			switch (c)
			{
			case '"':
				quoted = !quoted;
				break;

			case '\\':
				if (quoted)
					c = std::fgetc (fp);
				if (c == EOF)
					throw std::runtime_error (
					    "Reached end of options file at partially escaped character");

			/* fall-through */
			default:
				if (quoted)
					opt.push_back (c);
				else if (std::isspace (c))
				{
					if (!opt.empty ())
						options.push_back (opt);
					opt.clear ();
				}
				else
					opt.push_back (c);
				break;
			}
		}

		if (quoted)
			throw std::runtime_error ("Reached end of options file at partially quoted string");

		if (!opt.empty ())
			options.push_back (opt);

		return options;
	}
	catch (...)
	{
		std::fclose (fp);
		throw;
	}
}

ParseStatus parseOptions (std::vector<char *> &args)
{
	int c;

	// parse options
	while (
	    (c = ::getopt_long (
	         args.size (), args.data (), "d:f:H:hi:m:o:p:q:rs:tvz:", long_options, nullptr)) != -1)
	{
		switch (c)
		{
		case 'a':
			// atlas
			process_mode = PROCESS_ATLAS;
			break;

		case 'c':
			// cubemap
			process_mode = PROCESS_CUBEMAP;
			break;

		case 'd':
			// set dependency path option
			depends_path = getPath (optarg);
			break;

		case 'f':
		{
			// find matching output format
			auto format = std::lower_bound (std::begin (output_format_strings),
			    std::end (output_format_strings),
			    optarg,
			    ProcessFormatComparator ());

			// set output format option
			if (format != std::end (output_format_strings))
				process_format = format->second;
			else
			{
				std::fprintf (stderr, "Invalid format option '%s'\n", optarg);
				return PARSE_FAILURE;
			}

			break;
		}

		case 'H':
			// set header path option
			header_path = getPath (optarg);
			break;

		case 'h':
			// show help
			print_usage (prog);
			return PARSE_EXIT;

		case 'i':
			try
			{
				std::string optionsFile = getPath (optarg);
				std::string new_cwd;
				{
					std::vector<char> path (optionsFile.begin (), optionsFile.end ());
					path.push_back (0);
					new_cwd = ::dirname (path.data ());
				}

				int old_optind = optind;
				optind         = 1;

				std::vector<std::string> options = readOptions (optionsFile);

				std::vector<char *> o;
				for (const auto &opt : options)
				{
					// getopt only take non-const :(
					o.push_back (const_cast<char *> (opt.c_str ()));
				}

				include_stack.emplace_back (std::move (new_cwd));
				ParseStatus status = parseOptions (o);
				include_stack.pop_back ();

				optind = old_optind;

				if (status != PARSE_SUCCESS)
					return status;
			}
			catch (const std::exception &e)
			{
				std::fprintf (stderr, "%s\n", e.what ());
				return PARSE_FAILURE;
			}
			break;

		case 'm':
		{
			// find matching mipmap filter type
			auto filter = std::lower_bound (std::begin (filter_type_strings),
			    std::end (filter_type_strings),
			    optarg,
			    FilterTypeComparator ());

			// set mipmap filter type option
			if (filter != std::end (filter_type_strings))
				filter_type = filter->second;
			else
			{
				std::fprintf (stderr, "Invalid mipmap filter type '%s'\n", optarg);
				return PARSE_FAILURE;
			}

			break;
		}

		case 'o':
			// set output path option
			output_path = getPath (optarg);
			break;

		case 'p':
			// set preview path option
			preview_path = getPath (optarg);
			break;

		case 'q':
			// set ETC1 quality
			if (strcasecmp ("low", optarg) == 0)
				etc1_quality = rg_etc1::cLowQuality;
			else if (strcasecmp ("medium", optarg) == 0 || strcasecmp ("med", optarg) == 0)
				etc1_quality = rg_etc1::cMediumQuality;
			else if (strcasecmp ("high", optarg) == 0)
				etc1_quality = rg_etc1::cHighQuality;
			else
			{
				std::fprintf (stderr, "Invalid ETC1 quality '%s'\n", optarg);
				return PARSE_FAILURE;
			}
			break;

		case 'r':
			// output raw image data
			output_raw = true;
			break;

		case 's':
			// skybox
			process_mode = PROCESS_SKYBOX;
			break;

		case 't':
			// trim
			trim = true;
			break;

		case 'v':
			// print version
			print_version ();
			return PARSE_EXIT;

		case 'z':
		{
			// find matching compression format
			auto format = std::lower_bound (std::begin (compression_format_strings),
			    std::end (compression_format_strings),
			    optarg,
			    CompressionFormatComparator ());

			// set compression format option
			if (format != std::end (compression_format_strings))
				compression_format = format->second;
			else
			{
				std::fprintf (stderr, "Invalid compression option '%s'\n", optarg);
				return PARSE_FAILURE;
			}

			break;
		}

		default:
			std::fprintf (stderr, "Invalid option '%c'\n", optopt);
			return PARSE_FAILURE;
		}
	}

	assert (optind >= 0);

	while (static_cast<size_t> (optind) < args.size ())
	{
		std::string path = getPath (args[optind++]);
		input_files.emplace_back (path);
		dependencies.emplace (std::move (path));
	}

	return PARSE_SUCCESS;
}

}

/** @brief Program entry point
 *  @param[in] argc Number of command-line arguments
 *  @param[in] argv Command-line arguments
 *  @retval EXIT_SUCCESS
 *  @retval EXIT_FAILURE
 */
int main (int argc, char *argv[])
{
	prog = argv[0];

	std::setvbuf (stdout, nullptr, _IOLBF, 0);
	std::setvbuf (stderr, nullptr, _IOLBF, 0);

	std::vector<char *> args (argv, argv + argc);

	// parse options
	switch (parseOptions (args))
	{
	case PARSE_SUCCESS:
		break;
	case PARSE_FAILURE:
		return EXIT_FAILURE;
	case PARSE_EXIT:
		return EXIT_SUCCESS;
	}

	// check that input file(s) were provided
	if (input_files.empty ())
	{
		std::fprintf (stderr, "No image(s) provided\n");
		return EXIT_FAILURE;
	}

	// initialize rg_etc1 if ETC1/ETC1A format chosen
	if (process_format == ETC1 || process_format == ETC1A4 || process_format == AUTO_ETC1)
		rg_etc1::pack_etc1_block_init ();

	try
	{
		std::vector<Magick::Image> images;
		if (process_mode == PROCESS_ATLAS)
		{
			Atlas atlas (Atlas::build (input_files, trim));
			subimage_data.swap (atlas.subs);
			images = load_image (atlas.img);
		}
		else if (input_files.size () > 1)
		{
			std::fprintf (stderr, "Multiple inputs only supported with atlas mode\n");
			return EXIT_FAILURE;
		}
		else
		{
			Magick::Image img (input_files[0]);

			if (trim)
			{
				img.trim ();
				img.page (Magick::Geometry (img.columns (), img.rows ()));
			}

			images = load_image (img);
		}

		// finalize process format
		finalize_process_format (images);

		// process each sub-image
		for (size_t i = 0; i < images.size (); ++i)
			process_image (images[i]);

		// write output data
		write_output_data ();

		// write dependency file
		write_dependency ();

		// write header
		write_header ();
	}
	catch (const std::exception &e)
	{
		std::fprintf (stderr, "%s\n", e.what ());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
