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
 * File: MessageQueue.cpp
 *
 * --------------------------------------------------------------------------*/

#include <etk/kernel/Kernel.h>
#include <etk/support/Locker.h>

#include <etk/private/PrivateHandler.h>

#include "MessageQueue.h"


EMessageQueue::EMessageQueue()
	: fLocker(NULL)
{
	if((fLocker = etk_create_locker()) == NULL)
		ETK_ERROR("[APP]: %s --- Unable to create locker for looper.", __PRETTY_FUNCTION__);
}


EMessageQueue::~EMessageQueue()
{
	for(eint32 i = 0; i < fMessagesList.CountItems(); i++)
	{
		EMessage *msg = (EMessage*)fMessagesList.ItemAt(i);
		if(msg) delete msg;
	}

	fMessagesList.MakeEmpty();

	if(fLocker)
	{
		etk_close_locker(fLocker);
		etk_delete_locker(fLocker);
	}
}


e_status_t
EMessageQueue::LockWithTimeout(e_bigtime_t timeout)
{
	e_status_t retVal = E_ERROR;

	ELocker *hLocker = etk_get_handler_operator_locker();

	hLocker->Lock();
	void *locker = etk_clone_locker(fLocker);
	eint64 locksCount = hLocker->CountLocks();
	while(hLocker->CountLocks() > E_INT64_CONSTANT(0)) hLocker->Unlock();

	if(locker != NULL)
	{
		retVal = etk_lock_locker_etc(fLocker, E_TIMEOUT, timeout);
		etk_delete_locker(locker);
	}

	while(locksCount > E_INT64_CONSTANT(1)) {hLocker->Lock(); locksCount--;}

	return retVal;
}


bool
EMessageQueue::Lock()
{
	return(LockWithTimeout(E_INFINITE_TIMEOUT) == E_OK);
}


void
EMessageQueue::Unlock()
{
	if(etk_count_locker_locks(fLocker) > E_INT64_CONSTANT(0))
		etk_unlock_locker(fLocker);
	else
		ETK_WARNING("[APP]: %s -- MessageQueue wasn't locked by current thread.", __PRETTY_FUNCTION__);
}


eint32
EMessageQueue::CountMessages() const
{
	return fMessagesList.CountItems();
}


bool
EMessageQueue::IsEmpty() const
{
	return fMessagesList.IsEmpty();
}


bool
EMessageQueue::AddMessage(EMessage *an_event)
{
	if(!an_event) return false;

	if(fMessagesList.AddItem((void*)an_event)) return true;

	delete an_event;
	return false;
}


bool
EMessageQueue::RemoveMessage(EMessage *an_event)
{
	if(!an_event) return false;

	if(!fMessagesList.RemoveItem((void*)an_event)) return false;

	delete an_event;

	return true;
}


EMessage*
EMessageQueue::NextMessage()
{
	return((EMessage*)fMessagesList.RemoveItem(0));
}


EMessage*
EMessageQueue::FindMessage(eint32 index) const
{
	return((EMessage*)fMessagesList.ItemAt(index));
}


EMessage*
EMessageQueue::FindMessage(euint32 what, eint32 fromIndex) const
{
	for(eint32 i = fromIndex; i < fMessagesList.CountItems(); i++)
	{
		EMessage *msg = (EMessage*)fMessagesList.ItemAt(i);

		if(msg)
			if(msg->what == what) return msg;
	}

	return NULL;
}


EMessage*
EMessageQueue::FindMessage(euint32 what, eint32 fromIndex, eint32 count) const
{
	for(eint32 i = fromIndex, j = 0; i < fMessagesList.CountItems() && j < count; i++, j++)
	{
		EMessage *msg = (EMessage*)fMessagesList.ItemAt(i);

		if(msg)
			if(msg->what == what) return msg;
	}

	return NULL;
}


eint32
EMessageQueue::IndexOfMessage(EMessage *an_event) const
{
	return fMessagesList.IndexOf((void*)an_event);
}

