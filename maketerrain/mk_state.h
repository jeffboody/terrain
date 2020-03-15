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

#ifndef mk_state_H
#define mk_state_H

#include "libcc/cc_list.h"
#include "libcc/cc_map.h"
#include "mk_object.h"

typedef struct
{
	int latT;
	int lonL;
	int latB;
	int lonR;

	double t0;
	double count;
	double total;

	const char* path;

	// obj cache
	cc_map_t*  obj_map;
	cc_list_t* obj_list;

	// flt object references
	// refcount is not needed because they
	// are only created/evicted by z13
	int cnt_usgs;
	int cnt_aster;
	mk_object_t* obj_usgs[9];
	mk_object_t* obj_aster[9];
} mk_state_t;

mk_state_t*  mk_state_new(int latT, int lonL,
                          int latB, int lonR,
                          const char* path);
void         mk_state_delete(mk_state_t** _self);
void         mk_state_put(mk_state_t* self,
                          mk_object_t** _obj);
mk_object_t* mk_state_getTerrain(mk_state_t* self,
                                 int x, int y, int zoom);

#endif
