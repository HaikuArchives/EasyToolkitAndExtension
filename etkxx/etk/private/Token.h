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
 * File: Token.h
 *
 * --------------------------------------------------------------------------*/

#ifndef __ETK_PRIVATE_TOKEN_H__
#define __ETK_PRIVATE_TOKEN_H__

#include <etk/support/Locker.h>

#ifdef __cplusplus /* Just for C++ */


class ETokensDepot;


class _LOCAL EToken {
public:
	EToken();
	~EToken();

	euint64		Token() const;
	e_bigtime_t	TimeStamp() const;
	euint64		Vitalities() const;

	void		*Data() const;
	void		SetData(void *data);

	ETokensDepot	*Depot() const;
	void		MakeEmpty();

private:
	friend class ETokensDepot;

	bool fOriginal;
	euint64 fToken;

	ETokensDepot *fDepot;
};


class _LOCAL ETokensDepot {
public:
	ETokensDepot(ELocker *locker = NULL,
		     bool deconstruct_locker = false);
	virtual ~ETokensDepot();

	EToken		*CreateToken(void *data = NULL);
	EToken		*OpenToken(euint64 token, EToken *fetch_token = NULL);

	bool		PushToken(euint64 token);
	void		PopToken(euint64 token);

	ELocker		*Locker() const;
	bool		Lock();
	void		Unlock();

private:
	friend class EToken;

	ELocker *fLocker;
	bool fDeconstructLocker;

	void *fData;
};


#endif /* __cplusplus */

#endif /* __ETK_PRIVATE_TOKEN_H__ */

