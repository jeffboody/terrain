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

#ifndef terrain_util_H
#define terrain_util_H

void  terrain_tile2coord(float x, float y, int zoom,
                         double* lat, double* lon);
void  terrain_sample2coord(int x, int y, int zoom,
                           int m, int n,
                           double* lat, double* lon);
void  terrain_coord2tile(double lat, double lon, int zoom,
                         float* x, float* y);
void  terrain_coord2xy(double lat, double lon,
                       float* x, float* y);
void  terrain_xy2coord(float x, float y,
                       double* lat, double* lon);
void  terrain_geo2xyz(double lat, double lon, float alt,
                      double* x, double* y, double* z);
void  terrain_xyz2geo(double x, double y, double z,
                      double* lat, double* lon, float* alt);
void  terrain_xyz2xyh(float x1, float y1, float z1,
                      float* x2, float* y2, float* alt);
void  terrain_xyh2xyz(float x1, float y1, float alt,
                      float* x2, float* y2, float* z);
void  terrain_bounds(int x, int y, int zoom,
                     double* latT, double* lonL,
                     double* latB, double* lonR);
float terrain_m2ft(float m);
float terrain_ft2m(float f);

#endif
