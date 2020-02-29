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
#include <math.h>
#include "terrain_tile.h"
#include "terrain_util.h"

#define LOG_TAG "terrain"
#include "../libcc/cc_log.h"

void terrain_tile2coord(float x, float y, int zoom,
                        double* lat, double* lon)
{
	ASSERT(lat);
	ASSERT(lon);

	double worldu  = x/pow(2.0, (double) zoom);
	double worldv  = y/pow(2.0, (double) zoom);
	double cartx   = 2.0*M_PI*worldu;
	double carty   = 2.0*M_PI*worldv;
	double mercx   = cartx - M_PI;
	double mercy   = M_PI - carty;
	double rad_lon = mercx;
	double rad_lat = 2.0*atan(exp(mercy)) - M_PI/2.0;
	*lat           = rad_lat/(M_PI/180.0);
	*lon           = rad_lon/(M_PI/180.0);
}

void terrain_sample2coord(int x, int y, int zoom,
                          int m, int n,
                          double* lat, double* lon)
{
	ASSERT(lat);
	ASSERT(lon);

	float s  = (float) TERRAIN_SAMPLES_TILE;
	float xx = (float) x;
	float yy = (float) y;
	float nn = (float) n/(s - 1.0f);
	float mm = (float) m/(s - 1.0f);

	terrain_tile2coord(xx + nn, yy + mm,
	                   zoom, lat, lon);
}

void  terrain_coord2tile(double lat, double lon, int zoom,
                         float* x, float* y)
{
	ASSERT(x);
	ASSERT(y);

	double rad_lat = (double) lat*M_PI/180.0;
	double rad_lon = (double) lon*M_PI/180.0;
	double mercx   = rad_lon;
	double mercy   = log(tan(rad_lat) + 1.0/cos(rad_lat));
	double cartx   = mercx + M_PI;
	double carty   = M_PI - mercy;
	double worldu  = cartx/(2.0*M_PI);
	double worldv  = carty/(2.0*M_PI);
	*x             = (float) worldu*pow(2.0, (double) zoom);
	*y             = (float) worldv*pow(2.0, (double) zoom);
}

void terrain_coord2xy(double lat, double lon,
                      float* x, float* y)
{
	ASSERT(x);
	ASSERT(y);

	// use home as the origin
	double lat2meter = 111072.12110934;
	double lon2meter = 85337.868965619;
	double home_lat  = 40.061295;
	double home_lon  =-105.214552;

	*x = (float) ((lon - home_lon)*lon2meter);
	*y = (float) ((lat - home_lat)*lat2meter);
}

void terrain_xy2coord(float x, float y,
                      double* lat, double* lon)
{
	ASSERT(lat);
	ASSERT(lon);

	// use home as the origin
	double lat2meter = 111072.12110934;
	double lon2meter = 85337.868965619;
	double home_lat  = 40.061295;
	double home_lon  =-105.214552;

	double xd = (double) x;
	double yd = (double) y;
	*lat = (yd/lat2meter) + home_lat;
	*lon = (xd/lon2meter) + home_lon;
}

void terrain_bounds(int x, int y, int zoom,
                    double* latT, double* lonL,
                    double* latB, double* lonR)
{
	ASSERT(latT);
	ASSERT(lonL);
	ASSERT(latB);
	ASSERT(lonR);

	terrain_sample2coord(x, y, zoom, 0, 0, latT, lonL);
	terrain_sample2coord(x, y, zoom,
	                     TERRAIN_SAMPLES_TILE - 1,
	                     TERRAIN_SAMPLES_TILE - 1,
	                     latB, lonR);
}

float terrain_m2ft(float m)
{
	return m*5280.0f/1609.344f;
}

float terrain_ft2m(float f)
{
	return f*1609.344f/5280.0f;
}
