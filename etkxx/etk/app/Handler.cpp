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
 * File: Handler.cpp
 *
 * --------------------------------------------------------------------------*/

#include <etk/kernel/Kernel.h>
#include <etk/support/String.h>
#include <etk/support/List.h>
#include <etk/support/Locker.h>
#include <etk/support/Autolock.h>
#include <etk/support/ClassInfo.h>

#include <etk/private/Token.h>

#include "Handler.h"
#include "Looper.h"
#include "Messenger.h"
#include "MessageFilter.h"


class _EObserverList {
public:
	_EObserverList();
	~_EObserverList();

	e_status_t StartWatching(EMessenger msgr, euint32 what);
	e_status_t StopWatching(EMessenger msgr, euint32 what);
	EList* GetObservers(euint32 what);
	bool IsWatched(euint32 what) const;

private:
	EList fListWatching;
	EList fListWatchingAll;
};


static ELocker etk_handler_operator_locker;
static ETokensDepot handlers_depot(&etk_handler_operator_locker, false);


_LOCAL ELocker* etk_get_handler_operator_locker()
{
	return &etk_handler_operator_locker;
}


_LOCAL euint64 etk_get_handler_token(const EHandler *handler)
{
	return((handler == NULL || handler->fToken == NULL) ? E_MAXUINT64 : handler->fToken->Token());
}


_LOCAL euint64 etk_get_ref_handler_token(const EHandler *handler)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);

	euint64 token = etk_get_handler_token(handler);
	return(handlers_depot.PushToken(token) ? token : E_MAXUINT64);
}


_LOCAL EHandler* etk_get_handler(euint64 token)
{
	EHandler *retVal = NULL;

	EToken *aToken = handlers_depot.OpenToken(token);
	if(aToken != NULL)
	{
		retVal = reinterpret_cast<EHandler*>(aToken->Data());
		delete aToken;
	}

	return retVal;
}


_LOCAL e_bigtime_t etk_get_handler_create_time_stamp(euint64 token)
{
	e_bigtime_t retVal = E_MAXINT64;

	EToken *aToken = handlers_depot.OpenToken(token);
	if(aToken != NULL)
	{
		retVal = aToken->TimeStamp();
		delete aToken;
	}

	return retVal;
}


_LOCAL ELooper* etk_get_handler_looper(euint64 token)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);

	EHandler *handler = etk_get_handler(token);
	return(handler == NULL ? NULL : handler->fLooper);
}


_LOCAL euint64 etk_get_ref_looper_token(euint64 token)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);

	EHandler *handler = etk_get_handler(token);
	return(handler == NULL ? E_MAXUINT64 : etk_get_ref_handler_token(handler->fLooper));
}


_LOCAL e_status_t etk_lock_looper_of_handler(euint64 token, e_bigtime_t timeout)
{
	e_status_t retVal = E_ERROR;

	etk_handler_operator_locker.Lock();
	ELooper *looper = etk_get_handler_looper(token);
	ELooper *looper_proxy = (looper != NULL ? looper->_Proxy() : NULL);
	void *locker = ((looper == NULL || looper->fLocker == NULL) ? NULL : etk_clone_locker(looper->fLocker));
	eint64 locksCount = etk_handler_operator_locker.CountLocks();
	while(etk_handler_operator_locker.CountLocks() > E_INT64_CONSTANT(0)) etk_handler_operator_locker.Unlock();

	if(locker)
	{
		if((retVal = etk_lock_locker_etc(locker, E_TIMEOUT, timeout)) == E_OK)
		{
			etk_handler_operator_locker.Lock();

			if(looper != etk_get_handler_looper(token) || looper_proxy != looper->_Proxy()) retVal = E_ERROR;

			if(locksCount > E_INT64_CONSTANT(1))
				locksCount--;
			else
				etk_handler_operator_locker.Unlock();

			if(retVal != E_OK) etk_unlock_locker(locker);
		}

		etk_delete_locker(locker);
	}

	while(locksCount > E_INT64_CONSTANT(1)) {etk_handler_operator_locker.Lock(); locksCount--;}

	return retVal;
}


_LOCAL bool etk_is_current_at_looper_thread(euint64 token)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);

	ELooper *looper = e_cast_as(etk_get_handler(token), ELooper);
	if(looper == NULL) return false;

	bool retVal = (looper->Thread() == etk_get_current_thread_id() ? true : false);

	return retVal;
}


_LOCAL bool etk_ref_handler(euint64 token)
{
	return handlers_depot.PushToken(token);
}


_LOCAL void etk_unref_handler(euint64 token)
{
	handlers_depot.PopToken(token);
}


EHandler::EHandler(const char *name)
	: EArchivable(),
	  fName(NULL),
	  fLooper(NULL),
	  forceSetNextHandler(false), fNextHandler(NULL)
{
	fName = EStrdup(name);
	fToken = handlers_depot.CreateToken(reinterpret_cast<void*>(this));
	fObserverList = new _EObserverList();
}


EHandler::~EHandler()
{
	while(fFilters.CountItems() > 0)
	{
		EMessageFilter *filter = (EMessageFilter*)fFilters.ItemAt(0);
		EHandler::RemoveFilter(filter);
		delete filter;
	}

	if(fName != NULL) delete[] fName;
	if(fToken != NULL) delete fToken;
	delete fObserverList;
}


EHandler::EHandler(const EMessage *from)
	: EArchivable(from),
	  fName(NULL),
	  fLooper(NULL),
	  forceSetNextHandler(false), fNextHandler(NULL)
{
	ETK_ERROR("[APP]: %s --- unimplemented yet.", __PRETTY_FUNCTION__);
}


e_status_t
EHandler::Archive(EMessage *into, bool deep) const
{
	if(!into) return E_ERROR;

	EArchivable::Archive(into, deep);
	into->AddString("class", "EHandler");

	// TODO

	return E_OK;
}


EArchivable*
EHandler::Instantiate(const EMessage *from)
{
	return(e_validate_instantiation(from, "EHandler") == false ? NULL : new EHandler(from));
}


void
EHandler::SetName(const char *name)
{
	if(fName != NULL) delete[] fName;
	fName = EStrdup(name);
}


const char*
EHandler::Name() const
{
	return fName;
}


void
EHandler::MessageReceived(EMessage *message)
{
	if(fNextHandler != NULL) fNextHandler->MessageReceived(message);
}


void
EHandler::SetNextHandler(EHandler *handler)
{
	if(handler == this || fLooper == NULL || fNextHandler == handler)
	{
		if(handler == this) ETK_WARNING("[APP]: %s --- next-handler is this-handler.", __PRETTY_FUNCTION__);
		else if(fLooper == NULL) ETK_WARNING("[APP]: %s --- this-handler didn't belong to looper.", __PRETTY_FUNCTION__);
		else ETK_WARNING("[APP]: %s --- next-handler already be the next handler of this-handler.", __PRETTY_FUNCTION__);
		return;
	}

	if(forceSetNextHandler)
	{
		fNextHandler = handler;
		forceSetNextHandler = false;
	}
	else
	{
		if(handler == NULL || handler == fLooper)
		{
			ETK_WARNING("[APP]: %s --- next-handler can't be NULL or looper it belong to.", __PRETTY_FUNCTION__);
		}
		else if(handler->fLooper != fLooper)
		{
			ETK_WARNING("[APP]: %s --- this-handler and next-handler didn't belong to the same looper.", __PRETTY_FUNCTION__);
		}
		else
		{
			// reorder
			eint32 handlerIndex = fLooper->fHandlers.IndexOf(handler);
			eint32 selfIndex = fLooper->fHandlers.IndexOf(this);

			if(handlerIndex < 0 || selfIndex < 0 || handlerIndex == selfIndex) return;

			if(handlerIndex - 1 != selfIndex)
			{
				EHandler *handlerPrevHandler = (EHandler*)(fLooper->fHandlers.ItemAt(handlerIndex - 1));
				EHandler *handlerNextHandler = (EHandler*)(fLooper->fHandlers.ItemAt(handlerIndex + 1));
				eint32 toIndex = handlerIndex < selfIndex ? selfIndex : selfIndex + 1;
				if(!(fLooper->fHandlers.MoveItem(handlerIndex, toIndex))) return;

				if(handlerPrevHandler)
				{
					handlerPrevHandler->forceSetNextHandler = true;
					handlerPrevHandler->SetNextHandler(handlerNextHandler);
					if(handlerPrevHandler->forceSetNextHandler)
					{
						handlerPrevHandler->fNextHandler = handlerNextHandler;
						handlerPrevHandler->forceSetNextHandler = false;
					}
				}
			}
		
			fNextHandler = handler;
		}
	}
}


EHandler*
EHandler::NextHandler() const
{
	return fNextHandler;
}


ELooper*
EHandler::Looper() const
{
	return fLooper;
}


void
EHandler::SetLooper(ELooper *looper)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);
	fLooper = looper;
}


bool
EHandler::LockLooper()
{
	return(LockLooperWithTimeout(E_INFINITE_TIMEOUT) == E_OK);
}


e_status_t
EHandler::LockLooperWithTimeout(e_bigtime_t microseconds_timeout)
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);
	return(fLooper ? fLooper->LockWithTimeout(microseconds_timeout) : E_ERROR);
}


void
EHandler::UnlockLooper()
{
	EAutolock <ELocker>autolock(etk_handler_operator_locker);
	if(fLooper) fLooper->Unlock();
}


typedef struct _etk_watching_info_ {
	EMessenger msgr;
	EList whatsList;

	_etk_watching_info_(EMessenger _msgr_)
	{
		msgr = _msgr_;
	}

	~_etk_watching_info_()
	{
		if(!whatsList.IsEmpty())
		{
			for(eint32 i = 0; i < whatsList.CountItems(); i++)
			{
				euint32 *what = (euint32*)whatsList.ItemAt(i);
				if(what) delete what;
			}
			whatsList.MakeEmpty();
		}
	}

	bool IsValid() const
	{
		return msgr.IsValid();
	}

	bool IsSameMessenger(EMessenger _msgr_) const
	{
		return(msgr == _msgr_);
	}

	bool AddWhat(euint32 _what_)
	{
		if(_what_ == E_OBSERVER_OBSERVE_ALL || !msgr.IsValid()) return false;
		euint32 *what = new euint32;
		if(!what) return false;
		*what = _what_;
		if(!whatsList.AddItem((void*)what))
		{
			delete what;
			return false;
		}
		return true;
	}

	bool RemoveWhat(euint32 _what_)
	{
		if(_what_ == E_OBSERVER_OBSERVE_ALL || !msgr.IsValid()) return false;
		for(eint32 i = 0; i < whatsList.CountItems(); i++)
		{
			euint32 *what = (euint32*)whatsList.ItemAt(i);
			if(!what) continue;
			if(*what == _what_)
			{
				if((what = (euint32*)whatsList.RemoveItem(i)) != NULL)
				{
					delete what;
					return true;
				}

				break;
			}
		}
		return false;
	}

	bool HasWhat(euint32 _what_)
	{
		if(_what_ == E_OBSERVER_OBSERVE_ALL || !msgr.IsValid()) return false;
		for(eint32 i = 0; i < whatsList.CountItems(); i++)
		{
			euint32 *what = (euint32*)whatsList.ItemAt(i);
			if(!what) continue;
			if(*what == _what_) return true;
		}
		return false;
	}

	eint32 CountWhats() const
	{
		return whatsList.CountItems();
	}

	void MakeEmpty()
	{
		if(!whatsList.IsEmpty())
		{
			for(eint32 i = 0; i < whatsList.CountItems(); i++)
			{
				euint32 *what = (euint32*)whatsList.ItemAt(i);
				if(what) delete what;
			}
			whatsList.MakeEmpty();
		}
	}
} _etk_watching_info_;


_EObserverList::_EObserverList()
{
}


_EObserverList::~_EObserverList()
{
	if(!fListWatching.IsEmpty())
	{
		for(eint32 i = 0; i < fListWatching.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
			if(aInfo) delete aInfo;
		}
		fListWatching.MakeEmpty();
	}

	if(!fListWatchingAll.IsEmpty())
	{
		for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
			if(aInfo) delete aInfo;
		}
		fListWatchingAll.MakeEmpty();
	}
}


e_status_t
_EObserverList::StartWatching(EMessenger _msgr_, euint32 what)
{
	if(!_msgr_.IsValid()) return E_BAD_HANDLER;

	_etk_watching_info_ *info = NULL;

	eint32 msgrIndex = -1;
	for(eint32 i = 0; i < fListWatching.CountItems(); i++)
	{
		_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
		if(!aInfo) continue;
		if(aInfo->IsSameMessenger(_msgr_))
		{
			msgrIndex = i;
			break;
		}
	}

	eint32 msgrAllIndex = -1;
	for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
	{
		_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
		if(!aInfo) continue;
		if(aInfo->IsSameMessenger(_msgr_))
		{
			msgrAllIndex = i;
			break;
		}
	}

	if(what == E_OBSERVER_OBSERVE_ALL)
	{
		if(msgrIndex >= 0)
		{
			if((info = (_etk_watching_info_*)fListWatching.RemoveItem(msgrIndex)) == NULL) return E_ERROR;
			delete info;
		}

		info = (_etk_watching_info_*)fListWatchingAll.ItemAt(msgrAllIndex);

		if(!info)
		{
			info = new _etk_watching_info_(_msgr_);
			if(!info || !info->IsValid() || !fListWatchingAll.AddItem((void*)info))
			{
				if(info) delete info;
				return E_ERROR;
			}
		}
		else
		{
			info->MakeEmpty();
		}

		return E_OK;
	}
	else if(msgrAllIndex >= 0)
	{
		info = (_etk_watching_info_*)fListWatchingAll.ItemAt(msgrAllIndex);
		info->RemoveWhat(what);
		return E_OK;
	}

	info = (_etk_watching_info_*)fListWatching.ItemAt(msgrIndex);

	if(!info)
	{
		info = new _etk_watching_info_(_msgr_);
		if(!info || !info->IsValid() || !info->AddWhat(what) || !fListWatching.AddItem((void*)info))
		{
			if(info) delete info;
			return E_ERROR;
		}
		return E_OK;
	}

	if(info->HasWhat(what)) return E_OK;
	return(info->AddWhat(what) ? E_OK : E_ERROR);
}


e_status_t
_EObserverList::StopWatching(EMessenger _msgr_, euint32 what)
{
	if(!_msgr_.IsValid()) return E_BAD_HANDLER;

	_etk_watching_info_ *info = NULL;

	eint32 msgrIndex = -1;
	for(eint32 i = 0; i < fListWatching.CountItems(); i++)
	{
		_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
		if(!aInfo) continue;
		if(aInfo->IsSameMessenger(_msgr_))
		{
			msgrIndex = i;
			break;
		}
	}

	eint32 msgrAllIndex = -1;
	if(msgrIndex >= 0) for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
	{
		_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
		if(!aInfo) continue;
		if(aInfo->IsSameMessenger(_msgr_))
		{
			msgrAllIndex = i;
			break;
		}
	}

	if(what == E_OBSERVER_OBSERVE_ALL)
	{
		if(msgrIndex >= 0)
		{
			if((info = (_etk_watching_info_*)fListWatching.RemoveItem(msgrIndex)) == NULL) return E_ERROR;
			delete info;
		}
		else if(msgrAllIndex >= 0)
		{
			if((info = (_etk_watching_info_*)fListWatchingAll.RemoveItem(msgrAllIndex)) == NULL) return E_ERROR;
			delete info;
		}

		return E_OK;
	}

	if(msgrAllIndex >= 0)
	{
		info = (_etk_watching_info_*)fListWatchingAll.ItemAt(msgrAllIndex);
		if(!info->HasWhat(what)) return(info->AddWhat(what) ? E_OK : E_ERROR);

		return E_OK;
	}

	info = (_etk_watching_info_*)fListWatching.ItemAt(msgrIndex);

	if(!info || !info->HasWhat(what)) return E_OK;

	if(!info->RemoveWhat(what)) return E_ERROR;

	if(info->CountWhats() <= 0)
	{
		if(!fListWatching.RemoveItem(info)) return E_ERROR;
		delete info;
	}

	return E_OK;
}


EList*
_EObserverList::GetObservers(euint32 what)
{
	EList *retList = new EList();
	if(!retList) return NULL;

	if(what == E_OBSERVER_OBSERVE_ALL)
	{
		for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
			if(!aInfo) continue;
			retList->AddItem((void*)(&(aInfo->msgr)));
		}

		for(eint32 i = 0; i < fListWatching.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
			if(!aInfo) continue;
			retList->AddItem((void*)(&(aInfo->msgr)));
		}
	}
	else
	{
		for(eint32 i = 0; i < fListWatching.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
			if(!aInfo) continue;
			if(aInfo->HasWhat(what)) retList->AddItem((void*)(&(aInfo->msgr)));
		}

		for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
			if(!aInfo) continue;
			if(!aInfo->HasWhat(what)) retList->AddItem((void*)(&(aInfo->msgr)));
		}
	}

	if(retList->IsEmpty())
	{
		delete retList;
		retList = NULL;
	}

	return retList;
}


bool
_EObserverList::IsWatched(euint32 what) const
{
	if(what == E_OBSERVER_OBSERVE_ALL)
	{
		return(!fListWatching.IsEmpty() || !fListWatchingAll.IsEmpty());
	}
	else
	{
		for(eint32 i = 0; i < fListWatching.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatching.ItemAt(i);
			if(!aInfo) continue;
			if(aInfo->HasWhat(what)) return true;
		}

		for(eint32 i = 0; i < fListWatchingAll.CountItems(); i++)
		{
			_etk_watching_info_ *aInfo = (_etk_watching_info_*)fListWatchingAll.ItemAt(i);
			if(!aInfo) continue;
			if(!aInfo->HasWhat(what)) return true;
		}
	}

	return false;
}


e_status_t
EHandler::StartWatching(EMessenger msgr, euint32 what)
{
	if(!fObserverList) return E_ERROR;
	return fObserverList->StartWatching(msgr, what);
}


e_status_t
EHandler::StartWatchingAll(EMessenger msgr)
{
	return StartWatching(msgr, E_OBSERVER_OBSERVE_ALL);
}


e_status_t
EHandler::StopWatching(EMessenger msgr, euint32 what)
{
	if(!fObserverList) return E_ERROR;
	return fObserverList->StopWatching(msgr, what);
}


e_status_t
EHandler::StopWatchingAll(EMessenger msgr)
{
	return StopWatching(msgr, E_OBSERVER_OBSERVE_ALL);
}


e_status_t
EHandler::StartWatching(EHandler *handler, euint32 what)
{
	e_status_t status;
	EMessenger msgr(handler, NULL, &status);
	if(status != E_OK) return status;

	return StartWatching(msgr, what);
}


e_status_t
EHandler::StartWatchingAll(EHandler *handler)
{
	e_status_t status;
	EMessenger msgr(handler, NULL, &status);
	if(status != E_OK) return status;

	return StartWatchingAll(msgr);
}


e_status_t
EHandler::StopWatching(EHandler *handler, euint32 what)
{
	e_status_t status;
	EMessenger msgr(handler, NULL, &status);
	if(status != E_OK) return status;

	return StopWatching(msgr, what);
}


e_status_t
EHandler::StopWatchingAll(EHandler *handler)
{
	e_status_t status;
	EMessenger msgr(handler, NULL, &status);
	if(status != E_OK) return status;

	return StopWatchingAll(msgr);
}


void
EHandler::SendNotices(euint32 what, const EMessage *_msg_)
{
	if(!fObserverList) return;
	EList *msgrsList = fObserverList->GetObservers(what);
	if(!msgrsList) return;

	EMessage msg(E_OBSERVER_NOTICE_CHANGE);
	if(_msg_)
	{
		msg = *_msg_;
		msg.what = E_OBSERVER_NOTICE_CHANGE;
		msg.AddInt32(E_OBSERVE_ORIGINAL_WHAT, _msg_->what);
	}
	msg.AddInt32(E_OBSERVE_WHAT_CHANGE, what);

	for(eint32 i = 0; i < msgrsList->CountItems(); i++)
	{
		EMessenger *aMsgr = (EMessenger*)msgrsList->ItemAt(i);
		if(!aMsgr) continue;

		if(aMsgr->SendMessage(&msg, (EHandler*)NULL, 20000) != E_OK)
		{
			if(aMsgr->IsTargetLocal())
			{
				ELooper *looper = NULL;
				aMsgr->Target(&looper);
				if(looper == NULL) StopWatchingAll(*aMsgr);
			}
			else
			{
				// TODO
			}
		}
	}

	delete msgrsList;
}


bool
EHandler::IsWatched(euint32 what) const
{
	if(!fObserverList) return false;
	return fObserverList->IsWatched(what);
}


bool
EHandler::AddFilter(EMessageFilter *filter)
{
	if(filter == NULL || filter->fHandler != NULL || fFilters.AddItem(filter) == false) return false;
	filter->fHandler = this;
	filter->fLooper = Looper();
	return true;
}


bool
EHandler::RemoveFilter(EMessageFilter *filter)
{
	if(filter == NULL || filter->fHandler != this || fFilters.RemoveItem(filter) == false) return false;
	filter->fHandler = NULL;
	filter->fLooper = NULL;
	return true;
}


const EList*
EHandler::FilterList() const
{
	return(&fFilters);
}


bool
EHandler::SetFilterList(const EList *filterList)
{
	while(fFilters.CountItems() > 0)
	{
		EMessageFilter *filter = (EMessageFilter*)fFilters.ItemAt(0);
		EHandler::RemoveFilter(filter);
		delete filter;
	}

	if(filterList == NULL) return true;
	for(eint32 i = 0; i < filterList->CountItems(); i++) EHandler::AddFilter((EMessageFilter*)filterList->ItemAt(i));
	return true;
}

