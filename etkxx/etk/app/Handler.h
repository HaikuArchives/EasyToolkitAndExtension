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
 * File: Handler.h
 * Description: Basic object model for Application Kit
 *
 * --------------------------------------------------------------------------*/

#ifndef __ETK_HANDLER_H__
#define __ETK_HANDLER_H__

#include <etk/support/Archivable.h>
#include <etk/support/List.h>

#ifdef __cplusplus /* Just for C++ */

class ELooper;
class EMessage;
class EMessageFilter;
class EMessenger;
class EToken;
class _EObserverList;

#define E_OBSERVE_WHAT_CHANGE		"etk:observe_change_what"
#define E_OBSERVE_ORIGINAL_WHAT		"etk:observe_orig_what"
#define E_OBSERVER_OBSERVE_ALL		E_MAXUINT32

class _IMPEXP_ETK EHandler : public EArchivable {
public:
	EHandler(const char *name = NULL);
	virtual ~EHandler();

	// Archiving
	EHandler(const EMessage *from);
	virtual e_status_t Archive(EMessage *into, bool deep = true) const;
	static EArchivable *Instantiate(const EMessage *from);

	void		SetName(const char *name);
	const char	*Name() const;

	virtual void	MessageReceived(EMessage *message);

	ELooper		*Looper() const;

	virtual void	SetNextHandler(EHandler *handler);
	EHandler	*NextHandler() const;

	bool		LockLooper();
	e_status_t	LockLooperWithTimeout(e_bigtime_t microseconds_timeout);
	void		UnlockLooper();

	e_status_t	StartWatching(EMessenger msgr, euint32 what);
	e_status_t	StartWatchingAll(EMessenger msgr);
	e_status_t	StopWatching(EMessenger msgr, euint32 what);
	e_status_t	StopWatchingAll(EMessenger msgr);

	e_status_t	StartWatching(EHandler *handler, euint32 what);
	e_status_t	StartWatchingAll(EHandler *handler);
	e_status_t	StopWatching(EHandler *handler, euint32 what);
	e_status_t	StopWatchingAll(EHandler *handler);

	virtual void	SendNotices(euint32 what, const EMessage *msg = NULL);
	bool		IsWatched(euint32 what = E_OBSERVER_OBSERVE_ALL) const;

	virtual bool	AddFilter(EMessageFilter *filter);
	virtual bool	RemoveFilter(EMessageFilter *filter);
	virtual bool	SetFilterList(const EList *filterList);
	const EList	*FilterList() const;

private:
	friend class ELooper;
	friend class EMessage;

	friend euint64 etk_get_handler_token(const EHandler *handler);
	friend void etk_set_handler_token(EHandler *handler, euint64 token);
	friend ELooper* etk_get_handler_looper(euint64 token);
	friend euint64 etk_get_ref_looper_token(euint64 token);

	EToken *fToken;
	char *fName;
	ELooper *fLooper;
	bool forceSetNextHandler;
	EHandler *fNextHandler;
	_EObserverList *fObserverList;
	EList fFilters;

	void SetLooper(ELooper *looper);
};

#endif /* __cplusplus */

#endif /* __ETK_HANDLER_H__ */


