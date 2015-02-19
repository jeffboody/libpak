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
#include <assert.h>
#include <string.h>
#include "libpak/pak_file.h"

#define LOG_TAG "pak"
#include "libpak/pak_log.h"

static void usage(const char* argv0)
{
	LOGE("create:  %s -c file.pak <files>", argv0);
	LOGE("append:  %s -a file.pak <files>", argv0);
	LOGE("extract: %s -x file.pak <files>", argv0);
	LOGE("list:    %s -l file.pak", argv0);
}

static void extract(pak_file_t* pak, const char* fname)
{
	assert(pak);
	assert(fname);
	LOGD("debug fname=%s", fname);

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

static void append(pak_file_t* pak, const char* fname)
{
	assert(pak);
	assert(fname);
	LOGD("debug fname=%s", fname);

	if(pak_file_writek(pak, fname) == 0)
	{
		return;
	}

	FILE* f = fopen(fname, "r");
	if(f == 0)
	{
		LOGE("fopen %s failed", fname);
		return;
	}

	unsigned char buf[4096];
	int bytes = fread(buf, sizeof(unsigned char), 4096, f);
	while(bytes > 0)
	{
		if(pak_file_write(pak, buf, bytes, 1) != 1)
		{
			break;
		}

		bytes = fread(buf, sizeof(unsigned char), 4096, f);
	}

	fclose(f);
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

		int i;
		for(i = 3; i < argc; ++i)
		{
			append(pak, argv[i]);
		}

		pak_file_close(&pak);
	}
	else if(strcmp(cmd, "-a") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_APPEND);

		int i;
		for(i = 3; i < argc; ++i)
		{
			append(pak, argv[i]);
		}

		pak_file_close(&pak);
	}
	else if(strcmp(cmd, "-x") == 0)
	{
		pak = pak_file_open(fname, PAK_FLAG_READ);

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

		pak_key_t* item = pak->head;
		while(item)
		{
			LOGI("%i %s", item->size, item->key);
			item = item->next;
		}
	}
	else
	{
		usage(arg0);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
