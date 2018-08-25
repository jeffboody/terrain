/*
 * Copyright (c) 2018 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"

#define LOG_TAG "terrain"
#include "terrain/terrain_log.h"

/***********************************************************
* protected                                                *
***********************************************************/

extern int terrain_tile_export(terrain_tile_t* self,
                               const char* base);

/***********************************************************
* private                                                  *
***********************************************************/

static int crop(const char* src,
                const char* dst,
                int zoom, int x, int y,
                double latT, double lonL,
                double latB, double lonR)
{
	assert(src);
	assert(dst);

	short min;
	short max;
	int   flags;
	if(terrain_tile_header(src, x, y, zoom,
	                       &min, &max, &flags) == 0)
	{
		return 0;
	}

	int S = TERRAIN_SAMPLES_TILE - 1;
	int H = S/2;

	double tile_latT;
	double tile_lonL;
	double tile_latC;
	double tile_lonC;
	double tile_latB;
	double tile_lonR;
	terrain_sample2coord(x, y, zoom, 0, 0,
	                     &tile_latT, &tile_lonL);
	terrain_sample2coord(x, y, zoom, H, H,
	                     &tile_latC, &tile_lonC);
	terrain_sample2coord(x, y, zoom, S, S,
	                     &tile_latB, &tile_lonR);

	// check if next tiles have been cropped
	// or recursively crop subtiles
	int dirty = 0;
	if(flags & TERRAIN_NEXT_TL)
	{
		if((latT < tile_latC) ||
		   (latB > tile_latT) ||
		   (lonL > tile_lonC) ||
		   (lonR < tile_lonL))
		{
			LOGI("CROP: %i/%i/%i", zoom + 1, 2*x, 2*y);
			flags &= ~TERRAIN_NEXT_TL;
			dirty = 1;
		}
		else
		{
			LOGI("PICK: %i/%i/%i", zoom + 1, 2*x, 2*y);
			if(crop(src, dst, zoom + 1, 2*x, 2*y,
			        latT, lonL, latB, lonR) == 0)
			{
				return 0;
			}
		}
	}
	if(flags & TERRAIN_NEXT_TR)
	{
		if((latT < tile_latC) ||
		   (latB > tile_latT) ||
		   (lonL > tile_lonR) ||
		   (lonR < tile_lonC))
		{
			LOGI("CROP: %i/%i/%i", zoom + 1, 2*x + 1, 2*y);
			flags &= ~TERRAIN_NEXT_TR;
			dirty = 1;
		}
		else
		{
			LOGI("PICK: %i/%i/%i", zoom + 1, 2*x + 1, 2*y);
			if(crop(src, dst, zoom + 1, 2*x + 1, 2*y,
			        latT, lonL, latB, lonR) == 0)
			{
				return 0;
			}
		}
	}
	if(flags & TERRAIN_NEXT_BL)
	{
		if((latT < tile_latB) ||
		   (latB > tile_latC) ||
		   (lonL > tile_lonC) ||
		   (lonR < tile_lonL))
		{
			LOGI("CROP: %i/%i/%i", zoom + 1, 2*x, 2*y + 1);
			flags &= ~TERRAIN_NEXT_BL;
			dirty = 1;
		}
		else
		{
			LOGI("PICK: %i/%i/%i", zoom + 1, 2*x, 2*y + 1);
			if(crop(src, dst, zoom + 1, 2*x, 2*y + 1,
			        latT, lonL, latB, lonR) == 0)
			{
				return 0;
			}
		}
	}
	if(flags & TERRAIN_NEXT_BR)
	{
		if((latT < tile_latB) ||
		   (latB > tile_latC) ||
		   (lonL > tile_lonR) ||
		   (lonR < tile_lonC))
		{
			LOGI("CROP: %i/%i/%i", zoom + 1, 2*x + 1, 2*y + 1);
			flags &= ~TERRAIN_NEXT_BR;
			dirty = 1;
		}
		else
		{
			LOGI("PICK: %i/%i/%i", zoom + 1, 2*x + 1, 2*y + 1);
			if(crop(src, dst, zoom + 1, 2*x + 1, 2*y + 1,
			        latT, lonL, latB, lonR) == 0)
			{
				return 0;
			}
		}
	}

	// read tile from src
	terrain_tile_t* tile = terrain_tile_import(src, x, y, zoom);
	if(tile == NULL)
	{
		return 0;
	}

	// update flags for cropped tiles
	if(dirty)
	{
		tile->flags = flags;
	}

	// write tile to dst
	if(terrain_tile_export(tile, dst) == 0)
	{
		goto fail_export;
	}

	terrain_tile_delete(&tile);

	// success
	return 1;

	// failure
	fail_export:
		terrain_tile_delete(&tile);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, const char** argv)
{
	if(argc != 7)
	{
		LOGE("usage: %s [latT] [lonL] [latB] [lonR] [src] [dst]",
		      argv[0]);
		return EXIT_FAILURE;
	}

	double      latT = strtod(argv[1], NULL);
	double      lonL = strtod(argv[2], NULL);
	double      latB = strtod(argv[3], NULL);
	double      lonR = strtod(argv[4], NULL);
	const char* src  = argv[5];
	const char* dst  = argv[6];

	if(crop(src, dst, 0, 0, 0,
	        latT, lonL, latB, lonR) == 0)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
