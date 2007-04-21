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
 * File: _Message.h
 * Description: inline functions like BMessage
 *
 * --------------------------------------------------------------------------*/

#ifndef __ETK__MESSAGE_H__
#define __ETK__MESSAGE_H__

#include <etk/app/Message.h>

#ifdef __cplusplus /* Just for C++ */

// Usage:
// 	EMessage *msg;
// 	...
// 	eint32 val;
// 	if(((ETMessage*)msg)->FindInt32("something", &val) == E_OK)
// 	{
// 		...
// 	}
typedef struct ETMessage {
	EMessage fMessage;

	inline e_status_t GetInfo(e_type_code type, eint32 index,
				  char **nameFound, e_type_code *typeFound, eint32 *countFound = NULL) const
	{
		if(index < 0) return E_BAD_INDEX;

		eint32 aIndex = index;

		for(eint32 i = 0; i < fMessage.CountNames(E_ANY_TYPE, true); i++)
		{
			eint32 typesCount = fMessage.CountTypesByName(i);
			for(eint32 k = 0; k < typesCount; k++)
			{
				e_type_code aType;
				eint32 count = fMessage.CountItems(i, k, &aType);
				if(!(type == E_ANY_TYPE || aType == type) || (aIndex--) > 0) continue;
				if(nameFound) *nameFound = (char*)fMessage.NameAt(i);
				if(typeFound) *typeFound = aType;
				if(countFound) *countFound = count;
				return E_OK;
			}
		}

		return(aIndex == index ? E_BAD_TYPE : E_BAD_INDEX);
	}

	inline e_status_t FindString(const char *name, eint32 index, const char **str) const
	{
		return(fMessage.FindString(name, index, str) ? E_OK : E_ERROR);
	}

	inline e_status_t FindString(const char *name, const char **str) const
	{
		return FindString(name, 0, str);
	}

	inline e_status_t FindString(const char *name, eint32 index, EString *str) const
	{
		return(fMessage.FindString(name, index, str) ? E_OK : E_ERROR);
	}

	inline e_status_t FindString(const char *name, EString *str) const
	{
		return FindString(name, 0, str);
	}

	inline e_status_t FindInt8(const char *name, eint32 index, eint8 *val) const
	{
		return(fMessage.FindInt8(name, index, val) ? E_OK : E_ERROR);
	}

	inline e_status_t FindInt8(const char *name, eint8 *val) const
	{
		return FindInt8(name, 0, val);
	}

	inline e_status_t FindInt16(const char *name, eint32 index, eint16 *val) const
	{
		return(fMessage.FindInt16(name, index, val) ? E_OK : E_ERROR);
	}

	inline e_status_t FindInt16(const char *name, eint16 *val) const
	{
		return FindInt16(name, 0, val);
	}

	inline e_status_t FindInt32(const char *name, eint32 index, eint32 *val) const
	{
		return(fMessage.FindInt32(name, index, val) ? E_OK : E_ERROR);
	}

	inline e_status_t FindInt32(const char *name, eint32 *val) const
	{
		return FindInt32(name, 0, val);
	}

	inline e_status_t FindInt64(const char *name, eint32 index, eint64 *val) const
	{
		return(fMessage.FindInt64(name, index, val) ? E_OK : E_ERROR);
	}

	inline e_status_t FindInt64(const char *name, eint64 *val) const
	{
		return FindInt64(name, 0, val);
	}

	inline e_status_t FindBool(const char *name, eint32 index, bool *aBoolean) const
	{
		return(fMessage.FindBool(name, index, aBoolean) ? E_OK : E_ERROR);
	}

	inline e_status_t FindBool(const char *name, bool *aBoolean) const
	{
		return FindBool(name, 0, aBoolean);
	}

	inline e_status_t FindFloat(const char *name, eint32 index, float *f) const
	{
		return(fMessage.FindFloat(name, index, f) ? E_OK : E_ERROR);
	}

	inline e_status_t FindFloat(const char *name, float *f) const
	{
		return FindFloat(name, 0, f);
	}

	inline e_status_t FindDouble(const char *name, eint32 index, double *d) const
	{
		return(fMessage.FindDouble(name, index, d) ? E_OK : E_ERROR);
	}

	inline e_status_t FindDouble(const char *name, double *d) const
	{
		return FindDouble(name, 0, d);
	}

	inline e_status_t FindPoint(const char *name, eint32 index, EPoint *pt) const
	{
		return(fMessage.FindPoint(name, index, pt) ? E_OK : E_ERROR);
	}

	inline e_status_t FindPoint(const char *name, EPoint *pt) const
	{
		return FindPoint(name, 0, pt);
	}

	inline e_status_t FindRect(const char *name, eint32 index, ERect *r) const
	{
		return(fMessage.FindRect(name, index, r) ? E_OK : E_ERROR);
	}

	inline e_status_t FindRect(const char *name, ERect *r) const
	{
		return FindRect(name, 0, r);
	}

	inline e_status_t FindPointer(const char *name, eint32 index, void **ptr) const
	{
		return(fMessage.FindPointer(name, index, ptr) ? E_OK : E_ERROR);
	}

	inline e_status_t FindPointer(const char *name, void **ptr) const
	{
		return FindPointer(name, 0, ptr);
	}

	inline e_status_t FindMessage(const char *name, eint32 index, EMessage *msg) const
	{
		return(fMessage.FindMessage(name, index, msg) ? E_OK : E_ERROR);
	}

	inline e_status_t FindMessage(const char *name, EMessage *msg) const
	{
		return FindMessage(name, 0, msg);
	}

	inline e_status_t FindMessenger(const char *name, eint32 index, EMessenger *msgr) const
	{
		return(fMessage.FindMessenger(name, index, msgr) ? E_OK : E_ERROR);
	}

	inline e_status_t FindMessenger(const char *name, EMessenger *msgr) const
	{
		return FindMessenger(name, 0, msgr);
	}

	inline e_status_t FindData(const char *name, e_type_code type, eint32 index,
				   const void **data, ssize_t *numBytes) const
	{
		if(index < 0) return E_BAD_INDEX;

		eint32 nameIndex = fMessage.FindName(name);
		if(nameIndex < 0) return E_NAME_NOT_FOUND;

		eint32 typesCount = fMessage.CountTypesByName(nameIndex);
		eint32 aIndex = index;

		for(eint32 k = 0; k < typesCount; k++)
		{
			e_type_code aType;
			eint32 count = fMessage.CountItems(nameIndex, k, &aType);
			if(!(type == E_ANY_TYPE || aType == type)) continue;

			if(aIndex < count)
				return(fMessage.FindData(nameIndex, k, aIndex, data, numBytes) ? E_OK : E_ERROR);

			aIndex -= count;
		}

		return(aIndex == index ? E_BAD_TYPE : E_BAD_INDEX);
	}

	inline e_status_t FindData(const char *name, e_type_code type,
				   const void **data, ssize_t *numBytes) const
	{
		return FindData(name, type, 0, data, numBytes);
	}
} ETMessage;

#endif /* __cplusplus */

#endif /* __ETK__MESSAGE_H__ */

