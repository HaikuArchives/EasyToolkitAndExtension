/* --------------------------------------------------------------------------
 *
 * ETK++ --- The Easy Toolkit for C++ programing
 * Copyright (C) 2004-2006, Anthony Lee, All Rights Reserved
 *
 * ETK++ library is a freeware; it may be used and distributed according to
 * the terms of The MIT License.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * File: etk-image.cpp
 *
 * --------------------------------------------------------------------------*/

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

#include <etk/kernel/Kernel.h>
#include <etk/storage/Path.h>
#include <etk/support/String.h>

typedef struct etk_win32_image_t {
	etk_win32_image_t()
		: image(NULL), created(false)
	{
		bzero(name, E_MAXPATH + 1);
	}

	~etk_win32_image_t()
	{
		if(created)
		{
			created = false;
			etk_unload_addon((void*)this);
		}
	}

	char		name[E_MAXPATH + 1];
	HMODULE		image;

	bool		created;
} etk_win32_image_t;


_IMPEXP_ETK void*
etk_load_addon(const char* path)
{
	EPath aPath(path, NULL, true);
	EString filename = aPath.Path();

#ifdef __CYGWIN__
	char buf[E_MAXPATH + 1];
	bzero(buf, E_MAXPATH + 1);
	cygwin_conv_to_full_win32_path(filename.String(), buf);
	filename.SetTo(buf);
#endif

	filename.ReplaceAll("/", "\\");

	if(filename.Length() <= 0 || filename.Length() > E_MAXPATH) return NULL;

	etk_win32_image_t *image = new etk_win32_image_t();
	if(!image) return NULL;

	memcpy(image->name, aPath.Path(), (size_t)strlen(aPath.Path()));

	if((image->image = LoadLibrary(filename.String())) == NULL)
	{
		delete image;
		return NULL;
	}

	image->created = true;

	return (void*)image;
}


_IMPEXP_ETK e_status_t
etk_unload_addon(void *data)
{
	etk_win32_image_t *image = (etk_win32_image_t*)data;
	if(!image) return E_ERROR;

	if(FreeLibrary(image->image) == 0) return E_ERROR;

	if(image->created)
	{
		image->created = false;
		delete image;
	}

	return E_OK;
}


_IMPEXP_ETK e_status_t
etk_get_image_symbol(void *data, const char *name, void **ptr)
{
	etk_win32_image_t *image = (etk_win32_image_t*)data;
	if(!image || !name || *name == 0 || !ptr) return E_BAD_VALUE;

	FARPROC aProc = GetProcAddress(image->image, name);

	if(aProc == NULL) return E_ERROR;

	*ptr = (void*)aProc;

	return E_OK;
}

