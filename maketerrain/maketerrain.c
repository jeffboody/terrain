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

#include <stdint.h>
#include <stdlib.h>

#define LOG_TAG "maketerrain"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "mk_object.h"
#include "mk_state.h"

int main(int argc, char** argv)
{
	if(argc != 6)
	{
		LOGE("usage: %s [latT] [lonL] [latB] [lonR] [path]",
		     argv[0]);
		return EXIT_FAILURE;
	}

	int   latT = (int) strtol(argv[1], NULL, 0);
	int   lonL = (int) strtol(argv[2], NULL, 0);
	int   latB = (int) strtol(argv[3], NULL, 0);
	int   lonR = (int) strtol(argv[4], NULL, 0);
	char* path = argv[5];

	mk_state_t* state;
	state = mk_state_new(latT, lonL, latB, lonR, path);
	if(state == NULL)
	{
		return EXIT_FAILURE;
	}

	mk_object_t* obj;
	obj = mk_state_getTerrain(state, 0, 0, 0);
	if(obj == NULL)
	{
		goto fail_get;
	}

	mk_state_put(state, &obj);
	mk_state_delete(&state);

	// check for memory leak
	if(MEMSIZE() != 0)
	{
		LOGW("memory leak detected: %u", (uint32_t) MEMSIZE());
	}

	// success
	return EXIT_SUCCESS;

	// failure
	fail_get:
		mk_state_delete(&state);
	return EXIT_FAILURE;
}
