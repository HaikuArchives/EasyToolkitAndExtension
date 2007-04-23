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
 * File: FilePanel.h
 *
 * --------------------------------------------------------------------------*/

#ifndef __ETK_FILE_PANEL_H__
#define __ETK_FILE_PANEL_H__

#include <etk/interface/Window.h>
#include <etk/storage/Directory.h>

#ifdef __cplusplus /* Just for C++ */

class _IMPEXP_ETK EFilePanel {
public:
	EFilePanel(EMessenger *target = NULL,
		   EMessage *message = NULL,
		   const EDirectory *directory = NULL);
	virtual ~EFilePanel();

	void		Show();
	void		Hide();
	bool		IsHidden() const;

	EWindow		*Window() const;
	EMessenger	*Target() const;

	void		SetTarget(EMessenger *target);
	void		SetMessage(EMessage *msg);

	void		GetPanelDirectory(EEntry *entry) const;
	void		GetPanelDirectory(EPath *path) const;
	void		GetPanelDirectory(EDirectory *directory) const;

	void		SetPanelDirectory(const EEntry *entry);
	void		SetPanelDirectory(const EDirectory *directory);
	void		SetPanelDirectory(const char *directory);

	void		Refresh();
	void		Rewind();
	e_status_t	GetNextSelected(EEntry *entry);

private:
	EWindow *fWindow;
};

#endif /* __cplusplus */

#endif /* __ETK_FILE_PANEL_H__ */

