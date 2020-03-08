/*
 * Copyright (c) 2020 Jeff Boody
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

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#define LOG_TAG "terrain2image"
#include "libcc/cc_log.h"
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"
#include "texgz/texgz_png.h"
#include "texgz/texgz_tex.h"

int main(int argc, char** argv)
{
	if(argc != 4)
	{
		LOGE("usage: %s [zoom] [x] [y]", argv[0]);
		return EXIT_FAILURE;
	}

	int zoom = (int) strtol(argv[1], NULL, 0);
	int x    = (int) strtol(argv[2], NULL, 0);
	int y    = (int) strtol(argv[3], NULL, 0);

	terrain_tile_t* tile;
	tile = terrain_tile_import(".", x, y, zoom);
	if(tile == NULL)
	{
		return EXIT_FAILURE;
	}

	int samples = TERRAIN_SAMPLES_TOTAL;

	texgz_tex_t* tex;
	tex = texgz_tex_new(samples, samples,
	                    samples, samples,
	                    TEXGZ_UNSIGNED_BYTE,
	                    TEXGZ_RGBA, NULL);
	if(tex == NULL)
	{
		goto fail_tex;
	}

	// find min/max values
	int i;
	int j;
	short min = SHRT_MAX;
	short max = SHRT_MIN;
	for(i = 0; i < samples; ++i)
	{
		for(j = 0; j < samples; ++j)
		{
			short s = tile->data[i*samples + j];
			if(s > max)
			{
				max = s;
			}
			else if(s < min)
			{
				min = s;
			}
		}
	}
	LOGI("min=%i, max=%i", (int) min, (int) max);

	// fill tex
	if(max > min)
	{
		for(i = 0; i < samples; ++i)
		{
			for(j = 0; j < samples; ++j)
			{
				short s  = tile->data[i*samples + j];
				float fs = (float) s;
				float fmin = (float) min;
				float fmax = (float) max;
				float fp = 255.0f*(fs - fmin)/(fmax - fmin);
				unsigned char pix = (unsigned char) fp;
				tex->pixels[4*(i*tex->width + j) + 0] = pix;
				tex->pixels[4*(i*tex->width + j) + 1] = pix;
				tex->pixels[4*(i*tex->width + j) + 2] = pix;
				tex->pixels[4*(i*tex->width + j) + 3] = 255;
			}
		}
	}

	char fname[256];
	snprintf(fname, 256, "out-%i-%i-%i.png", zoom, x, y);
	texgz_png_export(tex, fname);
	texgz_tex_delete(&tex);
	terrain_tile_delete(&tile);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_tex:
		terrain_tile_delete(&tile);
	return EXIT_FAILURE;
}
