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
#include "pak_file.h"

#define LOG_TAG "pak"
#include "pak_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int pak_file_addkey(pak_file_t* self, int size, char* key, int copy)
{
	assert(self);
	assert(key);
	LOGD("debug size=%i, key=%s, copy=%i", size, key, copy);

	pak_item_t* item = (pak_item_t*) malloc(sizeof(pak_item_t));
	if(item == NULL)
	{
		LOGE("malloc failed");
		return 0;
	}

	int len = strnlen(key, 255) + 1;
	if(copy)
	{
		item->key = (char*) malloc(len*sizeof(char));
		if(item->key == NULL)
		{
			LOGE("malloc failed");
			goto fail_copy;
		}
		strncpy(item->key, key, len);
	}
	else
	{
		item->key = key;
	}
	item->key[len - 1] = '\0';
	item->size = size;
	item->next = NULL;

	// store item in list
	if(self->head == NULL)
	{
		self->head = item;
	}
	else
	{
		self->tail->next = item;
	}
	self->tail = item;

	// success
	return 1;

	// failure
	fail_copy:
		free(item);
	return 0;
}

static int pak_file_readkeys(pak_file_t* self, int* offset)
{
	assert(self);
	LOGD("debug");

	int magic = 0;
	if((fread(&magic, sizeof(int), 1, self->f) != 1) ||
	   (magic != PAK_MAGIC))
	{
		LOGE("invalid magic=0x%X", magic);
		return 0;
	}

	// read footer offset
	if(fread(offset, sizeof(int), 1, self->f) != 1)
	{
		LOGE("invalid offset");
		return 0;
	}

	// check footer offset
	if(*offset <= (int) sizeof(int))
	{
		LOGE("invalid offset=%i", *offset);
		return 0;
	}

	// seek to the footer
	if(fseek(self->f, (long) (*offset), SEEK_SET) == -1)
	{
		LOGE("fseek failed");
		return 0;
	}

	// read key count
	int count = 0;
	if(fread(&count, sizeof(int), 1, self->f) != 1)
	{
		LOGE("invalid count");
		return 0;
	}

	// read keys
	int i;
	for(i = 0; i < count; ++i)
	{
		// read key size
		int size;
		if(fread(&size, sizeof(int), 1, self->f) != 1)
		{
			LOGE("invalid size");
			return 0;
		}

		// read key len
		int len;
		if(fread(&len, sizeof(int), 1, self->f) != 1)
		{
			LOGE("invalid len");
			return 0;
		}

		// allocate key string
		char* key = (char*) malloc(len*sizeof(char));
		if(key == NULL)
		{
			LOGE("malloc failed");
			return 0;
		}

		// read key
		if(fread(key, len*sizeof(char), 1, self->f) != 1)
		{
			LOGE("invalid key");
			free(key);
			return 0;
		}
		key[len - 1] = '\0';

		// append key
		if(pak_file_addkey(self, size, key, 0) == 0)
		{
			free(key);
			return 0;
		}
	}

	return 1;
}

static void pak_file_freekeys(pak_file_t* self)
{
	assert(self);
	LOGD("debug");

	pak_item_t* item = self->head;
	while(item)
	{
		pak_item_t* next = item->next;
		free(item->key);
		free(item);
		item = next;
	}
	self->head = NULL;
	self->tail = NULL;
}

static void pak_file_updatetailsize(pak_file_t* self)
{
	assert(self);
	LOGD("debug");

	if(self->tail)
	{
		int start = 2*sizeof(int);
		pak_item_t* item = self->head;
		while(item != self->tail)
		{
			start += item->size;
			item   = item->next;
		}

		int end = (int) ftell(self->f);
		self->tail->size = end - start;
	}
}

static int pak_file_writefooter(pak_file_t* self)
{
	assert(self);
	LOGD("debug");

	int count = 0;
	pak_item_t* item = self->head;
	while(item)
	{
		++count;
		item = item->next;
	}

	// footer offset
	int footer = (int) ftell(self->f);

	// write the key count
	if(fwrite(&count, sizeof(int), 1, self->f) != 1)
	{
		LOGE("fwrite failed");
		return 0;
	}

	// write keys
	item = self->head;
	while(item)
	{
		if(fwrite(&item->size, sizeof(int), 1, self->f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}

		int len = strnlen(item->key, 255) + 1;
		if(fwrite(&len, sizeof(int), 1, self->f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}

		if(fwrite(item->key, len*sizeof(char), 1, self->f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}

		item = item->next;
	}

	// write the footer offset
	if(fseek(self->f, sizeof(int), SEEK_SET) == -1)
	{
		LOGE("fseek failed");
		return 0;
	}
	if(fwrite(&footer, sizeof(int), 1, self->f) != 1)
	{
		return 0;
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

pak_item_t* pak_item_next(pak_item_t* self)
{
	assert(self);
	LOGD("debug");

	return self->next;
}

const char* pak_item_key(pak_item_t* self)
{
	assert(self);
	LOGD("debug");

	return (const char*) self->key;
}

int pak_item_size(pak_item_t* self)
{
	assert(self);
	LOGD("debug");

	return self->size;
}

pak_file_t* pak_file_open(const char* fname, int flags)
{
	assert(fname);
	LOGD("debug fname=%s, flags=0x%X", fname, flags);

	pak_file_t* self = (pak_file_t*) malloc(sizeof(pak_file_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->f     = NULL;
	self->flags = flags;
	self->head  = NULL;
	self->tail  = NULL;

	if(flags == PAK_FLAG_READ)
	{
		self->f = fopen(fname, "r");
	}
	else if(flags == PAK_FLAG_WRITE)
	{
		self->f = fopen(fname, "w");
	}
	else if(flags == PAK_FLAG_APPEND)
	{
		// create files if they do not exist
		self->f = fopen(fname, "r+");
		if(self->f == NULL)
		{
			self->flags &= ~PAK_FLAG_READ;
			self->f = fopen(fname, "w");
		}
	}

	if(self->f == NULL)
	{
		LOGE("invalid fname=%s, flags=0x%X", fname, flags);
		goto fail_fopen;
	}

	if(self->flags & PAK_FLAG_READ)
	{
		int offset = 0;
		if(pak_file_readkeys(self, &offset) == 0)
		{
			goto fail_keys;
		}

		// append existing pak file
		if(self->flags & PAK_FLAG_WRITE)
		{
			// clear the read flag after reading keys
			self->flags &= ~PAK_FLAG_READ;

			// clear footer offset
			int zero = 0;
			if(fseek(self->f, (long) sizeof(int), SEEK_SET) == -1)
			{
				LOGE("fseek failed");
				goto fail_append;
			}

			if(fwrite(&zero, sizeof(int), 1, self->f) != 1)
			{
				LOGE("fwrite failed fname=%s", fname);
				goto fail_append;
			}

			// seek to the footer
			if(fseek(self->f, (long) offset, SEEK_SET) == -1)
			{
				LOGE("fseek failed");
				goto fail_append;
			}
		}
	}
	else if(self->flags & PAK_FLAG_WRITE)
	{
		// write magic and dummy footer offset
		int magic  = PAK_MAGIC;
		int offset = 0;
		if(fwrite(&magic, sizeof(int), 1, self->f) != 1)
		{
			LOGE("fwrite failed fname=%s", fname);
			goto fail_header;
		}
		if(fwrite(&offset, sizeof(int), 1, self->f) != 1)
		{
			LOGE("fwrite failed fname=%s", fname);
			goto fail_header;
		}
	}

	// success
	return self;

	// failure
	fail_header:
	fail_append:
	fail_keys:
		pak_file_freekeys(self);
		fclose(self->f);
	fail_fopen:
		free(self);
	return NULL;
}

int pak_file_close(pak_file_t** _self)
{
	assert(_self);

	int ret = 1;
	pak_file_t* self = *_self;
	if(self)
	{
		LOGD("debug");

		if(self->flags & PAK_FLAG_WRITE)
		{
			pak_file_updatetailsize(self);
			ret = pak_file_writefooter(self);
		}

		pak_file_freekeys(self);
		fclose(self->f);
		free(self);
		*_self = NULL;
	}
	return ret;
}

int pak_file_closeErr(pak_file_t** _self)
{
	assert(_self);

	int ret = 1;
	pak_file_t* self = *_self;
	if(self)
	{
		LOGD("debug");

		if(self->flags & PAK_FLAG_WRITE)
		{
			// do not write the footer on error
			// causing the footer offset to be zero
			// and the file to become invalid
		}

		pak_file_freekeys(self);
		fclose(self->f);
		free(self);
		*_self = NULL;
	}
	return ret;
}

int pak_file_writek(pak_file_t* self, const char* key)
{
	assert(self);
	assert(key);
	LOGD("debug key=%s", key);

	if((self->flags & PAK_FLAG_WRITE) == 0)
	{
		LOGE("invalid key=%s, flags=0x%X", key, self->flags);
		return 0;
	}

	pak_file_updatetailsize(self);

	return pak_file_addkey(self, 0, (char*) key, 1);
}

int pak_file_write(pak_file_t* self, const void* ptr, int size, int nmemb)
{
	assert(self);
	assert(ptr);
	LOGD("debug size=%i, nmemb=%i", size, nmemb);

	return fwrite(ptr, size, nmemb, self->f);
}

int pak_file_seek(pak_file_t* self, const char* key)
{
	assert(self);
	assert(key);
	LOGD("debug key=%s", key);

	if((self->flags & PAK_FLAG_READ) == 0)
	{
		LOGE("invalid key=%s, flags=0x%X", key, self->flags);
		return 0;
	}

	// find key
	int         start = 2*sizeof(int);
	pak_item_t* item  = self->head;
	while(item)
	{
		if(strncmp(key, item->key, 256) == 0)
		{
			break;
		}
		start += item->size;
		item   = item->next;
	}

	if(item == NULL)
	{
		// not found
		return 0;
	}

	// seek to desired offset
	if(fseek(self->f, (long) start, SEEK_SET) == -1)
	{
		LOGE("fseek failed");
		return 0;
	}
	return item->size;
}

int pak_file_read(pak_file_t* self, void* ptr, int size, int nmemb)
{
	assert(self);
	assert(ptr);
	LOGD("debug size=%i, nmemb=%i", size, nmemb);

	return fread(ptr, size, nmemb, self->f);
}

pak_item_t* pak_file_head(pak_file_t* self)
{
	assert(self);
	LOGD("debug");

	return self->head;
}
