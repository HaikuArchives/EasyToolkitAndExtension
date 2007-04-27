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

#include <time.h>

#include <etk/support/Autolock.h>
#include <etk/support/ClassInfo.h>
#include <etk/app/Application.h>
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
#define MSG_PANEL_SET_DIR	'setd'
#define MSG_PANEL_DONE		'done'

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


_LOCAL class EFilePanelListItem : public EListItem {
public:
	EFilePanelListItem(const char *path, EFilePanelView *panel_view, e_dev_t dev = -1);
	virtual ~EFilePanelListItem();

	const char	*Path() const;
	const char	*Leaf() const;

	bool		IsVolume() const;
	bool		IsDirectory() const;

	eint64		Size() const;
	e_bigtime_t	ModifiedTime() const;
	EFilePanelView	*PanelView() const;

private:
	EPath fPath;
	char *fLeaf;

	eint32 fFlags;
	eint64 fSize;
	e_bigtime_t fModifiedTime;

	EFilePanelView *fPanelView;

	virtual void	DrawItem(EView *owner, ERect itemRect, bool drawEverything);
	virtual void	Update(EView *owner, const EFont *font);
};


_LOCAL class EFilePanelListView : public EListView {
public:
	EFilePanelListView(ERect frame, const char *name, e_list_view_type type);

	virtual void	SelectionChanged();
	void		SortItems(int (*sort_func)(const EFilePanelListItem**, const EFilePanelListItem**),
				  bool clear_position);
	virtual void	KeyDown(const char *bytes, eint32 numBytes);
};


_LOCAL class EFilePanelTitleView : public EView {
public:
	EFilePanelTitleView(ERect parent_bounds);

	virtual void	Draw(ERect updateRect);
	virtual void	GetPreferredSize(float *width, float *height);
	virtual void	ScrollTo(EPoint where);
	virtual void	MouseUp(EPoint where);
};


_LOCAL class EFilePanelView : public EView {
public:
	EFilePanelView(ERect frame, bool allow_multiple_selection);
	virtual ~EFilePanelView();

	void		AddColumn(const char *name, float width,
				  void (*draw_func)(EView*, ERect, EFilePanelListItem*),
				  int (*sort_func)(const EFilePanelListItem**, const EFilePanelListItem**));
	void		RemoveColumn(eint32 index);
	void		SwapColumns(eint32 indexA, eint32 indexB);

	eint32		CountColumns() const;
	const char	*GetNameOfColumn(eint32 index) const;
	float		GetWidthOfColumn(eint32 index) const;

	void		DrawItem(EView *owner, ERect itemRect, EFilePanelListItem*);
	void		SortItems(eint32 nColumn);
	int		GetSortIndex(eint32 *index) const;

	virtual void	FrameResized(float new_width, float new_height);

private:
	struct column_data {
		char *name;
		float width;
		void (*draw_func)(EView*, ERect, EFilePanelListItem*);
		int (*sort_func)(const EFilePanelListItem**, const EFilePanelListItem**);
	};

	EList fColumns;
	EFilePanelTitleView *fTitleView;
	EFilePanelListView *fListView;
	EScrollBar *fHSB;
	EScrollBar *fVSB;
	eint32 fSort;
};


_LOCAL class EFilePanelWindow : public EWindow {
public:
	EFilePanelWindow(EFilePanel *panel, e_file_panel_mode mode, bool modal, bool allow_multiple_selection);
	virtual ~EFilePanelWindow();

	virtual void		MessageReceived(EMessage *msg);
	virtual bool		QuitRequested();

	EFilePanel		*Panel() const;
	EMessenger		*Target() const;
	e_file_panel_mode	PanelMode() const;

	void			SetTarget(const EMessenger *target);
	void			SetMessage(const EMessage *msg);

	void			Refresh();
	void			SetPanelDirectory(const char *path);
	const char		*PanelDirectory() const;

	void			SetFilter(EFilePanelFilter *filter);
	void			SetSaveText(const char *text);
	void			SetButtonLabel(e_file_panel_button btn, const char *label);
	void			SetHideWhenDone(bool state);

	void			Rewind();
	e_status_t		GetNextSelected(EEntry *entry);

	void			RefreshDirMenu();

private:
	EFilePanel *fPanel;

	EPath fPath;

	e_file_panel_mode fMode;
	EMessenger *fTarget;
	EMessage *fMessage;
	EFilePanelFilter *fFilter;
	bool fHidesWhenDone;

	EFilePanelView *fPanelView;
	EMenu *fDirMenu;

	eint32 fSelIndex;
	bool fShowHidden;

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


EFilePanelListItem::EFilePanelListItem(const char *path, EFilePanelView *panel_view, e_dev_t dev)
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


EFilePanelListItem::~EFilePanelListItem()
{
	if(fLeaf) free(fLeaf);
}


void
EFilePanelListItem::DrawItem(EView *owner, ERect itemRect, bool drawEverything)
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
EFilePanelListItem::Update(EView *owner, const EFont *font)
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
EFilePanelListItem::Path() const
{
	return fPath.Path();
}


const char*
EFilePanelListItem::Leaf() const
{
	return fLeaf;
}


bool
EFilePanelListItem::IsDirectory() const
{
	return(fFlags == 1);
}


bool
EFilePanelListItem::IsVolume() const
{
	return(fFlags == 2);
}


eint64
EFilePanelListItem::Size() const
{
	return fSize;
}


e_bigtime_t
EFilePanelListItem::ModifiedTime() const
{
	return fModifiedTime;
}


EFilePanelView*
EFilePanelListItem::PanelView() const
{
	return fPanelView;
}


EFilePanelListView::EFilePanelListView(ERect frame, const char *name, e_list_view_type type)
	: EListView(frame, name, type, E_FOLLOW_ALL)
{
}


void
EFilePanelListView::SelectionChanged()
{
	EFilePanelWindow *win = (EFilePanelWindow*)Window();
	if(win) win->Panel()->SelectionChanged();
}


void
EFilePanelListView::KeyDown(const char *bytes, eint32 numBytes)
{
	eint32 oldPos = Position();
	EListView::KeyDown(bytes, numBytes);
	if(oldPos != Position()) ScrollToItem(Position());
}


void
EFilePanelListView::SortItems(int (*sort_func)(const EFilePanelListItem**, const EFilePanelListItem**),
			      bool clear_position)
{
	if(clear_position) SetPosition(-1);
	EListView::SortItems((int (*)(const EListItem**, const EListItem**))sort_func);
}


static int column_name_sort_callback(const EFilePanelListItem **_itemA, const EFilePanelListItem **_itemB)
{
	const EFilePanelListItem *itemA = *_itemA;
	const EFilePanelListItem *itemB = *_itemB;

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

	return(itemA->PanelView()->GetSortIndex(NULL) * (strA < strB ? -1 : 1));
}


static void column_name_drawing_callback(EView *owner, ERect rect, EFilePanelListItem *item)
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

		owner->DrawBitmap(bitmap, EPoint(rect.left, rect.Center().y - ICON_HEIGHT / 2));
		delete bitmap;
	}

	if(item->Leaf())
	{
		e_font_height fontHeight;
		owner->GetFontHeight(&fontHeight);

		float sHeight = fontHeight.ascent + fontHeight.descent;

		EPoint penLocation;
		penLocation.x = rect.left + ICON_WIDTH + 5;
		penLocation.y = rect.Center().y - sHeight / 2.f;
		penLocation.y += fontHeight.ascent + 1;

		owner->DrawString(item->Leaf(), penLocation);
	}
}


static int column_size_sort_callback(const EFilePanelListItem **_itemA, const EFilePanelListItem **_itemB)
{
	const EFilePanelListItem *itemA = *_itemA;
	const EFilePanelListItem *itemB = *_itemB;

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

	return(itemA->PanelView()->GetSortIndex(NULL) * (itemA->Size() < itemB->Size() ? -1 : 1));
}


static void column_size_drawing_callback(EView *owner, ERect rect, EFilePanelListItem *item)
{
	if(!rect.IsValid() || item->IsVolume() || item->IsDirectory()) return;

	EString str;
	if(item->Size() >= 0x40000000) str << ((float)item->Size() / (float)0x40000000) << "G";
	else if(item->Size() >= 0x100000) str << ((float)item->Size() / (float)0x100000) << "M";
	else if(item->Size() >= 0x400) str << ((float)item->Size() / (float)0x400) << "K";
	else str << item->Size() << " bytes";

	EFont font;
	e_font_height fontHeight;
	owner->GetFont(&font);
	font.GetHeight(&fontHeight);

	float sHeight = fontHeight.ascent + fontHeight.descent;

	EPoint penLocation;
	penLocation.x = rect.right - font.StringWidth(str.String());
	penLocation.y = rect.Center().y - sHeight / 2.f;
	penLocation.y += fontHeight.ascent + 1;

	owner->DrawString(str.String(), penLocation);
}


static int column_modified_sort_callback(const EFilePanelListItem **_itemA, const EFilePanelListItem **_itemB)
{
	const EFilePanelListItem *itemA = *_itemA;
	const EFilePanelListItem *itemB = *_itemB;

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

	return(itemA->PanelView()->GetSortIndex(NULL) * (itemA->ModifiedTime() < itemB->ModifiedTime() ? -1 : 1));
}


static void column_modified_drawing_callback(EView *owner, ERect rect, EFilePanelListItem *item)
{
	if(!rect.IsValid() || item->IsVolume()) return;

	time_t timer = (time_t)(item->ModifiedTime() / E_INT64_CONSTANT(1000000));
	struct tm *tmTime = localtime(&timer);

	if(tmTime == NULL) return;

	EString str;
	str.AppendFormat("%d-%02d-%02d, %02d:%02d",
			 1900 + tmTime->tm_year, 1 + tmTime->tm_mon, tmTime->tm_mday,
			 tmTime->tm_hour, tmTime->tm_min);

	e_font_height fontHeight;
	owner->GetFontHeight(&fontHeight);

	float sHeight = fontHeight.ascent + fontHeight.descent;

	EPoint penLocation;
	penLocation.x = rect.left + 5;
	penLocation.y = rect.Center().y - sHeight / 2.f;
	penLocation.y += fontHeight.ascent + 1;

	owner->DrawString(str.String(), penLocation);
}


EFilePanelView::EFilePanelView(ERect frame, bool allow_multiple_selection)
	: EView(frame, NULL, E_FOLLOW_ALL, E_FRAME_EVENTS), fSort(1)
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
	fListView = new EFilePanelListView(rect, "PoseView",
					   allow_multiple_selection ? E_MULTIPLE_SELECTION_LIST : E_SINGLE_SELECTION_LIST);
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
			  void (*draw_func)(EView*, ERect, EFilePanelListItem*),
			  int (*sort_func)(const EFilePanelListItem**, const EFilePanelListItem**))
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
EFilePanelView::DrawItem(EView *owner, ERect itemRect, EFilePanelListItem *item)
{
	ERect rect = itemRect;
	rect.right = rect.left;

	for(eint32 i = 0; i < fColumns.CountItems(); i++)
	{
		struct column_data *data = (struct column_data*)fColumns.ItemAt(i);

		rect.left = rect.right + (i == 0 ? 0.f : 1.f);
		rect.right = rect.left + data->width;

		if(data->draw_func == NULL) continue;

		owner->PushState();
		ERect aRect = rect.InsetByCopy(2, 2) & itemRect;
		owner->ConstrainClippingRegion(aRect);
		data->draw_func(owner, aRect, item);
		owner->PopState();
	}
}


void
EFilePanelView::SortItems(eint32 nColumn)
{
	if(nColumn > 0) fSort = ((fSort < 0 ? -fSort : fSort) == nColumn ? -fSort : nColumn);

	struct column_data *data = (struct column_data*)fColumns.ItemAt((fSort < 0 ? -fSort : fSort) - 1);
	if(data == NULL || data->sort_func == NULL) return;

	fListView->SortItems(data->sort_func, nColumn != 0);
	fListView->Invalidate();
}


int
EFilePanelView::GetSortIndex(eint32 *index) const
{
	if(fSort == 0) return 0;
	if(index) *index = (fSort < 0 ? -fSort : fSort) - 1;
	return(fSort < 0 ? -1 : 1);
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
	eint32 sortIndex = -1;

	if(parent == NULL) return;
	parent->GetSortIndex(&sortIndex);

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
				float strWidth = font.StringWidth(name);

				penLocation.x = rect.Center().x - strWidth / 2.f;
				penLocation.y = rect.Center().y - (fontHeight.ascent + fontHeight.descent) / 2.f;
				penLocation.y += fontHeight.ascent + 1;

				SetHighColor(textColor);
				DrawString(name, penLocation);

				if(i == sortIndex)
					StrokeLine(penLocation, penLocation + EPoint(strWidth, 0));
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


void
EFilePanelTitleView::MouseUp(EPoint where)
{
	EFilePanelView *parent = (EFilePanelView*)Parent();

	float left = 0, right = 0;
	for(eint32 i = 0; i < parent->CountColumns(); i++)
	{
		left = right + (i == 0 ? 0.f : 1.f);
		right = left + parent->GetWidthOfColumn(i);
		if(where.x < left || where.x > right) continue;
		parent->SortItems(i + 1);
		Invalidate();
		break;
	}
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


EFilePanelWindow::EFilePanelWindow(EFilePanel *panel, e_file_panel_mode mode, bool modal, bool allow_multiple_selection)
	: EWindow(ERect(0, 0, 600, 400),
		  mode == E_OPEN_PANEL ? "Open File" : "Save File",
		  modal ? E_MODAL_WINDOW : E_TITLED_WINDOW, 0),
	  fPanel(panel), fMode(mode), fTarget(NULL), fMessage(NULL), fFilter(NULL), fHidesWhenDone(true),
	  fSelIndex(0), fShowHidden(false)
{
	EView *topView, *aView;
	EMenuBar *menuBar;
	EMenu *menu;
	EMenuItem *menuItem;
	EMenuField *menuField;
	EButton *button;

	ERect rect = Bounds();

	topView = new EView(rect, NULL, E_FOLLOW_ALL, 0);
	topView->SetViewColor(e_ui_color(E_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	menu = new EMenu("File", E_ITEMS_IN_COLUMN);
	menu->AddItem(menuItem = new EMenuItem(mode == E_OPEN_PANEL ? "Open" : "Save", new EMessage(MSG_PANEL_DONE)));
	menuItem->SetTarget(this);
	menu->AddItem(menuItem = new EMenuItem("Cancel", new EMessage(E_QUIT_REQUESTED), 'q', E_COMMAND_KEY));
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

	fDirMenu = new EMenu("[None]", E_ITEMS_IN_COLUMN);
	menuField = new EMenuField(rect, "DirMenuField", "Location:", fDirMenu, false);
	menuField->GetPreferredSize(NULL, &rect.bottom);
	menuField->ResizeTo(rect.Width(), rect.Height());
	menuField->MoveTo(0, 0);
	aView->AddChild(menuField);

	rect.bottom = aView->Bounds().bottom;
	rect.top = rect.bottom - 20;

	if(mode == E_SAVE_PANEL)
	{
		ETextControl *textControl = new ETextControl(rect, "text control",
							     NULL, NULL, NULL,
							     E_FOLLOW_LEFT | E_FOLLOW_BOTTOM);
		textControl->ResizeTo(200, rect.Height());
		aView->AddChild(textControl);
	}

	rect.left = rect.right - 100;
	button = new EButton(rect, "default button",
			     mode == E_OPEN_PANEL ? "Open" : "Save",
			     new EMessage(MSG_PANEL_DONE),
			     E_FOLLOW_RIGHT | E_FOLLOW_BOTTOM);
	aView->AddChild(button);

	rect.OffsetBy(-110, 0);
	button = new EButton(rect, "cancel button",
			     "Cancel",
			     new EMessage(E_QUIT_REQUESTED),
			     E_FOLLOW_RIGHT | E_FOLLOW_BOTTOM);
	aView->AddChild(button);

	rect = aView->Bounds();
	rect.top = menuField->Frame().bottom + 5;
	rect.bottom -= 25;
	fPanelView = new EFilePanelView(rect, allow_multiple_selection);
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
	if(!IsHidden())
	{
		Hide();
		fPanel->WasHidden();
	}
	return false;
}


void
EFilePanelWindow::MessageReceived(EMessage *msg)
{
	eint32 index;
	const char *dir;
	const EMessenger *msgr;
	EMessage *aMsg = NULL;

	switch(msg->what)
	{
		case MSG_PANEL_GET_DIR:
			msg->AddString("PanelDirectory", fPath.Path());
			msg->SendReply(msg);
			break;

		case MSG_PANEL_SET_DIR:
			if(msg->FindString("PanelDirectory", &dir) == false || dir == NULL) break;
			fPath.SetTo(dir);
			Refresh();
			break;

		case MSG_PANEL_SELECTED: /* TODO: correct me!!! */
			if(msg->FindInt32("index", &index))
			{
				EListView *listView = (EListView*)FindView("PoseView");
				EFilePanelListItem *item = (EFilePanelListItem*)listView->ItemAt(index);
				if(item == NULL || !(item->IsVolume() || item->IsDirectory())) break;
				fPath.SetTo(item->Path());
				Refresh();
			}
			break;

		case MSG_PANEL_DONE:
			msgr = (fTarget == NULL ? &etk_app_messenger : fTarget);
			if(fMessage) aMsg = new EMessage(*fMessage);
			fPanel->SendMessage(msgr, aMsg);
			if(aMsg) delete aMsg;
			if(fHidesWhenDone && !IsHidden())
			{
				Hide();
				fPanel->WasHidden();
			}
			break;

		default:
			EWindow::MessageReceived(msg);
			break;
	}
}


EFilePanel*
EFilePanelWindow::Panel() const
{
	return fPanel;
}


EMessenger*
EFilePanelWindow::Target() const
{
	return fTarget;
}


e_file_panel_mode
EFilePanelWindow::PanelMode() const
{
	return fMode;
}


void
EFilePanelWindow::SetTarget(const EMessenger *target)
{
	if(fTarget) delete fTarget;
	fTarget = (target != NULL ? new EMessenger(*target) : NULL);
}


void
EFilePanelWindow::SetMessage(const EMessage *msg)
{
	if(fMessage) delete fMessage;
	fMessage = (msg != NULL ? new EMessage(*msg) : NULL);
}


bool
EFilePanelWindow::RefreshCallback(const char *path, void *data)
{
	EListView *listView = (EListView*)data;
	EFilePanelWindow *self = (EFilePanelWindow*)listView->Window();

	EEntry aEntry(path);
	if(self->fShowHidden == false && aEntry.IsHidden()) return false;
	if(!(self->fFilter == NULL || self->fFilter->Filter(&aEntry))) return false;

	listView->AddItem(new EFilePanelListItem(path, self->fPanelView));

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

			listView->AddItem(new EFilePanelListItem(aPath.Path(), fPanelView, vol.Device()));
		}
	}

	EFilePanelLabel *label = (EFilePanelLabel*)FindView("CountVw");
	EString str;
	str << listView->CountItems() << " items";
	label->SetText(str.String());

	fPanelView->FrameResized(fPanelView->Frame().Width(), fPanelView->Frame().Height());
	fPanelView->SortItems(0);

	RefreshDirMenu();
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
EFilePanelWindow::SetFilter(EFilePanelFilter *filter)
{
	if(fFilter) delete fFilter;
	fFilter = filter;
}


void
EFilePanelWindow::SetSaveText(const char *text)
{
	ETextControl *textControl = (ETextControl*)FindView("text control");
	if(textControl) textControl->SetText(text);
}


void
EFilePanelWindow::SetButtonLabel(e_file_panel_button btn, const char *label)
{
	EButton *button = (EButton*)FindView(btn == E_CANCEL_BUTTON ? "cancel button" : "default button");
	if(button) button->SetLabel(label);
}


void
EFilePanelWindow::SetHideWhenDone(bool state)
{
	fHidesWhenDone = state;
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
	EFilePanelListItem *item = (EFilePanelListItem*)listView->ItemAt(listView->CurrentSelection(fSelIndex));

	if(item == NULL) return E_ERROR;
	entry->SetTo(item->Path());

	fSelIndex++;

	return E_OK;
}


void
EFilePanelWindow::RefreshDirMenu()
{
	EString str;
	EMessage *msg;
	EMenuItem *menuItem;

	if(fDirMenu->CountItems() == 0)
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

			msg = new EMessage(MSG_PANEL_SET_DIR);
			msg->AddString("PanelDirectory", aPath.Path());

			str.MakeEmpty();
			vol.GetName(&str);
			str.Prepend("[Volume] ");

			menuItem = new EMenuItem(str.String(), msg);
			menuItem->SetTarget(this);
			fDirMenu->AddItem(menuItem);
		}

		if(fDirMenu->CountItems() > 0) fDirMenu->AddItem(new EMenuSeparatorItem(), 0);
	}
	else
	{
		while((menuItem = fDirMenu->ItemAt(0)) != NULL)
		{
			if(e_is_instance_of(menuItem, EMenuSeparatorItem)) break;
			fDirMenu->RemoveItem(0);
			delete menuItem;
		}
	}

	EMenu *menu = new EMenu("Other directories", E_ITEMS_IN_COLUMN);
	fDirMenu->AddItem(menu, 0);

	EPath aPath = fPath;
	while(aPath.GetParent(&aPath) == E_OK)
	{
		msg = new EMessage(MSG_PANEL_SET_DIR);
		msg->AddString("PanelDirectory", aPath.Path());

		menuItem = new EMenuItem(aPath.Path(), msg);
		menuItem->SetTarget(this);
		menu->AddItem(menuItem);
	}

	if(menu->CountItems() == 0)
	{
		menuItem = new EMenuItem("[Empty]", NULL);
		menuItem->SetEnabled(false);
		menu->AddItem(menuItem);
	}

	fDirMenu->Superitem()->SetLabel(fPath.Path() == NULL ? "All volumes" : fPath.Path());
}


EFilePanel::EFilePanel(e_file_panel_mode mode,
		       const EMessenger *target,
		       const char *panel_directory,
		       bool allow_multiple_selection,
		       const EMessage *message,
		       EFilePanelFilter *filter,
		       bool modal,
		       bool hide_when_done)
{
	fWindow = new EFilePanelWindow(this, mode, modal, allow_multiple_selection);
	if(panel_directory) SetPanelDirectory(panel_directory);
	SetTarget(target);
	SetMessage(message);
	SetFilter(filter);
	SetHideWhenDone(hide_when_done);
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
EFilePanel::IsShowing() const
{
	return(!fWindow->IsHidden());
}


void
EFilePanel::WasHidden()
{
}


void
EFilePanel::SelectionChanged()
{
}


void
EFilePanel::SendMessage(const EMessenger *msgr, EMessage *msg)
{
	if(msgr && msg) msgr->SendMessage(msg);
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


e_file_panel_mode
EFilePanel::PanelMode() const
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	return win->PanelMode();
}


void
EFilePanel::SetTarget(const EMessenger *target)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetTarget(target);
}


void
EFilePanel::SetMessage(const EMessage *msg)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetMessage(msg);
}


void
EFilePanel::SetFilter(EFilePanelFilter *filter)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetFilter(filter);
}


void
EFilePanel::SetSaveText(const char *text)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetSaveText(text);
}


void
EFilePanel::SetButtonLabel(e_file_panel_button btn, const char *label)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetButtonLabel(btn, label);
}


void
EFilePanel::SetHideWhenDone(bool state)
{
	EFilePanelWindow *win = (EFilePanelWindow*)fWindow;
	EAutolock <EFilePanelWindow> autolock(win);

	win->SetHideWhenDone(state);
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

