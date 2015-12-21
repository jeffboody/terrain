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
#include <math.h>
#include "terrain_subtile.h"
#include "terrain_util.h"

#define LOG_TAG "terrain"
#include "terrain_log.h"

void  terrain_tile2coord(float x, float y, int zoom,
                         double* lat, double* lon)
{
	assert(lat);
	assert(lon);
	LOGD("debug x=%f, y=%f, zoom=%i", x, y, zoom);

	double worldu  = TERRAIN_SUBTILE_COUNT*x/pow(2.0, (double) zoom);
	double worldv  = TERRAIN_SUBTILE_COUNT*y/pow(2.0, (double) zoom);
	double cartx   = 2.0*M_PI*worldu;
	double carty   = 2.0*M_PI*worldv;
	double mercx   = cartx - M_PI;
	double mercy   = M_PI - carty;
	double rad_lon = mercx;
	double rad_lat = 2.0*atan(exp(mercy)) - M_PI/2.0;
	*lat           = rad_lat/(M_PI/180.0);
	*lon           = rad_lon/(M_PI/180.0);
}

void  terrain_subtile2coord(int x, int y, int zoom,
                            int i, int j, int m, int n,
                            double* lat, double* lon)
{
	assert(lat);
	assert(lon);
	LOGD("debug x=%i, y=%i, zoom=%i, i=%i, j=%i, m=%i, n=%i",
	     x, y, zoom, i, j, m, n);

	float s  = (float) TERRAIN_SAMPLE_COUNT;
	float c  = (float) TERRAIN_SUBTILE_COUNT;
	float xx = (float) x;
	float yy = (float) y;
	float jj = (float) j;
	float ii = (float) i;
	float nn = (float) n/(s - 1.0f);
	float mm = (float) m/(s - 1.0f);

	terrain_tile2coord(xx + (jj + nn)/c, yy + (ii + mm)/c,
	                   zoom, lat, lon);
}

void  terrain_coord2tile(double lat, double lon, int zoom,
                         float* x, float* y)
{
	assert(x);
	assert(y);
	LOGD("debug lat=%0.3lf, lon=%0.3lf, zoom=%i", lat, lon, zoom);

	double rad_lat = (double) lat*M_PI/180.0;
	double rad_lon = (double) lon*M_PI/180.0;
	double mercx   = rad_lon;
	double mercy   = log(tan(rad_lat) + 1.0/cos(rad_lat));
	double cartx   = mercx + M_PI;
	double carty   = M_PI - mercy;
	double worldu  = cartx/(2.0*M_PI);
	double worldv  = carty/(2.0*M_PI);
	*x             = (float) worldu*pow(2.0, (double) zoom)/TERRAIN_SUBTILE_COUNT;
	*y             = (float) worldv*pow(2.0, (double) zoom)/TERRAIN_SUBTILE_COUNT;
}

float terrain_m2ft(float m)
{
	return m*5280.0f/1609.344f;
}

float terrain_ft2m(float f)
{
	return f*1609.344f/5280.0f;
}
