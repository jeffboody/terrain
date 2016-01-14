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

#include "../texgz/texgz_tex.h"

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
 * header uncompressed consists of:
 * int magic
 * int min  (cast to short)
 * int max  (cast to short)
 * int next (cast to char)
 */
#define TERRAIN_MAGIC 0x7EBB00D9
#define TERRAIN_HSIZE 16

typedef struct
{
	// tile address
	int x;
	int y;
	int zoom;

	// tex is stored as SHORT+LUMINANCE
	// data units are measured in feet because the highest
	// point, Mt Everest is 29029 feet,  which matches up
	// nicely with range of shorts (-32768 to 32767)
	texgz_tex_t* tex;

	// min/max altitude for the tile
	short min;
	short max;

	// next LOD existance
	char next;
} terrain_tile_t;

terrain_tile_t* terrain_tile_new(int x, int y, int zoom);
void            terrain_tile_delete(terrain_tile_t** _self);
terrain_tile_t* terrain_tile_import(const char* base,
                                    int x, int y, int zoom);
terrain_tile_t* terrain_tile_importf(FILE* f, int size,
                                     int x, int y, int zoom);
int             terrain_tile_export(terrain_tile_t* self,
                                    const char* base);
void            terrain_tile_coord(terrain_tile_t* self,
                                   int m, int n,
                                   double* lat, double* lon);
void            terrain_tile_set(terrain_tile_t* self,
                                 int m, int n,
                                 short h);
short           terrain_tile_get(terrain_tile_t* self,
                                 int m, int n);
void            terrain_tile_getBlock(terrain_tile_t* self,
                                      int blocks,
                                      int r, int c,
                                      short* data);
void            terrain_tile_adjustMinMax(terrain_tile_t* self,
                                          short min, short max);
void            terrain_tile_exists(terrain_tile_t* self,
                                    char next);
int             terrain_tile_tl(terrain_tile_t* self);
int             terrain_tile_bl(terrain_tile_t* self);
int             terrain_tile_tr(terrain_tile_t* self);
int             terrain_tile_br(terrain_tile_t* self);
short           terrain_tile_min(terrain_tile_t* self);
short           terrain_tile_max(terrain_tile_t* self);

#endif