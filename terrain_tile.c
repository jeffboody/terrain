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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <zlib.h>
#include "terrain_tile.h"
#include "terrain_util.h"

#define LOG_TAG "terrain"
#include "terrain_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int terrain_mkdir(const char* fname)
{
	assert(fname);
	LOGD("debug fname=%s", fname);

	int  len = strnlen(fname, 255);
	char dir[256];
	int  i;
	for(i = 0; i < len; ++i)
	{
		dir[i]     = fname[i];
		dir[i + 1] = '\0';

		if(dir[i] == '/')
		{
			if(access(dir, R_OK) == 0)
			{
				// dir already exists
				continue;
			}

			// try to mkdir
			if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
			{
				if(errno == EEXIST)
				{
					// already exists
				}
				else
				{
					LOGE("mkdir %s failed", dir);
					return 0;
				}
			}
		}
	}

	return 1;
}

static void terrain_tile_updateMinMax(terrain_tile_t* self)
{
	assert(self);

	// check if the min/max has already been set
	if((self->min != TERRAIN_HEIGHT_MAX) &&
	   (self->max != TERRAIN_HEIGHT_MIN))
	{
		return;
	}

	int   m;
	int   n;
	short h;
	short min = TERRAIN_HEIGHT_MAX;
	short max = TERRAIN_HEIGHT_MIN;
	for(m = 0; m < TERRAIN_SAMPLES_TILE; ++m)
	{
		for(n = 0; n < TERRAIN_SAMPLES_TILE; ++n)
		{
			h = terrain_tile_get(self, m, n);
			if(h < min)
			{
				min = h;
			}
			if(h > max)
			{
				max = h;
			}
		}
	}

	self->min = min;
	self->max = max;
}

static int readintle(const unsigned char* buffer,
                     int offset)
{
	assert(buffer);
	assert(offset >= 0);

	int b0 = (int) buffer[offset + 0];
	int b1 = (int) buffer[offset + 1];
	int b2 = (int) buffer[offset + 2];
	int b3 = (int) buffer[offset + 3];
	int o = (b3 << 24) & 0xFF000000;
	o = o | ((b2 << 16) & 0x00FF0000);
	o = o | ((b1 << 8) & 0x0000FF00);
	o = o | (b0 & 0x000000FF);
	return o;
}

static int readintbe(const unsigned char* buffer,
                     int offset)
{
	assert(buffer);
	assert(offset >= 0);

	int b0 = (int) buffer[offset + 3];
	int b1 = (int) buffer[offset + 2];
	int b2 = (int) buffer[offset + 1];
	int b3 = (int) buffer[offset + 0];
	int o = (b3 << 24) & 0xFF000000;
	o = o | ((b2 << 16) & 0x00FF0000);
	o = o | ((b1 << 8) & 0x0000FF00);
	o = o | (b0 & 0x000000FF);
	return o;
}

static int swapendian(int i)
{
	int o = (i << 24) & 0xFF000000;
	o = o | ((i << 8) & 0x00FF0000);
	o = o | ((i >> 8) & 0x0000FF00);
	o = o | ((i >> 24) & 0x000000FF);
	return o;
}

/***********************************************************
* public                                                   *
***********************************************************/

terrain_tile_t* terrain_tile_new(int x, int y, int zoom)
{
	terrain_tile_t* self = (terrain_tile_t*)
	                       malloc(sizeof(terrain_tile_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	int samples = TERRAIN_SAMPLES_TOTAL*
	              TERRAIN_SAMPLES_TOTAL*
	              sizeof(short);
	memset(self->data, 0, samples);

	self->x    = x;
	self->y    = y;
	self->zoom = zoom;
	self->next = 0;

	// updated on export if not set
	self->min = TERRAIN_HEIGHT_MAX;
	self->max = TERRAIN_HEIGHT_MIN;

	// success
	return self;
}

void terrain_tile_delete(terrain_tile_t** _self)
{
	assert(_self);

	terrain_tile_t* self = *_self;
	if(self)
	{
		free(self);
		*_self = NULL;
	}
}

terrain_tile_t* terrain_tile_import(const char* base,
                                    int x, int y, int zoom)
{
	assert(base);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, zoom, x, y);
	fname[255] = '\0';

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("invalid %s", fname);
		return NULL;
	}

	// get file size including header
	fseek(f, (long) 0, SEEK_END);
	int size = (int) ftell(f);
	rewind(f);

	terrain_tile_t* self;
	self = terrain_tile_importf(f, size, x, y, zoom);
	if(self == NULL)
	{
		goto fail_import;
	}

	fclose(f);

	// success
	return self;

	// failure
	fail_import:
		fclose(f);
	return NULL;
}

terrain_tile_t* terrain_tile_importf(FILE* f, int size,
                                     int x, int y, int zoom)
{
	assert(f);

	terrain_tile_t* self = (terrain_tile_t*)
	                       malloc(sizeof(terrain_tile_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	unsigned char buffer[TERRAIN_HSIZE];
	int bytes_read = fread(buffer, sizeof(unsigned char),
	                       TERRAIN_HSIZE, f);
	if(bytes_read != TERRAIN_HSIZE)
	{
		LOGE("failed to read header");
		goto fail_header;
	}

	// parse the header
	int magic = readintle(buffer, 0);
	if(magic == TERRAIN_MAGIC)
	{
		self->min  = (short) readintle(buffer, 4);
		self->max  = (short) readintle(buffer, 8);
		self->next = (char)  readintle(buffer, 12);
	}
	else if(swapendian(magic) == TERRAIN_MAGIC)
	{
		self->min  = (short) readintbe(buffer, 4);
		self->max  = (short) readintbe(buffer, 8);
		self->next = (char)  readintbe(buffer, 12);
	}

	// allocate src buffer
	size -= TERRAIN_HSIZE;
	char* src = (char*) malloc(size*sizeof(char));
	if(src == NULL)
	{
		LOGE("malloc failed");
		goto fail_src;
	}

	// read the samples
	if(fread((void*) src, sizeof(char), size, f) != size)
	{
		LOGE("fread failed");
		goto fail_read;
	}

	int    bytes    = TERRAIN_SAMPLES_TOTAL*
	                  TERRAIN_SAMPLES_TOTAL*
	                  sizeof(short);
	uLong  dst_size = (uLong) (bytes);
	Bytef* dst      = (Bytef*) self->data;
	uLong  src_size = (uLong) size;
	if(uncompress(dst, &dst_size, (const Bytef*) src,
	              src_size) != Z_OK)
	{
		LOGE("fail uncompress");
		goto fail_uncompress;
	}

	self->x    = x;
	self->y    = y;
	self->zoom = zoom;

	// success
	return self;

	// failure
	fail_uncompress:
	fail_read:
		free(src);
	fail_src:
	fail_header:
		free(self);
	return NULL;
}

int terrain_tile_export(terrain_tile_t* self,
                        const char* base)
{
	assert(self);
	assert(base);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, self->zoom, self->x, self->y);
	fname[255] = '\0';

	if(terrain_mkdir(fname) == 0)
	{
		return 0;
	}

	FILE* f = fopen(fname, "w");
	if(f == NULL)
	{
		LOGE("invalid %s", fname);
		return 0;
	}

	// export the header
	int magic = TERRAIN_MAGIC;
	if(fwrite(&magic, sizeof(int), 1, f) != 1)
	{
		LOGE("fwrite failed");
		goto fail_header;
	}

	// update min/max sample heights
	terrain_tile_updateMinMax(self);

	int min = (int) self->min;
	if(fwrite(&min, sizeof(int), 1, f) != 1)
	{
		LOGE("fwrite failed");
		goto fail_header;
	}

	int max = (int) self->max;
	if(fwrite(&max, sizeof(int), 1, f) != 1)
	{
		LOGE("fwrite failed");
		goto fail_header;
	}

	int next = (int) self->next;
	if(fwrite(&next, sizeof(int), 1, f) != 1)
	{
		LOGE("fwrite failed");
		goto fail_header;
	}

	// allocate dst buffer
	int bytes = TERRAIN_SAMPLES_TOTAL*
	            TERRAIN_SAMPLES_TOTAL*sizeof(short);
	uLong src_size = (uLong) (bytes);
	uLong dst_size = compressBound(src_size);
	unsigned char* dst = (unsigned char*)
	                     malloc(dst_size*sizeof(unsigned char));
	if(dst == NULL)
	{
		LOGE("malloc failed");
		goto fail_dst;
	}

	// compress buffer
	unsigned char* src = (unsigned char*) self->data;
	if(compress((Bytef*) dst, (uLongf*) &dst_size,
	            (const Bytef*) src, src_size) != Z_OK)
	{
		LOGE("compress failed");
		goto fail_compress;
	}

	// write buffer
	if(fwrite(dst, sizeof(unsigned char), dst_size, f) != dst_size)
	{
		LOGE("fwrite failed");
		goto fail_fwrite;
	}

	free(dst);
	fclose(f);

	// success
	return 1;

	// failure
	fail_fwrite:
	fail_compress:
		free(dst);
	fail_dst:
	fail_header:
		fclose(f);
		unlink(fname);
	return 0;
}

void terrain_tile_coord(terrain_tile_t* self,
                        int m, int n,
                        double* lat, double* lon)
{
	assert(self);
	assert(lat);
	assert(lon);

	terrain_sample2coord(self->x, self->y, self->zoom,
	                     m, n, lat, lon);
}

void terrain_tile_set(terrain_tile_t* self,
                      int m, int n,
                      short h)
{
	assert(self);

	// offset indices by border
	m += TERRAIN_SAMPLES_BORDER;
	n += TERRAIN_SAMPLES_BORDER;

	int samples = TERRAIN_SAMPLES_TOTAL;
	if((m < 0) || (m >= samples) ||
	   (n < 0) || (n >= samples))
	{
		LOGW("invalid m=%i, n=%i", m, n);
		return;
	}

	int idx = m*samples + n;
	self->data[idx] = h;
}

short terrain_tile_get(terrain_tile_t* self,
                       int m, int n)
{
	assert(self);

	// offset indices by border
	m += TERRAIN_SAMPLES_BORDER;
	n += TERRAIN_SAMPLES_BORDER;

	int samples = TERRAIN_SAMPLES_TOTAL;
	if((m < 0) || (m >= samples) ||
	   (n < 0) || (n >= samples))
	{
		LOGW("invalid m=%i, n=%i", m, n);
		return TERRAIN_NODATA;
	}

	int idx = m*samples + n;
	return self->data[idx];
}

void terrain_tile_getBlock(terrain_tile_t* self,
                           int blocks, int r, int c,
                           short* data)
{
	assert(self);
	assert(data);
	assert(((TERRAIN_SAMPLES_TILE - 1) % blocks) == 0);

	int m;
	int n;
	int step = (TERRAIN_SAMPLES_TILE - 1)/blocks;
	int size = step + 1;
	for(m = 0; m < size; ++m)
	{
		for(n = 0; n < size; ++n)
		{
			int mm = step*r + m;
			int nn = step*c + n;
			data[size*m + n] = terrain_tile_get(self,
			                                    mm, nn);
		}
	}
}

void terrain_tile_adjustMinMax(terrain_tile_t* self,
                               short min, short max)
{
	assert(self);

	if(min < self->min)
	{
		self->min = min;
	}

	if(max > self->max)
	{
		self->max = max;
	}
}

void terrain_tile_exists(terrain_tile_t* self,
                         char next)
{
	assert(self);

	self->next |= next;
}

int terrain_tile_tl(terrain_tile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_TL;
}

int terrain_tile_bl(terrain_tile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_BL;
}

int terrain_tile_tr(terrain_tile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_TR;
}

int terrain_tile_br(terrain_tile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_BR;
}

short terrain_tile_min(terrain_tile_t* self)
{
	assert(self);

	return self->min;
}

short terrain_tile_max(terrain_tile_t* self)
{
	assert(self);

	return self->max;
}
