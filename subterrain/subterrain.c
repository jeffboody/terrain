/*
 * Copyright (c) 2015 Jeff Boody
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

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_TAG "subterrain"
#include "libcc/cc_log.h"
#include "libcc/cc_timestamp.h"
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"
#include "texgz/texgz_tex.h"

// protected function(s)
extern int terrain_tile_export(terrain_tile_t* self,
                               const char* base);
extern void terrain_tile_set(terrain_tile_t* self,
                             int m, int n,
                             short h);
extern void terrain_tile_adjustMinMax(terrain_tile_t* self,
                                      short min,
                                      short max);
extern void terrain_tile_exists(terrain_tile_t* self,
                                int flags);

static void sample_lod00(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// top-left border sample
	short h = terrain_tile_get(next,
	                           TERRAIN_SAMPLES_TILE - 3,
	                           TERRAIN_SAMPLES_TILE - 3);
	terrain_tile_set(ter, -1, -1, h);
}

static void sample_lod01(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// top border samples
	int nn;
	int n = 0;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h;
		h = terrain_tile_get(next,
		                     TERRAIN_SAMPLES_TILE - 3,
		                     nn);
		terrain_tile_set(ter, -1, n++, h);
	}
}

static void sample_lod02(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// top border samples
	int nn;
	int n = 128;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h;
		h = terrain_tile_get(next,
		                     TERRAIN_SAMPLES_TILE - 3,
		                     nn);
		terrain_tile_set(ter, -1, n++, h);
	}
}

static void sample_lod03(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// top-right border sample
	short h;
	h = terrain_tile_get(next,
	                     TERRAIN_SAMPLES_TILE - 3,
	                     2);
	terrain_tile_set(ter, -1, 257, h);
}

static void sample_lod10(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// left border samples
	int mm;
	int m = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h;
		h = terrain_tile_get(next, mm,
		                     TERRAIN_SAMPLES_TILE - 3);
		terrain_tile_set(ter, m++, -1, h);
	}
}

static void sample_lod11(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		terrain_tile_adjustMinMax(ter, 0, 0);
		return;
	}

	short min = terrain_tile_min(next);
	short max = terrain_tile_max(next);
	terrain_tile_adjustMinMax(ter, min, max);
	terrain_tile_exists(ter, TERRAIN_NEXT_TL);

	// center border samples
	int mm;
	int nn;
	int m = 0;
	int n = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next, mm, nn);
			terrain_tile_set(ter, m, n++, h);
		}
		++m;
		n = 0;
	}
}

static void sample_lod12(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		terrain_tile_adjustMinMax(ter, 0, 0);
		return;
	}

	short min = terrain_tile_min(next);
	short max = terrain_tile_max(next);
	terrain_tile_adjustMinMax(ter, min, max);
	terrain_tile_exists(ter, TERRAIN_NEXT_TR);

	// center border samples
	int mm;
	int nn;
	int m = 0;
	int n = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next, mm, nn);
			terrain_tile_set(ter, m, n++, h);
		}
		++m;
		n = 128;
	}
}

static void sample_lod13(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// right border samples
	int mm;
	int m = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h = terrain_tile_get(next, mm, 2);
		terrain_tile_set(ter, m++, 257, h);
	}
}

static void sample_lod20(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// left border samples
	int mm;
	int m = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h;
		h = terrain_tile_get(next, mm,
		                     TERRAIN_SAMPLES_TILE - 3);
		terrain_tile_set(ter, m++, -1, h);
	}
}

static void sample_lod21(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		terrain_tile_adjustMinMax(ter, 0, 0);
		return;
	}

	short min = terrain_tile_min(next);
	short max = terrain_tile_max(next);
	terrain_tile_adjustMinMax(ter, min, max);
	terrain_tile_exists(ter, TERRAIN_NEXT_BL);

	// center border samples
	int mm;
	int nn;
	int m = 128;
	int n = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next, mm, nn);
			terrain_tile_set(ter, m, n++, h);
		}
		++m;
		n = 0;
	}
}

static void sample_lod22(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		terrain_tile_adjustMinMax(ter, 0, 0);
		return;
	}

	short min = terrain_tile_min(next);
	short max = terrain_tile_max(next);
	terrain_tile_adjustMinMax(ter, min, max);
	terrain_tile_exists(ter, TERRAIN_NEXT_BR);

	// center border samples
	int mm;
	int nn;
	int m = 128;
	int n = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next, mm, nn);
			terrain_tile_set(ter, m, n++, h);
		}
		++m;
		n = 128;
	}
}

static void sample_lod23(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// right border samples
	int mm;
	int m = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h = terrain_tile_get(next, mm, 2);
		terrain_tile_set(ter, m++, 257, h);
	}
}

static void sample_lod30(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// bottom-left border sample
	short h;
	h = terrain_tile_get(next, 2,
	                     TERRAIN_SAMPLES_TILE - 3);
	terrain_tile_set(ter, 257, -1, h);
}

static void sample_lod31(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// bottom border samples
	int nn;
	int n = 0;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h = terrain_tile_get(next, 2, nn);
		terrain_tile_set(ter, 257, n++, h);
	}
}

static void sample_lod32(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// bottom border samples
	int nn;
	int n = 128;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h = terrain_tile_get(next, 2, nn);
		terrain_tile_set(ter, 257, n++, h);
	}
}

static void sample_lod33(terrain_tile_t* ter,
                         terrain_tile_t* next)
{
	if(next == NULL)
	{
		return;
	}

	// bottom-right border sample
	short h = terrain_tile_get(next, 2, 2);
	terrain_tile_set(ter, 257, 257, h);
}

static void sample_tile(int x, int y, int zoom, char* output)
{
	int xx = 2*x;
	int yy = 2*y;
	int zz = zoom + 1;

	// load the surrounding tiles in the next zoom level
	int r;
	int c;
	int done = 1;
	terrain_tile_t* next[16] =
	{
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
	};

	char fname[256];
	for(r = 0; r < 4; ++r)
	{
		for(c = 0; c < 4; ++c)
		{
			snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
			         output, zz, xx + c - 1, yy + r - 1);

			if(access(fname, F_OK) != 0)
			{
				// fname may not exist
				continue;
			}

			int idx = 4*r + c;
			next[idx] = terrain_tile_import(output,
			                                xx + c - 1,
			                                yy + r - 1,
			                                zz);
			if(next[idx])
			{
				done = 0;
			}
		}
	}

	if(done)
	{
		return;
	}

	terrain_tile_t* ter = terrain_tile_new(x, y, zoom);
	if(ter == NULL)
	{
		goto fail_ter;
	}

	// sample the next lods
	sample_lod00(ter, next[0]);
	sample_lod01(ter, next[1]);
	sample_lod02(ter, next[2]);
	sample_lod03(ter, next[3]);
	sample_lod10(ter, next[4]);
	sample_lod11(ter, next[5]);
	sample_lod12(ter, next[6]);
	sample_lod13(ter, next[7]);
	sample_lod20(ter, next[8]);
	sample_lod21(ter, next[9]);
	sample_lod22(ter, next[10]);
	sample_lod23(ter, next[11]);
	sample_lod30(ter, next[12]);
	sample_lod31(ter, next[13]);
	sample_lod32(ter, next[14]);
	sample_lod33(ter, next[15]);

	// export this lod
	if(terrain_tile_export(ter, output) == 0)
	{
		goto fail_export;
	}

	// delete the lod tiles
	int idx;
	for(idx = 0; idx < 16; ++idx)
	{
		terrain_tile_delete(&next[idx]);
	}
	terrain_tile_delete(&ter);

	// success
	return;

	// failure
	fail_export:
		terrain_tile_delete(&ter);
	fail_ter:
	{
		for(idx = 0; idx < 16; ++idx)
		{
			terrain_tile_delete(&next[idx]);
		}
	}
}

static void
sample_tile_range(int x0, int y0, int x1, int y1, int zoom,
                  char* output)
{
	int x;
	int y;
	int idx   = 1;
	int count = (x1 - x0 + 1)*(y1 - y0 + 1);
	for(y = y0; y <= y1; ++y)
	{
		for(x = x0; x <= x1; ++x)
		{
			if(idx%100 == 1)
			{
				LOGI("%i/%i: %i/%i/%i", idx, count, zoom, x, y);
			}
			++idx;

			sample_tile(x, y, zoom, output);
		}
	}
}

int main(int argc, char** argv)
{
	if(argc != 6)
	{
		LOGE("usage: %s [latT] [lonL] [latB] [lonR] [output]", argv[0]);
		return EXIT_FAILURE;
	}

	int   latT   = (int) strtol(argv[1], NULL, 0) + 1;
	int   lonL   = (int) strtol(argv[2], NULL, 0);
	int   latB   = (int) strtol(argv[3], NULL, 0);
	int   lonR   = (int) strtol(argv[4], NULL, 0) + 1;
	char* output = argv[5];

	double t0 = cc_timestamp();

	int zoom;
	for(zoom = 14; zoom >= 0; --zoom)
	{
		// determine tile range
		float x0f;
		float y0f;
		float x1f;
		float y1f;
		terrain_coord2tile(latT, lonL, zoom, &x0f, &y0f);
		terrain_coord2tile(latB, lonR, zoom, &x1f, &y1f);

		// determine range of candidate tiles
		int x0 = (int) x0f;
		int y0 = (int) y0f;
		int x1 = (int) (x1f + 1.0f);
		int y1 = (int) (y1f + 1.0f);

		// check the tile range
		int range = (int) pow(2.0, (double) zoom);
		if(x0 < 0)
		{
			x0 = 0;
		}
		if(y0 < 0)
		{
			y0 = 0;
		}
		if(x1 >= range)
		{
			x1 = range - 1;
		}
		if(y1 >= range)
		{
			y1 = range - 1;
		}

		// sample the set of tiles whose origin should cover range
		// again, due to overlap with other flt tiles the sampling
		// actually occurs over the entire flt_xx set
		sample_tile_range(x0, y0, x1, y1, zoom, output);
	}

	// success
	LOGI("dt=%lf", cc_timestamp() - t0);
	return EXIT_SUCCESS;
}
