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

#include <stdlib.h>

#define LOG_TAG "maketerrain"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "mk_object.h"

/***********************************************************
* protected                                                *
***********************************************************/

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

/***********************************************************
* public                                                   *
***********************************************************/

mk_object_t* mk_object_newTerrain(int x, int y, int zoom)
{
	mk_object_t* self;
	self = (mk_object_t*)
	       CALLOC(1, sizeof(mk_object_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->type = MK_OBJECT_TYPE_TERRAIN;

	self->terrain = terrain_tile_new(x, y, zoom);
	if(self->terrain == NULL)
	{
		goto fail_terrain;
	}

	// success
	return self;

	// failure
	fail_terrain:
		FREE(self);
	return NULL;
}

mk_object_t*
mk_object_importTerrain(const char* base,
                        int x, int y, int zoom)
{
	mk_object_t* self;
	self = (mk_object_t*)
	       CALLOC(1, sizeof(mk_object_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->type = MK_OBJECT_TYPE_TERRAIN;

	self->terrain = terrain_tile_import(base, x, y, zoom);
	if(self->terrain == NULL)
	{
		goto fail_terrain;
	}

	// success
	return self;

	// failure
	fail_terrain:
		FREE(self);
	return NULL;
}

mk_object_t*
mk_object_importFlt(int type, int lat, int lon)
{
	mk_object_t* self;
	self = (mk_object_t*)
	       CALLOC(1, sizeof(mk_object_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->type = MK_OBJECT_TYPE_FLT;

	self->flt = flt_tile_import(type, lat, lon);
	if(self->flt == NULL)
	{
		goto fail_flt;
	}

	// success
	return self;

	// failure
	fail_flt:
		FREE(self);
	return NULL;
}

void mk_object_delete(mk_object_t** _self)
{
	ASSERT(_self);

	mk_object_t* self = *_self;
	if(self)
	{
		if(self->type == MK_OBJECT_TYPE_TERRAIN)
		{
			terrain_tile_delete(&self->terrain);
		}
		else
		{
			flt_tile_delete(&self->flt);
		}

		FREE(self);
		*_self = NULL;
	}
}

void mk_object_incref(mk_object_t* self)
{
	ASSERT(self);

	++self->refcount;
}

int mk_object_decref(mk_object_t* self)
{
	ASSERT(self);

	--self->refcount;
	return (self->refcount == 0) ? 1 : 0;
}

int mk_object_refcount(mk_object_t* self)
{
	ASSERT(self);

	return self->refcount;
}

int mk_object_exportTerrain(mk_object_t* self,
                            const char* base)
{
	ASSERT(self);
	ASSERT(self->type == MK_OBJECT_TYPE_TERRAIN);
	ASSERT(base);

	return terrain_tile_export(self->terrain, base);
}

void mk_object_key(mk_object_t* self, char* key)
{
	ASSERT(self);
	ASSERT(key);

	if(self->type == MK_OBJECT_TYPE_TERRAIN)
	{
		snprintf(key, 256, "T/%i/%i/%i",
		         self->terrain->zoom,
		         self->terrain->x,
		         self->terrain->y);
	}
	else
	{
		snprintf(key, 256, "F/%i/%i/%i", self->flt->type,
		         self->flt->lat, self->flt->lon);
	}
}

void mk_object_sample00(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// top-left border sample
	short h = terrain_tile_get(next->terrain,
	                           TERRAIN_SAMPLES_TILE - 3,
	                           TERRAIN_SAMPLES_TILE - 3);
	terrain_tile_set(self->terrain, -1, -1, h);
}

void mk_object_sample01(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

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
		h = terrain_tile_get(next->terrain,
		                     TERRAIN_SAMPLES_TILE - 3,
		                     nn);
		terrain_tile_set(self->terrain, -1, n++, h);
	}
}

void mk_object_sample02(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

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
		h = terrain_tile_get(next->terrain,
		                     TERRAIN_SAMPLES_TILE - 3,
		                     nn);
		terrain_tile_set(self->terrain, -1, n++, h);
	}
}

void mk_object_sample03(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// top-right border sample
	short h;
	h = terrain_tile_get(next->terrain,
	                     TERRAIN_SAMPLES_TILE - 3,
	                     2);
	terrain_tile_set(self->terrain, -1, 257, h);
}

void mk_object_sample10(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

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
		h = terrain_tile_get(next->terrain, mm,
		                     TERRAIN_SAMPLES_TILE - 3);
		terrain_tile_set(self->terrain, m++, -1, h);
	}
}

void mk_object_sample11(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		terrain_tile_adjustMinMax(self->terrain, 0, 0);
		return;
	}

	short min = terrain_tile_min(next->terrain);
	short max = terrain_tile_max(next->terrain);
	terrain_tile_adjustMinMax(self->terrain, min, max);
	terrain_tile_exists(self->terrain, TERRAIN_NEXT_TL);

	// center border samples
	int mm;
	int nn;
	int m = 0;
	int n = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next->terrain, mm, nn);
			terrain_tile_set(self->terrain, m, n++, h);
		}
		++m;
		n = 0;
	}
}

void mk_object_sample12(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		terrain_tile_adjustMinMax(self->terrain, 0, 0);
		return;
	}

	short min = terrain_tile_min(next->terrain);
	short max = terrain_tile_max(next->terrain);
	terrain_tile_adjustMinMax(self->terrain, min, max);
	terrain_tile_exists(self->terrain, TERRAIN_NEXT_TR);

	// center border samples
	int mm;
	int nn;
	int m = 0;
	int n = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next->terrain, mm, nn);
			terrain_tile_set(self->terrain, m, n++, h);
		}
		++m;
		n = 128;
	}
}

void mk_object_sample13(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// right border samples
	int mm;
	int m = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h = terrain_tile_get(next->terrain, mm, 2);
		terrain_tile_set(self->terrain, m++, 257, h);
	}
}

void mk_object_sample20(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

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
		h = terrain_tile_get(next->terrain, mm,
		                     TERRAIN_SAMPLES_TILE - 3);
		terrain_tile_set(self->terrain, m++, -1, h);
	}
}

void mk_object_sample21(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		terrain_tile_adjustMinMax(self->terrain, 0, 0);
		return;
	}

	short min = terrain_tile_min(next->terrain);
	short max = terrain_tile_max(next->terrain);
	terrain_tile_adjustMinMax(self->terrain, min, max);
	terrain_tile_exists(self->terrain, TERRAIN_NEXT_BL);

	// center border samples
	int mm;
	int nn;
	int m = 128;
	int n = 0;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next->terrain, mm, nn);
			terrain_tile_set(self->terrain, m, n++, h);
		}
		++m;
		n = 0;
	}
}

void mk_object_sample22(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		terrain_tile_adjustMinMax(self->terrain, 0, 0);
		return;
	}

	short min = terrain_tile_min(next->terrain);
	short max = terrain_tile_max(next->terrain);
	terrain_tile_adjustMinMax(self->terrain, min, max);
	terrain_tile_exists(self->terrain, TERRAIN_NEXT_BR);

	// center border samples
	int mm;
	int nn;
	int m = 128;
	int n = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
		{
			short h = terrain_tile_get(next->terrain, mm, nn);
			terrain_tile_set(self->terrain, m, n++, h);
		}
		++m;
		n = 128;
	}
}

void mk_object_sample23(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// right border samples
	int mm;
	int m = 128;
	for(mm = 0; mm < TERRAIN_SAMPLES_TILE; mm += 2)
	{
		short h = terrain_tile_get(next->terrain, mm, 2);
		terrain_tile_set(self->terrain, m++, 257, h);
	}
}

void mk_object_sample30(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// bottom-left border sample
	short h;
	h = terrain_tile_get(next->terrain, 2,
	                     TERRAIN_SAMPLES_TILE - 3);
	terrain_tile_set(self->terrain, 257, -1, h);
}

void mk_object_sample31(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// bottom border samples
	int nn;
	int n = 0;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h = terrain_tile_get(next->terrain, 2, nn);
		terrain_tile_set(self->terrain, 257, n++, h);
	}
}

void mk_object_sample32(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// bottom border samples
	int nn;
	int n = 128;
	for(nn = 0; nn < TERRAIN_SAMPLES_TILE; nn += 2)
	{
		short h = terrain_tile_get(next->terrain, 2, nn);
		terrain_tile_set(self->terrain, 257, n++, h);
	}
}

void mk_object_sample33(mk_object_t* self, mk_object_t* next)
{
	ASSERT(self);

	if(next == NULL)
	{
		return;
	}

	// bottom-right border sample
	short h = terrain_tile_get(next->terrain, 2, 2);
	terrain_tile_set(self->terrain, 257, 257, h);
}
