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
 * File: etk-os.cpp
 *
 * --------------------------------------------------------------------------*/

#include <windows.h>

#include <etk/kernel/Kernel.h>
#include <etk/support/String.h>

#include <etk/private/PrivateApplication.h>


HINSTANCE etk_dll_hinstance = NULL;


extern "C" {
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	etk_dll_hinstance = hinstDLL;

	switch(fdwReason) 
	{
		case DLL_PROCESS_ATTACH:
		/* The DLL is being mapped into process's address space */
		/* Do any required initialization on a per application basis, return FALSE if failed */
		{
			WSADATA wsaData;
			WSAStartup(0x202, &wsaData);
			etk_system_boot_time();
			EApplicationConnector::Init();
			break;
		}

		case DLL_THREAD_ATTACH:
		/* A thread is created. Do any required initialization on a per thread basis*/
		{
			break;
		}

		case DLL_PROCESS_DETACH:
		/* The DLL unmapped from process's address space. Do necessary cleanup */
		{
			EApplicationConnector::Quit();
			WSACleanup();
			break;
		}

		case DLL_THREAD_DETACH:
		/* Thread exits with cleanup */
		{
			break;
		}
	}

	return TRUE;
}


char* etk_win32_convert_active_to_utf8(const char *str, eint32 length)
{
	if(str == NULL || *str == 0 || length == 0) return NULL;

	eint32 nChars = (eint32)strlen(str);
	if(length < 0 || length > nChars) length = nChars;

	WCHAR *wStr = (WCHAR*)malloc(sizeof(WCHAR) * (size_t)(length + 1));
	if(wStr == NULL) return NULL;
	bzero(wStr, sizeof(WCHAR) * (size_t)(length + 1));
	MultiByteToWideChar(CP_ACP, 0, str, length, wStr, length);

	char *uStr = e_unicode_convert_to_utf8((const eunichar*)wStr, -1);
	free(wStr);

	return uStr;
}


char* etk_win32_convert_utf8_to_active(const char *str, eint32 length)
{
	eunichar *wStr = e_utf8_convert_to_unicode(str, length);
	if(wStr == NULL) return NULL;

	eint32 len = e_unicode_strlen(wStr);
	char *aStr = (char*)malloc((size_t)len * 3 + 1);
	if(aStr == NULL)
	{
		free(wStr);
		return NULL;
	}

	bzero(aStr, (size_t)len * 3 + 1);
	WideCharToMultiByte(CP_ACP, 0, (WCHAR*)wStr, -1, aStr, len * 3, NULL, NULL);

	free(wStr);

	return aStr;
}

} // extern "C"

