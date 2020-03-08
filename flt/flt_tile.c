/*
 * Copyright (c) 2013 Jeff Boody
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
#include <unistd.h>

#define LOG_TAG "flt"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libxmlstream/xml_istream.h"
#include "flt_tile.h"

/***********************************************************
* private                                                  *
***********************************************************/

static float meters2feet(float m)
{
	return m*5280.0f/1609.344f;
}

static int keyval(char* s, const char** k, const char** v)
{
	ASSERT(s);
	ASSERT(k);
	ASSERT(v);

	// find key
	*k = s;
	while(*s != ' ')
	{
		if(*s == '\0')
		{
			// fail silently
			return 0;
		}
		++s;
	}
	*s = '\0';
	++s;

	// find value
	while(*s == ' ')
	{
		if(*s == '\0')
		{
			// skip silently
			return 0;
		}
		++s;
	}
	*v = s;
	++s;

	// find end
	while(*s != '\0')
	{
		if((*s == '\n') ||
		   (*s == '\r') ||
		   (*s == '\t') ||
		   (*s == ' '))
		{
			*s = '\0';
			break;
		}
		++s;
	}

	return 1;
}

static int
flt_tile_importhdr(flt_tile_t* self, const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	// ignore if file does not exist
	if(access(fname, F_OK) != 0)
	{
		return 0;
	}

	LOGI("fname=%s", fname);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("fopen %s failed", fname);
		return 0;
	}

	const char* key;
	const char* value;
	char        buffer[256];
	int         ncols     = 0;
	int         nrows     = 0;
	double      xllcorner = 0.0;
	double      yllcorner = 0.0;
	double      cellsize  = 0.0;
	float       nodata    = 0.0f;
	int         byteorder = 0;
	while(fgets(buffer, 256, f))
	{
		if(keyval(buffer, &key, &value) == 0)
		{
			// skip silently
			continue;
		}

		if(strcmp(key, "ncols") == 0)
		{
			ncols = (int) strtol(value, NULL, 0);
		}
		else if(strcmp(key, "nrows") == 0)
		{
			nrows = (int) strtol(value, NULL, 0);
		}
		else if(strcmp(key, "xllcorner") == 0)
		{
			xllcorner = strtod(value, NULL);
		}
		else if(strcmp(key, "yllcorner") == 0)
		{
			yllcorner = strtod(value, NULL);
		}
		else if(strcmp(key, "cellsize") == 0)
		{
			cellsize = strtod(value, NULL);
		}
		else if(strcmp(key, "NODATA_value") == 0)
		{
			nodata = strtof(value, NULL);
		}
		else if(strcmp(key, "byteorder") == 0)
		{
			if(strcmp(value, "MSBFIRST") == 0)
			{
				byteorder = FLT_MSBFIRST;
			}
			else if(strcmp(value, "LSBFIRST") == 0)
			{
				byteorder = FLT_LSBFIRST;
			}
		}
		else
		{
			LOGW("unknown key=%s, value=%s", key, value);
		}
	}

	fclose(f);

	// verfy required fields
	if((ncols == 0)       ||
	   (nrows == 0)       ||
	   (cellsize  == 0.0) ||
	   (byteorder == 0))
	{
		LOGE("invalid nrows=%i, ncols=%i, xllcorner=%0.3lf, yllcorner=%0.3lf, cellsize=%0.6lf, byteorder=%i",
		     nrows, ncols, xllcorner, yllcorner, cellsize, byteorder);
		return 0;
	}

	self->latB      = yllcorner;
	self->lonL      = xllcorner;
	self->latT      = yllcorner + (double) ncols*cellsize;
	self->lonR      = xllcorner + (double) nrows*cellsize;
	self->nodata    = nodata;
	self->byteorder = byteorder;
	self->nrows     = nrows;
	self->ncols     = ncols;

	return 1;
}

static int
flt_tile_importprj(flt_tile_t* self, const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		// skip silently
		return 0;
	}

	const char* key;
	const char* value;
	char        buffer[256];
	while(fgets(buffer, 256, f))
	{
		if(keyval(buffer, &key, &value) == 0)
		{
			// skip silently
			continue;
		}

		if(strcmp(key, "Projection") == 0)
		{
			if(strcmp(value, "GEOGRAPHIC") != 0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Datum") == 0)
		{
			if(strcmp(value, "NAD83") != 0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Zunits") == 0)
		{
			if(strcmp(value, "METERS") != 0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Units") == 0)
		{
			if(strcmp(value, "DD") != 0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Spheroid") == 0)
		{
			if(strcmp(value, "GRS1980") != 0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Xshift") == 0)
		{
			if(strtod(value, NULL) != 0.0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Yshift") == 0)
		{
			if(strtod(value, NULL) != 0.0)
			{
				LOGW("%s=%s", key, value);
			}
		}
		else if(strcmp(key, "Parameters") == 0)
		{
			// skip
		}
		else
		{
			LOGW("unknown key=%s, value=%s", key, value);
		}
	}

	fclose(f);

	return 1;
}

static int
flt_tile_importflt(flt_tile_t* self, const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		// skip silently
		return 0;
	}

	size_t size = self->nrows*self->ncols*sizeof(short);
	self->height = (short*) MALLOC(size);
	if(self->height == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_height;
	}

	size_t rsize = self->ncols*sizeof(float);
	float* rdata = (float*) MALLOC(rsize);
	if(rdata == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_rdata;
	}

	int row;
	for(row = 0; row < self->nrows; ++row)
	{
		size_t left = rsize;
		unsigned char* data = (unsigned char*) rdata;
		while(left > 0)
		{
			size_t bytes;
			bytes = fread(data, sizeof(unsigned char), left, f);
			if(bytes == 0)
			{
				LOGE("fread failed");
				goto fail_read;
			}
			left -= bytes;
			data += bytes;
		}

		// need to swap byte order for big endian
		data = (unsigned char*) rdata;
		if(self->byteorder == FLT_MSBFIRST)
		{
			int i;
			for(i = 0; i < self->ncols; ++i)
			{
				unsigned char d0 = data[4*i + 0];
				unsigned char d1 = data[4*i + 1];
				unsigned char d2 = data[4*i + 2];
				unsigned char d3 = data[4*i + 3];
				data[4*i + 0] = d3;
				data[4*i + 1] = d2;
				data[4*i + 2] = d1;
				data[4*i + 3] = d0;

				// convert data to feet
				float* height = (float*) &data[4*i];
				int    idx    = row*self->ncols + i;
				self->height[idx] = (short)
				                    (meters2feet(*height) + 0.5f);
			}
		}
		else
		{
			int i;
			for(i = 0; i < self->ncols; ++i)
			{
				// convert data to feet
				float* height = (float*) &data[4*i];
				int    idx    = row*self->ncols + i;
				self->height[idx] = (short)
				                    (meters2feet(*height) + 0.5f);
			}
		}
	}

	FREE(rdata);
	fclose(f);

	return 1;

	// failure
	fail_read:
		FREE(rdata);
		rdata = NULL;
	fail_rdata:
		FREE(self->height);
		self->height = NULL;
	fail_height:
		fclose(f);
	return 0;
}

static int
flt_tile_importtif(flt_tile_t* self, const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	// ASTER v3
	// https://lpdaac.usgs.gov/products/astgtmv003/

	// ignore if file does not exist
	if(access(fname, F_OK) != 0)
	{
		return 0;
	}

	LOGI("fname=%s", fname);

	TIFF *tiff = TIFFOpen(fname, "r");
	if(tiff == NULL)
	{
		LOGE("TIFFOpen failed");
		return 0;
	}

	uint32_t w;
	uint32_t h;
	uint32_t tw;
	uint32_t th;
	uint16_t samples;
	uint16_t bits;
	uint16_t format;
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);
	TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits);
	TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &format);
	TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tw);
	TIFFGetField(tiff, TIFFTAG_TILELENGTH, &th);

	if((samples != 1) || (bits != 16) ||
	   (format != SAMPLEFORMAT_INT))
	{
		LOGE("invalid params");
		goto fail_param;
	}

	short* tile = (short*) MALLOC(tw*th*sizeof(short));
	if(tile == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_tile;
	}

	self->height = (short*) MALLOC(w*h*sizeof(short));
	if(self->height == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_height;
	}

	// read tiles
	int i;
	int j;
	int m;
	int n;
	for(i = 0; i < h; i += th)
	{
		for(j = 0; j < w; j += tw)
		{
			TIFFReadTile(tiff, tile, j, i, 0, 0);

			// untile data and convert to feet
			for(m = 0; m < th; ++m)
			{
				if(i + m >= h)
				{
					break;
				}

				for(n = 0; n < tw; ++n)
				{
					if(j + n >= w)
					{
						break;
					}

					// ignore nodata values
					short t = tile[m*tw + n];
					if(t == -9999)
					{
						t = 0;
					}

					int idx = (i+m)*w + j + n;
					self->height[idx] = (short)
					                    (meters2feet((float) t) + 0.5f);
				}
			}
		}
	}

	self->nrows = h;
	self->ncols = w;

	FREE(tile);
	TIFFClose(tiff);

	// success
	return 1;

	// failure
	fail_height:
		FREE(tile);
	fail_tile:
	fail_param:
		TIFFClose(tiff);
	return 0;
}

static int
flt_tile_xmlStart(void* priv, int line, const char* name,
                  const char** atts)
{
	// ignore
	return 1;
}

static int
flt_tile_xmlEnd(void* priv, int line, const char* name,
                const char* content)
{
	// content may be NULL
	ASSERT(priv);
	ASSERT(name);

	flt_tile_t* self = (flt_tile_t*) priv;

	if(strcmp(name, "NorthBoundingCoordinate") == 0)
	{
		if(content == NULL)
		{
			goto fail_content;
		}
		self->latT = strtod(content, NULL);
	}
	else if(strcmp(name, "WestBoundingCoordinate") == 0)
	{
		if(content == NULL)
		{
			goto fail_content;
		}
		self->lonL = strtod(content, NULL);
	}
	else if(strcmp(name, "SouthBoundingCoordinate") == 0)
	{
		if(content == NULL)
		{
			goto fail_content;
		}
		self->latB = strtod(content, NULL);
	}
	else if(strcmp(name, "EastBoundingCoordinate") == 0)
	{
		if(content == NULL)
		{
			goto fail_content;
		}
		self->lonR = strtod(content, NULL);
	}

	// success
	return 1;

	// failure
	fail_content:
		LOGE("%i/%i: content is NULL",
		     self->lat, self->lon);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

flt_tile_t*
flt_tile_import(int type, int lat, int lon)
{
	char flt_fbase[256];
	char flt_fname[256];
	char hdr_fname[256];
	char prj_fname[256];
	char tif_fname[256];
	char xml_fname[256];

	snprintf(flt_fbase, 256, "%s%i%s%03i",
	         (lat >= 0) ? "n" : "s", abs(lat),
	         (lon >= 0) ? "e" : "w", abs(lon));
	snprintf(flt_fname, 256, "usgs-ned/data/%s/float%s_13",
	         flt_fbase, flt_fbase);
	snprintf(hdr_fname, 256, "%s.hdr", flt_fname);
	snprintf(prj_fname, 256, "%s.prj", flt_fname);
	snprintf(tif_fname, 256,
	         "ASTERv3/data/ASTGTMV003_%s%02i%s%03i_dem.tif",
	         (lat >= 0) ? "N" : "S", abs(lat),
	         (lon >= 0) ? "E" : "W", abs(lon));
	snprintf(xml_fname, 256,
	         "ASTERv3/zip/ASTGTMV003_%s%02i%s%03i.zip.xml",
	         (lat >= 0) ? "N" : "S", abs(lat),
	         (lon >= 0) ? "E" : "W", abs(lon));

	flt_tile_t* self;
	self = (flt_tile_t*) MALLOC(sizeof(flt_tile_t));
	if(self == NULL)
	{
		LOGE("MALLOC failed");
		return 0;
	}

	// init defaults
	self->type      = type;
	self->lat       = lat;
	self->lon       = lon;
	self->lonL      = (double) lon;
	self->latB      = (double) lat;
	self->lonR      = (double) lon + 1.0;
	self->latT      = (double) lat + 1.0;
	self->nodata    = 0.0;
	self->byteorder = FLT_LSBFIRST;
	self->nrows     = 0;
	self->ncols     = 0;
	self->height    = NULL;

	if(type == FLT_TILE_TYPE_USGS)
	{
		// import flt/tif files but prefer flt files since
		// they are higher resolution
		if(flt_tile_importhdr(self, hdr_fname) == 0)
		{
			// silently fail
			goto fail_import;
		}
	}
	else
	{
		if(flt_tile_importtif(self, tif_fname))
		{
			// parse extent
			if(xml_istream_parse((void*) self,
			                     flt_tile_xmlStart,
			                     flt_tile_xmlEnd,
			                     xml_fname) == 0)
			{
				LOGE("invalid %s", xml_fname);
				goto fail_import;
			}

			// success
			return self;
		}

		// silently fail
		goto fail_import;
	}

	// if hdr exists then prj and flt
	// must also exist
	if(flt_tile_importprj(self, prj_fname) == 0)
	{
		LOGE("flt_tile_importprj %s failed", prj_fname);
		goto fail_prj;
	}

	if(flt_tile_importflt(self, flt_fname) == 0)
	{
		// filenames in source files are inconsistent
		snprintf(flt_fname, 256,
		         "usgs-ned/data/%s/float%s_13.flt",
		         flt_fbase, flt_fbase);
		if(flt_tile_importflt(self, flt_fname) == 0)
		{
			LOGE("flt_tile_importflt %s failed", flt_fname);
			goto fail_flt;
		}
	}

	// success
	return self;

	// failure
	fail_flt:
	fail_prj:
	fail_import:
		FREE(self);
	return NULL;
}

void flt_tile_delete(flt_tile_t** _self)
{
	ASSERT(_self);

	flt_tile_t* self = *_self;
	if(self)
	{
		FREE(self->height);
		FREE(self);
		*_self = NULL;
	}
}

int flt_tile_sample(flt_tile_t* self,
                    double lat, double lon,
                    short* height)
{
	ASSERT(self);
	ASSERT(height);

	double lonu = (lon - self->lonL)/
	              (self->lonR - self->lonL);
	double latv = 1.0 - ((lat - self->latB)/
	                     (self->latT - self->latB));
	if((lonu >= 0.0) && (lonu <= 1.0) &&
	   (latv >= 0.0) && (latv <= 1.0))
	{
		// "float indices"
		float lon = (float) (lonu*(self->ncols - 1));
		float lat = (float) (latv*(self->nrows - 1));

		// determine indices to sample
		int   lon0  = (int) lon;
		int   lat0  = (int) lat;
		int   lon1  = (int) (lon + 1.0f);
		int   lat1  = (int) (lat + 1.0f);

		// double check the indices
		if(lon0 < 0)
		{
			lon0 = 0;
		}
		if(lon1 >= self->ncols)
		{
			lon1 = self->ncols - 1;
		}
		if(lat0 < 0)
		{
			lat0 = 0;
		}
		if(lat1 >= self->nrows)
		{
			lat1 = self->nrows - 1;
		}

		// compute interpolation coordinates
		float lon0f = (float) lon0;
		float lat0f = (float) lat0;
		float u     = lon - lon0f;
		float v     = lat - lat0f;

		// sample interpolation values
		float h00 = (float) self->height[lat0*self->ncols + lon0];
		float h01 = (float) self->height[lat0*self->ncols + lon1];
		float h10 = (float) self->height[lat1*self->ncols + lon0];
		float h11 = (float) self->height[lat1*self->ncols + lon1];

		// workaround for incorrect source data around coastlines
		if((h00 > 32000) || (h00 == self->nodata))
		{
			h00 = 0.0;
		}
		if((h01 > 32000) || (h01 == self->nodata))
		{
			h01 = 0.0;
		}
		if((h10 > 32000) || (h10 == self->nodata))
		{
			h10 = 0.0;
		}
		if((h11 > 32000) || (h11 == self->nodata))
		{
			h11 = 0.0;
		}

		// interpolate longitude
		float h0001 = h00 + u*(h01 - h00);
		float h1011 = h10 + u*(h11 - h10);

		// interpolate latitude
		*height = (short) (h0001 + v*(h1011 - h0001) + 0.5f);

		return 1;
	}

	return 0;
}
