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

#ifndef terrain_tile_H
#define terrain_tile_H

#include <stdio.h>

/*
 * There are 257x257 samples to ensure that the tile
 * can be subdivided evenly for multiple LOD. e.g. 257
 * samples means 256 segments. The range of m,n is 0..256
 * for samples and -1..257 including the border.
 *
 * The 1 pixel border can be used to compute the derivative
 * for hill/relief shading. The sample count does not
 * include the border.
 */
#define TERRAIN_SAMPLES_TOTAL  259
#define TERRAIN_SAMPLES_TILE   257
#define TERRAIN_SAMPLES_BORDER 1
#define TERRAIN_NODATA         0

/*
 * The normal map is computed from (1,0,dzdx)x(0,1,dzdy).
 * Resulting in (-dzdx, -dzdy, 1) = (nx, ny, 1). The values
 * for nx,ny are stored as a luminance+alpha texture with
 * unsigned bytes. The range of nx and ny is clamped to
 * (-2,2) prior to conversion to unsigned byte.
 * The steepest slope that can be represented is ~60
 * degrees.
 */
#define TERRAIN_SAMPLES_NORMAL 256

/*
 * flags for next LOD existance
 */
#define TERRAIN_NEXT_TL  0X1
#define TERRAIN_NEXT_BL  0X2
#define TERRAIN_NEXT_TR  0X4
#define TERRAIN_NEXT_BR  0X8
#define TERRAIN_NEXT_ALL 0XF

/*
 * range of shorts
 */
#define TERRAIN_HEIGHT_MIN -32768
#define TERRAIN_HEIGHT_MAX 32767

/*
 * 16 byte header
 * int magic
 * int min (cast to short)
 * int max (cast to short)
 * int flags
 */
#define TERRAIN_MAGIC 0x7EBB00D9
#define TERRAIN_HSIZE 16

typedef struct
{
	// tile address
	int x;
	int y;
	int zoom;

	// data units are measured in feet because the highest
	// point, Mt Everest is 29029 feet,  which matches up
	// nicely with range of shorts (-32768 to 32767)
	short data[TERRAIN_SAMPLES_TOTAL*TERRAIN_SAMPLES_TOTAL];

	// min/max altitude for the tile
	short min;
	short max;

	// LOD existance flags
	int flags;
} terrain_tile_t;

terrain_tile_t* terrain_tile_new(int x, int y, int zoom);
void            terrain_tile_delete(terrain_tile_t** _self);
terrain_tile_t* terrain_tile_import(const char* base,
                                    int x, int y, int zoom);
terrain_tile_t* terrain_tile_importf(FILE* f, int size,
                                     int x, int y, int zoom);
int             terrain_tile_header(const char* base,
                                    int x, int y, int zoom,
                                    short* min, short* max,
                                    int* flags);
int             terrain_tile_headerb(unsigned char* buffer,
                                     int size,
                                     short* min, short* max,
                                     int* flags);
int             terrain_tile_headerf(FILE* f,
                                     short* min, short* max,
                                     int* flags);
void            terrain_tile_coord(terrain_tile_t* self,
                                   int m, int n,
                                   double* lat, double* lon);
void            terrain_tile_bounds(terrain_tile_t* self,
                                    double* latT, double* lonL,
                                    double* latB, double* lonR);
short           terrain_tile_get(terrain_tile_t* self,
                                 int m, int n);
short           terrain_tile_sample(terrain_tile_t* self,
                                    double lat, double lon);
void            terrain_tile_getBlock(terrain_tile_t* self,
                                      int blocks,
                                      int r, int c,
                                      short* data);
void            terrain_tile_getBlockf(terrain_tile_t* self,
                                       int blocks,
                                       int r, int c,
                                       float* data);
void            terrain_tile_getNormalMap(terrain_tile_t* self,
                                          unsigned char* data);
int             terrain_tile_tl(terrain_tile_t* self);
int             terrain_tile_bl(terrain_tile_t* self);
int             terrain_tile_tr(terrain_tile_t* self);
int             terrain_tile_br(terrain_tile_t* self);
short           terrain_tile_min(terrain_tile_t* self);
short           terrain_tile_max(terrain_tile_t* self);

#endif
