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

#ifndef terrain_subtile_H
#define terrain_subtile_H

#include "../texgz/texgz_tex.h"

/*
 * A terrain tile is a array of 8x8 subtiles.
 *
 * There are 257x257 samples to ensure that the subtile
 * can be subdivided evenly for multiple LOD. e.g. 257
 * samples means 256 segments. The range of m,n is 0..256
 * for samples and -1..257 including the border.
 *
 * The 1 pixel border can be used to compute the derivative
 * for hill/relief shading. The sample count does not
 * include the border.
 */
#define TERRAIN_SUBTILE_COUNT   8
#define TERRAIN_SAMPLES_SUBTILE 257
#define TERRAIN_BORDER_SIZE     1
#define TERRAIN_NODATA          0

/*
 * flags for next LOD existance
 */
#define TERRAIN_NEXT_TL  0X1
#define TERRAIN_NEXT_BL  0X2
#define TERRAIN_NEXT_TR  0X4
#define TERRAIN_NEXT_BR  0X8
#define TERRAIN_NEXT_ALL 0XF

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

	struct
	{
		// subtile address
		char i;
		char j;

		// next LOD existance
		char next;

		// padding for 4-byte alignment
		char pad;
	};

	// min/max altitude for the subtile
	short min;
	short max;

	// tex is stored as SHORT+LUMINANCE
	// data units are measured in feet because the highest
	// point, Mt Everest is 29029 feet,  which matches up
	// nicely with range of shorts (-32768 to 32767)
	texgz_tex_t* tex;
} terrain_subtile_t;

terrain_subtile_t* terrain_subtile_new(int x, int y, int zoom,
                                       int i, int j);
void               terrain_subtile_delete(terrain_subtile_t** _self);
terrain_subtile_t* terrain_subtile_import(const char* base,
                                          int xx, int yy, int zoom);
terrain_subtile_t* terrain_subtile_importf(FILE* f, int size,
                                          int x, int y, int zoom,
                                          int i, int j);
int                terrain_subtile_export(terrain_subtile_t* self,
                                          const char* base);
void               terrain_subtile_coord(terrain_subtile_t* self,
                                         int m, int n,
                                         double* lat, double* lon);
void               terrain_subtile_set(terrain_subtile_t* self,
                                       int m, int n,
                                       short h);
short              terrain_subtile_get(terrain_subtile_t* self,
                                       int m, int n);
void               terrain_subtile_exists(terrain_subtile_t* self,
                                          char next);
int                terrain_subtile_tl(terrain_subtile_t* self);
int                terrain_subtile_bl(terrain_subtile_t* self);
int                terrain_subtile_tr(terrain_subtile_t* self);
int                terrain_subtile_br(terrain_subtile_t* self);
short              terrain_subtile_min(terrain_subtile_t* self);
short              terrain_subtile_max(terrain_subtile_t* self);

#endif
