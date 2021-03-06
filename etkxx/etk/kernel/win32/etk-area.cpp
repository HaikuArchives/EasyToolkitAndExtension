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
 * File: etk-area.cpp
 *
 * --------------------------------------------------------------------------*/

#include <windows.h>
//#include <sddl.h>

#include <etk/config.h>
#include <etk/kernel/Kernel.h>
#include <etk/support/String.h>

#define WIN32_AREA_INFO_MAGIC		0xABFC

// return value must be free by "free()"
static char* etk_area_ipc_name(const char *name, const char *domain)
{
	if(name == NULL || *name == 0 || strlen(name) > E_OS_NAME_LENGTH || !domain || strlen(domain) != 4) return NULL;

	const char *prefix = "__etk_";

	return e_strdup_printf("%s%s%s%s", prefix, domain, "_area_", name);
}


class etk_win32_area_locker_t {
public:
	etk_win32_area_locker_t()
	{
		const char *lockerName = "_etk_area_global_";
		if((iLocker = OpenMutex(MUTEX_ALL_ACCESS, FALSE, lockerName)) == NULL)
			iLocker = CreateMutex(NULL, FALSE, lockerName);
		if(iLocker == NULL) ETK_ERROR("[KERNEL]: Can't initialize global area!");
	}

	~etk_win32_area_locker_t()
	{
		if(iLocker) CloseHandle(iLocker);
	}

	void Lock() {WaitForSingleObject(iLocker, INFINITE);}
	void Unlock() {ReleaseMutex(iLocker);}

	HANDLE iLocker;
};

static etk_win32_area_locker_t __etk_area_locker__;

static void _ETK_LOCK_AREA_()
{
	__etk_area_locker__.Lock();
}

static void _ETK_UNLOCK_AREA_()
{
	__etk_area_locker__.Unlock();
}


typedef struct etk_win32_area_info_t {
	eint32		magic;
	bool		closed;
	size_t		length;
} etk_win32_area_info_t;


typedef struct etk_win32_area_t {
	etk_win32_area_t()
		: name(NULL), domain(NULL), ipc_name(NULL), prot(0), length(0), addr(NULL), mapping(NULL), openedIPC(false), created(false)
	{
	}

	~etk_win32_area_t()
	{
		if(created)
		{
			created = false;
			etk_delete_area((void*)this);
		}
	}

	char		*name;
	char		*domain;
	char		*ipc_name;
	euint32		prot;
	size_t		length;
	void		*addr;
	HANDLE		mapping;
	bool		openedIPC;
	bool		created;
} etk_win32_area_t;


_IMPEXP_ETK void*
etk_create_area(const char *name, void **start_addr, size_t size, euint32 protection, const char *domain, etk_area_access area_access)
{
	if(size <= 0) return NULL;

	char *ipc_name = etk_area_ipc_name(name, domain);
	if(!ipc_name) return NULL;

	etk_win32_area_t *area = new etk_win32_area_t();
	if(!area)
	{
		free(ipc_name);
		return NULL;
	}

	area->prot = protection;

#if 0
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	EString szStr;
	szStr << "D:";				// Discretionary ACL
	szStr << "(D;OICI;GA;;;BG)";		// Deny access to built-in guests
	szStr << "(D;OICI;GA;;;AN)";		// Deny access to anonymous logon
	szStr << "(A;OICI;GA;;;BA)";		// Allow full control to administrators
	szStr << "(A;OICI;GA;;;CO)";		// Allow full control to creator owner

	if((area_access & ETK_AREA_ACCESS_GROUP_READ) || (area_access & ETK_AREA_ACCESS_GROUP_WRITE))
	{
		if(area_access & ETK_AREA_ACCESS_GROUP_WRITE)
			szStr << "(A;OICI;GA;;;CG)";	// Allow full control to creator group
		else
			szStr << "(A;OICI;GR;;;CG)";	// Allow read control to creator group
	}

	if((area_access & ETK_AREA_ACCESS_OTHERS_READ) || (area_access & ETK_AREA_ACCESS_OTHERS_WRITE))
	{
		if(area_access & ETK_AREA_ACCESS_OTHERS_WRITE)
			szStr << "(A;OICI;GA;;;BU)";	// Allow full control to others
		else
			szStr << "(A;OICI;GR;;;BU)";	// Allow read control to others
	}

	if(!ConvertStringSecurityDescriptorToSecurityDescriptor(szStr.String(), SDDL_REVISION_1, (PSECURITY_DESCRIPTOR*)&(sa.lpSecurityDescriptor), NULL))
	{
		delete area;
		free(ipc_name);
		return NULL;
	}
#endif

	HANDLE handler;

	_ETK_LOCK_AREA_();
	if((handler = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, size + sizeof(etk_win32_area_info_t), ipc_name)) == NULL)
	{
		_ETK_UNLOCK_AREA_();
		ETK_DEBUG("[KERNEL]: %s --- Can't create area : CreateFileMapping failed.", __PRETTY_FUNCTION__);
		free(ipc_name);
		delete area;
		return NULL;
	}

//	if(sa.lpSecurityDescriptor != NULL) LocalFree(sa.lpSecurityDescriptor);

	DWORD prot = (protection & E_WRITE_AREA ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ);

	if((area->addr = MapViewOfFile(handler, prot, 0, 0, 0)) == NULL)
	{
		ETK_DEBUG("[KERNEL]: %s --- Can't create area : MapViewOfFile failed.", __PRETTY_FUNCTION__);
		CloseHandle(handler);
		_ETK_UNLOCK_AREA_();
		free(ipc_name);
		delete area;
		return NULL;
	}

	etk_win32_area_info_t area_info;
	area_info.magic = WIN32_AREA_INFO_MAGIC;
	area_info.closed = false;
	area_info.length = size;
	memcpy(area->addr, &area_info, sizeof(etk_win32_area_info_t));

	area->length = size;
	area->mapping = handler;
	area->name = e_strdup(name);
	area->domain = e_strdup(domain);
	area->ipc_name = ipc_name;
	area->created = true;

	_ETK_UNLOCK_AREA_();

	if(start_addr) *start_addr = (void*)((char*)area->addr + sizeof(etk_win32_area_info_t));
	return area;
}


_IMPEXP_ETK void*
etk_clone_area(const char *name, void **dest_addr, euint32 protection, const char *domain)
{
	char *ipc_name = etk_area_ipc_name(name, domain);
	if(!ipc_name) return NULL;

	etk_win32_area_t *area = new etk_win32_area_t();
	if(!area)
	{
		free(ipc_name);
		return NULL;
	}

	area->prot = protection;

	DWORD prot = (protection & E_WRITE_AREA ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ);

	HANDLE handler;

	_ETK_LOCK_AREA_();
	if((handler = OpenFileMapping(prot, FALSE, ipc_name)) == NULL)
	{
//		ETK_DEBUG("[KERNEL]: %s --- Can't clone area : open file mapping failed."", __PRETTY_FUNCTION__);
		_ETK_UNLOCK_AREA_();
		free(ipc_name);
		delete area;
		return NULL;
	}

	if((area->addr = MapViewOfFile(handler, prot, 0, 0, 0)) == NULL)
	{
		ETK_DEBUG("[KERNEL]: %s --- Can't clone area : MapViewOfFile failed.", __PRETTY_FUNCTION__);
		CloseHandle(handler);
		_ETK_UNLOCK_AREA_();
		free(ipc_name);
		delete area;
		return NULL;
	}

	etk_win32_area_info_t area_info;
	bzero(&area_info, sizeof(etk_win32_area_info_t));
	memcpy(&area_info, area->addr, sizeof(etk_win32_area_info_t));
	if(area_info.magic != WIN32_AREA_INFO_MAGIC || area_info.closed)
	{
		ETK_WARNING("[KERNEL]: %s --- FileMapping(%s) seem not created by ETK", __PRETTY_FUNCTION__, ipc_name);
		UnmapViewOfFile(area->addr);
		CloseHandle(handler);
		_ETK_UNLOCK_AREA_();
		free(ipc_name);
		delete area;
		return NULL;
	}

	area->length = area_info.length;
	area->mapping = handler;
	area->name = e_strdup(name);
	area->domain = e_strdup(domain);
	area->ipc_name = ipc_name;
	area->openedIPC = true;
	area->created = true;

	_ETK_UNLOCK_AREA_();

	if(dest_addr) *dest_addr = (void*)((char*)area->addr + sizeof(etk_win32_area_info_t));
	return area;
}


_IMPEXP_ETK void*
etk_clone_area_by_source(void *source_data, void **dest_addr, euint32 protection)
{
	etk_win32_area_t *source_area = (etk_win32_area_t*)source_data;
	if(!source_area) return NULL;

	return etk_clone_area(source_area->name, dest_addr, protection, source_area->domain);
}


_IMPEXP_ETK e_status_t
etk_get_area_info(void *data, etk_area_info *info)
{
	etk_win32_area_t *area = (etk_win32_area_t*)data;
	if(!area || !info) return E_BAD_VALUE;
	if(!area->name || *(area->name) == 0 || strlen(area->name) > E_OS_NAME_LENGTH ||
	   !area->domain || strlen(area->domain) != 4 ||
	   area->addr == NULL) return E_ERROR;

	bzero(info->name, E_OS_NAME_LENGTH + 1);
	bzero(info->domain, 5);

	info->size = area->length;
	strcpy(info->name, area->name);
	strcpy(info->domain, area->domain);
	info->protection = area->prot;
	info->address = (void*)((char*)area->addr + sizeof(etk_win32_area_info_t));

	return E_OK;
}


_IMPEXP_ETK e_status_t
etk_delete_area(void *data)
{
	etk_win32_area_t *area = (etk_win32_area_t*)data;
	if(!area) return E_BAD_VALUE;

	if(!area->openedIPC && area->mapping && area->addr)
	{
		if(!(area->prot & E_WRITE_AREA))
		{
			void *new_addr = MapViewOfFile(area->mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if(new_addr == NULL) ETK_ERROR("[KERNEL]: %s --- MapViewOfFile again failed.", __PRETTY_FUNCTION__);
			UnmapViewOfFile(area->addr);
			area->addr = new_addr;
		}

		_ETK_LOCK_AREA_();
		etk_win32_area_info_t area_info;
		bzero(&area_info, sizeof(etk_win32_area_info_t));
		memcpy(&area_info, area->addr, sizeof(etk_win32_area_info_t));
		if(area_info.magic == WIN32_AREA_INFO_MAGIC && !area_info.closed)
		{
			area_info.closed = true;
			memcpy(area->addr, &area_info, sizeof(etk_win32_area_info_t));
		}
		_ETK_UNLOCK_AREA_();
	}

	if(area->addr) UnmapViewOfFile(area->addr);
	if(area->mapping) CloseHandle(area->mapping);

	free(area->ipc_name);

	free(area->name);
	free(area->domain);

	if(area->created)
	{
		area->created = false;
		delete area;
	}

	return E_OK;
}


_IMPEXP_ETK e_status_t
etk_delete_area_etc(void *data, bool no_clone)
{
	etk_win32_area_t *area = (etk_win32_area_t*)data;
	if(!area) return E_BAD_VALUE;

	if(no_clone && (area->openedIPC ? (area->prot & E_WRITE_AREA) : true) && area->mapping && area->addr)
	{
		if(!(area->prot & E_WRITE_AREA))
		{
			void *new_addr = MapViewOfFile(area->mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if(new_addr == NULL) ETK_ERROR("[KERNEL]: %s --- MapViewOfFile again failed.", __PRETTY_FUNCTION__);
			UnmapViewOfFile(area->addr);
			area->addr = new_addr;
		}

		_ETK_LOCK_AREA_();
		etk_win32_area_info_t area_info;
		bzero(&area_info, sizeof(etk_win32_area_info_t));
		memcpy(&area_info, area->addr, sizeof(etk_win32_area_info_t));
		if(area_info.magic == WIN32_AREA_INFO_MAGIC && !area_info.closed)
		{
			area_info.closed = true;
			memcpy(area->addr, &area_info, sizeof(etk_win32_area_info_t));
		}
		_ETK_UNLOCK_AREA_();
	}

	if(area->addr) UnmapViewOfFile(area->addr);
	if(area->mapping) CloseHandle(area->mapping);

	free(area->ipc_name);

	free(area->name);
	free(area->domain);

	if(area->created)
	{
		area->created = false;
		delete area;
	}

	return E_OK;
}


_IMPEXP_ETK e_status_t
etk_resize_area(void *data, void **start_addr, size_t new_size)
{
	ETK_WARNING("%s: Not supported.", __PRETTY_FUNCTION__);
	return E_ERROR;
}


_IMPEXP_ETK e_status_t
etk_set_area_protection(void *data, euint32 new_protection)
{
	ETK_WARNING("%s: Not supported.", __PRETTY_FUNCTION__);
	return E_ERROR;
}

