/*
 * Copyright (c) 2025 Jeff Boody
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

#define LOG_TAG "bigfoot"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "bigfoot.h"

// 00: Same delta size
// 01: Increment delta size
// 10: Decrement delta size
// 11: Reset delta size
#define BIGFOOT_CTRL_SAME  0
#define BIGFOOT_CTRL_INC   1
#define BIGFOOT_CTRL_DEC   2
#define BIGFOOT_CTRL_RESET 3

typedef struct
{
	uint64_t count_ctrl;
	uint64_t count_reset;
	uint64_t count_data;
} bigfoot_stats_t;

static uint16_t bigfoot_delta16(int16_t a, int16_t b)
{
	uint16_t x = (uint16_t) (b - a);

	// zero bit delta
	if(x == 0)
	{
		return 0;
	}

	// compute the delta in bits
	uint16_t mask  = 0x8000;
	uint16_t delta = 16;
	if(mask & x)
	{
		// find the first zero bit
		while(mask)
		{
			mask >>= 1;
			if((x&mask) == 0)
			{
				break;
			}
			delta -= 1;
		}
	}
	else
	{
		// find the first one bit
		while(mask)
		{
			mask >>= 1;
			if(x&mask)
			{
				break;
			}
			delta -= 1;
		}
	}

	// clamp delta for sign bit
	if(delta < 2)
	{
		delta = 2;
	}

	return delta;
}

static int
bigfoot_store16(uint16_t* _bit, uint16_t bits, uint16_t data,
                size_t* _zsize, void** _zdata)
{
	ASSERT(_bit);
	ASSERT(_zsize);
	ASSERT(_zdata);

	// TODO - bigfoot_store16
	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

int bigfoot_encode16(size_t size, const int16_t* data,
                     size_t* _zsize, void** _zdata)
{
	ASSERT((size%sizeof(int16_t)) == 0);
	ASSERT(data);
	ASSERT(_zsize);
	ASSERT(_zdata);

	bigfoot_stats_t stats = { 0 };

	// initialize zsize
	*_zsize = 0;

	// encode first element
	uint16_t bit   = 0;
	uint32_t count = size/sizeof(int16_t);
	if(count)
	{
		if(bigfoot_store16(&bit, 16, (uint16_t) data[0],
		                   _zsize, _zdata) == 0)
		{
			return 0;
		}
		stats.count_data = 16;
	}

	// encode remaining elements
	uint32_t i;
	uint16_t delta0 = 0;
	uint16_t delta1;
	uint16_t ctrl;
	for(i = 1; i < count; ++i)
	{
		// compute delta
		delta1 = bigfoot_delta16(data[i], data[i - 1]);

		// compute ctrl and adjust delta
		if(delta1 == delta0)
		{
			ctrl = BIGFOOT_CTRL_SAME;
		}
		else if(delta1 > delta0)
		{
			ctrl = BIGFOOT_CTRL_INC;
			if((delta1 - delta0) > 2)
			{
				ctrl = BIGFOOT_CTRL_RESET;
			}
			else if(delta0 == 0)
			{
				delta1 = 4;
			}
			else
			{
				delta1 = delta0 + 2;
				if(delta1 > 16)
				{
					delta1 = 16;
				}
			}
		}
		else
		{
			ctrl = BIGFOOT_CTRL_DEC;
			if((delta0 - delta1) >= 4)
			{
				ctrl = BIGFOOT_CTRL_RESET;
			}
			else if(delta0 == 2)
			{
				delta1 = 0;
			}
			else
			{
				delta1 = delta0 - 1;
			}
		}

		// store ctrl
		if(bigfoot_store16(&bit, 2, ctrl, _zsize, _zdata) == 0)
		{
			return 0;
		}
		stats.count_ctrl += 2;

		// optionally store reset
		if(ctrl == BIGFOOT_CTRL_RESET)
		{
			if(bigfoot_store16(&bit, 4, delta1,
			                   _zsize, _zdata) == 0)
			{
				return 0;
			}
			stats.count_reset += 4;
		}

		// optionally store data
		if(delta1)
		{
			if(bigfoot_store16(&bit, delta1, (uint16_t) data[i],
			                   _zsize, _zdata) == 0)
			{
				return 0;
			}
			stats.count_data += delta1;
		}

		delta0 = delta1;
	}

	// compute zsize
	uint64_t zsize = *_zsize;
	if(*_zsize == 0)
	{
		zsize = (stats.count_ctrl + stats.count_reset +
		         stats.count_data)/8;
	}

	// compute stats
	if(zsize)
	{
		float ratio = ((float) size)/((float) zsize);
		LOGI("stats: count_ctrl=%u, count_reset=%u, count_data=%u, ratio=%f",
		     stats.count_ctrl, stats.count_reset,
		     stats.count_data, ratio);
	}

	return 1;
}

int bigfoot_decode16(size_t zsize, const void* zdata,
                     size_t* _size, int16_t** _data)
{
	ASSERT(zdata);
	ASSERT(_size);
	ASSERT(_data);

	// TODO - bigfoot_decode16
	return 0;
}
