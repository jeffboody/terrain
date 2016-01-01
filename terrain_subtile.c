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
#include "terrain_subtile.h"
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

static void terrain_subtile_minmax(terrain_subtile_t* self)
{
	assert(self);

	int   m;
	int   n;
	short h;
	short min = 32767;
	short max = -32768;
	for(m = 0; m < TERRAIN_SAMPLES_SUBTILE; ++m)
	{
		for(n = 0; n < TERRAIN_SAMPLES_SUBTILE; ++n)
		{
			h = terrain_subtile_get(self, m, n);
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

terrain_subtile_t* terrain_subtile_new(int x, int y, int zoom,
                                       int i, int j)
{
	terrain_subtile_t* self = (terrain_subtile_t*)
	                          malloc(sizeof(terrain_subtile_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	int samples = TERRAIN_SAMPLES_SUBTILE + 2*TERRAIN_BORDER_SIZE;
	self->tex = texgz_tex_new(samples, samples,
	                          samples, samples,
	                          TEXGZ_SHORT,
	                          TEXGZ_LUMINANCE,
	                          NULL);
	if(self->tex == NULL)
	{
		goto fail_tex;
	}

	self->x    = x;
	self->y    = y;
	self->zoom = zoom;
	self->i    = i;
	self->j    = j;
	self->next = 0;
	self->pad  = 0;

	// updated on export
	self->min = 0;
	self->max = 0;

	// success
	return self;

	// failure
	fail_tex:
		free(self);
	return NULL;
}

void terrain_subtile_delete(terrain_subtile_t** _self)
{
	assert(_self);

	terrain_subtile_t* self = *_self;
	if(self)
	{
		texgz_tex_delete(&self->tex);
		free(self);
		*_self = NULL;
	}
}

terrain_subtile_t* terrain_subtile_import(const char* base,
                                          int xx, int yy, int zoom)
{
	assert(base);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, zoom, xx, yy);
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

	int x = xx/TERRAIN_SUBTILE_COUNT;
	int y = yy/TERRAIN_SUBTILE_COUNT;
	int i = yy % TERRAIN_SUBTILE_COUNT;
	int j = xx % TERRAIN_SUBTILE_COUNT;

	terrain_subtile_t* self;
	self = terrain_subtile_importf(f, size, x, y,
	                               zoom, i, j);
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

terrain_subtile_t* terrain_subtile_importf(FILE* f, int size,
                                          int x, int y, int zoom,
                                          int i, int j)
{
	assert(f);

	terrain_subtile_t* self = (terrain_subtile_t*)
	                          malloc(sizeof(terrain_subtile_t));
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
		self->min  = (short) readintle(buffer, 8);
		self->next = (char)  readintle(buffer, 12);
	}
	else if(swapendian(magic) == TERRAIN_MAGIC)
	{
		self->min  = (short) readintbe(buffer, 4);
		self->min  = (short) readintbe(buffer, 8);
		self->next = (char)  readintbe(buffer, 12);
	}

	// read the samples
	size -= TERRAIN_HSIZE;
	texgz_tex_t* tex = texgz_tex_importf(f, size);
	if(tex == NULL)
	{
		goto fail_tex;
	}
	self->tex = tex;

	// verify tex parameters
	int samples = TERRAIN_SAMPLES_SUBTILE + 2*TERRAIN_BORDER_SIZE;
	if((tex->width   == samples) &&
	   (tex->height  == samples) &&
	   (tex->stride  == samples) &&
	   (tex->vstride == samples) &&
	   (tex->type    == TEXGZ_SHORT) &&
	   (tex->format  == TEXGZ_LUMINANCE))
	{
		// ok
	}
	else
	{
		LOGE("invalid %ix%i, %ix%i, type=0x%x, format=0x%X",
		     tex->width, tex->height,
		     tex->stride, tex->vstride,
		     tex->type, tex->format);
		goto fail_param;
	}

	self->x    = x;
	self->y    = y;
	self->zoom = zoom;
	self->i    = i;
	self->j    = j;

	// success
	return self;

	// failure
	fail_param:
		texgz_tex_delete(&self->tex);
	fail_tex:
	fail_header:
		free(self);
	return NULL;
}

int terrain_subtile_export(terrain_subtile_t* self,
                           const char* base)
{
	assert(self);
	assert(base);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, self->zoom,
	         TERRAIN_SUBTILE_COUNT*self->x + self->j,
	         TERRAIN_SUBTILE_COUNT*self->y + self->i);
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
	terrain_subtile_minmax(self);

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

	// export the samples
	if(texgz_tex_exportf(self->tex, f) == 0)
	{
		goto fail_tex;
	}

	fclose(f);

	// success
	return 1;

	// failure
	fail_tex:
	fail_header:
		fclose(f);
		unlink(fname);
	return 0;
}

void terrain_subtile_coord(terrain_subtile_t* self,
                           int m, int n,
                           double* lat, double* lon)
{
	terrain_subtile2coord(self->x, self->y, self->zoom,
	                      self->i, self->j, m, n,
	                      lat, lon);
}

void terrain_subtile_set(terrain_subtile_t* self,
                         int m, int n,
                         short h)
{
	assert(self);

	// offset indices by border
	m += TERRAIN_BORDER_SIZE;
	n += TERRAIN_BORDER_SIZE;

	int samples = TERRAIN_SAMPLES_SUBTILE + 2*TERRAIN_BORDER_SIZE;
	if((m < 0) || (m >= samples) ||
	   (n < 0) || (n >= samples))
	{
		LOGW("invalid m=%i, n=%i", m, n);
		return;
	}

	int idx = m*samples + n;
	texgz_tex_t* tex    = self->tex;
	short*       pixels = (short*) tex->pixels;
	pixels[idx] = h;
}

short terrain_subtile_get(terrain_subtile_t* self,
                          int m, int n)
{
	assert(self);

	// offset indices by border
	m += TERRAIN_BORDER_SIZE;
	n += TERRAIN_BORDER_SIZE;

	int samples = TERRAIN_SAMPLES_SUBTILE + 2*TERRAIN_BORDER_SIZE;
	if((m < 0) || (m >= samples) ||
	   (n < 0) || (n >= samples))
	{
		LOGW("invalid m=%i, n=%i", m, n);
		return TERRAIN_NODATA;
	}

	int idx = m*samples + n;
	texgz_tex_t* tex    = self->tex;
	short*       pixels = (short*) tex->pixels;
	return pixels[idx];
}

void terrain_subtile_exists(terrain_subtile_t* self,
                            char next)
{
	assert(self);

	self->next |= next;
}

int terrain_subtile_tl(terrain_subtile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_TL;
}

int terrain_subtile_bl(terrain_subtile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_BL;
}

int terrain_subtile_tr(terrain_subtile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_TR;
}

int terrain_subtile_br(terrain_subtile_t* self)
{
	assert(self);

	return self->next & TERRAIN_NEXT_BR;
}

short terrain_subtile_min(terrain_subtile_t* self)
{
	assert(self);

	return self->min;
}

short terrain_subtile_max(terrain_subtile_t* self)
{
	assert(self);

	return self->max;
}
