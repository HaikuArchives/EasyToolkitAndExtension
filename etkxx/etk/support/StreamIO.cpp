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
 * File: StreamIO.cpp
 *
 * --------------------------------------------------------------------------*/

#include <etk/support/String.h>
#include <etk/private/StandardIO.h>

#include "StreamIO.h"


static EStandardIO _EIn(0);
static EStandardIO _EOut(1);
static EStandardIO _EErr(2);

_IMPEXP_ETK EStreamIO &EIn = _EIn;
_IMPEXP_ETK EStreamIO &EOut = _EOut;
_IMPEXP_ETK EStreamIO &EErr = _EErr;


EStreamIO::EStreamIO()
	: EDataIO()
{
}


EStreamIO::~EStreamIO()
{
}


ssize_t
EStreamIO::Read(void *buffer, size_t size)
{
	return E_ERROR;
}


ssize_t
EStreamIO::Write(const void *buffer, size_t size)
{
	return E_ERROR;
}


EStreamIO&
EStreamIO::operator<<(const char *str)
{
	if(!(str == NULL || *str == 0)) Write(str, strlen(str));
	return *this;
}


EStreamIO&
EStreamIO::operator<<(char c)
{
	Write(&c, 1);
	return *this;
}


EStreamIO&
EStreamIO::operator<<(eint8 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(euint8 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(eint16 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(euint16 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(eint32 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(euint32 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(eint64 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(euint64 value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(float value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}


EStreamIO&
EStreamIO::operator<<(double value)
{
	EString str;
	str << value;
	Write(str.String(), str.Length());
	return *this;
}

