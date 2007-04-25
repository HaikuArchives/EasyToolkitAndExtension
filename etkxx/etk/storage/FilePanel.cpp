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

#include <etk/interface/Button.h>
#include <etk/interface/MenuField.h>
#include <etk/interface/ListView.h>
#include <etk/interface/ScrollView.h>
#include <etk/interface/TextControl.h>
#include <etk/interface/StringView.h>

#include "FindDirectory.h"
#include "FilePanel.h"

#define MSG_GET_PANEL_DIR	'getd'


_LOCAL class EFilePanelItem : public EListItem {
public:
	EFilePanelItem(const char *path);

private:
	friend class EFilePanelWindow;

	EPath fPath;

	virtual void	DrawItem(EView *owner, ERect itemRect, bool drawEverything);
	virtual void	Update(EView *owner, const EFont *font);
};


EFilePanelItem::EFilePanelItem(const char *path)
	: EListItem(), fPath(path)
{
}


void
EFilePanelItem::DrawItem(EView *owner, ERect itemRect, bool drawEverything)
{
	e_rgb_color bkColor = (IsSelected() ? e_ui_color(E_DOCUMENT_HIGHLIGHT_COLOR): owner->ViewColor());
	e_rgb_color fgColor = e_ui_color(E_DOCUMENT_TEXT_COLOR);

	if(!IsEnabled())
	{
		bkColor.disable(owner->ViewColor());
		fgColor.disable(owner->ViewColor());
	}

	if(IsSelected() || !IsEnabled())
	{
		owner->SetHighColor(bkColor);
		owner->FillRect(itemRect);
	}

	owner->SetHighColor(fgColor);
	owner->SetLowColor(bkColor);

	DrawLeader(owner, &itemRect);
	if(itemRect.IsValid() == false) return;

	if(fPath.Leaf())
	{
		e_font_height fontHeight;
		owner->GetFontHeight(&fontHeight);

		float sHeight = fontHeight.ascent + fontHeight.descent;

		EPoint penLocation;
		penLocation.x = itemRect.left;
		penLocation.y = itemRect.Center().y - sHeight / 2.f;
		penLocation.y += fontHeight.ascent + 1;

		owner->DrawString(fPath.Leaf(), penLocation);
	}
}


void
EFilePanelItem::Update(EView *owner, const EFont *font)
{
	e_font_height fontHeight;
	font->GetHeight(&fontHeight);
	SetHeight(fontHeight.ascent + fontHeight.descent);

	float width = 0;
	GetLeaderSize(&width, NULL);
	width += 300;
	SetWidth(width);
}


_LOCAL class EFilePanelLabel : public EStringView {
public:
	EFilePanelLabel(ERect frame, const char *name, const char *text, euint32 resizeMode);
	virtual ~EFilePanelLabel();

	virtual void	Draw(ERect updateRect);
	virtual void	GetPreferredSize(float *width, float *height);
};


EFilePanelLabel::EFilePanelLabel(ERect frame, const char *name, const char *text, euint32 resizeMode)
	: EStringView(frame, name, text, resizeMode, E_WILL_DRAW)
{
	SetAlignment(E_ALIGN_CENTER);
	SetVerticalAlignment(E_ALIGN_MIDDLE);
}


EFilePanelLabel::~EFilePanelLabel()
{
}


void
EFilePanelLabel::Draw(ERect updateRect)
{
	EStringView::Draw(updateRect);

	PushState();
	SetDrawingMode(E_OP_COPY);
	SetPenSize(0);
	SetHighColor(e_ui_color(E_SHINE_COLOR));
	StrokeRect(Bounds());
	SetHighColor(e_ui_color(E_SHADOW_COLOR));
	StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
	StrokeLine(Bounds().RightTop());
	PopState();
}


void
EFilePanelLabel::GetPreferredSize(float *width, float *height)
{
	EStringView::GetPreferredSize(width, height);
	if(width != NULL) *width += 2;
	if(height != NULL) *height += 2;
}


_LOCAL class EFilePanelTitleView : public EView {
public:
	EFilePanelTitleView(ERect frame);
};


EFilePanelTitleView::EFilePanelTitleView(ERect frame)
	: EView(frame, "TitleView", E_FOLLOW_LEFT_RIGHT | E_FOLLOW_TOP, 0)
{
	ERect rect;

	rect = Bounds();
	rect.right = 250;
	AddChild(new EFilePanelLabel(rect, "TitleView_Name", "Name", E_FOLLOW_LEFT | E_FOLLOW_TOP_BOTTOM));

	rect.OffsetBy(251, 0);
	rect.right = rect.left + 50;
	AddChild(new EFilePanelLabel(rect, "TitleView_Size", "Size", E_FOLLOW_LEFT | E_FOLLOW_TOP_BOTTOM));

	rect.OffsetBy(51, 0);
	rect.right = Bounds().right;
	AddChild(new EFilePanelLabel(rect, "TitleView_Modified", "Modified", E_FOLLOW_ALL));
}


_LOCAL class EFilePanelWindow : public EWindow {
public:
	EFilePanelWindow();
	virtual ~EFilePanelWindow();

	virtual void	MessageReceived(EMessage *msg);
	virtual bool	QuitRequested();

	EMessenger	*Target() const;

	void		SetTarget(EMessenger *target);
	void		SetMessage(EMessage *msg);

	void		Refresh();
	void		SetPanelDirectory(const char *path);

	void		Rewind();
	e_status_t	GetNextSelected(EEntry *entry);

private:
	EPath fPath;
	EMessenger *fTarget;
	EMessage *fMessage;

	eint32 fSelIndex;

	static bool	RefreshCallback(const char *path, void *data);
};


EFilePanelWindow::EFilePanelWindow()
	: EWindow(ERect(-400, -400, -10, -10), "FilePanel: uncompleted", E_TITLED_WINDOW, 0),
	  fTarget(NULL), fMessage(NULL), fSelIndex(0)
{
	EView *topView, *aView;
	EMenuBar *menuBar;
	EMenu *menu;
	EMenuItem *menuItem;
	EMenuField *menuField;
	ETextControl *textControl;
	EButton *button;
	EListView *listView;
	EScrollBar *hScrollBar, *vScrollBar;
	EFilePanelTitleView *titleView;
	EFilePanelLabel *label;

	ERect rect = Bounds();

	topView = new EView(rect, NULL, E_FOLLOW_ALL, 0);
	topView->SetViewColor(e_ui_color(E_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	menu = new EMenu("File", E_ITEMS_IN_COLUMN);
	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new EMenuItem("Quit", new EMessage(E_QUIT_REQUESTED), 'q', E_COMMAND_KEY));
	menuItem->SetTarget(this);

	menuBar = new EMenuBar(rect, NULL,
			       E_FOLLOW_LEFT_RIGHT | E_FOLLOW_TOP,
			       E_ITEMS_IN_ROW, false);
	menuBar->SetName("MenuBar");
	menuBar->AddItem(menu);
	menuBar->GetPreferredSize(NULL, &rect.bottom);
	menuBar->ResizeTo(rect.Width(), rect.Height());
	topView->AddChild(menuBar);

	rect.top = rect.bottom + 1;
	rect.bottom = Bounds().bottom;
	rect.InsetBy(5, 5);
	aView = new EView(rect, NULL, E_FOLLOW_ALL, 0);
	topView->AddChild(aView);

	rect.OffsetTo(E_ORIGIN);

	menu = new EMenu(NULL, E_ITEMS_IN_COLUMN);
	menu->SetLabelFromMarked(true);
	menu->AddItem(new EMenuItem("home", NULL));
	menu->ItemAt(0)->SetMarked(true);
	menuField = new EMenuField(rect, "DirMenuField", NULL, menu, false);
	menuField->GetPreferredSize(NULL, &rect.bottom);
	menuField->ResizeTo(rect.Width(), rect.Height());
	menuField->MoveTo(0, 0);
	aView->AddChild(menuField);

	rect.bottom = aView->Bounds().bottom;
	rect.top = rect.bottom - 20;
	textControl = new ETextControl(rect, "text control",
				       NULL, NULL, NULL,
				       E_FOLLOW_LEFT | E_FOLLOW_BOTTOM);
	textControl->ResizeTo(100, rect.Height());
	aView->AddChild(textControl);

	rect.left = rect.right - 100;
	button = new EButton(rect, "default button", "OK",
			     NULL, E_FOLLOW_RIGHT | E_FOLLOW_BOTTOM);
	aView->AddChild(button);

	rect.OffsetBy(-110, 0);
	button = new EButton(rect, "cancel button", "Cancel",
			     NULL, E_FOLLOW_RIGHT | E_FOLLOW_BOTTOM);
	aView->AddChild(button);

	rect = aView->Bounds();
	rect.top = menuField->Frame().bottom + 5;
	rect.bottom -= 25;

	titleView = new EFilePanelTitleView(rect);
	titleView->ResizeTo(rect.Width() - E_V_SCROLL_BAR_WIDTH - 1, 16);
	titleView->MoveTo(0, rect.top);
	aView->AddChild(titleView);

	hScrollBar = new EScrollBar(rect, "HScrollBar", 0, 0, 0, E_HORIZONTAL);
	hScrollBar->ResizeBy(-(105 + E_V_SCROLL_BAR_WIDTH), E_H_SCROLL_BAR_HEIGHT - rect.Height());
	hScrollBar->MoveTo(105, rect.bottom - E_H_SCROLL_BAR_HEIGHT);
	aView->AddChild(hScrollBar);

	label = new EFilePanelLabel(ERect(0, rect.bottom - E_H_SCROLL_BAR_HEIGHT, 100, rect.bottom),
				    "CountVw", NULL,
				    E_FOLLOW_LEFT | E_FOLLOW_BOTTOM);
	aView->AddChild(label);

	vScrollBar = new EScrollBar(rect, "VScrollBar", 0, 0, 0, E_VERTICAL);
	vScrollBar->ResizeBy(E_V_SCROLL_BAR_WIDTH - rect.Width(), -E_H_SCROLL_BAR_HEIGHT);
	vScrollBar->MoveTo(rect.right - E_V_SCROLL_BAR_WIDTH, rect.top);
	aView->AddChild(vScrollBar);

	rect.right -= E_V_SCROLL_BAR_WIDTH + 1;
	rect.top += titleView->Frame().Height() + 1;
	rect.bottom -= E_H_SCROLL_BAR_HEIGHT + 1;
	listView = new EListView(rect, "PoseView", E_SINGLE_SELECTION_LIST, E_FOLLOW_ALL);
	aView->AddChild(listView);

	MoveToCenter();

	e_find_directory(E_USER_DIRECTORY, &fPath);
}


EFilePanelWindow::~EFilePanelWindow()
{
	if(fTarget) delete fTarget;
	if(fMessage) delete fMessage;
}


bool
EFilePanelWindow::QuitRequested()
{
	Hide();
	return false;
}


void
EFilePanelWindow::MessageReceived(EMessage *msg)
{
	switch(msg->what)
	{
		case MSG_GET_PANEL_DIR:
			msg->AddString("PanelDirectory", fPath.Path());
			msg->SendReply(msg);
			break;

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


bool
EFilePanelWindow::RefreshCallback(const char *path, void *data)
{
	EListView *listView = (EListView*)data;

	listView->AddItem(new EFilePanelItem(path));

	return false;
}


void
EFilePanelWindow::Refresh()
{
	EListView *listView = (EListView*)FindView("PoseView");

	listView->RemoveItems(0, -1, true);

	EDirectory dir(fPath.Path());
	dir.DoForEach(RefreshCallback, (void*)listView);

	EScrollBar *vScrollBar = (EScrollBar*)FindView("VScrollBar");
	EFilePanelLabel *label = (EFilePanelLabel*)FindView("CountVw");
	EString str;

	vScrollBar->SetEnabled(listView->CountItems() != 0);
	str << listView->CountItems() << " items";
	label->SetText(str.String());
}


void
EFilePanelWindow::SetPanelDirectory(const char *path)
{
	fPath.SetTo(path);
	Refresh();
}


void
EFilePanelWindow::Rewind()
{
	fSelIndex = 0;
}


e_status_t
EFilePanelWindow::GetNextSelected(EEntry *entry)
{
	EListView *listView = (EListView*)FindView("PoseView");
	EFilePanelItem *item = (EFilePanelItem*)listView->ItemAt(listView->CurrentSelection(fSelIndex));

	if(item == NULL) return E_ERROR;
	entry->SetTo(item->fPath.Path());

	fSelIndex++;

	return E_OK;
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
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	if(win->IsHidden())
	{
		win->Show();
		win->Refresh();
	}
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
	if(entry == NULL) return;

	EMessenger msgr(fWindow, fWindow);
	EMessage msg(MSG_GET_PANEL_DIR);
	const char *path = NULL;

	entry->Unset();
	if(!(msgr.SendMessage(&msg, &msg) != E_OK ||
	     msg.FindString("PanelDirectory", &path) != E_OK)) entry->SetTo(path);
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
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetPanelDirectory(directory);
}


void
EFilePanel::Refresh()
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->Refresh();
}


void
EFilePanel::Rewind()
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->Rewind();
}


e_status_t
EFilePanel::GetNextSelected(EEntry *entry)
{
	if(entry == NULL) return E_ERROR;

	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	return win->GetNextSelected(entry);
}

