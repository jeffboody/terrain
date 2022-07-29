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

/*
 * See geodetic algorithms by Karl Osen
 *
 * Accurate Conversion of Earth-Fixed Earth-Centered
 * Coordinates to Geodetic Coordinates
 *
 */

#define WGS84_AADC     +7.79540464078689228919e+0007 // (a^2)/c
#define WGS84_BBDCC    +1.48379031586596594555e+0002 // (b^2)/(c^2)
#define WGS84_EED2     +3.34718999507065852867e-0003 // (e^2)/2
#define WGS84_EEEED4   +1.12036808631011150655e-0005 // (e^4)/4
#define WGS84_EEEE     +4.48147234524044602618e-0005 // e^4
#define WGS84_HMIN     +2.25010182030430273673e-0014 // (e^12)/4
#define WGS84_INV3     +3.33333333333333333333e-0001 // 1/3
#define WGS84_INV6     +1.66666666666666666667e-0001 // 1/6
#define WGS84_INVAA    +2.45817225764733181057e-0014 // 1/(a^2)
#define WGS84_INVCBRT2 +7.93700525984099737380e-0001 // 1/(2^(1/3))
#define WGS84_P1MEE    +9.93305620009858682943e-0001 // 1-(e^2)
#define WGS84_P1MEEDAA +2.44171631847341700642e-0014 // (1-(e^2))/(a^2)

void terrain_tile2coord(float x, float y, int zoom,
                        double* lat, double* lon)
{
	ASSERT(lat);
	ASSERT(lon);

	// TODO - convert terrain_tile2coord to double

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

	// TODO - convert terrain_coord2tile to double

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

	// TODO - depreciate terrain_coord2xy

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

	// TODO - depreciate terrain_xy2coord

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

void terrain_geo2xyz(double lat, double lon, float alt,
                     double* x, double* y, double* z)
{
	ASSERT(x);
	ASSERT(y);
	ASSERT(z);

	/*
	 * See geodetic algorithms by Karl Osen
	 *
	 * Accurate Conversion of Earth-Fixed Earth-Centered
	 * Coordinates to Geodetic Coordinates
	 *
	 */

	const static double aadc  = WGS84_AADC;
	const static double bbdcc = WGS84_BBDCC;
	const static double p1mee = WGS84_P1MEE;

	// convert lat/lon to radians
	lat *= M_PI/180.0;
	lon *= M_PI/180.0;

	double coslat = cos(lat);
	double sinlat = sin(lat);
	double coslon = cos(lon);
	double sinlon = sin(lon);
	double N = aadc/sqrt(coslat*coslat + bbdcc);
	double d = (N + alt)*coslat;
	*x = d*coslon;
	*y = d*sinlon;
	*z = (p1mee*N + alt)*sinlat;
}

void terrain_xyz2geo(double x, double y, double z,
                     double* lat, double* lon, float* alt)
{
	ASSERT(lat);
	ASSERT(lon);
	ASSERT(alt);

	/*
	 * See geodetic algorithms by Karl Osen
	 *
	 * Accurate Conversion of Earth-Fixed Earth-Centered
	 * Coordinates to Geodetic Coordinates
	 *
	 */

	const static double Hmin     = WGS84_HMIN;
	const static double inv3     = WGS84_INV3;
	const static double inv6     = WGS84_INV6;
	const static double invaa    = WGS84_INVAA;
	const static double invcbrt2 = WGS84_INVCBRT2;
	const static double l        = WGS84_EED2;
	const static double ll4      = WGS84_EEEE;
	const static double ll       = WGS84_EEEED4;
	const static double p1meedaa = WGS84_P1MEEDAA;
	const static double p1mee    = WGS84_P1MEE;

	double beta;
	double C;
	double dFdt;
	double dt;
	double dw;
	double dz;
	double F;
	double G;
	double H;
	double i;
	double k;
	double m;
	double n;
	double p;
	double P;
	double t;
	double u;
	double v;
	double w;

	// intermediate variables
	double j;
	double ww;    // w^2
	double mpn;   // m+n
	double g;
	double tt;    // t^2
	double ttt;   // t^3
	double tttt;  // t^4
	double zu;    // z * u
	double wv;    // w * v
	double invuv; // 1 / (u * v)
	double da;
	double t1, t2, t3, t4, t5, t6, t7;

	ww  = x*x + y*y;
	m   = ww*invaa;
	n   = z*z*p1meedaa;
	mpn = m + n;
	p   = inv6*(mpn - ll4);
	G   = m*n*ll;
	H   = 2*p*p*p + G;

	if(H < Hmin)
	{
		LOGW("invalid H=%lf, Hmin=%lf", H, Hmin);
		*lat = 0.0;
		*lon = 0.0;
		*alt = 0.0f;
		return;
	}

	C    = pow(H + G + 2*sqrt(H*G), inv3)*invcbrt2;
	i    = -ll - 0.5*mpn;
	P    = p * p;
	beta = inv3*i - C - P/C;
	k    = ll*(ll - mpn);

	// compute left part of t
	t1 = beta*beta - k;
	t2 = sqrt(t1);
	t3 = t2 - 0.5*(beta + i);
	t4 = sqrt(t3);

	// compute right part of t
	t5 = 0.5*(beta - i);

	// t5 may accidentally drop just below zero due to numeric
	// turbulence (this only occurs at latitudes close to
	// +/- 45.3 degrees)
	t5 = fabs(t5);
	t6 = sqrt(t5);
	t7 = (m < n) ? t6 : -t6;

	// add left and right parts
	t = t4 + t7;

	// use Newton-Raphson's method to compute t correction
	j    = l*(m - n);
	g    = 2*j;
	tt   = t*t;
	ttt  = tt*t;
	tttt = tt*tt;
	F    = tttt + 2*i*tt + g*t + k;
	dFdt = 4*ttt + 4*i*t + g;
	dt   = -F/dFdt;

	// compute latitude (range -PI/2..PI/2)
	u    = t + dt + l;
	v    = t + dt - l;
	w    = sqrt(ww);
	zu   = z*u;
	wv   = w*v;
	*lat = atan2(zu, wv)*180.0/M_PI;

	// compute altitude
	invuv = 1.0/(u*v);
	dw    = w - wv*invuv;
	dz    = z - zu*p1mee*invuv;
	da    = sqrt(dw*dw + dz*dz);
	*alt  = (float) ((u < 1.0) ? -da : da);

	// compute longitude (range -PI..PI)
	*lon = atan2(y, x)*180.0/M_PI;
}

void terrain_xyz2xyh(float x1, float y1, float z1,
                     float* x2, float* y2, float* alt)
{
	ASSERT(x2);
	ASSERT(y2);
	ASSERT(alt);

	// TODO - depreciate terrain_xyz2xyh

	double lat;
	double lon;
	terrain_xyz2geo(x1, y1, z1, &lat, &lon, alt);
	terrain_coord2xy(lat, lon, x2, y2);
}

void terrain_xyh2xyz(float x1, float y1, float alt,
                     float* x2, float* y2, float* z2)
{
	ASSERT(x2);
	ASSERT(y2);
	ASSERT(z2);

	// TODO - depreciate terrain_xyh2xyz

	double lat;
	double lon;
	double  x2d;
	double  y2d;
	double  z2d;
	terrain_xy2coord(x1, y1, &lat, &lon);
	terrain_geo2xyz(lat, lon, alt, &x2d, &y2d, &z2d);
	*x2 = x2d;
	*y2 = y2d;
	*z2 = z2d;
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
