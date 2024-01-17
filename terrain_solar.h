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

#ifndef terrain_solar_H
#define terrain_solar_H

#include <time.h>

#include "../libcc/math/cc_vec3f.h"

// Based on NOAA General Solar Position Calculations
// https://www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF

void terrain_solar_initTm(int year, int month, int day,
                          int hh, int mm, int ss,
                          struct tm* tm);
void terrain_solar_nowTm(struct tm* now_utc,
                         struct tm* now_local);
void terrain_solar_shiftTm(struct tm* tm,
                           int month, float dt);
void terrain_solar_subsolarPoint(struct tm* now_utc,
                                 double* _lat_ss,
                                 double* _lon_ss,
                                 double* _declR,
                                 double* _eqtime);
void terrain_solar_sunAbsolute(double lat_ss,
                               double lon_ss,
                               cc_vec3f_t* sun);
void terrain_solar_sunRelative(double lat, double lon,
                               double lat_ss,
                               double lon_ss,
                               cc_vec3f_t* sun);
void terrain_solar_daylight(double lat, double lon,
                            double declR, double eqtime,
                            struct tm* now_utc,
                            struct tm* now_local,
                            int* _sunrise_hh,
                            int* _sunrise_mm,
                            int* _sunset_hh,
                            int* _sunset_mm);

#endif
