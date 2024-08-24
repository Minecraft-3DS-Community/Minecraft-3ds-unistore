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
/** @file mkbcfnt.cpp
 *  @brief mkbcfnt program entry point
 */
#include "bcfnt.h"
#include "freetype.h"
#include "future.h"

#include <getopt.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

namespace
{
/** @brief Print version information */
void printVersion ()
{
	std::printf ("mkbcfnt v1.0.1\n"
	             "Copyright (c) 2019\n"
	             "    Michael Theall (mtheall)\n"
	             "    piepie62\n\n"

	             "mkbcfnt is free software: you can redistribute it and/or modify\n"
	             "it under the terms of the GNU General Public License as published by\n"
	             "the Free Software Foundation, either version 3 of the License, or\n"
	             "(at your option) any later version.\n\n"

	             "mkbcfnt is distributed in the hope that it will be useful,\n"
	             "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	             "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	             "GNU General Public License for more details.\n\n"

	             "You should have received a copy of the GNU General Public License\n"
	             "along with mkbcfnt.  If not, see <http://www.gnu.org/licenses/>.\n");
}

/** @brief Print usage information
 *  @param[in] prog Program invocation
 */
void printUsage (const char *prog)
{
	std::printf ("Usage: %s [OPTIONS...] <input1> [input2...]\n", prog);

	std::printf (
	    "  Options:\n"
	    "    -h, --help                   Show this help message\n"
	    "    -o, --output <output>        Output file\n"
	    "    -s, --size <size>            Set font size in points\n"
	    "    -b, --blacklist <file>       Excludes the whitespace-separated list of codepoints\n"
	    "    -w, --whitelist <file>       Includes only the whitespace-separated list of "
	    "codepoints\n"
	    "    -v, --version                Show version and copyright information\n"
	    "    <inputN>                     Input file(s). Lower numbers get priority\n\n");
}

bool parseList (std::vector<std::uint16_t> &out, const char *path)
{
	FILE *fp = std::fopen (path, "r");
	if (!fp)
	{
		std::fprintf (stderr, "Error opening list file '%s': %s\n", path, std::strerror (errno));
		return false;
	}

	while (!std::feof (fp) && !std::ferror (fp))
	{
		int v;
		if (std::fscanf (fp, "%i", &v) != 1)
			break;

		out.emplace_back (static_cast<std::uint16_t> (v));
	}

	if (std::ferror (fp))
	{
		std::fprintf (stderr, "Error while reading list file %s\n", path);
		std::fclose (fp);
		return false;
	}

	std::fclose (fp);

	std::sort (std::begin (out), std::end (out));
	return true;
}

/** @brief Program long options */
const struct option longOptions[] = {
    /* clang-format off */
	{ "blacklist", required_argument, nullptr, 'b', },
	{ "help",      no_argument,       nullptr, 'h', },
	{ "output",    required_argument, nullptr, 'o', },
	{ "size",      required_argument, nullptr, 's', },
	{ "version",   no_argument,       nullptr, 'v', },
	{ "whitelist", required_argument, nullptr, 'w', },
	{ nullptr,     no_argument,       nullptr,   0, },
    /* clang-format on */
};
}

/** @brief Program entry point
 *  @param[in] argc Number of command-line arguments
 *  @param[in] argv Command-line arguments
 *  @retval EXIT_SUCCESS
 *  @retval EXIT_FAILURE
 */
int main (int argc, char *argv[])
{
	const char *prog = argv[0];

	// set line buffering
	std::setvbuf (stdout, nullptr, _IOLBF, 0);
	std::setvbuf (stderr, nullptr, _IOLBF, 0);

	std::string outputPath;
	std::vector<std::uint16_t> list;
	bool isBlacklist = true;
	double ptSize    = 22.0;

	// parse options
	int c;
	while ((c = ::getopt_long (argc, argv, "b:ho:s:vw:", longOptions, nullptr)) != -1)
	{
		switch (c)
		{
		case 'h':
			// show help
			printUsage (prog);
			return EXIT_SUCCESS;

		case 'o':
			// set output path option
			outputPath = optarg;
			break;

		case 's':
			// set font size
			try
			{
				ptSize = std::stod (optarg);
				if (!std::isfinite (ptSize) || ptSize == 0.0)
				{
					std::fprintf (stderr, "Invalid point size '%s'\n", optarg);
					return EXIT_FAILURE;
				}
			}
			catch (...)
			{
				std::fprintf (stderr, "Invalid point size '%s'\n", optarg);
				return EXIT_FAILURE;
			}
			break;

		case 'v':
			// print version
			printVersion ();
			return EXIT_SUCCESS;

		case 'b':
			// set blacklist
			parseList (list, optarg);
			isBlacklist = true;
			break;

		case 'w':
			// set whitelist
			parseList (list, optarg);
			isBlacklist = false;
			break;

		default:
			printUsage (prog);
			return EXIT_FAILURE;
		}
	}

	// output path required
	if (outputPath.empty ())
	{
		std::fprintf (stderr, "No output file provided\n");
		return EXIT_FAILURE;
	}

	// input path required
	if (optind >= argc)
	{
		std::fprintf (stderr, "No input file provided\n");
		return EXIT_FAILURE;
	}

	// collect input paths
	std::vector<std::string> inputs;
	while (optind < argc)
		inputs.emplace_back (argv[optind++]);

	auto library = freetype::Library::makeLibrary ();
	if (!library)
		return EXIT_FAILURE;

	auto bcfnt = future::make_unique<bcfnt::BCFNT> ();
	for (const auto &input : inputs)
	{
		FILE *fp = std::fopen (input.c_str (), "rb");
		if (!fp)
		{
			std::fprintf (stderr, "fopen '%s': %s\n", input.c_str (), std::strerror (errno));
			return EXIT_FAILURE;
		}

		// try to read BCFNT magic
		char magic[4];
		std::size_t rc = std::fread (magic, 1, sizeof (magic), fp);
		if (rc != sizeof (magic))
		{
			if (std::ferror (fp))
				std::fprintf (stderr, "fread: %s\n", std::strerror (errno));
			else if (std::feof (fp))
				std::fprintf (stderr, "fread: Unexpected end-of-file\n");
			else
				std::fprintf (stderr, "fread: Unknown read failure\n");

			std::fclose (fp);
			return EXIT_FAILURE;
		}

		// check if BCFNT
		if (std::memcmp (magic, "CFNT", sizeof (magic)) != 0)
		{
			// not BCFNT; try loading with freetype
			std::fclose (fp);

			auto face = freetype::Face::makeFace (library, input, ptSize);
			if (!face)
				return EXIT_FAILURE;

			bcfnt->addFont (std::move (face), list, isBlacklist);
			continue;
		}

		// BCFNT; try to decode
		if (std::fseek (fp, 0, SEEK_END) != 0)
		{
			std::fprintf (stderr, "fseek: %s\n", std::strerror (errno));
			std::fclose (fp);
			return EXIT_FAILURE;
		}

		std::size_t fileSize = std::ftell (fp);
		if (fileSize == static_cast<std::size_t> (-1L))
		{
			std::fprintf (stderr, "ftell: %s\n", std::strerror (errno));
			std::fclose (fp);
			return EXIT_FAILURE;
		}

		if (std::fseek (fp, 0, SEEK_SET) != 0)
		{
			std::fprintf (stderr, "fseek: %s\n", std::strerror (errno));
			std::fclose (fp);
			return EXIT_FAILURE;
		}

		std::vector<std::uint8_t> data (fileSize);

		std::size_t offset = 0;
		while (offset < fileSize)
		{
			std::size_t rc = std::fread (&data[offset], 1, fileSize - offset, fp);
			if (rc != fileSize - offset)
			{
				if (rc == 0 || std::ferror (fp))
				{
					if (std::ferror (fp))
						std::fprintf (stderr, "fread: %s\n", std::strerror (errno));
					else if (std::feof (fp))
						std::fprintf (stderr, "fread: Unexpected end-of-file\n");
					else
						std::fprintf (stderr, "fread: Unknown read failure\n");

					std::fclose (fp);
					return EXIT_FAILURE;
				}
			}

			offset += rc;
		}

		std::fclose (fp);

		auto font = future::make_unique<bcfnt::BCFNT> (data);
		bcfnt->addFont (*font, list, isBlacklist);
	}

	return bcfnt->serialize (outputPath) ? EXIT_SUCCESS : EXIT_FAILURE;
}
