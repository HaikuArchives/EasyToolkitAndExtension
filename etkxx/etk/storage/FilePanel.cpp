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

#include <etk/support/Autolock.h>

#include "FilePanel.h"


_LOCAL class EFilePanelWindow : public EWindow {
public:
	EFilePanelWindow();
	virtual ~EFilePanelWindow();

	virtual void	MessageReceived(EMessage *msg);

	EMessenger	*Target() const;

	void		SetTarget(EMessenger *target);
	void		SetMessage(EMessage *msg);

private:
	EMessenger *fTarget;
	EMessage *fMessage;
};


EFilePanelWindow::EFilePanelWindow()
	: EWindow(ERect(-100, -100, -10, -10), NULL, E_TITLED_WINDOW, 0),
	  fTarget(NULL), fMessage(NULL)
{
	// TODO
	MoveToCenter();
}


EFilePanelWindow::~EFilePanelWindow()
{
	if(fTarget) delete fTarget;
	if(fMessage) delete fMessage;
}


void
EFilePanelWindow::MessageReceived(EMessage *msg)
{
	// TODO
	switch(msg->what)
	{
		default:
			EWindow::MessageReceived(msg);
			break;
	}
}


EMessenger*
EFilePanelWindow::Target() const
{
	return fTarget;
}


void
EFilePanelWindow::SetTarget(EMessenger *target)
{
	if(fTarget) delete fTarget;
	fTarget = target;
}


void
EFilePanelWindow::SetMessage(EMessage *msg)
{
	if(fMessage) delete fMessage;
	fMessage = msg;
}


EFilePanel::EFilePanel(EMessenger *target,
		       EMessage *message,
		       const EDirectory *directory)
{
	fWindow = new EFilePanelWindow();
	SetPanelDirectory(directory);
	SetTarget(target);
	SetMessage(message);
}


EFilePanel::~EFilePanel()
{
	fWindow->Lock();
	fWindow->Quit();
}


void
EFilePanel::Show()
{
	EAutolock <EWindow> autolock(fWindow);

	fWindow->Show();
}


void
EFilePanel::Hide()
{
	EAutolock <EWindow> autolock(fWindow);

	fWindow->Hide();
}


bool
EFilePanel::IsHidden() const
{
	return fWindow->IsHidden();
}



EWindow*
EFilePanel::Window() const
{
	return fWindow;
}


EMessenger*
EFilePanel::Target() const
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	return win->Target();
}


void
EFilePanel::SetTarget(EMessenger *target)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetTarget(target);
}


void
EFilePanel::SetMessage(EMessage *msg)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetMessage(msg);
}


void
EFilePanel::GetPanelDirectory(EEntry *entry) const
{
	// TODO
}


void
EFilePanel::GetPanelDirectory(EPath *path) const
{
	EEntry aEntry;

	if(path == NULL) return;

	path->Unset();
	GetPanelDirectory(&aEntry);
	aEntry.GetPath(path);
}


void
EFilePanel::GetPanelDirectory(EDirectory *directory) const
{
	EPath aPath;

	if(directory == NULL) return;

	directory->Unset();
	GetPanelDirectory(&aPath);
	directory->SetTo(aPath.Path());
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

