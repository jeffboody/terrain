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

	terrain_subtile_t* self = (terrain_subtile_t*)
	                          malloc(sizeof(terrain_subtile_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.texz",
	         base, zoom, xx, yy);
	fname[255] = '\0';

	texgz_tex_t* tex = texgz_tex_importz(fname);
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

	self->x    = xx/TERRAIN_SUBTILE_COUNT;
	self->y    = yy/TERRAIN_SUBTILE_COUNT;
	self->zoom = zoom;
	self->i    = yy % TERRAIN_SUBTILE_COUNT;
	self->j    = xx % TERRAIN_SUBTILE_COUNT;

	// success
	return self;

	// failure
	fail_param:
		texgz_tex_delete(&self->tex);
	fail_tex:
		free(self);
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

	texgz_tex_t* tex = texgz_tex_importf(f, size);
	if(self->tex == NULL)
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
		free(self);
	return NULL;
}

int terrain_subtile_export(terrain_subtile_t* self,
                           const char* base)
{
	assert(self);
	assert(base);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.texz",
	         base, self->zoom,
	         TERRAIN_SUBTILE_COUNT*self->x + self->j,
	         TERRAIN_SUBTILE_COUNT*self->y + self->i);
	fname[255] = '\0';

	if(terrain_mkdir(fname) == 0)
	{
		return 0;
	}

	if(texgz_tex_exportz(self->tex, fname) == 0)
	{
		return 0;
	}

	return 1;
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
