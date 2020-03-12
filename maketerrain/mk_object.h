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

#ifndef mk_object_H
#define mk_object_H

#include "terrain/terrain_tile.h"
#include "flt/flt_tile.h"

#define MK_OBJECT_TYPE_TERRAIN 0
#define MK_OBJECT_TYPE_FLT     1

typedef struct
{
	int type;
	int refcount;

	union
	{
		terrain_tile_t* terrain;
		flt_tile_t*     flt;
	};
} mk_object_t;

mk_object_t* mk_object_newTerrain(int x, int y, int zoom);
mk_object_t* mk_object_importTerrain(const char* base,
                                     int x, int y, int zoom);
mk_object_t* mk_object_importFlt(int type,
                                 int lat, int lon);
void         mk_object_delete(mk_object_t** _self);
void         mk_object_incref(mk_object_t* self);
int          mk_object_decref(mk_object_t* self);
int          mk_object_refcount(mk_object_t* self);
int          mk_object_exportTerrain(mk_object_t* self,
                                     const char* base);
void         mk_object_key(mk_object_t* self, char* key);
void         mk_object_sample00(mk_object_t* self, mk_object_t* next);
void         mk_object_sample01(mk_object_t* self, mk_object_t* next);
void         mk_object_sample02(mk_object_t* self, mk_object_t* next);
void         mk_object_sample03(mk_object_t* self, mk_object_t* next);
void         mk_object_sample10(mk_object_t* self, mk_object_t* next);
void         mk_object_sample11(mk_object_t* self, mk_object_t* next);
void         mk_object_sample12(mk_object_t* self, mk_object_t* next);
void         mk_object_sample13(mk_object_t* self, mk_object_t* next);
void         mk_object_sample20(mk_object_t* self, mk_object_t* next);
void         mk_object_sample21(mk_object_t* self, mk_object_t* next);
void         mk_object_sample22(mk_object_t* self, mk_object_t* next);
void         mk_object_sample23(mk_object_t* self, mk_object_t* next);
void         mk_object_sample30(mk_object_t* self, mk_object_t* next);
void         mk_object_sample31(mk_object_t* self, mk_object_t* next);
void         mk_object_sample32(mk_object_t* self, mk_object_t* next);
void         mk_object_sample33(mk_object_t* self, mk_object_t* next);

#endif
