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
 * File: Layout.h
 *
 * --------------------------------------------------------------------------*/

#ifndef __ETK_LAYOUT_H__
#define __ETK_LAYOUT_H__

#include <etk/interface/View.h>

#ifdef __cplusplus /* Just for C++ */

class ELayoutItem;


class _IMPEXP_ETK ELayoutContainer {
public:
	ELayoutContainer();
	virtual ~ELayoutContainer();

	virtual bool	AddItem(ELayoutItem *item, eint32 index = -1);
	virtual bool	RemoveItem(ELayoutItem *item);

	ELayoutItem	*ItemAt(eint32 index) const;
	eint32		IndexOf(ELayoutItem *item) const;
	eint32		CountItems() const;

private:
	void *fItems;
};


class _IMPEXP_ETK ELayoutItem : public ELayoutContainer {
public:
	ELayoutItem(ERect frame, euint32 resizingMode);
	virtual ~ELayoutItem();

	virtual bool	AddItem(ELayoutItem *item, eint32 index = -1);
	virtual bool	RemoveItem(ELayoutItem *item);
	bool		RemoveSelf();

	virtual void	SetResizingMode(euint32 mode);
	euint32		ResizingMode() const;

	virtual void	Show();
	virtual void	Hide();
	bool		IsHidden() const;

	virtual void	SendBehind(ELayoutItem *item);
	virtual void	MoveTo(EPoint where);
	virtual void	ScrollTo(EPoint where);
	virtual void	ResizeTo(float width, float height);

	virtual void	GetPreferredSize(float *width, float *height);
	virtual void	ResizeToPreferred();

	ERect		Bounds() const;
	ERect		Frame() const;
	ERegion		VisibleRegion() const;

	void		ConvertToParent(EPoint *pt) const;
	EPoint		ConvertToParent(EPoint pt) const;
	void		ConvertFromParent(EPoint *pt) const;
	EPoint		ConvertFromParent(EPoint pt) const;

private:
	ELayoutContainer *fContainer;
	EPoint fLocalOrigin;
	ERect fFrame;
	euint32 fResizingMode;
	bool fHidden;
};


class _IMPEXP_ETK ELayoutForm : public ELayoutItem {
public:
	ELayoutForm(ERect frame, euint32 resizingMode);
	virtual ~ELayoutForm();

	virtual bool	AddItem(ELayoutItem *item,
				eint32 row_index = -1,
				eint32 column_index = -1,
				float row_ratio = -1,
				float column_ratio = -1);
	virtual bool	RemoveItem(ELayoutItem *item);

	virtual void	SetItemRatio(ELayoutItem *item, float row_ratio, float column_ratio);
	void		GetItemRatio(ELayoutItem *item, float *row_ratio, float *column_ratio) const;

	virtual void	MoveTo(EPoint where);
	virtual void	ScrollTo(EPoint where);
	virtual void	ResizeTo(float width, float height);

	virtual void	GetPreferredSize(float *width, float *height);
	virtual void	ResizeToPreferred();

private:
	eint32 fRows;
	eint32 fColumns;
};

#endif /* __cplusplus */

#endif /* __ETK_LAYOUT_H__ */

