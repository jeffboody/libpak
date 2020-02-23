/*
 * Copyright (c) 2014 Jeff Boody
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
#include <string.h>
#include "libpak/pak_file.h"

#define LOG_TAG "pak"
#include "libcc/cc_log.h"

static void usage(const char* argv0)
{
	LOGE("create:  %s -c file.pak <files>", argv0);
	LOGE("append:  %s -a file.pak <files>", argv0);
	LOGE("extract: %s -x file.pak <files>", argv0);
	LOGE("list:    %s -l file.pak", argv0);
}

static void extract(pak_file_t* pak, const char* fname)
{
	ASSERT(pak);
	ASSERT(fname);

	int size = pak_file_seek(pak, fname);
	if(size == 0)
	{
		return;
	}

	FILE* f = fopen(fname, "w");
	if(f == 0)
	{
		LOGE("fopen %s failed", fname);
		return;
	}

	unsigned char buf[4096];
	while(size > 0)
	{
		int bytes = (size > 4096) ? 4096 : size;
		if(pak_file_read(pak, buf, bytes, 1) != 1)
		{
			break;
		}

		if(fwrite(buf, bytes, 1, f) != 1)
		{
			LOGE("fwrite %s failed", fname);
			break;
		}

		size -= bytes;
	}

	fclose(f);
}

static int append(pak_file_t* pak, const char* fname)
{
	ASSERT(pak);
	ASSERT(fname);

	if(pak_file_writek(pak, fname) == 0)
	{
		return 0;
	}

	FILE* f = fopen(fname, "r");
	if(f == 0)
	{
		LOGE("fopen %s failed", fname);
		return 0;
	}

	unsigned char buf[4096];
	int bytes = fread(buf, sizeof(unsigned char), 4096, f);
	while(bytes > 0)
	{
		if(pak_file_write(pak, buf, bytes, 1) != 1)
		{
			goto fail_write;
		}

		bytes = fread(buf, sizeof(unsigned char), 4096, f);
	}

	// succcess
	fclose(f);
	return 1;

	// failure
	fail_write:
		fclose(f);
	return 0;
}

int main(int argc, char** argv)
{
	const char* arg0 = argv[0];
	if(argc < 3)
	{
		usage(arg0);
		return EXIT_FAILURE;
	}

	pak_file_t* pak;
	const char* cmd   = argv[1];
	const char* fname = argv[2];
	if(strcmp(cmd, "-c") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_WRITE);
		if(pak == NULL)
		{
			return EXIT_FAILURE;
		}

		int i;
		for(i = 3; i < argc; ++i)
		{
			if(append(pak, argv[i]) == 0)
			{
				goto fail_append;
			}
		}

		pak_file_close(&pak);
	}
	else if(strcmp(cmd, "-a") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_APPEND);
		if(pak == NULL)
		{
			return EXIT_FAILURE;
		}

		int i;
		for(i = 3; i < argc; ++i)
		{
			if(append(pak, argv[i]) == 0)
			{
				goto fail_append;
			}
		}

		pak_file_close(&pak);
	}
	else if(strcmp(cmd, "-x") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_READ);
		if(pak == NULL)
		{
			return EXIT_FAILURE;
		}

		int i;
		for(i = 3; i < argc; ++i)
		{
			extract(pak, argv[i]);
		}

		pak_file_close(&pak);
	}
	else if(strcmp(cmd, "-l") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_READ);
		if(pak == NULL)
		{
			return EXIT_FAILURE;
		}

		pak_item_t* item = pak_file_head(pak);
		while(item)
		{
			LOGI("%i %s", pak_item_size(item), pak_item_key(item));
			item = pak_item_next(item);
		}

		pak_file_close(&pak);
	}
	else
	{
		usage(arg0);
		return EXIT_FAILURE;
	}

	// success
	return EXIT_SUCCESS;

	// failure
	fail_append:
		pak_file_closeErr(&pak);
	return EXIT_FAILURE;
}
