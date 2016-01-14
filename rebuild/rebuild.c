/*
 * Copyright (c) 2016 Jeff Boody
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "terrain/terrain_tile.h"
#include "terrain/terrain_util.h"

#define LOG_TAG "rebuild"
#include "terrain/terrain_log.h"

static int parseY(char* s, int* zoom, int* x, int* y)
{
	// find the pattern
	char* p = strchr(s, '.');
	if(p == NULL)
	{
		LOGE("invalid s=%s", s);
		return 0;
	}
	*p = '\0';
	p = &(p[1]);

	// parse y
	*y = (int) strtol(s, NULL, 0);
	return 1;
}

static int parseX(char* s, int* zoom, int* x, int* y)
{
	// find the pattern
	char* p = strchr(s, '/');
	if(p == NULL)
	{
		LOGE("invalid s=%s", s);
		return 0;
	}
	*p = '\0';
	p = &(p[1]);

	// parse x
	*x = (int) strtol(s, NULL, 0);

	return parseY(p, zoom, x, y);
}

static int parseZoom(char* s, int* zoom, int* x, int* y)
{
	// find the pattern
	char* p = strchr(s, '/');
	if(p == NULL)
	{
		LOGE("invalid s=%s", s);
		return 0;
	}
	*p = '\0';
	p = &(p[1]);

	// parse the zoom
	*zoom = (int) strtol(s, NULL, 0);

	return parseX(p, zoom, x, y);
}

static int parseType(char* s, int* zoom, int* x, int* y)
{
	// find the pattern
	char* p = strchr(s, '/');
	if(p == NULL)
	{
		LOGE("invalid s=%s", s);
		return 0;
	}
	*p = '\0';
	p = &(p[1]);

	// parse the type
	if(strcmp(s, "terrain") == 0)
	{
		return parseZoom(p, zoom, x, y);
	}

	LOGE("invalid s=%s", s);
	return 0;
}

static void rebuild(const char* path)
{
	assert(path);

	DIR* dir = opendir(path);
	if(dir == NULL)
	{
		LOGE("invalid path=%s", path);
		return;
	}

	// rebuild the database
	struct dirent  entry;
	struct dirent* result;
	terrain_tile_t* ter = NULL;
	while((readdir_r(dir, &entry, &result) == 0) &&
	      (result != NULL))
	{
		if(result->d_type == DT_DIR)
		{
			// ignore "." and ".."
			if(strcmp(result->d_name, ".") == 0)
			{
				continue;
			}
			else if(strcmp(result->d_name, "..") == 0)
			{
				continue;
			}

			char dname[256];
			snprintf(dname, 256, "%s/%s", path, result->d_name);
			dname[255] = '\0';

			// search directories recursively
			rebuild(dname);
		}
		else if(result->d_type == DT_REG)
		{
			char fname[256];
			snprintf(fname, 256, "%s/%s", path, result->d_name);
			fname[255] = '\0';

			LOGI("fname=%s", fname);

			int x;
			int y;
			int zoom;
			if(parseType(fname, &zoom, &x, &y) == 0)
			{
				goto fail_type;
			}

			ter = terrain_tile_import(".",
			                          x, y, zoom);
			if(ter == NULL)
			{
				goto fail_import;
			}

			if(terrain_tile_export(ter, "terrain2") == 0)
			{
				goto fail_export;
			}

			terrain_tile_delete(&ter);
		}
	}

	// success
	closedir(dir);
	return;

	// failure
	fail_export:
		terrain_tile_delete(&ter);
	fail_import:
	fail_type:
		closedir(dir);
}

int main(int argc, const char** argv)
{
	rebuild("terrain");
	return EXIT_SUCCESS;
}
