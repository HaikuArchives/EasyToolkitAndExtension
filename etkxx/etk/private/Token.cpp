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
 * File: Token.cpp
 *
 * --------------------------------------------------------------------------*/

#include <etk/kernel/Kernel.h>
#include <etk/support/Locker.h>
#include <etk/support/List.h>

#include "Token.h"


struct _LOCAL _etk_token_t {
	euint64 count;
	e_bigtime_t time_stamp;
	void *data;
};


class _LOCAL ETokensDepotPrivateData : public EList {
public:
	ETokensDepotPrivateData();
	virtual ~ETokensDepotPrivateData();

	euint64		AddToken(void *data);
	void		RemoveToken(euint64 token);
	_etk_token_t	*TokenAt(euint64 token) const;

	bool		PushToken(euint64 token);
	void		PopToken(euint64 token);
};


ETokensDepotPrivateData::ETokensDepotPrivateData()
	: EList()
{
}


ETokensDepotPrivateData::~ETokensDepotPrivateData()
{
	EList *list;

	while((list = (EList*)RemoveItem(0)) != NULL)
	{
		for(eint32 i = 0; i < list->CountItems(); i++)
		{
			_etk_token_t *aToken = (_etk_token_t*)list->ItemAt(i);
			free(aToken);
		}
		delete list;
	}
}


euint64
ETokensDepotPrivateData::AddToken(void *data)
{
	_etk_token_t *aToken = (_etk_token_t*)malloc(sizeof(_etk_token_t));

	if(aToken == NULL) return E_MAXUINT64;

	euint64 token = E_MAXUINT64;

	EList *list = NULL;
	for(eint32 i = 0; i < CountItems(); i++)
	{
		list = (EList*)ItemAt(i);
		if(list->CountItems() >= E_MAXINT32 - 1 || list->AddItem(aToken) == false)
		{
			list = NULL;
			continue;
		}
		token = ((euint64)i << 32) | (euint64)(list->CountItems() - 1);
	}

	if(list == NULL && CountItems() < E_MAXINT32 - 1)
	{
		if(AddItem(list = new EList()) == false || list->AddItem(aToken) == false)
		{
			delete list;
		}
		else
		{
			token = ((euint64)(CountItems() - 1) << 32) | (euint64)(list->CountItems() - 1);
		}
	}
	else for(eint32 i = 0; i < CountItems(); i++)
	{
		list = (EList*)ItemAt(i);
		eint32 index = list->IndexOf(NULL);
		if(index < 0)
		{
			list = NULL;
			continue;
		}
		list->ReplaceItem(index, aToken);
		token = ((euint64)i << 32) | (euint64)index;
	}

	if(token != E_MAXUINT64)
	{
		aToken->count = 1;
		aToken->time_stamp = e_system_time();
		while(aToken->time_stamp == e_system_time())
		{
			// do nothing, waiting till "e_system_time()" changed.
		}
		aToken->data = data;
	}
	else
	{
		free(aToken);
	}

	return token;
}


void
ETokensDepotPrivateData::RemoveToken(euint64 token)
{
	euint64 index = token / (euint64)(E_MAXINT32 - 1);
	if(index > (euint64)E_MAXINT32 - 1) return;

	EList *list = (EList*)ItemAt((eint32)index);
	if(list == NULL) return;

	index = token % (euint64)(E_MAXINT32 - 1);
	_etk_token_t *aToken = (_etk_token_t*)(list->ItemAt((eint32)index));
	if(aToken == NULL) return;

	if(aToken->count > 1)
	{
		aToken->count -= 1;
		aToken->data = NULL;
	}
	else
	{
		if(index < (euint64)list->CountItems() - 1)
		{
			list->ReplaceItem((eint32)index, NULL);
		}
		else
		{
			list->RemoveItem((eint32)index);
			while(list->LastItem() == NULL && list->IsEmpty() == false) list->RemoveItem(list->CountItems() - 1);
			if(list->IsEmpty() && LastItem() == (void*)list) delete	(EList*)RemoveItem(CountItems() - 1);
		}
		free(aToken);
	}
}


_etk_token_t*
ETokensDepotPrivateData::TokenAt(euint64 token) const
{
	euint64 index = token / (euint64)(E_MAXINT32 - 1);
	if(index > (euint64)E_MAXINT32 - 1) return NULL;

	EList *list = (EList*)ItemAt((eint32)index);
	index = token % (euint64)(E_MAXINT32 - 1);
	return((_etk_token_t*)(list->ItemAt((eint32)index)));
}


bool
ETokensDepotPrivateData::PushToken(euint64 token)
{
	_etk_token_t *aToken = TokenAt(token);
	if(aToken == NULL || aToken->count == E_MAXUINT64) return false;
	aToken->count += 1;
	return true;
}


void
ETokensDepotPrivateData::PopToken(euint64 token)
{
	_etk_token_t *aToken = TokenAt(token);
	if(!(aToken == NULL || aToken->count == 0))
	{
		aToken->count -= 1;
		if(aToken->count == 0) RemoveToken(token);
	}
}


ETokensDepot::ETokensDepot(ELocker *locker, bool deconstruct_locker)
	: fLocker(locker), fDeconstructLocker(deconstruct_locker)
{
	fData = reinterpret_cast<void*>(new ETokensDepotPrivateData());
}


ETokensDepot::~ETokensDepot()
{
	delete reinterpret_cast<ETokensDepotPrivateData*>(fData);
	if(fDeconstructLocker && fLocker != NULL) delete fLocker;
}


EToken*
ETokensDepot::CreateToken(void *data)
{
	EToken *aToken = NULL;

	if(Lock())
	{
		euint64 token = (reinterpret_cast<ETokensDepotPrivateData*>(fData))->AddToken(data);
		if(token != E_MAXUINT64)
		{
			aToken = new EToken();
			aToken->fOriginal = true;
			aToken->fToken = token;
			aToken->fDepot = this;
		}
		Unlock();
	}

	return aToken;
}


EToken*
ETokensDepot::OpenToken(euint64 token, EToken *fetch_token)
{
	EToken *aToken = NULL;

	if(fetch_token == NULL || fetch_token->fDepot == NULL)
	{
		if(Lock())
		{
			if((reinterpret_cast<ETokensDepotPrivateData*>(fData))->PushToken(token))
			{
				aToken = (fetch_token != NULL ? fetch_token : new EToken());
				aToken->fToken = token;
				aToken->fDepot = this;
			}
			Unlock();
		}
	}
	else
	{
		ETK_WARNING("[PRIVATE]: %s --- fetch_token->fDepot != NULL\n", __PRETTY_FUNCTION__);
	}

	return aToken;
}


bool
ETokensDepot::PushToken(euint64 token)
{
	bool retVal = false;

	if(Lock())
	{
		retVal = (reinterpret_cast<ETokensDepotPrivateData*>(fData))->PushToken(token);
		Unlock();
	}

	return retVal;
}


void
ETokensDepot::PopToken(euint64 token)
{
	if(Lock())
	{
		(reinterpret_cast<ETokensDepotPrivateData*>(fData))->PopToken(token);
		Unlock();
	}
}


bool
ETokensDepot::Lock()
{
	if(fLocker == NULL) return true;
	return fLocker->Lock();
}


void
ETokensDepot::Unlock()
{
	if(fLocker != NULL) fLocker->Unlock();
}


EToken::EToken()
	: fOriginal(false), fToken(E_MAXUINT64), fDepot(NULL)
{
}


EToken::~EToken()
{
	Empty();
}


euint64
EToken::Token() const
{
	return fToken;
}


e_bigtime_t
EToken::TimeStamp() const
{
	e_bigtime_t retVal = E_MAXINT64;

	if(fToken != E_MAXUINT64 && fDepot != NULL)
	{
		if(fDepot->Lock())
		{
			ETokensDepotPrivateData *depot_private = reinterpret_cast<ETokensDepotPrivateData*>(fDepot->fData);
			_etk_token_t *aToken = depot_private->TokenAt(fToken);
			if(aToken != NULL) retVal = aToken->time_stamp;
			fDepot->Unlock();
		}
	}

	return retVal;
}


void*
EToken::Data() const
{
	void *retVal = NULL;

	if(fToken != E_MAXUINT64 && fDepot != NULL)
	{
		if(fDepot->Lock())
		{
			ETokensDepotPrivateData *depot_private = reinterpret_cast<ETokensDepotPrivateData*>(fDepot->fData);
			_etk_token_t *aToken = depot_private->TokenAt(fToken);
			if(aToken != NULL) retVal = aToken->data;
			fDepot->Unlock();
		}
	}

	return retVal;
}


void
EToken::SetData(void *data)
{
	if(fOriginal == false || fToken == E_MAXUINT64 || fDepot == NULL) return;

	if(fDepot->Lock())
	{
		ETokensDepotPrivateData *depot_private = reinterpret_cast<ETokensDepotPrivateData*>(fDepot->fData);
		_etk_token_t *aToken = depot_private->TokenAt(fToken);
		if(aToken != NULL) aToken->data = data;
		fDepot->Unlock();
	}
}


ETokensDepot*
EToken::Depot() const
{
	return fDepot;
}


void
EToken::Empty()
{
	if(fToken != E_MAXUINT64 && fDepot != NULL)
	{
		if(fDepot->Lock())
		{
			ETokensDepotPrivateData *depot_private = reinterpret_cast<ETokensDepotPrivateData*>(fDepot->fData);
			if(fOriginal)
				depot_private->RemoveToken(fToken);
			else
				depot_private->PopToken(fToken);
			fDepot->Unlock();
		}
	}

	fToken = E_MAXUINT64;
	fDepot = NULL;
}

