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
 * File: NetEndpoint.cpp
 *
 * --------------------------------------------------------------------------*/

#include <errno.h>
#include <string.h>

#ifndef _WIN32
#	include <netdb.h>
#	include <netinet/in.h>

#	ifdef __BEOS__
#		include <BeBuild.h>
#	endif

#	if !(defined(__BEOS__) && B_BEOS_VERSION < 0x0510)
#		include <arpa/inet.h>
#	endif
#else
#	include <winsock2.h>
#endif

#include "NetEndpoint.h"


ENetEndpoint::ENetEndpoint(int proto)
	: EArchivable(), fSocket(-1), fTimeout(0)
{
	// TODO
}


ENetEndpoint::ENetEndpoint(const ENetEndpoint &from)
	: EArchivable(), fSocket(-1), fTimeout(0)
{
	ENetEndpoint::operator=(from);
}


ENetEndpoint::~ENetEndpoint()
{
	ENetEndpoint::Close();
}


ENetEndpoint::ENetEndpoint(EMessage *from)
	: EArchivable(from), fSocket(-1), fTimeout(0)
{
	// TODO
}


e_status_t
ENetEndpoint::Archive(EMessage *into, bool deep) const
{
	if(!into) return E_ERROR;

	EArchivable::Archive(into, deep);
	into->AddString("class", "ENetEndpoint");

	// TODO

	return E_OK;
}


EArchivable*
ENetEndpoint::Instantiate(EMessage *from)
{
	if(e_validate_instantiation(from, "ENetEndpoint"))
		return new ENetEndpoint(from);
	return NULL;
}


e_status_t
ENetEndpoint::InitCheck() const
{
	return(fSocket == -1 ? E_ERROR : E_OK);
}


ENetEndpoint&
ENetEndpoint::operator=(const ENetEndpoint &endpoint)
{
	// TODO
	return *this;
}


e_status_t
ENetEndpoint::SetProtocol(int proto)
{
	// TODO
	return E_ERROR;
}


int
ENetEndpoint::SetOption(eint32 option, eint32 level, const void *data, size_t data_len)
{
	// TODO
	return -1;
}


int
ENetEndpoint::SetNonBlocking(bool state)
{
	// TODO
	return -1;
}


int
ENetEndpoint::SetReuseAddr(bool state)
{
	// TODO
	return -1;
}


const ENetAddress&
ENetEndpoint::LocalAddr() const
{
	return fLocalAddr;
}


const ENetAddress&
ENetEndpoint::RemoteAddr() const
{
	return fRemoteAddr;
}


int
ENetEndpoint::Socket() const
{
	return fSocket;
}


void
ENetEndpoint::Close()
{
	// TODO
	if(fSocket != -1)
	{
		close(fSocket);
	}
}


e_status_t
ENetEndpoint::Bind(const ENetAddress &addr)
{
	// TODO
	return E_ERROR;
}


e_status_t
ENetEndpoint::Bind(euint16 port)
{
	// TODO
	return E_ERROR;
}


e_status_t
ENetEndpoint::Connect(const ENetAddress &addr)
{
	// TODO
	return E_ERROR;
}


e_status_t
ENetEndpoint::Connect(const char *addr, euint16 port)
{
	// TODO
	return E_ERROR;
}


e_status_t
ENetEndpoint::Listen(int backlog)
{
	// TODO
	return E_ERROR;
}


ENetEndpoint*
ENetEndpoint::Accept(eint32 timeout_msec)
{
	// TODO
	return NULL;
}


int
ENetEndpoint::Error() const
{
	return errno;
}


const char*
ENetEndpoint::ErrorStr() const
{
	return strerror(errno);
}


eint32
ENetEndpoint::Send(const void *buf, size_t len, int flags)
{
	// TODO
	return -1;
}


eint32
ENetEndpoint::Send(const ENetBuffer &buf, int flags)
{
	return Send(buf.Data(), buf.Size(), flags);
}


eint32
ENetEndpoint::SendTo(const void *buf, size_t len, const ENetAddress &to, int flags)
{
	// TODO
	return -1;
}


eint32
ENetEndpoint::SendTo(const ENetBuffer &buf, const ENetAddress &to, int flags)
{
	return SendTo(buf.Data(), buf.Size(), to, flags);
}


void
ENetEndpoint::SetTimeout(e_bigtime_t timeout)
{
	if(timeout < 0) timeout = 0;
	fTimeout = timeout;

	// TODO
}


eint32
ENetEndpoint::Receive(void *buf, size_t len, int flags)
{
	// TODO
	return -1;
}


eint32
ENetEndpoint::Receive(ENetBuffer &buf, size_t len, int flags)
{
	void *data = (len != 0 ? malloc(len) : NULL);
	if(data == NULL) return -1;

	eint32 bytes = Receive(data, len, flags);
	if(bytes < 0)
	{
		free(data);
		return -1;
	}

	buf = ENetBuffer(bytes);
	buf.AppendData(data, (size_t)bytes);
	free(data);

	return bytes;
}


eint32
ENetEndpoint::ReceiveFrom(void *buf, size_t len, const ENetAddress &from, int flags)
{
	// TODO
	return -1;
}


eint32
ENetEndpoint::ReceiveFrom(ENetBuffer &buf, size_t len, const ENetAddress &from, int flags)
{
	void *data = (len != 0 ? malloc(len) : NULL);
	if(data == NULL) return -1;

	eint32 bytes = ReceiveFrom(data, len, from, flags);
	if(bytes < 0)
	{
		free(data);
		return -1;
	}

	buf = ENetBuffer(bytes);
	buf.AppendData(data, (size_t)bytes);
	free(data);

	return bytes;
}


bool
ENetEndpoint::IsDataPending(e_bigtime_t timeout)
{
	// TODO
	return false;
}

