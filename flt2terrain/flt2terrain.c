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
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define LOG_TAG "flt2terrain"
#include "flt/flt_tile.h"
#include "libcc/cc_log.h"
#include "libcc/cc_timestamp.h"
#include "libcc/cc_jobq.h"
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"

// protected function(s)
extern int terrain_tile_export(terrain_tile_t* self,
                               const char* base);
extern void terrain_tile_set(terrain_tile_t* self,
                             int m, int n,
                             short h);

// flt_cc is centered on the current tile being sampled
// load neighboring flt tiles since they may overlap
// only sample ned tiles whose origin is in flt_cc
// uflt is for USGS
// aflt is for ASTERv3
static flt_tile_t* uflt_tl = NULL;
static flt_tile_t* uflt_tc = NULL;
static flt_tile_t* uflt_tr = NULL;
static flt_tile_t* uflt_cl = NULL;
static flt_tile_t* uflt_cc = NULL;
static flt_tile_t* uflt_cr = NULL;
static flt_tile_t* uflt_bl = NULL;
static flt_tile_t* uflt_bc = NULL;
static flt_tile_t* uflt_br = NULL;
static flt_tile_t* aflt_tl = NULL;
static flt_tile_t* aflt_tc = NULL;
static flt_tile_t* aflt_tr = NULL;
static flt_tile_t* aflt_cl = NULL;
static flt_tile_t* aflt_cc = NULL;
static flt_tile_t* aflt_cr = NULL;
static flt_tile_t* aflt_bl = NULL;
static flt_tile_t* aflt_bc = NULL;
static flt_tile_t* aflt_br = NULL;

static const char* output = NULL;
static cc_jobq_t*  jobq   = NULL;
static int         error  = 0;

typedef struct
{
	int zoom;
	int x;
	int y;
	int complete;
} tile_job_t;

static tile_job_t*
tile_job_new(int zoom, int x, int y, int complete)
{
	tile_job_t* self;
	self = (tile_job_t*) malloc(sizeof(tile_job_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->zoom     = zoom;
	self->x        = x;
	self->y        = y;
	self->complete = complete;

	return self;
}

static void
tile_job_delete(tile_job_t** _self)
{
	assert(_self);

	tile_job_t* self = *_self;
	if(self)
	{
		free(self);
		*_self = NULL;
	}
}

static void
sample_tile_run(int tid, void* owner, void* task)
{
	assert(task);

	tile_job_t* job = (tile_job_t*) task;

	terrain_tile_t* ter;
	ter = terrain_tile_new(job->x, job->y, job->zoom);
	if(ter == NULL)
	{
		goto fail_terrain;
	}

	int m;
	int n;
	double latT;
	double lonL;
	double latB;
	double lonR;
	int    min = -TERRAIN_SAMPLES_BORDER;
	int    max = TERRAIN_SAMPLES_TILE + TERRAIN_SAMPLES_BORDER;
	double d   = (double) (max - min - 1);
	terrain_tile_coord(ter, min, min, &latT, &lonL);
	terrain_tile_coord(ter, max - 1, max - 1, &latB, &lonR);
	for(m = min; m < max; ++m)
	{
		for(n = min; n < max; ++n)
		{
			double u   = ((double) (n - min))/d;
			double v   = ((double) (m - min))/d;
			double lat = latT + v*(latB - latT);
			double lon = lonL + u*(lonR - lonL);

			// flt_cc most likely place to find sample
			// At edges of range a tile may not be
			// fully covered by flt_xx
			short h;
			if(job->complete)
			{
				// only sample USGS
				if((uflt_cc && flt_tile_sample(uflt_cc, lat, lon, &h)) ||
				   (uflt_tc && flt_tile_sample(uflt_tc, lat, lon, &h)) ||
				   (uflt_bc && flt_tile_sample(uflt_bc, lat, lon, &h)) ||
				   (uflt_cl && flt_tile_sample(uflt_cl, lat, lon, &h)) ||
				   (uflt_cr && flt_tile_sample(uflt_cr, lat, lon, &h)) ||
				   (uflt_tl && flt_tile_sample(uflt_tl, lat, lon, &h)) ||
				   (uflt_bl && flt_tile_sample(uflt_bl, lat, lon, &h)) ||
				   (uflt_tr && flt_tile_sample(uflt_tr, lat, lon, &h)) ||
				   (uflt_br && flt_tile_sample(uflt_br, lat, lon, &h)))
				{
					terrain_tile_set(ter, m, n, h);
				}
			}
			else if(uflt_cc)
			{
				// initialize with ASTERv3
				if((aflt_cc && flt_tile_sample(aflt_cc, lat, lon, &h)) ||
				   (aflt_tc && flt_tile_sample(aflt_tc, lat, lon, &h)) ||
				   (aflt_bc && flt_tile_sample(aflt_bc, lat, lon, &h)) ||
				   (aflt_cl && flt_tile_sample(aflt_cl, lat, lon, &h)) ||
				   (aflt_cr && flt_tile_sample(aflt_cr, lat, lon, &h)) ||
				   (aflt_tl && flt_tile_sample(aflt_tl, lat, lon, &h)) ||
				   (aflt_bl && flt_tile_sample(aflt_bl, lat, lon, &h)) ||
				   (aflt_tr && flt_tile_sample(aflt_tr, lat, lon, &h)) ||
				   (aflt_br && flt_tile_sample(aflt_br, lat, lon, &h)))
				{
					terrain_tile_set(ter, m, n, h);
				}

				// override with USGS
				if((uflt_cc && flt_tile_sample(uflt_cc, lat, lon, &h)) ||
				   (uflt_tc && flt_tile_sample(uflt_tc, lat, lon, &h)) ||
				   (uflt_bc && flt_tile_sample(uflt_bc, lat, lon, &h)) ||
				   (uflt_cl && flt_tile_sample(uflt_cl, lat, lon, &h)) ||
				   (uflt_cr && flt_tile_sample(uflt_cr, lat, lon, &h)) ||
				   (uflt_tl && flt_tile_sample(uflt_tl, lat, lon, &h)) ||
				   (uflt_bl && flt_tile_sample(uflt_bl, lat, lon, &h)) ||
				   (uflt_tr && flt_tile_sample(uflt_tr, lat, lon, &h)) ||
				   (uflt_br && flt_tile_sample(uflt_br, lat, lon, &h)))
				{
					terrain_tile_set(ter, m, n, h);
				}
			}
			else
			{
				// only sample with ASTERv3
				if((aflt_cc && flt_tile_sample(aflt_cc, lat, lon, &h)) ||
				   (aflt_tc && flt_tile_sample(aflt_tc, lat, lon, &h)) ||
				   (aflt_bc && flt_tile_sample(aflt_bc, lat, lon, &h)) ||
				   (aflt_cl && flt_tile_sample(aflt_cl, lat, lon, &h)) ||
				   (aflt_cr && flt_tile_sample(aflt_cr, lat, lon, &h)) ||
				   (aflt_tl && flt_tile_sample(aflt_tl, lat, lon, &h)) ||
				   (aflt_bl && flt_tile_sample(aflt_bl, lat, lon, &h)) ||
				   (aflt_tr && flt_tile_sample(aflt_tr, lat, lon, &h)) ||
				   (aflt_br && flt_tile_sample(aflt_br, lat, lon, &h)))
				{
					terrain_tile_set(ter, m, n, h);
				}
			}
		}
	}

	if(terrain_tile_export(ter, output) == 0)
	{
		goto fail_export;
	}

	terrain_tile_delete(&ter);
	tile_job_delete(&job);

	// success
	return;

	// failure
	fail_export:
		terrain_tile_delete(&ter);
	fail_terrain:
	{
		LOGE("%i/%i/%i: error",
		     job->zoom, job->x, job->y);
		tile_job_delete(&job);
		error = 1;
	}
}

static int
sample_tile_range(int x0, int y0, int x1, int y1,
                  int zoom, int complete)
{
	// sample tiles whose origin should be in flt_cc
	int x;
	int y;
	for(y = y0; y <= y1; ++y)
	{
		for(x = x0; x <= x1; ++x)
		{
			tile_job_t* job;
			job = tile_job_new(zoom, x, y, complete);
			if(job == NULL)
			{
				return 0;
			}

			if(cc_jobq_run(jobq, (void*) job) == 0)
			{
				tile_job_delete(&job);
				return 0;
			}
		}
	}

	cc_jobq_finish(jobq);
	return error ? 0 : 1;
}

int main(int argc, char** argv)
{
	if(argc != 7)
	{
		LOGE("usage: %s [zoom] [latT] [lonL] [latB] [lonR] [output]",
		     argv[0]);
		return EXIT_FAILURE;
	}

	int zoom = (int) strtol(argv[1], NULL, 0);
	int latT = (int) strtol(argv[2], NULL, 0);
	int lonL = (int) strtol(argv[3], NULL, 0);
	int latB = (int) strtol(argv[4], NULL, 0);
	int lonR = (int) strtol(argv[5], NULL, 0);

	// initialize output path
	output = argv[6];

	// check for supported zoom levels
	if((zoom == 15) || (zoom == 13))
	{
		// continue
	}
	else
	{
		LOGE("zoom must be 15 (USGS + ASTERv3) or 13 (ASTERv3)");
		return EXIT_FAILURE;
	}

	// initialize jobq
	jobq = cc_jobq_new(NULL, 4,
	                   CC_JOBQ_THREAD_PRIORITY_DEFAULT,
	                   sample_tile_run);
	if(jobq == NULL)
	{
		return EXIT_FAILURE;
	}

	double t0 = cc_timestamp();
	double t1 = cc_timestamp();
	double t2 = cc_timestamp();

	int lati;
	int lonj;
	int idx   = 1;
	int count = (latT - latB + 1)*(lonR - lonL + 1);
	for(lati = latB; lati <= latT; ++lati)
	{
		for(lonj = lonL; lonj <= lonR; ++lonj)
		{
			// status message
			t2 = cc_timestamp();
			LOGI("%i/%i: dt=%lf/%lf, lat=%i, lon=%i",
			     idx, count, t2 - t1, t2 - t0, lati, lonj);
			++idx;
			t1 = t2;

			// initialize flt center
			// only sample USGS for z15
			if((uflt_cc == NULL) && (zoom == 15))
			{
				uflt_cc = flt_tile_import(FLT_TILE_TYPE_USGS,
				                          lati, lonj);
			}

			// uflt_cc may be NULL for sparse data
			// only sample USGS for z15
			int sample   = 0;
			int complete = 0;
			if(uflt_cc && (zoom == 15))
			{
				// initialize flt boundary
				if(uflt_tl == NULL)
				{
					uflt_tl = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati + 1, lonj - 1);
				}
				if(uflt_tc == NULL)
				{
					uflt_tc = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati + 1, lonj);
				}
				if(uflt_tr == NULL)
				{
					uflt_tr = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati + 1, lonj + 1);
				}
				if(uflt_cl == NULL)
				{
					uflt_cl = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati, lonj - 1);
				}
				if(uflt_cr == NULL)
				{
					uflt_cr = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati, lonj + 1);
				}
				if(uflt_bl == NULL)
				{
					uflt_bl = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati - 1, lonj - 1);
				}
				if(uflt_bc == NULL)
				{
					uflt_bc = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati - 1, lonj);
				}
				if(uflt_br == NULL)
				{
					uflt_br = flt_tile_import(FLT_TILE_TYPE_USGS,
					                          lati - 1, lonj + 1);
				}

				sample   = 1;
				complete = uflt_tl && uflt_cl && uflt_bl &&
				           uflt_tc && uflt_cc && uflt_bc &&
				           uflt_tr && uflt_cr && uflt_br;
			}

			// only sample ASTERv3 when USGS is not complete
			// or for z13
			if(((aflt_cc == NULL) && uflt_cc && (complete == 0)) ||
			   ((aflt_cc == NULL) && (zoom == 13)))
			{
				aflt_cc = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
				                          lati, lonj);
			}

			// aflt_cc may be NULL for sparse data
			// only sample ASTERv3 when USGS is not complete
			// or for z13
			if((uflt_cc && (complete == 0)) ||
			   (aflt_cc && (zoom == 13)))
			{
				// initialize flt boundary
				if(aflt_tl == NULL)
				{
					aflt_tl = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati + 1, lonj - 1);
				}
				if(aflt_tc == NULL)
				{
					aflt_tc = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati + 1, lonj);
				}
				if(aflt_tr == NULL)
				{
					aflt_tr = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati + 1, lonj + 1);
				}
				if(aflt_cl == NULL)
				{
					aflt_cl = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati, lonj - 1);
				}
				if(aflt_cr == NULL)
				{
					aflt_cr = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati, lonj + 1);
				}
				if(aflt_bl == NULL)
				{
					aflt_bl = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati - 1, lonj - 1);
				}
				if(aflt_bc == NULL)
				{
					aflt_bc = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati - 1, lonj);
				}
				if(aflt_br == NULL)
				{
					aflt_br = flt_tile_import(FLT_TILE_TYPE_ASTERV3,
					                          lati - 1, lonj + 1);
				}

				sample = 1;
			}

			if(sample)
			{
				// when sampling z15 we want to ensure there are no
				// cracks in z13 when merging USGS with ASTERv3

				// sample z13 tiles whose origin should be in flt_cc
				float x0f;
				float y0f;
				float x1f;
				float y1f;
				terrain_coord2tile((double) lati,
				                   (double) lonj,
				                   13,
				                   &x0f,
				                   &y0f);
				terrain_coord2tile((double) (lati - 1),
				                   (double) (lonj + 1),
				                   13,
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

				// convert z13 tiles to z15 tiles
				if(zoom == 15)
				{
					x0 = 4*x0;
					y0 = 4*y0;
					x1 = 4*x1 + 3;
					y1 = 4*y1 + 3;
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

				// sample the set of tiles whose origin should cover
				// flt_cc again, due to overlap with other flt tiles
				// the sampling actually occurs over the entire flt_xx
				// set
				if(sample_tile_range(x0, y0, x1, y1, zoom,
				                     complete) == 0)
				{
					goto fail_sample;
				}
			}

			// next step, shift right
			flt_tile_delete(&uflt_tl);
			flt_tile_delete(&uflt_cl);
			flt_tile_delete(&uflt_bl);
			flt_tile_delete(&aflt_tl);
			flt_tile_delete(&aflt_cl);
			flt_tile_delete(&aflt_bl);
			uflt_tl = uflt_tc;
			uflt_cl = uflt_cc;
			uflt_bl = uflt_bc;
			uflt_tc = uflt_tr;
			uflt_cc = uflt_cr;
			uflt_bc = uflt_br;
			aflt_tl = aflt_tc;
			aflt_cl = aflt_cc;
			aflt_bl = aflt_bc;
			aflt_tc = aflt_tr;
			aflt_cc = aflt_cr;
			aflt_bc = aflt_br;
			uflt_tr = NULL;
			uflt_cr = NULL;
			uflt_br = NULL;
			aflt_tr = NULL;
			aflt_cr = NULL;
			aflt_br = NULL;
		}

		// next lati
		flt_tile_delete(&uflt_tl);
		flt_tile_delete(&uflt_cl);
		flt_tile_delete(&uflt_bl);
		flt_tile_delete(&uflt_tc);
		flt_tile_delete(&uflt_cc);
		flt_tile_delete(&uflt_bc);
		flt_tile_delete(&uflt_tr);
		flt_tile_delete(&uflt_cr);
		flt_tile_delete(&uflt_br);
		flt_tile_delete(&aflt_tl);
		flt_tile_delete(&aflt_cl);
		flt_tile_delete(&aflt_bl);
		flt_tile_delete(&aflt_tc);
		flt_tile_delete(&aflt_cc);
		flt_tile_delete(&aflt_bc);
		flt_tile_delete(&aflt_tr);
		flt_tile_delete(&aflt_cr);
		flt_tile_delete(&aflt_br);
	}

	cc_jobq_delete(&jobq);

	// success
	LOGI("SUCCESS: dt=%lf", cc_timestamp() - t0);
	return EXIT_SUCCESS;

	// failure
	fail_sample:
		cc_jobq_finish(jobq);
		flt_tile_delete(&uflt_tl);
		flt_tile_delete(&uflt_cl);
		flt_tile_delete(&uflt_bl);
		flt_tile_delete(&uflt_tc);
		flt_tile_delete(&uflt_cc);
		flt_tile_delete(&uflt_bc);
		flt_tile_delete(&uflt_tr);
		flt_tile_delete(&uflt_cr);
		flt_tile_delete(&uflt_br);
		flt_tile_delete(&aflt_tl);
		flt_tile_delete(&aflt_cl);
		flt_tile_delete(&aflt_bl);
		flt_tile_delete(&aflt_tc);
		flt_tile_delete(&aflt_cc);
		flt_tile_delete(&aflt_bc);
		flt_tile_delete(&aflt_tr);
		flt_tile_delete(&aflt_cr);
		flt_tile_delete(&aflt_br);
		cc_jobq_delete(&jobq);
		LOGE("FAILURE: dt=%lf", cc_timestamp() - t0);
	return EXIT_FAILURE;
}
