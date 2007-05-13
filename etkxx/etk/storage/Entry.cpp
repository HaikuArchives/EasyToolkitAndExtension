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
 * File: Entry.cpp
 *
 * --------------------------------------------------------------------------*/

#ifndef _WIN32
#define __USE_LARGEFILE64
#define __USE_FILE_OFFSET64
#include <unistd.h>
#else
#include <io.h>
#include <windows.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <etk/config.h>
#include <etk/support/String.h>

#include "Entry.h"

// implement in "Path.cpp"
extern e_status_t etk_path_expound(EString &path, const char *dir, const char *leaf, bool *normalize);
extern e_status_t etk_path_get_parent(EString &parent, const char *path);


EEntry::EEntry()
	: fName(NULL)
{
}


EEntry::EEntry(const char *path, bool traverse)
	: fName(NULL)
{
	SetTo(path, traverse);
}


EEntry::~EEntry()
{
	if(fName != NULL) delete[] fName;
}


e_status_t
EEntry::SetTo(const char *path, bool traverse)
{
	if(path == NULL) return E_BAD_VALUE;

	EString str;
	if(etk_path_expound(str, path, NULL, NULL) != E_OK) return E_BAD_VALUE;

	EString parent;
	e_status_t status = etk_path_get_parent(parent, str.String());
	if(status == E_ENTRY_NOT_FOUND) parent = str;
	else if(status != E_OK) return E_BAD_VALUE;

#ifdef _WIN32
	parent.ReplaceAll("/", "\\");
#endif

	bool parentExists;
#ifndef _WIN32
	parentExists = (access(parent.String(), F_OK) == 0);
#else
	parentExists = (_access(parent.String(), 0) == 0);
#endif
	if(!parentExists) return E_ENTRY_NOT_FOUND;

	// TODO: traverse

	char *name = EStrdup(str.String());
	if(name == NULL) return E_NO_MEMORY;

	if(fName != NULL) delete[] fName;
	fName = name;

	return E_OK;
}


void
EEntry::Unset()
{
	if(fName != NULL) delete[] fName;
	fName = NULL;
}


e_status_t
EEntry::InitCheck() const
{
	if(fName == NULL) return E_NO_INIT;
	return E_OK;
}


bool
EEntry::Exists() const
{
	if(fName == NULL) return false;

	const char *filename = (const char*)fName;

#ifdef _WIN32
	EString str(fName);
	str.ReplaceAll("/", "\\");
	filename = str.String();
#endif

#ifndef _WIN32
	return(access(filename, F_OK) == 0);
#else
	return(_access(filename, 0) == 0);
#endif
}


bool
EEntry::IsHidden() const
{
	bool retVal = false;

	EPath aPath(fName);
	const char *leaf = aPath.Leaf();
	if(!(leaf == NULL || *leaf != '.')) retVal = true;

#ifdef _WIN32
	EString str(fName);
	str.ReplaceAll("/", "\\");

	if(GetFileAttributes(str.String()) & FILE_ATTRIBUTE_HIDDEN) retVal = true;
#endif

	return retVal;
}


bool
EEntry::IsDirectory() const
{
	if(fName == NULL) return false;

	const char *filename = (const char*)fName;

#ifdef _WIN32
	EString str(fName);
	str.ReplaceAll("/", "\\");
	filename = str.String();

	struct _stat st;
	if(_stat(filename, &st) != 0) return false;
	return((st.st_mode & _S_IFDIR) ? true : false);
#else
	struct stat st;
	if(stat(filename, &st) != 0) return false;
	return S_ISDIR(st.st_mode);
#endif
}


e_status_t
EEntry::GetSize(eint64 *file_size) const
{
	if(fName == NULL || file_size == NULL) return E_ERROR;

	const char *filename = (const char*)fName;

#ifdef _WIN32
	EString str(fName);
	str.ReplaceAll("/", "\\");
	filename = str.String();

	struct _stati64 stat;
	if(_stati64(filename, &stat) != 0) return E_ERROR;
	*file_size = (eint64)stat.st_size;
#elif defined(HAVE_STAT64)
	struct stat64 stat;
	if(stat64(filename, &stat) != 0) return E_ERROR;
	*file_size = (eint64)stat.st_size;
#else
	struct stat st;
	if(stat(filename, &st) != 0) return E_ERROR;
	*file_size = (eint64)st.st_size;
#endif

	return E_OK;
}


e_status_t
EEntry::GetModifiedTime(e_bigtime_t *time) const
{
	if(fName == NULL || time == NULL) return E_ERROR;

	const char *filename = (const char*)fName;

#ifdef _WIN32
	EString str(fName);
	str.ReplaceAll("/", "\\");
	filename = str.String();

	struct _stat stat;
	if(_stat(filename, &stat) != 0) return E_ERROR;
	*time = E_INT64_CONSTANT(1000000) * (e_bigtime_t)stat.st_mtime;
#else
	struct stat st;
	if(stat(filename, &st) != 0) return E_ERROR;
	*time = E_INT64_CONSTANT(1000000) * (e_bigtime_t)st.st_mtime;
#endif

	return E_OK;
}


e_status_t
EEntry::GetPath(EPath *path) const
{
	if(path == NULL) return E_BAD_VALUE;
	if(fName == NULL) {path->Unset(); return E_NO_INIT;}

	path->SetTo(fName, NULL, false);
	return E_OK;
}

