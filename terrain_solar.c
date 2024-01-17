/*
 * Copyright (c) 2024 Jeff Boody
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

#include <math.h>
#include <stdlib.h>

#define LOG_TAG "terrain"
#include "../libcc/math/cc_mat3f.h"
#include "../libcc/cc_log.h"
#include "terrain_solar.h"
#include "terrain_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
terrain_solar_offsetTm(struct tm* now_utc,
                       struct tm* now_local)
{
	ASSERT(now_utc);
	ASSERT(now_local);

	int diy           = 365;
	int utc_minutes   = 60*now_utc->tm_hour + now_utc->tm_min +
	                    24*60*now_utc->tm_yday;
	int local_minutes = 60*now_local->tm_hour + now_local->tm_min +
	                    24*60*now_local->tm_yday;
	if(now_utc->tm_year > now_local->tm_year)
	{
		int yeari = now_utc->tm_year + 1900;
		if((yeari % 4) > 0)
		{
			// not leap year
		}
		else if((yeari % 100) > 0)
		{
			diy = 366;
		}
		else if((yeari % 400) > 0)
		{
			// not leap year
		}
		else
		{
			diy = 366;
		}

		utc_minutes += diy*24*60;
	}
	else if(now_utc->tm_year < now_local->tm_year)
	{
		int yeari = now_local->tm_year + 1900;
		if((yeari % 4) > 0)
		{
			// not leap year
		}
		else if((yeari % 100) > 0)
		{
			diy = 366;
		}
		else if((yeari % 400) > 0)
		{
			// not leap year
		}
		else
		{
			diy = 366;
		}

		local_minutes += diy*24*60;
	}

	return local_minutes - utc_minutes;
}

static void
terrain_solar_overflowTm(int* _hh, int* _mm)
{
	ASSERT(_hh);
	ASSERT(_mm);

	while(*_mm >= 60)
	{
		*_mm -= 60;
		*_hh += 1;
	}
	while(*_mm < 0)
	{
		*_mm += 60;
		*_hh -= 1;
	}

	while(*_hh >= 24)
	{
		*_hh -= 24;
	}
	while(*_hh < 0)
	{
		*_hh += 24;
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

void terrain_solar_initTm(int year, int month, int day,
                          int hh, int mm, int ss,
                          struct tm* tm)
{
	ASSERT(tm);

	// use normal dates
	// year:  2024 (i.e. do not subract 1900)
	// month: 1-12 (i.e. month does not start at 0)
	// day:   1-31
	//
	// use 24-hour time
	// hh:    0-23
	// mm:    0-59
	// ss:    0-59
	tm->tm_sec   = ss;
	tm->tm_min   = mm;
	tm->tm_hour  = hh;
	tm->tm_mday  = day;
	tm->tm_mon   = month - 1;
	tm->tm_year  = year - 1900;
	tm->tm_wday  = 0;
	tm->tm_yday  = 0;
	tm->tm_isdst = 0;
}

void terrain_solar_nowTm(struct tm* now_utc,
                         struct tm* now_local)
{
	ASSERT(now_utc);
	ASSERT(now_local);

	// get the current time
	time_t now_time = time(NULL);

	// convert to UTC time
	if(gmtime_r(&now_time, now_utc) == NULL)
	{
		LOGW("gmtime_r failed");
	}

	// convert to local time
	if(localtime_r(&now_time, now_local) == NULL)
	{
		LOGW("localtime_r failed");
	}
}

void terrain_solar_shiftTm(struct tm* tm,
                           int month, float dt)
{
	ASSERT(tm);

	// This function is used to simulate relative sun positions
	// which vary by season (month) or by day (hours).

	// optionally shift date
	// month: 1-12
	if(month > 0)
	{
		tm->tm_mon  = month - 1;
		tm->tm_mday = 15;
	}

	// optionally shift time
	// dt: time delta (hours)
	if(dt != 0.0f)
	{
		int minutes = (int) (60.0f*dt + 0.5f);
		tm->tm_min  += minutes%60;
		tm->tm_hour += minutes/60;

		// The shiftTm function causes time to overflow which may
		// result in negative numbers.
		terrain_solar_overflowTm(&tm->tm_hour, &tm->tm_min);
	}
}

void terrain_solar_subsolarPoint(struct tm* now_utc,
                                 double* _lat_ss,
                                 double* _lon_ss,
                                 double* _declR,
                                 double* _eqtime)
{
	ASSERT(now_utc);
	ASSERT(_lat_ss);
	ASSERT(_lon_ss);
	ASSERT(_declR);
	ASSERT(_eqtime);

	// compute days in a month
	int yeari    = now_utc->tm_year + 1900;
	int leapyr   = 0;
	int days[12] = { 31, 28, 31, 30, 31, 30,
	                 31, 31, 30, 31, 30, 31 };
	if((yeari % 4) > 0)
	{
		// not leap year
	}
	else if((yeari % 100) > 0)
	{
		days[1] = 29;
		leapyr  = 1;
	}
	else if((yeari % 400) > 0)
	{
		// not leap year
	}
	else
	{
		days[1] = 29;
		leapyr  = 1;
	}

	// count days passed at beginning of month
	int i;
	int days_cnt[12] = { 0 };
	for(i = 1; i < 12; ++i)
	{
		days_cnt[i] = days[i - 1] + days_cnt[i - 1];
	}

	// compute the lat/lon of the subsolar point
	double degs    = 180.0/M_PI;
	double diy     = leapyr ? 366.0 : 365.0;
	double hour    = (double) now_utc->tm_hour;
	double min     = (double) now_utc->tm_min;
	double sec     = (double) now_utc->tm_sec;
	double mday    = (double) now_utc->tm_mday;
	int    mon     = now_utc->tm_mon;
	double yday    = (double) (days_cnt[mon] + mday - 1);
	double y       = (2.0*M_PI/diy)*
	                 (yday + ((hour - 12.0)/24.0));
	double eqtime  = 229.18*(0.000075 +
	                         0.001868*cos(y) -
	                         0.032077*sin(y) -
	                         0.014615*cos(2.0*y) -
	                         0.040849*sin(2.0*y));
	double declR   = 0.006918 -
	                 0.399912*cos(y) +
	                 0.070257*sin(y) -
	                 0.006758*cos(2.0*y) +
	                 0.000907*sin(2.0*y);
	double decl    = declR*degs;
	double now     = 60.0*hour + min + sec/60.0;
	double revs    = 0.997;

	*_lat_ss = decl;
	*_lon_ss = fmod(360.0*revs*(720.0 - now - eqtime)/1440.0,
	                360.0);
	*_declR  = declR;
	*_eqtime = eqtime;
}

void terrain_solar_sunAbsolute(double lat_ss,
                               double lon_ss,
                               cc_vec3f_t* sun)
{
	ASSERT(sun);

	double sunx;
	double suny;
	double sunz;
	terrain_geo2xyz(lat_ss, lon_ss, 0.0f,
	                &sunx, &suny, &sunz);
	cc_vec3f_load(sun, sunx, suny, sunz);
	cc_vec3f_normalize(sun);
}

void terrain_solar_sunRelative(double lat, double lon,
                               double lat_ss,
                               double lon_ss,
                               cc_vec3f_t* sun)
{
	ASSERT(sun);

	// sun absolute vector
	terrain_solar_sunAbsolute(lat_ss, lon_ss, sun);

	// compute orthornormal basis vectors
	cc_vec3f_t x3;
	cc_vec3f_t y3;
	cc_vec3f_t z3;
	double     z3x;
	double     z3y;
	double     z3z;
	cc_vec3f_t north3 = { .x=0.0f, .y=0.0f, .z=1.0f };
	terrain_geo2xyz(lat, lon, 0.0f,
	                &z3x, &z3y, &z3z);
	cc_vec3f_load(&z3, z3x, z3y, z3z);
	cc_vec3f_normalize(&z3);
	cc_vec3f_cross_copy(&north3, &z3, &x3);
	cc_vec3f_normalize(&x3);
	cc_vec3f_cross_copy(&z3, &x3, &y3);
	cc_vec3f_normalize(&y3);

	// basis rotation
	cc_mat3f_t R =
	{
		.m00 = x3.x,
		.m01 = y3.x,
		.m02 = z3.x,
		.m10 = x3.y,
		.m11 = y3.y,
		.m12 = z3.y,
		.m20 = x3.z,
		.m21 = y3.z,
		.m22 = z3.z,
	};

	// sun relative vector
	cc_mat3f_transpose(&R);
	cc_mat3f_mulv(&R, sun);
}

void terrain_solar_daylight(double lat, double lon,
                            double declR, double eqtime,
                            struct tm* now_utc,
                            struct tm* now_local,
                            int* _sunrise_hh,
                            int* _sunrise_mm,
                            int* _sunset_hh,
                            int* _sunset_mm)
{
	ASSERT(now_utc);
	ASSERT(now_local);
	ASSERT(_sunrise_hh);
	ASSERT(_sunrise_mm);
	ASSERT(_sunset_hh);
	ASSERT(_sunset_mm);

	double degs  = 180.0/M_PI;
	double rads  = M_PI/180.0;
	double latR  = lat*rads;
	double num   = cos(90.833*rads);
	double den   = cos(latR)*cos(declR);
	double ha    = degs*acos(num/den - tan(latR)*tan(declR));
	double uoff  = (double) terrain_solar_offsetTm(now_utc,
	                                               now_local);

	// sunrise/sunset in minutes (local time)
	int sunrise = (int)
	              (720.0 - 4.0*(lon + ha) -
	               eqtime + uoff + 0.5);
	int sunset  = (int)
	              (720.0 - 4.0*(lon - ha) - eqtime + uoff + 0.5);

	// sunrise/sunset in 24-hour time (local time)
	*_sunrise_hh = sunrise/60;
	*_sunrise_mm = sunrise%60;
	*_sunset_hh  = sunset/60;
	*_sunset_mm  = sunset%60;

	// The shiftTm function causes time to overflow which may
	// result in negative numbers.
	terrain_solar_overflowTm(_sunrise_hh, _sunrise_mm);
	terrain_solar_overflowTm(_sunset_hh, _sunset_mm);
}
