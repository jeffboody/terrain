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

#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "flt/flt_tile.h"
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"

// protected function(s)
extern int terrain_tile_export(terrain_tile_t* self,
                               const char* base);
extern void terrain_tile_set(terrain_tile_t* self,
                             int m, int n,
                             short h);

#define LOG_TAG "flt2terrain"
#include "terrain/terrain_log.h"

// flt_cc is centered on the current tile being sampled
// load neighboring flt tiles since they may overlap
// only sample ned tiles whose origin is in flt_cc
static flt_tile_t* flt_tl = NULL;
static flt_tile_t* flt_tc = NULL;
static flt_tile_t* flt_tr = NULL;
static flt_tile_t* flt_cl = NULL;
static flt_tile_t* flt_cc = NULL;
static flt_tile_t* flt_cr = NULL;
static flt_tile_t* flt_bl = NULL;
static flt_tile_t* flt_bc = NULL;
static flt_tile_t* flt_br = NULL;

static double cc_timestamp(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (double) t.tv_sec +
	       ((double) t.tv_usec)/1000000.0;
}

static int
sample_tile(int x, int y, int zoom, const char* output)
{
	assert(output);

	terrain_tile_t* ter;
	ter = terrain_tile_new(x, y, zoom);
	if(ter == NULL)
	{
		return 0;
	}

	int m;
	int n;
	int min = -TERRAIN_SAMPLES_BORDER;
	int max = TERRAIN_SAMPLES_TILE + TERRAIN_SAMPLES_BORDER;
	for(m = min; m < max; ++m)
	{
		for(n = min; n < max; ++n)
		{
			double lat;
			double lon;
			terrain_tile_coord(ter, m, n, &lat, &lon);

			// flt_cc most likely place to find sample
			// At edges of range a tile may not be
			// fully covered by flt_xx
			short h;
			if((flt_cc && flt_tile_sample(flt_cc, lat, lon, &h)) ||
			   (flt_tc && flt_tile_sample(flt_tc, lat, lon, &h)) ||
			   (flt_bc && flt_tile_sample(flt_bc, lat, lon, &h)) ||
			   (flt_cl && flt_tile_sample(flt_cl, lat, lon, &h)) ||
			   (flt_cr && flt_tile_sample(flt_cr, lat, lon, &h)) ||
			   (flt_tl && flt_tile_sample(flt_tl, lat, lon, &h)) ||
			   (flt_bl && flt_tile_sample(flt_bl, lat, lon, &h)) ||
			   (flt_tr && flt_tile_sample(flt_tr, lat, lon, &h)) ||
			   (flt_br && flt_tile_sample(flt_br, lat, lon, &h)))
			{
				terrain_tile_set(ter, m, n, h);
			}
		}
	}

	if(terrain_tile_export(ter, output) == 0)
	{
		goto fail_export;
	}

	terrain_tile_delete(&ter);

	// success
	return 1;

	// failure
	fail_export:
		terrain_tile_delete(&ter);
	return 0;
}

static int
sample_tile_range(int x0, int y0, int x1, int y1, int zoom,
                  const char* output)
{
	assert(output);

	// sample tiles whose origin should be in flt_cc
	int x;
	int y;
	for(y = y0; y <= y1; ++y)
	{
		for(x = x0; x <= x1; ++x)
		{
			if(sample_tile(x, y, zoom, output) == 0)
			{
				return 0;
			}
		}
	}

	return 1;
}

int main(int argc, char** argv)
{
	if(argc != 7)
	{
		LOGE("usage: %s [zoom] [latT] [lonL] [latB] [lonR] [output]", argv[0]);
		return EXIT_FAILURE;
	}

	int zoom = (int) strtol(argv[1], NULL, 0);
	int latT = (int) strtol(argv[2], NULL, 0);
	int lonL = (int) strtol(argv[3], NULL, 0);
	int latB = (int) strtol(argv[4], NULL, 0);
	int lonR = (int) strtol(argv[5], NULL, 0);

	const char* output = argv[6];

	double t0 = cc_timestamp();
	double t1 = cc_timestamp();
	double t2 = cc_timestamp();

	int lati;
	int lonj;
	int idx   = 0;
	int count = (latT - latB + 1)*(lonR - lonL + 1);
	for(lati = latB; lati <= latT; ++lati)
	{
		for(lonj = lonL; lonj <= lonR; ++lonj)
		{
			// status message
			++idx;
			t2 = cc_timestamp();
			LOGI("%i/%i/%lf/%lf: lat=%i, lon=%i",
			     idx, count, t2 - t0, t2 - t1, lati, lonj);
			t1 = t2;

			// initialize flt data
			if(flt_tl == NULL)
			{
				flt_tl = flt_tile_import(lati + 1, lonj - 1);
			}
			if(flt_tc == NULL)
			{
				flt_tc = flt_tile_import(lati + 1, lonj);
			}
			if(flt_tr == NULL)
			{
				flt_tr = flt_tile_import(lati + 1, lonj + 1);
			}
			if(flt_cl == NULL)
			{
				flt_cl = flt_tile_import(lati, lonj - 1);
			}
			if(flt_cc == NULL)
			{
				flt_cc = flt_tile_import(lati, lonj);
			}
			if(flt_cr == NULL)
			{
				flt_cr = flt_tile_import(lati, lonj + 1);
			}
			if(flt_bl == NULL)
			{
				flt_bl = flt_tile_import(lati - 1, lonj - 1);
			}
			if(flt_bc == NULL)
			{
				flt_bc = flt_tile_import(lati - 1, lonj);
			}
			if(flt_br == NULL)
			{
				flt_br = flt_tile_import(lati - 1, lonj + 1);
			}

			// flt_cc may be NULL for sparse data
			if(flt_cc)
			{
				// sample tiles whose origin should be in flt_cc
				float x0f;
				float y0f;
				float x1f;
				float y1f;
				terrain_coord2tile(flt_cc->latT,
				                   flt_cc->lonL,
				                   zoom,
				                   &x0f,
				                   &y0f);
				terrain_coord2tile(flt_cc->latB,
				                   flt_cc->lonR,
				                   zoom,
				                   &x1f,
				                   &y1f);

				// determine range of candidate tiles
				// tile origin must be in lat/lon region
				// but may overlap with flt_xx
				int x0 = (int) (x0f + 1.0f);
				int y0 = (int) (y0f + 1.0f);
				int x1 = (int) x1f;
				int y1 = (int) y1f;

				// check for corner case of tile origin
				// being on exact edge of flt_cc
				if((x0f - floor(x0f)) == 0.0f)
				{
					x0 = (int) x0f;
				}
				if((y0f - floor(y0f)) == 0.0f)
				{
					y0 = (int) y0f;
				}

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

				// sample the set of tiles whose origin should cover flt_cc
				// again, due to overlap with other flt tiles the sampling
				// actually occurs over the entire flt_xx set
				if(sample_tile_range(x0, y0, x1, y1, zoom, output) == 0)
				{
					goto fail_sample;
				}
			}

			// next step, shift right
			flt_tile_delete(&flt_tl);
			flt_tile_delete(&flt_cl);
			flt_tile_delete(&flt_bl);
			flt_tl = flt_tc;
			flt_cl = flt_cc;
			flt_bl = flt_bc;
			flt_tc = flt_tr;
			flt_cc = flt_cr;
			flt_bc = flt_br;
			flt_tr = NULL;
			flt_cr = NULL;
			flt_br = NULL;
		}

		// next lati
		flt_tile_delete(&flt_tl);
		flt_tile_delete(&flt_cl);
		flt_tile_delete(&flt_bl);
		flt_tile_delete(&flt_tc);
		flt_tile_delete(&flt_cc);
		flt_tile_delete(&flt_bc);
		flt_tile_delete(&flt_tr);
		flt_tile_delete(&flt_cr);
		flt_tile_delete(&flt_br);
	}

	LOGI("dt=%lf", cc_timestamp() - t0);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_sample:
		flt_tile_delete(&flt_tl);
		flt_tile_delete(&flt_cl);
		flt_tile_delete(&flt_bl);
		flt_tile_delete(&flt_tc);
		flt_tile_delete(&flt_cc);
		flt_tile_delete(&flt_bc);
		flt_tile_delete(&flt_tr);
		flt_tile_delete(&flt_cr);
		flt_tile_delete(&flt_br);
	return EXIT_FAILURE;
}
