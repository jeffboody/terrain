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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_TAG "maketerrain"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "libcc/cc_timestamp.h"
#include "terrain/terrain_util.h"
#include "mk_state.h"

#define MB (1024*1024)

/***********************************************************
* protected                                                *
***********************************************************/

extern void terrain_tile_set(terrain_tile_t* self,
                             int m, int n,
                             short h);

/***********************************************************
* private                                                  *
***********************************************************/

static int
mk_state_existsFlt(mk_state_t* self, int type,
                   int lat, int lon)
{
	ASSERT(self);

	// USGS is top-left origin
	// ASTERv3 is bottom-left origin
	char fname[256];
	if(type == FLT_TILE_TYPE_USGS)
	{
		int ulat = lat + 1;
		char flt_fbase[256];
		snprintf(flt_fbase, 256, "%s%i%s%03i",
		         (ulat >= 0) ? "n" : "s", abs(ulat),
		         (lon >= 0) ? "e" : "w", abs(lon));
		snprintf(fname, 256, "usgs-ned/data/%s/float%s_13.hdr",
		         flt_fbase, flt_fbase);
	}
	else
	{
		snprintf(fname, 256,
		         "ASTERv3/data/ASTGTMV003_%s%02i%s%03i_dem.tif",
		         (lat >= 0) ? "N" : "S", abs(lat),
		         (lon >= 0) ? "E" : "W", abs(lon));
	}

	if(access(fname, F_OK) == 0)
	{
		return 1;
	}

	return 0;
}

static void
mk_state_evictObject(mk_state_t* self,
                     cc_listIter_t** _iter)
{
	ASSERT(self);
	ASSERT(_iter);

	cc_listIter_t* iter = *_iter;
	if(iter)
	{
		mk_object_t* obj;
		obj = (mk_object_t*)
		      cc_list_remove(self->obj_list, _iter);
		ASSERT(mk_object_refcount(obj) == 0);

		// find/remove object from map
		char          key[256];
		cc_mapIter_t  miterator;
		cc_mapIter_t* miter = &miterator;
		mk_object_key(obj, key);
		if(cc_map_find(self->obj_map, miter, key))
		{
			ASSERT(obj == cc_map_val(miter));
			cc_map_remove(self->obj_map, &miter);
		}
		else
		{
			LOGE("invalid");
		}

		mk_object_delete(&obj);
	}
}

static void mk_state_trim13(mk_state_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter = cc_list_head(self->obj_list);
	size_t max = (size_t) 4000*MB;
	while(iter)
	{
		if(MEMSIZE() < max)
		{
			return;
		}

		// try to evict object
		mk_object_t* obj;
		obj = (mk_object_t*) cc_list_peekIter(iter);
		if(mk_object_refcount(obj))
		{
			iter = cc_list_next(iter);
			continue;
		}

		mk_state_evictObject(self, &iter);
	}
}

static mk_object_t*
mk_state_findTerrain(mk_state_t* self,
                     int x, int y, int zoom)
{
	ASSERT(self);

	cc_mapIter_t   miterator;
	cc_listIter_t* iter;
	iter = (cc_listIter_t*)
	       cc_map_findf(self->obj_map, &miterator,
	                    "T/%i/%i/%i", zoom, x, y);
	if(iter == NULL)
	{
		return NULL;
	}

	// update LRU
	cc_list_moven(self->obj_list, iter, NULL);

	return (mk_object_t*) cc_list_peekIter(iter);
}

static cc_listIter_t*
mk_state_newTerrain(mk_state_t* self,
                    int x, int y, int zoom)
{
	ASSERT(self);

	mk_object_t* obj = mk_object_newTerrain(x, y, zoom);
	if(obj == NULL)
	{
		return NULL;
	}

	cc_listIter_t* iter;
	iter = cc_list_append(self->obj_list, NULL,
	                      (const void*) obj);
	if(iter == NULL)
	{
		goto fail_list;
	}

	if(cc_map_addf(self->obj_map, (const void*) iter,
	               "T/%i/%i/%i", zoom, x, y) == 0)
	{
		goto fail_map;
	}

	// success
	return iter;

	// failure
	fail_map:
		cc_list_remove(self->obj_list, &iter);
	fail_list:
		mk_object_delete(&obj);
	return NULL;
}

static mk_object_t*
mk_state_importTerrain(mk_state_t* self,
                       int x, int y, int zoom)
{
	ASSERT(self);

	// avoid error message if file doesn't exist
	// since flt files are sparse
	char fname[256];
	snprintf(fname, 256, "%s/terrainv1/%i/%i/%i.terrain",
	         self->path, zoom, x, y);
	if(access(fname, F_OK) != 0)
	{
		return NULL;
	}

	mk_object_t* obj;
	obj = mk_object_importTerrain(self->path, x, y, zoom);
	if(obj == NULL)
	{
		return NULL;
	}

	cc_listIter_t* iter;
	iter = cc_list_append(self->obj_list, NULL,
	                      (const void*) obj);
	if(iter == NULL)
	{
		goto fail_list;
	}

	if(cc_map_addf(self->obj_map, (const void*) iter,
	               "T/%i/%i/%i", zoom, x, y) == 0)
	{
		goto fail_map;
	}

	// success
	return obj;

	// failure
	fail_map:
		cc_list_remove(self->obj_list, &iter);
	fail_list:
		mk_object_delete(&obj);
	return NULL;
}

static mk_object_t*
mk_state_findFlt(mk_state_t* self,
                 int type, int lat, int lon)
{
	ASSERT(self);

	cc_mapIter_t   miterator;
	cc_listIter_t* iter;
	iter = (cc_listIter_t*)
	       cc_map_findf(self->obj_map, &miterator,
	                    "F/%i/%i/%i", type, lat, lon);
	if(iter == NULL)
	{
		return NULL;
	}

	// update LRU
	cc_list_moven(self->obj_list, iter, NULL);

	return (mk_object_t*) cc_list_peekIter(iter);
}

static mk_object_t*
mk_state_getFlt(mk_state_t* self, int type,
                int lat, int lon)
{
	ASSERT(self);

	// check if the object is cached
	mk_object_t* obj;
	obj = mk_state_findFlt(self, type, lat, lon);
	if(obj)
	{
		return obj;
	}

	// USGS is top-left origin
	// ASTERv3 is bottom-left origin
	char fname[256];
	if(type == FLT_TILE_TYPE_USGS)
	{
		int ulat = lat + 1;
		char flt_fbase[256];
		snprintf(flt_fbase, 256, "%s%i%s%03i",
		         (ulat >= 0) ? "n" : "s", abs(ulat),
		         (lon >= 0) ? "e" : "w", abs(lon));
		snprintf(fname, 256, "usgs-ned/data/%s/float%s_13.hdr",
		         flt_fbase, flt_fbase);
	}
	else
	{
		snprintf(fname, 256,
		         "ASTERv3/data/ASTGTMV003_%s%02i%s%03i_dem.tif",
		         (lat >= 0) ? "N" : "S", abs(lat),
		         (lon >= 0) ? "E" : "W", abs(lon));
	}

	// avoid error message if file doesn't exist
	// since flt files are sparse
	if(mk_state_existsFlt(self, type, lat, lon) == 0)
	{
		return NULL;
	}

	// import the object
	obj = mk_object_importFlt(type, lat, lon);
	if(obj == NULL)
	{
		return NULL;
	}

	cc_listIter_t* iter;
	iter = cc_list_append(self->obj_list, NULL,
	                      (const void*) obj);
	if(iter == NULL)
	{
		goto fail_list;
	}

	if(cc_map_addf(self->obj_map, (const void*) iter,
	               "F/%i/%i/%i", type, lat, lon) == 0)
	{
		goto fail_map;
	}

	// success
	return obj;

	// failure
	fail_map:
		cc_list_remove(self->obj_list, &iter);
	fail_list:
		mk_object_delete(&obj);
	return NULL;
}

static int
mk_state_prefetch13(mk_state_t* self, int x, int y)
{
	ASSERT(self);

	self->count += 1.0;

	self->cnt_usgs  = 0;
	self->cnt_aster = 0;

	// get bounds and select origin of the terrain tile
	double latT;
	double lonL;
	double latB;
	double lonR;
	terrain_bounds(x, y, 13, &latT, &lonL, &latB, &lonR);
	int lat = (int) latT;
	int lon = (int) lonL;

	// check if flt exists for surrounding USGS tiles
	int row;
	int col;
	int idx  = 0;
	int lat0 = lat - 1;
	int lon0 = lon - 1;
	int lat1 = lat + 1;
	int lon1 = lon + 1;
	double dt = cc_timestamp() - self->t0;
	LOGI("13/%i/%i: lat=%i, lon=%i, dt=%0.3lf, mem=%0.lf MB, %0.1lf%%",
	     x, y, lat, lon, dt, (double) (MEMSIZE()/MB),
	     100.0*self->count/self->total);
	for(row = lat0; row <= lat1; ++row)
	{
		for(col = lon0; col <= lon1; ++col)
		{
			self->obj_usgs[idx] = mk_state_getFlt(self,
			                                      FLT_TILE_TYPE_USGS,
			                                      row, col);
			if(self->obj_usgs[idx])
			{
				++idx;
			}
		}
	}
	self->cnt_usgs = idx;

	// proceed to z15 if z13 completly covered USGS
	if(self->cnt_usgs == 9)
	{
		return 15;
	}

	idx = 0;
	for(row = lat0; row <= lat1; ++row)
	{
		for(col = lon0; col <= lon1; ++col)
		{
			self->obj_aster[idx] = mk_state_getFlt(self,
			                                       FLT_TILE_TYPE_ASTERV3,
			                                       row, col);
			if(self->obj_aster[idx])
			{
				++idx;
			}
		}
	}
	self->cnt_aster = idx;

	// proceed to z15 if z13 partially covered by USGS
	// or fall back to z13 if covered by ASTERv3
	if(self->cnt_usgs)
	{
		return 15;
	}
	else if(self->cnt_aster)
	{
		return 13;
	}

	return 0;
}

static mk_object_t*
mk_state_make(mk_state_t* self, int x, int y, int zoom)
{
	ASSERT(self);

	// create a new object
	cc_listIter_t* iter = NULL;
	iter = mk_state_newTerrain(self, x, y, zoom);
	if(iter == NULL)
	{
		return NULL;
	}

	mk_object_t* obj;
	obj = (mk_object_t*) cc_list_peekIter(iter);

	int m;
	int n;
	double latT;
	double lonL;
	double latB;
	double lonR;
	int    min = -TERRAIN_SAMPLES_BORDER;
	int    max = TERRAIN_SAMPLES_TILE + TERRAIN_SAMPLES_BORDER;
	double d   = (double) (max - min - 1);
	terrain_tile_coord(obj->terrain, min, min, &latT, &lonL);
	terrain_tile_coord(obj->terrain, max - 1, max - 1, &latB, &lonR);
	for(m = min; m < max; ++m)
	{
		for(n = min; n < max; ++n)
		{
			double u   = ((double) (n - min))/d;
			double v   = ((double) (m - min))/d;
			double lat = latT + v*(latB - latT);
			double lon = lonL + u*(lonR - lonL);

			// try to sample ASTERv3
			flt_tile_t* flt;
			short h;
			int   idx;
			for(idx = 0; idx < self->cnt_aster; ++idx)
			{
				flt = self->obj_aster[idx]->flt;
				if(flt_tile_sample(flt, lat, lon, &h))
				{
					terrain_tile_set(obj->terrain, m, n, h);
					break;
				}
			}

			// try to sample USGS
			for(idx = 0; idx < self->cnt_usgs; ++idx)
			{
				flt = self->obj_usgs[idx]->flt;
				if(flt_tile_sample(flt, lat, lon, &h))
				{
					terrain_tile_set(obj->terrain, m, n, h);
					break;
				}
			}
		}
	}

	if(mk_object_exportTerrain(obj, self->path) == 0)
	{
		goto fail_export;
	}

	mk_object_incref(obj);

	// success
	return obj;

	// failure
	fail_export:
		mk_state_evictObject(self, &iter);
	return NULL;
}

/***********************************************************
* public                                                   *
***********************************************************/

mk_state_t*
mk_state_new(int latT, int lonL, int latB, int lonR,
             const char* path)
{
	ASSERT(path);

	mk_state_t* self;
	self = CALLOC(1, sizeof(mk_state_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	float xtl;
	float ytl;
	float xbr;
	float ybr;
	terrain_coord2tile(latT, lonL, 13, &xtl, &ytl);
	terrain_coord2tile(latB, lonR, 13, &xbr, &ybr);
	double w = (double) (xbr - xtl);
	double h = (double) (ybr - ytl);

	self->latT  = latT;
	self->lonL  = lonL;
	self->latB  = latB;
	self->lonR  = lonR;
	self->t0    = cc_timestamp();
	self->total = w*h;
	self->path  = path;

	LOGI("latT=%i, lonL=%i, latB=%i, lonR=%i, path=%s, total=%lf",
	     latT, lonL, latB, lonR, path, self->total);

	self->obj_map = cc_map_new();
	if(self->obj_map == NULL)
	{
		goto fail_obj_map;
	}

	self->obj_list = cc_list_new();
	if(self->obj_list == NULL)
	{
		goto fail_obj_list;
	}

	self->null_map = cc_map_new();
	if(self->null_map == NULL)
	{
		goto fail_null_map;
	}

	// success
	return self;

	// failure
	fail_null_map:
		cc_list_delete(&self->obj_list);
	fail_obj_list:
		cc_map_delete(&self->obj_map);
	fail_obj_map:
		FREE(self);
	return NULL;
}

void mk_state_delete(mk_state_t** _self)
{
	ASSERT(_self);

	mk_state_t* self = *_self;
	if(self)
	{
		cc_mapIter_t  miterator;
		cc_mapIter_t* miter;
		miter = cc_map_head(self->obj_map, &miterator);
		while(miter)
		{
			cc_listIter_t* iter;
			iter = (cc_listIter_t*)
			       cc_map_val(miter);

			mk_object_t* obj;
			obj = (mk_object_t*) cc_list_peekIter(iter);
			cc_list_remove(self->obj_list, &iter);
			cc_map_remove(self->obj_map, &miter);
			mk_object_delete(&obj);
		}

		cc_map_discard(self->null_map);
		cc_map_delete(&self->null_map);
		cc_list_delete(&self->obj_list);
		cc_map_delete(&self->obj_map);
		FREE(self);
		*_self = NULL;
	}
}

void mk_state_put(mk_state_t* self, mk_object_t** _obj)
{
	ASSERT(self);
	ASSERT(_obj);

	mk_object_t* obj = *_obj;
	if(obj)
	{
		ASSERT(obj->type == MK_OBJECT_TYPE_TERRAIN);
		mk_object_decref(obj);
		*_obj = NULL;
	}
}

mk_object_t*
mk_state_getTerrain(mk_state_t* self,
                    int x, int y, int zoom)
{
	ASSERT(self);

	// check range
	int range = (int) pow(2.0, (double) zoom);
	if((x < 0)      || (y < 0) ||
	   (x >= range) || (y >= range))
	{
		return NULL;
	}

	// clip tile
	double latT;
	double lonL;
	double latB;
	double lonR;
	terrain_bounds(x, y, zoom, &latT, &lonL, &latB, &lonR);
	if((self->latT < latB) || (self->lonL > lonR) ||
	   (self->latB > latT) || (self->lonR < lonL))
	{
		return NULL;
	}

	// check if the object is cached
	mk_object_t* obj;
	obj = mk_state_findTerrain(self, x, y, zoom);
	if(obj)
	{
		mk_object_incref(obj);
		if(zoom == 13)
		{
			mk_state_trim13(self);
		}
		return obj;
	}

	// check if the object is null
	if(zoom <= 13)
	{
		cc_mapIter_t miterator;
		if(cc_map_findf(self->null_map, &miterator,
		                "%i/%i/%i", zoom, x, y))
		{
			return NULL;
		}
	}

	// check if the object was created
	// note: this z13 check isn't normally necessary however
	// due to an unknown error while processing the terrainv1
	// data these files cannot be trusted and must be
	// recreated if the z13 level is not found
	if(zoom <= 13)
	{
		obj = mk_state_importTerrain(self, x, y, zoom);
		if(obj)
		{
			mk_object_incref(obj);
			if(zoom == 13)
			{
				mk_state_trim13(self);
			}
			return obj;
		}
	}

	// end recursion
	if(zoom == 15)
	{
		return mk_state_make(self, x, y, zoom);
	}
	else if(zoom == 13)
	{
		int prefetch = mk_state_prefetch13(self, x, y);
		if(prefetch == 13)
		{
			obj = mk_state_make(self, x, y, zoom);
			mk_state_trim13(self);
			return obj;
		}
		else if(prefetch == 0)
		{
			cc_map_addf(self->null_map,
			            (const void*) &self->null_val,
			            "%i/%i/%i", zoom, x, y);
			mk_state_trim13(self);
			return NULL;
		}
		else
		{
			// otherwise get next LOD
		}
	}

	// get surrounding tiles in the next zoom level
	int r;
	int c;
	int idx;
	int done = 1;
	int xx = 2*x;
	int yy = 2*y;
	int zz = zoom + 1;
	mk_object_t* next[16] =
	{
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
	};
	for(r = 0; r < 4; ++r)
	{
		for(c = 0; c < 4; ++c)
		{
			idx = 4*r + c;
			next[idx] = mk_state_getTerrain(self,
			                                xx + c - 1,
			                                yy + r - 1, zz);
			if(next[idx])
			{
				done = 0;
			}
		}
	}

	// check if sampling can be performed
	if(done)
	{
		if(zoom <= 13)
		{
			cc_map_addf(self->null_map,
			            (const void*) &self->null_val,
			            "%i/%i/%i", zoom, x, y);
			if(zoom == 13)
			{
				mk_state_trim13(self);
			}
		}
		return NULL;
	}

	// create a new object
	cc_listIter_t* iter = NULL;
	iter = mk_state_newTerrain(self, x, y, zoom);
	if(iter == NULL)
	{
		return NULL;
	}
	obj = (mk_object_t*) cc_list_peekIter(iter);

	// sample the next LOD
	mk_object_sample00(obj, next[0]);
	mk_object_sample01(obj, next[1]);
	mk_object_sample02(obj, next[2]);
	mk_object_sample03(obj, next[3]);
	mk_object_sample10(obj, next[4]);
	mk_object_sample11(obj, next[5]);
	mk_object_sample12(obj, next[6]);
	mk_object_sample13(obj, next[7]);
	mk_object_sample20(obj, next[8]);
	mk_object_sample21(obj, next[9]);
	mk_object_sample22(obj, next[10]);
	mk_object_sample23(obj, next[11]);
	mk_object_sample30(obj, next[12]);
	mk_object_sample31(obj, next[13]);
	mk_object_sample32(obj, next[14]);
	mk_object_sample33(obj, next[15]);

	// export the object
	if(mk_object_exportTerrain(obj, self->path) == 0)
	{
		goto fail_export;
	}

	// update refcounts
	for(r = 0; r < 4; ++r)
	{
		for(c = 0; c < 4; ++c)
		{
			idx = 4*r + c;
			mk_state_put(self, &next[idx]);
		}
	}
	mk_object_incref(obj);

	// trim cache
	if(zoom == 13)
	{
		mk_state_trim13(self);
	}

	// success
	return obj;

	// failure
	fail_export:
	{
		// put objects
		for(r = 0; r < 4; ++r)
		{
			for(c = 0; c < 4; ++c)
			{
				idx = 4*r + c;
				mk_state_put(self, &next[idx]);
			}
		}
		mk_state_evictObject(self, &iter);

		// trim cache
		if(zoom == 13)
		{
			mk_state_trim13(self);
		}
	}
	return NULL;
}
