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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#define LOG_TAG "terrain"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "terrain_tile.h"
#include "terrain_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int terrain_mkdir(const char* fname)
{
	ASSERT(fname);

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
			if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH |
			              S_IXOTH) == -1)
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
	ASSERT(self);

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
	ASSERT(buffer);
	ASSERT(offset >= 0);

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
	ASSERT(buffer);
	ASSERT(offset >= 0);

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

static short
terrain_tile_interpolate(terrain_tile_t* self,
                         float u, float v)
{
	ASSERT(self);

	// "float indices"
	float pu = u*(TERRAIN_SAMPLES_TILE - 1);
	float pv = v*(TERRAIN_SAMPLES_TILE - 1);

	// determine first pair of indices
	int n0  = floorf(pu);
	int m0  = floorf(pv);
	int min = -TERRAIN_SAMPLES_BORDER;
	if(n0 < min) { n0 = min; }
	if(m0 < min) { m0 = min; }

	// determine the second pair of indices
	int n1  = n0 + 1;
	int m1  = m0 + 1;
	int max = TERRAIN_SAMPLES_TILE +
	          TERRAIN_SAMPLES_BORDER - 1;
	if(n1 > max) { n1 = max; }
	if(m1 > max) { m1 = max; }

	// sample interpolation values
	float h00 = (float) terrain_tile_get(self, m0, n0);
	float h01 = (float) terrain_tile_get(self, m0, n1);
	float h10 = (float) terrain_tile_get(self, m1, n0);
	float h11 = (float) terrain_tile_get(self, m1, n1);

	// compute interpolation coordinates
	float s0 = (float) n0;
	float t0 = (float) m0;
	float s  = pu - s0;
	float t  = pv - t0;

	// interpolate v
	float h0010 = h00 + t*(h10 - h00);
	float h0111 = h01 + t*(h11 - h01);

	// interpolate u
	return (short) (h0010 + s*(h0111 - h0010) + 0.5f);
}

static void
terrain_tile_computeNormal(terrain_tile_t* self,
                           int i, int j, float dx, float dy,
                           unsigned char* pnx,
                           unsigned char* pny)
{
	ASSERT(self);
	ASSERT(pnx);
	ASSERT(pny);

	// compute the distance to next sample
	float dist = 1.0f/(float) TERRAIN_SAMPLES_TILE;

	// interpolate n/e/s/w samples
	float fmax = (float) (TERRAIN_SAMPLES_NORMAL - 1);
	float u    = (float) j/fmax;
	float v    = (float) i/fmax;
	short n    = terrain_tile_interpolate(self, u, v - dist);
	short e    = terrain_tile_interpolate(self, u + dist, v);
	short s    = terrain_tile_interpolate(self, u, v + dist);
	short w    = terrain_tile_interpolate(self, u - dist, v);

#if 1
	// compute normal from (1,0,dzdx)x(0,1,dzdy).
	// mask:
	//        -0.5*n
	// -0.5*w  (u,v) 0.5*e
	//         0.5*s
	// (-dzdx, -dzdy, 1) = (nx, ny, 1)
	// normalize dx,dy to 1
	float dzdx = 0.5f*(e - w);
	float dzdy = 0.5f*(s - n);
#else
	short nw;
	short ne;
	short sw;
	short se;
	nw = terrain_tile_interpolate(self, u - dist, v - dist);
	ne = terrain_tile_interpolate(self, u + dist, v - dist);
	sw = terrain_tile_interpolate(self, u - dist, v + dist);
	se = terrain_tile_interpolate(self, u + dist, v + dist);

	// initialize edge masks
	float mask_x[] =
	{
		-1.0f/4.0f, 0.0f, 1.0f/4.0f,
		-2.0f/4.0f, 0.0f, 2.0f/4.0f,
		-1.0f/4.0f, 0.0f, 1.0f/4.0f,
	};
	float mask_y[] =
	{
		-1.0f/4.0f, -2.0f/4.0f, -1.0f/4.0f,
		      0.0f,       0.0f,       0.0f,
		 1.0f/4.0f,  2.0f/4.0f,  1.0f/4.0f,
	};

	// compute dzx, dzy
	float dzdx;
	float dzdy;
	dzdx  = nw*mask_x[0];
	dzdy  = nw*mask_y[0];
	dzdy += n*mask_y[1];
	dzdx += ne*mask_x[2];
	dzdy += ne*mask_y[2];
	dzdx += w*mask_x[3];
	dzdx += e*mask_x[5];
	dzdx += se*mask_x[6];
	dzdy += se*mask_y[6];
	dzdy += s*mask_y[7];
	dzdx += sw*mask_x[8];
	dzdy += sw*mask_y[8];
#endif

	float nx   = -dzdx/dx;
	float ny   = -dzdy/dy;

	// clamp nx and ny to (-2.0, 2.0)
	if(nx < -2.0f) { nx = -2.0f; }
	if(nx >  2.0f) { nx =  2.0f; }
	if(ny < -2.0f) { ny = -2.0f; }
	if(ny >  2.0f) { ny =  2.0f; }

	// scale nx and ny to (0.0, 1.0)
	nx = (nx/4.0f) + 0.5f;
	ny = (ny/4.0f) + 0.5f;

	// scale nx and ny to (0, 255)
	unsigned char inx = (unsigned char) (nx*255.0f);
	unsigned char iny = (unsigned char) (ny*255.0f);

	// store nx and ny
	*pnx = inx;
	*pny = iny;
}

/***********************************************************
* protected                                                *
***********************************************************/

int terrain_tile_export(terrain_tile_t* self,
                        const char* base)
{
	ASSERT(self);
	ASSERT(base);

	char fname[256];
	char pname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, self->zoom, self->x, self->y);
	snprintf(pname, 256, "%s.part", fname);

	if(terrain_mkdir(fname) == 0)
	{
		return 0;
	}

	FILE* f = fopen(pname, "w");
	if(f == NULL)
	{
		LOGE("invalid %s", pname);
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

	int flags = self->flags;
	if(fwrite(&flags, sizeof(int), 1, f) != 1)
	{
		LOGE("fwrite failed");
		goto fail_header;
	}

	// allocate dst buffer
	int bytes = TERRAIN_SAMPLES_TOTAL*
	            TERRAIN_SAMPLES_TOTAL*sizeof(short);
	uLong src_size = (uLong) (bytes);
	uLong dst_size = compressBound(src_size);
	unsigned char* dst;
	dst = (unsigned char*)
	      MALLOC(dst_size*sizeof(unsigned char));
	if(dst == NULL)
	{
		LOGE("MALLOC failed");
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
	if(fwrite(dst, sizeof(unsigned char), dst_size,
	          f) != dst_size)
	{
		LOGE("fwrite failed");
		goto fail_fwrite;
	}

	FREE(dst);
	fclose(f);
	rename(pname, fname);

	// success
	return 1;

	// failure
	fail_fwrite:
	fail_compress:
		FREE(dst);
	fail_dst:
	fail_header:
		fclose(f);
		unlink(pname);
	return 0;
}

void terrain_tile_set(terrain_tile_t* self, int m, int n,
                      short h)
{
	ASSERT(self);

	// offset indices by border
	m += TERRAIN_SAMPLES_BORDER;
	n += TERRAIN_SAMPLES_BORDER;

	const int S = TERRAIN_SAMPLES_TOTAL;
	#if 0
		if((m < 0) || (m >= S) ||
		   (n < 0) || (n >= S))
		{
			LOGW("invalid m=%i, n=%i", m, n);
			return;
		}
	#endif

	int idx = m*S + n;
	self->data[idx] = h;
}

void terrain_tile_adjustMinMax(terrain_tile_t* self,
                               short min, short max)
{
	ASSERT(self);

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
                         int flags)
{
	ASSERT(self);

	self->flags |= flags;
}

/***********************************************************
* public                                                   *
***********************************************************/

terrain_tile_t* terrain_tile_new(int x, int y, int zoom)
{
	terrain_tile_t* self;
	self = (terrain_tile_t*) MALLOC(sizeof(terrain_tile_t));
	if(self == NULL)
	{
		LOGE("MALLOC failed");
		return NULL;
	}

	int samples = TERRAIN_SAMPLES_TOTAL*
	              TERRAIN_SAMPLES_TOTAL*
	              sizeof(short);
	memset(self->data, 0, samples);

	self->x     = x;
	self->y     = y;
	self->zoom  = zoom;
	self->flags = 0;

	// updated on export if not set
	self->min = TERRAIN_HEIGHT_MAX;
	self->max = TERRAIN_HEIGHT_MIN;

	// success
	return self;
}

void terrain_tile_delete(terrain_tile_t** _self)
{
	ASSERT(_self);

	terrain_tile_t* self = *_self;
	if(self)
	{
		FREE(self);
		*_self = NULL;
	}
}

terrain_tile_t*
terrain_tile_import(const char* base, int x, int y,
                    int zoom)
{
	ASSERT(base);

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

terrain_tile_t*
terrain_tile_importf(FILE* f, int size, int x, int y,
                     int zoom)
{
	ASSERT(f);

	terrain_tile_t* self;
	self = (terrain_tile_t*) MALLOC(sizeof(terrain_tile_t));
	if(self == NULL)
	{
		LOGE("MALLOC failed");
		return NULL;
	}

	if(terrain_tile_headerf(f, &self->min, &self->max,
	                        &self->flags) == 0)
	{
		goto fail_header;
	}

	// allocate src buffer
	size -= TERRAIN_HSIZE;
	char* src = (char*) MALLOC(size*sizeof(char));
	if(src == NULL)
	{
		LOGE("MALLOC failed");
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

	FREE(src);

	// success
	return self;

	// failure
	fail_uncompress:
	fail_read:
		FREE(src);
	fail_src:
	fail_header:
		FREE(self);
	return NULL;
}

int terrain_tile_header(const char* base,
                        int x, int y, int zoom,
                        short* min, short* max,
                        int* flags)
{
	ASSERT(base);
	ASSERT(min);
	ASSERT(max);
	ASSERT(flags);

	char fname[256];
	snprintf(fname, 256, "%s/terrain/%i/%i/%i.terrain",
	         base, zoom, x, y);
	fname[255] = '\0';

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("fopen %s failed", fname);
		return 0;
	}

	if(terrain_tile_headerf(f, min, max, flags) == 0)
	{
		goto fail_header;
	}

	fclose(f);

	// success
	return 1;

	// failure
	fail_header:
		fclose(f);
	return 0;
}

int terrain_tile_headerb(unsigned char* buffer,
                         int size,
                         short* min, short* max,
                         int* flags)
{
	ASSERT(buffer);
	ASSERT(min);
	ASSERT(max);
	ASSERT(flags);

	if(size < TERRAIN_HSIZE)
	{
		LOGE("invalid size=%i", size);
		return 0;
	}

	// parse the header
	int magic = readintle(buffer, 0);
	if(magic == TERRAIN_MAGIC)
	{
		*min   = (short) readintle(buffer, 4);
		*max   = (short) readintle(buffer, 8);
		*flags = readintle(buffer, 12);
	}
	else if(swapendian(magic) == TERRAIN_MAGIC)
	{
		*min   = (short) readintbe(buffer, 4);
		*max   = (short) readintbe(buffer, 8);
		*flags = readintbe(buffer, 12);
	}
	else
	{
		LOGE("invalid magic=0x%X", magic);
		return 0;
	}

	return 1;
}

int terrain_tile_headerf(FILE* f, short* min, short* max,
                         int* flags)
{
	ASSERT(f);
	ASSERT(min);
	ASSERT(max);
	ASSERT(flags);

	unsigned char buffer[TERRAIN_HSIZE];
	int size = fread(buffer, sizeof(unsigned char),
	                 TERRAIN_HSIZE, f);

	return terrain_tile_headerb(buffer, size,
	                            min, max, flags);
}

void terrain_tile_coord(terrain_tile_t* self, int m, int n,
                        double* lat, double* lon)
{
	ASSERT(self);
	ASSERT(lat);
	ASSERT(lon);

	terrain_sample2coord(self->x, self->y, self->zoom,
	                     m, n, lat, lon);
}

void terrain_tile_bounds(terrain_tile_t* self,
                         double* latT, double* lonL,
                         double* latB, double* lonR)
{
	ASSERT(self);
	ASSERT(latT);
	ASSERT(lonL);
	ASSERT(latB);
	ASSERT(lonR);

	return terrain_bounds(self->x, self->y, self->zoom,
	                      latT, lonL, latB, lonR);
}

short terrain_tile_get(terrain_tile_t* self, int m, int n)
{
	ASSERT(self);

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

short terrain_tile_sample(terrain_tile_t* self,
                          double lat, double lon)
{
	ASSERT(self);

	// sample in interpolated space
	float  S    = (float) TERRAIN_SAMPLES_TILE;
	double lat0 = 0.0;
	double lon0 = 0.0;
	double lat1 = 0.0;
	double lon1 = 0.0;
	terrain_tile2coord((float) self->x, (float) self->y,
	                   self->zoom,
	                   &lat0, &lon0);
	terrain_tile2coord((float) (self->x + 1),
	                   (float) (self->y + 1),
	                   self->zoom,
	                   &lat1, &lon1);
	float u = (float) ((lon - lon0)/(lon1 - lon0));
	float v = (float) ((lat - lat0)/(lat1 - lat0));
	int   m = (int) (S*v + 0.5f);
	int   n = (int) (S*u + 0.5f);
	return terrain_tile_get(self, m, n);
}

void terrain_tile_getBlock(terrain_tile_t* self,
                           int blocks, int r, int c,
                           short* data)
{
	ASSERT(self);
	ASSERT(data);
	ASSERT(((TERRAIN_SAMPLES_TILE - 1) % blocks) == 0);

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

void terrain_tile_getBlockf(terrain_tile_t* self,
                            int blocks, int r, int c,
                            float* data)
{
	ASSERT(self);
	ASSERT(data);
	ASSERT(((TERRAIN_SAMPLES_TILE - 1) % blocks) == 0);

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
			data[size*m + n] = (float) terrain_tile_get(self,
			                                            mm, nn);
		}
	}
}

void terrain_tile_getNormalMap(terrain_tile_t* self,
                               unsigned char* data)
{
	ASSERT(self);
	ASSERT(data);

	// compute dx and dy of mask
	double lat0;
	double lon0;
	double lat1;
	double lon1;
	terrain_tile_coord(self, 0, 0, &lat0, &lon0);
	terrain_tile_coord(self, 2, 2, &lat1, &lon1);

	float  x0;
	float  y0;
	float  x1;
	float  y1;
	terrain_coord2xy(lat0, lon0, &x0, &y0);
	terrain_coord2xy(lat1, lon1, &x1, &y1);

	float dx = x1 - x0;
	float dy = y1 - y0;

	// compute normal map
	int i;
	int j;
	for(i = 0; i < TERRAIN_SAMPLES_NORMAL; ++i)
	{
		for(j = 0; j < TERRAIN_SAMPLES_NORMAL; ++j)
		{
			int idx = 2*(TERRAIN_SAMPLES_NORMAL*i + j);
			unsigned char* pnx = &(data[idx]);
			unsigned char* pny = &(data[idx + 1]);
			terrain_tile_computeNormal(self, i, j,
			                           dx, dy, pnx, pny);
		}
	}
}

int terrain_tile_tl(terrain_tile_t* self)
{
	ASSERT(self);

	return self->flags & TERRAIN_NEXT_TL;
}

int terrain_tile_bl(terrain_tile_t* self)
{
	ASSERT(self);

	return self->flags & TERRAIN_NEXT_BL;
}

int terrain_tile_tr(terrain_tile_t* self)
{
	ASSERT(self);

	return self->flags & TERRAIN_NEXT_TR;
}

int terrain_tile_br(terrain_tile_t* self)
{
	ASSERT(self);

	return self->flags & TERRAIN_NEXT_BR;
}

short terrain_tile_min(terrain_tile_t* self)
{
	ASSERT(self);

	return self->min;
}

short terrain_tile_max(terrain_tile_t* self)
{
	ASSERT(self);

	return self->max;
}
