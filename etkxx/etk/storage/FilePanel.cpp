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
 * File: FilePanel.cpp
 *
 * --------------------------------------------------------------------------*/

#include "FilePanel.h"

EFilePanel::EFilePanel(EMessenger *target,
		       EMessage *message,
		       const EDirectory *directory)
	: fWindow(NULL), fTarget(target), fMessage(message)
{
	// TODO
	SetPanelDirectory(directory);
}


EFilePanel::~EFilePanel()
{
	// TODO
	if(fTarget) delete fTarget;
	if(fMessage) delete fMessage;
}


void
EFilePanel::Show()
{
	// TODO
}


void
EFilePanel::Hide()
{
	// TODO
}


bool
EFilePanel::IsHidden() const
{
	return (fWindow ? fWindow->IsHidden() : true);
}



EWindow*
EFilePanel::Window() const
{
	return fWindow;
}


EMessenger*
EFilePanel::Target() const
{
	return fTarget;
}


void
EFilePanel::GetPanelDirectory(EDirectory *directory) const
{
	// TODO
}


void
EFilePanel::SetTarget(EMessenger *target)
{
	if(fTarget) delete fTarget;
	fTarget = target;
}


void
EFilePanel::SetMessage(EMessage *msg)
{
	if(fMessage) delete fMessage;
	fMessage = msg;
}


void
EFilePanel::SetPanelDirectory(const EEntry *entry)
{
	EPath path;

	if(entry) entry->GetPath(&path);
	SetPanelDirectory(path.Path());
}


void
EFilePanel::SetPanelDirectory(const EDirectory *directory)
{
	EEntry entry;

	if(directory) directory->GetEntry(&entry);
	SetPanelDirectory(&entry);
}


void
EFilePanel::SetPanelDirectory(const char *directory)
{
	// TODO
}


void
EFilePanel::Refresh()
{
	// TODO
}


void
EFilePanel::Rewind()
{
	// TODO
}


e_status_t
EFilePanel::GetNextSelected(EEntry *entry)
{
	// TODO
	return E_ERROR;
}

