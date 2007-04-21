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
 * File: ScrollView.cpp
 *
 * --------------------------------------------------------------------------*/

#include "ScrollView.h"

ERect
EScrollView::TargetValidFrame(bool ignore_scrollbar) const
{
	// TODO: affect by border style

	if(fTarget == NULL) return ERect();

	ERect r = Bounds();

	if(!ignore_scrollbar)
	{
		if(!(fVSB == NULL || fVSB->IsHidden())) r.right -= (E_V_SCROLL_BAR_HEIGHT + UnitsPerPixel());
		if(!(fHSB == NULL || fHSB->IsHidden())) r.bottom -= (E_H_SCROLL_BAR_WIDTH + UnitsPerPixel());
	}

	return r;
}


EScrollView::EScrollView(ERect frame, const char *name, EView *target, euint32 resizingMode, euint32 flags,
			 bool alwaysShowHorizontal, bool alwaysShowVertical, e_border_style border)
	: EView(frame, name, resizingMode, 0), fTarget(NULL)
{
	fBorder = border;
	fAlwaysShowHorizontal = alwaysShowHorizontal;
	fAlwaysShowVertical = alwaysShowVertical;

	ERect hR = Bounds();
	ERect vR = Bounds();
	hR.top = hR.bottom - E_H_SCROLL_BAR_WIDTH;
	hR.right -= E_V_SCROLL_BAR_HEIGHT;
	vR.left = vR.right - E_V_SCROLL_BAR_HEIGHT;
	vR.bottom -= E_H_SCROLL_BAR_WIDTH;

	fHSB = new EScrollBar(hR, NULL, 0, 0, 0, E_HORIZONTAL);
	fVSB = new EScrollBar(vR, NULL, 0, 0, 0, E_VERTICAL);
	fHSB->Hide();
	fVSB->Hide();
	AddChild(fHSB);
	AddChild(fVSB);

	if(fHSB->Parent() != this) {delete fHSB; fHSB = NULL;}
	if(fVSB->Parent() != this) {delete fVSB; fVSB = NULL;}

	flags |= E_FRAME_EVENTS;

	if(fBorder != E_NO_BORDER)
		EView::SetFlags(flags | E_WILL_DRAW);
	else
		EView::SetFlags(flags & ~E_WILL_DRAW);

	if(target != NULL) SetTarget(target);
}


EScrollView::~EScrollView()
{
	SetTarget(NULL);
}


EScrollView::EScrollView(EMessage *from)
	: EView(from), fBorder(E_NO_BORDER), fAlwaysShowHorizontal(false), fAlwaysShowVertical(false), fTarget(NULL)
{
	ERect hR = Bounds();
	ERect vR = Bounds();
	hR.top = hR.bottom - E_H_SCROLL_BAR_WIDTH;
	hR.right -= E_V_SCROLL_BAR_HEIGHT;
	vR.left = vR.right - E_V_SCROLL_BAR_HEIGHT;
	vR.bottom -= E_H_SCROLL_BAR_WIDTH;

	fHSB = new EScrollBar(hR, NULL, 0, 0, 0, E_HORIZONTAL);
	fVSB = new EScrollBar(vR, NULL, 0, 0, 0, E_VERTICAL);
	fHSB->Hide();
	fVSB->Hide();
	AddChild(fHSB);
	AddChild(fVSB);

	if(fHSB->Parent() != this) {delete fHSB; fHSB = NULL;}
	if(fVSB->Parent() != this) {delete fVSB; fVSB = NULL;}

	euint32 flags = (Flags() | E_FRAME_EVENTS);

	if(fBorder != E_NO_BORDER)
		EView::SetFlags(flags | E_WILL_DRAW);
	else
		EView::SetFlags(flags & ~E_WILL_DRAW);
}


e_status_t
EScrollView::Archive(EMessage *into, bool deep) const
{
	if(!into) return E_ERROR;

	EView::Archive(into, deep);
	into->AddString("class", "EScrollView");

	// TODO

	return E_OK;
}


EArchivable*
EScrollView::Instantiate(EMessage *from)
{
	if(e_validate_instantiation(from, "EScrollView"))
		return new EScrollView(from);
	return NULL;
}


e_status_t
EScrollView::SetTarget(EView *newTarget)
{
	if(newTarget == fTarget || newTarget == this) return E_ERROR;
	if(newTarget != NULL)
		if(newTarget->Window() != NULL || newTarget->Parent() != NULL) return E_ERROR;

	if(fTarget != NULL)
	{
		EView *target = fTarget;
		fTarget = NULL;

		target->RemoveSelf();
		delete target;
	}

	if(newTarget == NULL)
	{
		if(fHSB != NULL)
		{
			if(!fAlwaysShowHorizontal) fHSB->Hide();
			fHSB->SetRange(0, 0);
		}

		if(fVSB != NULL)
		{
			if(!fAlwaysShowVertical) fVSB->Hide();
			fVSB->SetRange(0, 0);
		}

		return E_OK;
	}

	fTarget = newTarget;
	AddChild(newTarget);
	if(newTarget->Parent() != this)
	{
		ETK_WARNING("[INTERFACE]: %s --- Unable to add target.", __PRETTY_FUNCTION__);
		fTarget = NULL;
		return E_ERROR;
	}

	if(fHSB != NULL) fHSB->SetTarget(fTarget);
	if(fVSB != NULL) fVSB->SetTarget(fTarget);
	EScrollView::FrameResized(Frame().Width(), Frame().Height());
	fTarget->TargetedByScrollView(this);

	return E_OK;
}


EView*
EScrollView::Target() const
{
	return fTarget;
}


void
EScrollView::SetBorder(e_border_style border)
{
	if(fBorder != border)
	{
		fBorder = border;

		if(fBorder != E_NO_BORDER)
			EView::SetFlags(Flags() | E_WILL_DRAW);
		else
			EView::SetFlags(Flags() & ~E_WILL_DRAW);

		Invalidate();
	}
}


e_border_style
EScrollView::Border() const
{
	return fBorder;
}


EScrollBar*
EScrollView::ScrollBar(e_orientation direction) const
{
	return(direction == E_HORIZONTAL ? fHSB : fVSB);
}


void
EScrollView::Draw(ERect updateRect)
{
	// TODO
}


void
EScrollView::FrameResized(float new_width, float new_height)
{
	if(fTarget == NULL) return;

	if(fHSB != NULL)
	{
		if(!fAlwaysShowHorizontal && TargetValidFrame(true).Width() >= fTarget->Bounds().Width()) fHSB->Hide();
		else fHSB->Show();
		fHSB->SetRange(0, max_c(fTarget->Bounds().Width() - TargetValidFrame(false).Width(), 0.f));
	}

	if(fVSB != NULL)
	{
		if(!fAlwaysShowVertical && TargetValidFrame(true).Height() >= fTarget->Bounds().Height()) fVSB->Hide();
		else fVSB->Show();
		fVSB->SetRange(0, max_c(fTarget->Bounds().Height() - TargetValidFrame(false).Height(), 0.f));
	}

	bool hsbHidden = (fHSB == NULL ? true : fHSB->IsHidden());
	bool vsbHidden = (fVSB == NULL ? true : fVSB->IsHidden());

	if((hsbHidden != vsbHidden) && (hsbHidden || vsbHidden))
	{
		if(vsbHidden && fHSB != NULL)
		{
			ERect hR = Bounds();
			hR.top = hR.bottom - E_H_SCROLL_BAR_WIDTH;
			fHSB->ResizeTo(hR.Width(), hR.Height());
			fHSB->MoveTo(hR.LeftTop());
		}
		else if(hsbHidden && fVSB != NULL)
		{
			ERect vR = Bounds();
			vR.left = vR.right - E_V_SCROLL_BAR_HEIGHT;
			fVSB->ResizeTo(vR.Width(), vR.Height());
			fVSB->MoveTo(vR.LeftTop());
		}
	}
	else
	{
		ERect hR = Bounds();
		ERect vR = Bounds();
		hR.top = hR.bottom - E_H_SCROLL_BAR_WIDTH;
		hR.right -= E_V_SCROLL_BAR_HEIGHT;
		vR.left = vR.right - E_V_SCROLL_BAR_HEIGHT;
		vR.bottom -= E_H_SCROLL_BAR_WIDTH;

		fHSB->ResizeTo(hR.Width(), hR.Height());
		fHSB->MoveTo(hR.LeftTop());
		fVSB->ResizeTo(vR.Width(), vR.Height());
		fVSB->MoveTo(vR.LeftTop());
	}
}


void
EScrollView::SetScrollBarAutoState(bool alwaysShowHorizontal, bool alwaysShowVertical)
{
	fAlwaysShowHorizontal = alwaysShowHorizontal;
	fAlwaysShowVertical = alwaysShowVertical;

	EScrollView::FrameResized(Frame().Width(), Frame().Height());
}


void
EScrollView::GetScrollBarAutoState(bool *alwaysShowHorizontal, bool *alwaysShowVertical) const
{
	if(alwaysShowHorizontal) *alwaysShowHorizontal = fAlwaysShowHorizontal;
	if(alwaysShowVertical) *alwaysShowVertical = fAlwaysShowVertical;
}


void
EScrollView::SetFlags(euint32 flags)
{
	flags |= E_FRAME_EVENTS;
	if(fBorder != E_NO_BORDER)
		flags |= E_WILL_DRAW;
	else
		flags &= ~E_WILL_DRAW;
	EView::SetFlags(flags);
}


void
EScrollView::ChildRemoving(EView *child)
{
	if(fHSB == child)
	{
		fHSB = NULL;
		if(fTarget) fTarget->_UpdateOriginAndVisibleRegion(true);
	}
	else if(fVSB == child)
	{
		fVSB = NULL;
		fTarget->_UpdateOriginAndVisibleRegion(true);
	}
	else if(fTarget == child)
	{
		fTarget->TargetedByScrollView(NULL);
		fTarget = NULL;
	}
}


ERect
EScrollView::TargetFrame() const
{
	ERect r = TargetValidFrame(false);
	if(fTarget) r &= fTarget->ConvertToParent(fTarget->Bounds());
	return r;
}

