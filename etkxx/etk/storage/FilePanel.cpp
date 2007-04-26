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
#include <etk/interface/Bitmap.h>
#include <etk/render/Pixmap.h>

#include "VolumeRoster.h"
#include "FindDirectory.h"
#include "FilePanel.h"

#define ICON_WIDTH	16
#define ICON_HEIGHT	16

#include "icons/volume.xpm"
#include "icons/folder.xpm"
#include "icons/file.xpm"

#define MSG_PANEL_GET_DIR	'getd'
#define MSG_PANEL_SELECTED	'sele'

#ifdef ETK_OS_WIN32
extern "C" {
// free it by "free"
extern char* etk_win32_convert_utf8_to_active(const char *str, eint32 length);
extern char* etk_win32_convert_active_to_utf8(const char *str, eint32 length);
}
#endif

_LOCAL class EFilePanelView;
_LOCAL class EFilePanelWindow;


_LOCAL class EFilePanelLabel : public EStringView {
public:
	EFilePanelLabel(ERect frame, const char *name, const char *text, euint32 resizeMode);

	virtual void	Draw(ERect updateRect);
	virtual void	GetPreferredSize(float *width, float *height);
};


_LOCAL class EFilePanelItem : public EListItem {
public:
	EFilePanelItem(const char *path, EFilePanelView *panel_view, e_dev_t dev = -1);
	virtual ~EFilePanelItem();

	const char	*Path() const;
	const char	*Leaf() const;

	bool		IsVolume() const;
	bool		IsDirectory() const;

	eint64		Size() const;
	e_bigtime_t	ModifiedTime() const;

private:
	friend class EFilePanelWindow;

	EPath fPath;
	char *fLeaf;

	eint32 fFlags;
	eint64 fSize;
	e_bigtime_t fModifiedTime;

	EFilePanelView *fPanelView;

	virtual void	DrawItem(EView *owner, ERect itemRect, bool drawEverything);
	virtual void	Update(EView *owner, const EFont *font);
};


_LOCAL class EFilePanelTitleView : public EView {
public:
	EFilePanelTitleView(ERect parent_bounds);

	virtual void	Draw(ERect updateRect);
	virtual void	GetPreferredSize(float *width, float *height);
	virtual void	ScrollTo(EPoint where);
};


_LOCAL class EFilePanelView : public EView {
public:
	EFilePanelView(ERect frame);
	virtual ~EFilePanelView();

	void		AddColumn(const char *name, float width,
				  void (*draw_func)(EView*, ERect, EFilePanelItem*),
				  int (*sort_func)(const EFilePanelItem**, const EFilePanelItem**));
	void		RemoveColumn(eint32 index);
	void		SwapColumns(eint32 indexA, eint32 indexB);

	eint32		CountColumns() const;
	const char	*GetNameOfColumn(eint32 index) const;
	float		GetWidthOfColumn(eint32 index) const;

	void		DrawItem(EView *owner, ERect itemRect, EFilePanelItem*);
	void		SortItems(eint32 columnIndex);

	virtual void	FrameResized(float new_width, float new_height);

private:
	struct column_data {
		char *name;
		float width;
		void (*draw_func)(EView*, ERect, EFilePanelItem*);
		int (*sort_func)(const EFilePanelItem**, const EFilePanelItem**);
	};

	EList fColumns;
	EFilePanelTitleView *fTitleView;
	EListView *fListView;
	EScrollBar *fHSB;
	EScrollBar *fVSB;
};


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
	const char	*PanelDirectory() const;

	void		Rewind();
	e_status_t	GetNextSelected(EEntry *entry);

private:
	EPath fPath;
	EMessenger *fTarget;
	EMessage *fMessage;
	EFilePanelView *fPanelView;

	eint32 fSelIndex;
	bool fShowHidden;
	eint32 fSort;

	static bool	RefreshCallback(const char *path, void *data);
};


EFilePanelLabel::EFilePanelLabel(ERect frame, const char *name, const char *text, euint32 resizeMode)
	: EStringView(frame, name, text, resizeMode, E_WILL_DRAW)
{
	SetAlignment(E_ALIGN_CENTER);
	SetVerticalAlignment(E_ALIGN_MIDDLE);
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


EFilePanelItem::EFilePanelItem(const char *path, EFilePanelView *panel_view, e_dev_t dev)
	: EListItem(), fPath(path), fSize(0), fModifiedTime(0), fPanelView(panel_view)
{
	if(dev >= 0)
	{
		EVolume vol(dev);
		EString name;
		vol.GetName(&name);
		fFlags = 2;
		fLeaf = e_strdup(name.String());
	}
	else
	{
		EEntry aEntry(path);
		fFlags = (aEntry.IsDirectory() ? 1 : 0);

#ifndef ETK_OS_WIN32
		fLeaf = e_strdup(fPath.Leaf());
#else
		fLeaf = etk_win32_convert_active_to_utf8(fPath.Leaf(), -1);
#endif

		if(fFlags == 0) aEntry.GetSize(&fSize);
		aEntry.GetModifiedTime(&fModifiedTime);
	}
}


EFilePanelItem::~EFilePanelItem()
{
	if(fLeaf) free(fLeaf);
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

		owner->SetHighColor(fgColor);
		owner->SetLowColor(bkColor);
	}
	else
	{
		owner->SetHighColor(fgColor);
		owner->SetLowColor(owner->ViewColor());
	}

	DrawLeader(owner, &itemRect);
	if(itemRect.IsValid() == false) return;

	fPanelView->DrawItem(owner, itemRect, this);
}


void
EFilePanelItem::Update(EView *owner, const EFont *font)
{
	e_font_height fontHeight;
	font->GetHeight(&fontHeight);
	SetHeight(max_c(fontHeight.ascent + fontHeight.descent, ICON_HEIGHT) + 4);

	float width = 0;
	GetLeaderSize(&width, NULL);
	for(eint32 i = 0; i < fPanelView->CountColumns(); i++) width += fPanelView->GetWidthOfColumn(i);
	SetWidth(width);
}


const char*
EFilePanelItem::Path() const
{
	return fPath.Path();
}


const char*
EFilePanelItem::Leaf() const
{
	return fLeaf;
}


bool
EFilePanelItem::IsDirectory() const
{
	return(fFlags == 1);
}


bool
EFilePanelItem::IsVolume() const
{
	return(fFlags == 2);
}


eint64
EFilePanelItem::Size() const
{
	return fSize;
}


e_bigtime_t
EFilePanelItem::ModifiedTime() const
{
	return fModifiedTime;
}


static int column_name_sort_callback(const EFilePanelItem **_itemA, const EFilePanelItem **_itemB)
{
	const EFilePanelItem *itemA = *_itemA;
	const EFilePanelItem *itemB = *_itemB;

	if(itemA->IsVolume() != itemB->IsVolume() &&
	   (itemA->IsVolume() || itemB->IsVolume()))
	{
		return(itemA->IsVolume() ? -1 : 1);
	}
	else if (itemA->IsDirectory() != itemB->IsDirectory() &&
		 (itemA->IsDirectory() || itemB->IsDirectory()))
	{
		return(itemA->IsDirectory() ? -1 : 1);
	}

	EString strA(itemA->Leaf()), strB(itemB->Leaf());
	if(strA == strB) return 0;
	return(strA < strB ? -1 : 1);
}


static void column_name_drawing_callback(EView *owner, ERect rect, EFilePanelItem *item)
{
	if(!rect.IsValid()) return;

	const char **xpm_data = (const char**)file_xpm;

	if(item->IsVolume()) xpm_data = (const char**)volume_xpm;
	else if(item->IsDirectory()) xpm_data = (const char**)folder_xpm;

	if(xpm_data != NULL)
	{
		EPixmap *pixmap = new EPixmap(ICON_WIDTH, ICON_HEIGHT, E_RGB24);
		pixmap->SetDrawingMode(E_OP_COPY);
		pixmap->SetHighColor(owner->LowColor());
		pixmap->FillRect(0, 0, ICON_WIDTH, ICON_HEIGHT);
		pixmap->DrawXPM(xpm_data, 0, 0, 0, 0);

		EBitmap *bitmap = new EBitmap(pixmap);
		delete pixmap;

		owner->DrawBitmap(bitmap, EPoint(rect.left + 5, rect.Center().y - ICON_HEIGHT / 2));
		delete bitmap;
	}

	if(item->Leaf())
	{
		e_font_height fontHeight;
		owner->GetFontHeight(&fontHeight);

		float sHeight = fontHeight.ascent + fontHeight.descent;

		EPoint penLocation;
		penLocation.x = rect.left + ICON_WIDTH + 10;
		penLocation.y = rect.Center().y - sHeight / 2.f;
		penLocation.y += fontHeight.ascent + 1;

		owner->DrawString(item->Leaf(), penLocation);
	}
}


static int column_size_sort_callback(const EFilePanelItem **_itemA, const EFilePanelItem **_itemB)
{
	const EFilePanelItem *itemA = *_itemA;
	const EFilePanelItem *itemB = *_itemB;

	if(itemA->IsVolume() != itemB->IsVolume() &&
	   (itemA->IsVolume() || itemB->IsVolume()))
	{
		return(itemA->IsVolume() ? -1 : 1);
	}
	else if (itemA->IsDirectory() != itemB->IsDirectory() &&
		 (itemA->IsDirectory() || itemB->IsDirectory()))
	{
		return(itemA->IsDirectory() ? -1 : 1);
	}

	if(itemA->Size() == itemB->Size()) return 0;
	return(itemA->Size() < itemB->Size() ? -1 : 1);
}


static void column_size_drawing_callback(EView *owner, ERect rect, EFilePanelItem *item)
{
	if(!rect.IsValid()) return;

	if(item->IsVolume() || item->IsDirectory()) return;

	EString str;
	if(item->Size() >= 0x40000000) str << ((float)item->Size() / (float)0x40000000) << "G";
	else if(item->Size() >= 0x100000) str << ((float)item->Size() / (float)0x100000) << "M";
	else if(item->Size() >= 0x400) str << ((float)item->Size() / (float)0x400) << "K";
	else str << item->Size();

	e_font_height fontHeight;
	owner->GetFontHeight(&fontHeight);

	float sHeight = fontHeight.ascent + fontHeight.descent;

	EPoint penLocation;
	penLocation.x = rect.left + 2;
	penLocation.y = rect.Center().y - sHeight / 2.f;
	penLocation.y += fontHeight.ascent + 1;

	owner->DrawString(str.String(), penLocation);
}


static int column_modified_sort_callback(const EFilePanelItem **_itemA, const EFilePanelItem **_itemB)
{
	const EFilePanelItem *itemA = *_itemA;
	const EFilePanelItem *itemB = *_itemB;

	if(itemA->IsVolume() != itemB->IsVolume() &&
	   (itemA->IsVolume() || itemB->IsVolume()))
	{
		return(itemA->IsVolume() ? -1 : 1);
	}
	else if (itemA->IsDirectory() != itemB->IsDirectory() &&
		 (itemA->IsDirectory() || itemB->IsDirectory()))
	{
		return(itemA->IsDirectory() ? -1 : 1);
	}

	if(itemA->ModifiedTime() == itemB->ModifiedTime()) return 0;
	return(itemA->ModifiedTime() < itemB->ModifiedTime() ? -1 : 1);
}


static void column_modified_drawing_callback(EView *owner, ERect rect, EFilePanelItem *item)
{
	if(!rect.IsValid() || item->IsVolume()) return;

	EString str;

	// TODO
	str << item->ModifiedTime();

	e_font_height fontHeight;
	owner->GetFontHeight(&fontHeight);

	float sHeight = fontHeight.ascent + fontHeight.descent;

	EPoint penLocation;
	penLocation.x = rect.left + 2;
	penLocation.y = rect.Center().y - sHeight / 2.f;
	penLocation.y += fontHeight.ascent + 1;

	owner->DrawString(str.String(), penLocation);
}


EFilePanelView::EFilePanelView(ERect frame)
	: EView(frame, NULL, E_FOLLOW_ALL, E_FRAME_EVENTS)
{
	EFilePanelLabel *label;

	ERect rect = Bounds();

	fTitleView = new EFilePanelTitleView(rect);
	AddChild(fTitleView);

	fHSB = new EScrollBar(rect, "HScrollBar", 0, 0, 0, E_HORIZONTAL);
	fHSB->ResizeBy(-(105 + E_V_SCROLL_BAR_WIDTH), E_H_SCROLL_BAR_HEIGHT - rect.Height());
	fHSB->MoveTo(105, rect.bottom - E_H_SCROLL_BAR_HEIGHT);
	AddChild(fHSB);

	label = new EFilePanelLabel(ERect(0, rect.bottom - E_H_SCROLL_BAR_HEIGHT, 100, rect.bottom),
				    "CountVw", NULL,
				    E_FOLLOW_LEFT | E_FOLLOW_BOTTOM);
	AddChild(label);

	fVSB = new EScrollBar(rect, "VScrollBar", 0, 0, 0, E_VERTICAL);
	fVSB->ResizeBy(E_V_SCROLL_BAR_WIDTH - rect.Width(), -E_H_SCROLL_BAR_HEIGHT);
	fVSB->MoveTo(rect.right - E_V_SCROLL_BAR_WIDTH, 0);
	AddChild(fVSB);

	rect.right -= E_V_SCROLL_BAR_WIDTH + 1;
	rect.top += fTitleView->Frame().Height() + 1;
	rect.bottom -= E_H_SCROLL_BAR_HEIGHT + 1;
	fListView = new EListView(rect, "PoseView", E_SINGLE_SELECTION_LIST, E_FOLLOW_ALL);
	fListView->SetMessage(new EMessage(MSG_PANEL_SELECTED));
	AddChild(fListView);

	fHSB->SetTarget(fTitleView);
	fHSB->SetEnabled(false);

	fVSB->SetTarget(fListView);
	fVSB->SetEnabled(false);

	AddColumn("Name", 250, column_name_drawing_callback, column_name_sort_callback);
	AddColumn("Size", 100, column_size_drawing_callback, column_size_sort_callback);
	AddColumn("Modified", 200, column_modified_drawing_callback, column_modified_sort_callback);
}


EFilePanelView::~EFilePanelView()
{
	struct column_data *data;
	while((data = (struct column_data*)fColumns.RemoveItem(0)) != NULL)
	{
		if(data->name != NULL) delete[] data->name;
		delete data;
	}
}


void
EFilePanelView::AddColumn(const char *name, float width,
			  void (*draw_func)(EView*, ERect, EFilePanelItem*),
			  int (*sort_func)(const EFilePanelItem**, const EFilePanelItem**))
{
	struct column_data *data = new struct column_data;

	data->name = EStrdup(name);
	data->width = width;
	data->draw_func = draw_func;
	data->sort_func = sort_func;

	fColumns.AddItem(data);

	FrameResized(Bounds().Width(), Bounds().Height());

	Invalidate();
}


void
EFilePanelView::RemoveColumn(eint32 index)
{
	struct column_data *data = (struct column_data*)fColumns.RemoveItem(index);
	if(data != NULL)
	{
		if(data->name != NULL) delete[] data->name;
		delete data;
	}

	FrameResized(Bounds().Width(), Bounds().Height());

	Invalidate();
}


void
EFilePanelView::SwapColumns(eint32 indexA, eint32 indexB)
{
	fColumns.SwapItems(indexA, indexB);
	Invalidate();
}


eint32
EFilePanelView::CountColumns() const
{
	return fColumns.CountItems();
}


const char*
EFilePanelView::GetNameOfColumn(eint32 index) const
{
	struct column_data *data = (struct column_data*)fColumns.ItemAt(index);
	return(data != NULL ? data->name : NULL);
}


float
EFilePanelView::GetWidthOfColumn(eint32 index) const
{
	struct column_data *data = (struct column_data*)fColumns.ItemAt(index);
	return(data != NULL ? data->width : 0);
}


void
EFilePanelView::DrawItem(EView *owner, ERect itemRect, EFilePanelItem *item)
{
	ERect rect = itemRect;
	rect.right = rect.left;

	for(eint32 i = 0; i < fColumns.CountItems(); i++)
	{
		struct column_data *data = (struct column_data*)fColumns.ItemAt(i);

		rect.left = rect.right + (i == 0 ? 0.f : 1.f);
		rect.right = rect.left + data->width;

		if(data->draw_func == NULL) continue;
		data->draw_func(owner, rect & itemRect, item);
	}
}


void
EFilePanelView::SortItems(eint32 columnIndex)
{
	struct column_data *data = (struct column_data*)fColumns.ItemAt(columnIndex);
	if(data == NULL || data->sort_func == NULL) return;

	fListView->SortItems((int (*)(const EListItem**, const EListItem**))data->sort_func);
	fListView->Invalidate();
}


void
EFilePanelView::FrameResized(float new_width, float new_height)
{
	float w = 0;
	ERect rect;
	for(eint32 i = 0; i < CountColumns(); i++) w += GetWidthOfColumn(i);
	for(eint32 i = 0; i < fListView->CountItems(); i++) rect |= fListView->ItemFrame(i);

	fHSB->SetRange(0, max_c(w - new_width, 0));
	fHSB->SetEnabled(new_width < w);

	fVSB->SetRange(0, max_c(rect.Height() - fListView->Bounds().Height(), 0));
	fVSB->SetEnabled(fListView->Bounds().Height() < rect.Height());
}


EFilePanelTitleView::EFilePanelTitleView(ERect parent_bounds)
	: EView(parent_bounds, "TitleView", E_FOLLOW_LEFT_RIGHT | E_FOLLOW_TOP, E_WILL_DRAW)
{
	e_font_height fontHeight;
	GetFontHeight(&fontHeight);

	ResizeTo(parent_bounds.Width() - E_V_SCROLL_BAR_WIDTH - 1,
		 fontHeight.ascent + fontHeight.descent + 4);
	MoveTo(0, 0);
}


void
EFilePanelTitleView::Draw(ERect updateRect)
{
	EFilePanelView *parent = (EFilePanelView*)Parent();
	if(parent == NULL) return;

	e_rgb_color textColor = e_ui_color(E_PANEL_TEXT_COLOR);
	e_rgb_color shineColor = e_ui_color(E_SHINE_COLOR);
	e_rgb_color shadowColor = e_ui_color(E_SHADOW_COLOR);
	if(!IsEnabled())
	{
		textColor.disable(ViewColor());
		shineColor.disable(ViewColor());
		shadowColor.disable(ViewColor());
	}

	EFont font;
	e_font_height fontHeight;
	GetFont(&font);
	font.GetHeight(&fontHeight);

	PushState();

	SetDrawingMode(E_OP_COPY);
	SetPenSize(0);

	SetHighColor(ViewColor());
	FillRect(Bounds());

	ERect rect = Bounds();
	rect.right = rect.left;
	for(eint32 i = 0; i <= parent->CountColumns(); i++)
	{
		if(i == parent->CountColumns())
		{
			if(rect.right >= Bounds().right) break;
			rect.left = rect.right + (i == 0 ? 0.f : 1.f);
			rect.right = Bounds().right + 1;
		}
		else
		{
			const char *name = parent->GetNameOfColumn(i);

			rect.left = rect.right + (i == 0 ? 0.f : 1.f);
			rect.right = rect.left + parent->GetWidthOfColumn(i);

			if(name)
			{
				EPoint penLocation;
				penLocation.x = rect.Center().x - font.StringWidth(name) / 2.f;
				penLocation.y = rect.Center().y - (fontHeight.ascent + fontHeight.descent) / 2.f;
				penLocation.y += fontHeight.ascent + 1;

				SetHighColor(textColor);
				DrawString(name, penLocation);
			}
		}

		SetHighColor(shineColor);
		StrokeRect(rect);
		SetHighColor(shadowColor);
		StrokeLine(rect.LeftBottom(), rect.RightBottom());
		StrokeLine(rect.RightTop());
	}

	PopState();
}


void
EFilePanelTitleView::GetPreferredSize(float *width, float *height)
{
	if(width)
	{
		*width = 0;

		EFilePanelView *parent = (EFilePanelView*)Parent();
		for(eint32 i = 0; i < parent->CountColumns(); i++) *width += parent->GetWidthOfColumn(i);
	}

	if(height)
	{
		e_font_height fontHeight;
		GetFontHeight(&fontHeight);
		*height = fontHeight.ascent + fontHeight.descent + 4;
	}
}


void
EFilePanelTitleView::ScrollTo(EPoint where)
{
	EListView *listView = (EListView*)Parent()->FindView("PoseView");

	EView::ScrollTo(where);
	listView->ScrollTo(where.x, listView->ConvertToParent(EPoint(0, 0)).y - listView->Frame().top);
}


static e_filter_result filter_key_down_hook(EMessage *message, EHandler **target, EMessageFilter *filter)
{
	eint32 modifiers;
	const char *bytes;

	EFilePanelWindow *panelWindow = (EFilePanelWindow*)filter->Looper();

	if(message->FindInt32("modifiers", &modifiers) == false) return E_DISPATCH_MESSAGE;
	if(message->FindString("bytes", &bytes) == false || bytes == NULL) return E_DISPATCH_MESSAGE;

	if((modifiers & (E_COMMAND_KEY | E_CONTROL_KEY)) && !(bytes[0] != E_UP_ARROW || bytes[1] != 0))
	{
		EPath aPath(panelWindow->PanelDirectory());
		if(aPath.GetParent(&aPath) != E_OK) aPath.Unset();
		panelWindow->SetPanelDirectory(aPath.Path());
		return E_SKIP_MESSAGE;
	}

	return E_DISPATCH_MESSAGE;
}


EFilePanelWindow::EFilePanelWindow()
	: EWindow(ERect(-400, -400, -10, -10), "FilePanel: uncompleted", E_TITLED_WINDOW, 0),
	  fTarget(NULL), fMessage(NULL), fSelIndex(0), fShowHidden(false), fSort(0)
{
	EView *topView, *aView;
	EMenuBar *menuBar;
	EMenu *menu;
	EMenuItem *menuItem;
	EMenuField *menuField;
	ETextControl *textControl;
	EButton *button;

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
	fPanelView = new EFilePanelView(rect);
	aView->AddChild(fPanelView);

	MoveToCenter();

	e_find_directory(E_USER_DIRECTORY, &fPath);
	AddCommonFilter(new EMessageFilter(E_KEY_DOWN, filter_key_down_hook));
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
	eint32 index;

	switch(msg->what)
	{
		case MSG_PANEL_GET_DIR:
			msg->AddString("PanelDirectory", fPath.Path());
			msg->SendReply(msg);
			break;

		case MSG_PANEL_SELECTED:
			if(msg->FindInt32("index", &index))
			{
				EListView *listView = (EListView*)FindView("PoseView");
				EFilePanelItem *item = (EFilePanelItem*)listView->ItemAt(index);
				if(item == NULL || !(item->IsVolume() || item->IsDirectory())) break;
				fPath.SetTo(item->Path());
				Refresh();
			}
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
	EFilePanelWindow *self = (EFilePanelWindow*)listView->Window();

	if(self->fShowHidden == false)
	{
		EEntry aEntry(path);
		if(aEntry.IsHidden()) return false;
	}

	listView->AddItem(new EFilePanelItem(path, self->fPanelView));

	return false;
}


void
EFilePanelWindow::Refresh()
{
	EListView *listView = (EListView*)FindView("PoseView");

	listView->RemoveItems(0, -1, true);

	if(fPath.Path() != NULL)
	{
		EDirectory dir(fPath.Path());
		dir.DoForEach(RefreshCallback, (void*)listView);
	}
	else
	{
		EVolumeRoster volRoster;
		EVolume vol;

		while(volRoster.GetNextVolume(&vol) == E_NO_ERROR)
		{
			EDirectory volRootDir;
			EEntry aEntry;
			EPath aPath;

			vol.GetRootDirectory(&volRootDir);
			volRootDir.GetEntry(&aEntry);
			aEntry.GetPath(&aPath);

			if(aPath.Path() == NULL) continue;

			listView->AddItem(new EFilePanelItem(aPath.Path(), fPanelView, vol.Device()));
		}
	}

	EFilePanelLabel *label = (EFilePanelLabel*)FindView("CountVw");
	EString str;
	str << listView->CountItems() << " items";
	label->SetText(str.String());

	fPanelView->FrameResized(fPanelView->Frame().Width(), fPanelView->Frame().Height());
	fPanelView->SortItems(fSort);
}


void
EFilePanelWindow::SetPanelDirectory(const char *path)
{
	fPath.Unset();
	fPath.SetTo(path);
	Refresh();
}


const char*
EFilePanelWindow::PanelDirectory() const
{
	return fPath.Path();
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
	entry->SetTo(item->Path());

	fSelIndex++;

	return E_OK;
}


EFilePanel::EFilePanel(EMessenger *target,
		       EMessage *message,
		       const EDirectory *directory)
{
	fWindow = new EFilePanelWindow();
	if(directory) SetPanelDirectory(directory);
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
	EMessage msg(MSG_PANEL_GET_DIR);
	const char *path = NULL;

	entry->Unset();
	if(msgr.SendMessage(&msg, &msg) != E_OK) return;
	if(msg.FindString("PanelDirectory", &path) == false) return;
	entry->SetTo(path);
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

