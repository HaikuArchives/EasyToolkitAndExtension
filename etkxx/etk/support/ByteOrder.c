/* --------------------------------------------------------------------------
 *
 * ETK++ --- The Easy Toolkit for C++ programing
 * Copyright (C) 2004-2007, Anthony Lee, All Rights Reserved
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
 * File: ByteOrder.c
 *
 * --------------------------------------------------------------------------*/

#include "ByteOrder.h"


_IMPEXP_ETK e_status_t e_swap_data(e_type_code type, void *_data, size_t len, e_swap_action action)
{
	e_status_t retVal = E_BAD_VALUE;

	if(_data == NULL || len == 0) return E_BAD_VALUE;

	switch(action)
	{
#ifdef ETK_LITTLE_ENDIAN
		case E_SWAP_HOST_TO_LENDIAN:
		case E_SWAP_LENDIAN_TO_HOST:
#else
		case E_SWAP_HOST_TO_BENDIAN:
		case E_SWAP_BENDIAN_TO_HOST:
#endif
			return E_OK;

		default:
			break;
	}

	switch(type)
	{
		case E_INT8_TYPE:
		case E_UINT8_TYPE:
		case E_CHAR_TYPE:
		case E_STRING_TYPE:
			retVal = E_OK;
			break;

		case E_INT16_TYPE:
		case E_UINT16_TYPE:
			if(len % 2 == 0)
			{
				euint16 *data = (euint16*)_data;
				for(len = len / 2; len > 0; len--, data++) *data = E_SWAP_INT16(*data);
				retVal = E_OK;
			}
			break;

		case E_INT32_TYPE:
		case E_UINT32_TYPE:
			if(len % 4 == 0)
			{
				euint32 *data = (euint32*)_data;
				for(len = len / 4; len > 0; len--, data++) *data = E_SWAP_INT32(*data);
				retVal = E_OK;
			}
			break;

		case E_INT64_TYPE:
		case E_UINT64_TYPE:
			if(len % 8 == 0)
			{
				euint64 *data = (euint64*)_data;
				for(len = len / 8; len > 0; len--, data++) *data = E_SWAP_INT64(*data);
				retVal = E_OK;
			}
			break;

		default:
			/* TODO: other types */
			break;
	}

	return retVal;
}


_IMPEXP_ETK bool e_is_type_swapped(e_type_code type)
{
	switch(type)
	{
		case E_ANY_TYPE:
		case E_BOOL_TYPE:
		case E_CHAR_TYPE:
		case E_DOUBLE_TYPE:
		case E_FLOAT_TYPE:
		case E_INT64_TYPE:
		case E_INT32_TYPE:
		case E_INT16_TYPE:
		case E_INT8_TYPE:
		case E_MESSAGE_TYPE:
		case E_MESSENGER_TYPE:
		case E_POINTER_TYPE:
		case E_SIZE_T_TYPE:
		case E_SSIZE_T_TYPE:
		case E_STRING_TYPE:
		case E_UINT64_TYPE:
		case E_UINT32_TYPE:
		case E_UINT16_TYPE:
		case E_UINT8_TYPE:
		case E_POINT_TYPE:
		case E_RECT_TYPE:
		case E_MIME_TYPE:
			return true;

		default:
			return false;
	}
}

