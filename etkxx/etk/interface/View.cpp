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
 * File: View.cpp
 * Description: EView --- drawing/layout/control within EWindow
 *
 * --------------------------------------------------------------------------*/

#include <math.h>
#include <etk/config.h>

#include <etk/support/ClassInfo.h>
#include <etk/add-ons/font/FontEngine.h>
#include <etk/support/List.h>
#include <etk/add-ons/graphics/GraphicsEngine.h>
#include <etk/app/Application.h>

#include "GraphicsDefs.h"
#include "Window.h"
#include "View.h"
#include "ScrollView.h"
#include "Box.h"
#include "Bitmap.h"

#ifndef HAVE_ROUND
inline double etk_round(double value)
{
	double iValue = 0;
	double fValue = modf(value, &iValue);

	if(fValue >= 0.5) iValue += 1;
	else if(fValue <= -0.5) iValue -= 1;

	return iValue;
}
#else
#define etk_round(a) round(a)
#endif // HAVE_ROUND

typedef struct _EViewState_ {
	e_drawing_mode		DrawingMode;
	EPoint			PenLocation;
	float			PenSize;
	e_rgb_color		HighColor;
	e_rgb_color		LowColor;
	bool			CustomHighColor;
	bool			CustomLowColor;
	EFont			Font;
	ERegion			Clipping;
	bool			HasClipping;
	bool			SquarePointStyle;

	struct _EViewState_	*prev;
} _EViewState_;


EView::EView(ERect frame, const char *name, euint32 resizingMode, euint32 flags)
	: EHandler(name), fDC(NULL), fOrigin(0, 0), fLocalOrigin(0, 0),
	  fParent(NULL), fNextSibling(NULL), fPrevSibling(NULL), fHidden(false), fEnabled(true),
	  fPen(0, 0), fPenSize(0), fDrawingMode(E_OP_COPY),
	  fIsCustomViewColor(false), fIsCustomHighColor(false), fIsCustomLowColor(false),
	  fForceFontAliasing(false), fSquarePointStyle(false),
	  fMouseGrabbed(false), fKeyboardGrabbed(false),
	  fEventStored(false), fEventMaskStored(0), fEventOptionsStored(0), fEventMask(0), fEventOptions(0), fMouseInside(false),
	  fHasClipping(false), fStatesList(NULL), fScrollTimestamp(E_INT64_CONSTANT(0))
{
	fFrame = frame;
	fViewResizingMode = resizingMode;
	fViewFlags = flags;

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
		ETK_ERROR("[INTERFACE]: %s --- View must created within a application which has graphics-engine!", __PRETTY_FUNCTION__);

	if((fDC = etk_app->fGraphicsEngine->CreateContext()) == NULL)
	{
		ETK_ERROR("[INTERFACE]: %s --- Unable to create draw context!", __PRETTY_FUNCTION__);
		return;
	}

	fDC->SetDrawingMode(E_OP_COPY);
	fDC->SetPenSize(0);

	fFont = *etk_plain_font;
}


EView::~EView()
{
	EWindow *win = Window();
	if(fParent || win)
	{
		ETK_WARNING("[INTERFACE]: %s --- It's recommended that remove self from parent before delete.", __PRETTY_FUNCTION__);
		if(win)
		{
			win->Lock();
			win->RemoveChild(this);
			win->Unlock();
		}
		else
		{
			fParent->RemoveChild(this);
		}
	}

	EView *child = NULL;
	while((child = (EView*)fViewsList.LastItem()) != NULL)
	{
		RemoveChild(child);
		delete child;
	}

	if(fDC) delete fDC;

	_EViewState_ *statesList = (_EViewState_*)fStatesList;
	while(statesList != NULL)
	{
		_EViewState_ *attr = statesList;
		statesList = attr->prev;
		delete attr;
	}
}


EView::EView(EMessage *from)
	: EHandler(from), fDC(NULL), fOrigin(0, 0), fLocalOrigin(0, 0),
	  fViewResizingMode(0), fViewFlags(0), fParent(NULL), fNextSibling(NULL), fPrevSibling(NULL), fHidden(false), fEnabled(true),
	  fPen(0, 0), fPenSize(0), fDrawingMode(E_OP_COPY),
	  fIsCustomViewColor(false), fIsCustomHighColor(false), fIsCustomLowColor(false),
	  fForceFontAliasing(false), fSquarePointStyle(false),
	  fMouseGrabbed(false), fKeyboardGrabbed(false),
	  fEventStored(false), fEventMaskStored(0), fEventOptionsStored(0), fEventMask(0), fEventOptions(0), fMouseInside(false),
	  fHasClipping(false), fStatesList(NULL), fScrollTimestamp(E_INT64_CONSTANT(0))
{
	// TODO
}


e_status_t
EView::Archive(EMessage *into, bool deep) const
{
	if(!into) return E_ERROR;

	EHandler::Archive(into, deep);
	into->AddString("class", "EView");

	// TODO

	return E_OK;
}


EArchivable*
EView::Instantiate(EMessage *from)
{
	if(e_validate_instantiation(from, "EView"))
		return new EView(from);
	return NULL;
}


void
EView::MessageReceived(EMessage *msg)
{
	switch(msg->what)
	{
		case E_PULSE:
			Pulse();
			break;

		case E_VIEW_MOVED:
			{
				EPoint where;
				if(msg->FindPoint("where", &where) == false) break;
				FrameMoved(where);
			}
			break;

		case E_VIEW_RESIZED:
			{
				float width, height;
				if(msg->FindFloat("width", &width) == false) break;
				if(msg->FindFloat("height", &height) == false) break;
				FrameResized(width, height);
			}
			break;

		case E_MOUSE_DOWN:
		case E_MOUSE_UP:
		case E_MOUSE_MOVED:
			{
				EWindow *win = Window();
				if(!win) break;

				EPoint where;
				if(msg->FindPoint("where", &where) == false) break;

				bool IsCurrentMessage = (win->CurrentMessage() == msg ? true : false);

				if(msg->what == E_MOUSE_DOWN)
				{
					MouseDown(where);
				}
				else if(msg->what == E_MOUSE_UP)
				{
					if(fEventStored)
					{
						if(fMouseGrabbed)
						{
							win->_UngrabMouse();
							fMouseGrabbed = false;
						}

						euint32 eventMask = fEventMask & ~E_POINTER_EVENTS;

						if(eventMask == 0)
						{
							_SetEventMask(fEventMaskStored, fEventOptionsStored);
							fEventStored = false;
						}
						else
						{
							_SetEventMask(eventMask, fEventOptions);
							if(!fKeyboardGrabbed) fEventOptions &= ~E_LOCK_WINDOW_FOCUS;
						}
					}

					MouseUp(where);
				}
				else // E_MOUSE_MOVED
				{
					bool insided = fVisibleRegion.Contains(ConvertToParent(where));
					euint32 transit = (insided ? E_INSIDE_VIEW : E_OUTSIDE_VIEW);
					if(fMouseInside != insided)
					{
						if(!insided)
						{
							fMouseInside = false;
							transit = E_EXITED_VIEW;
							win->fMouseInsideViews.RemoveItem(this);
						}
						else
						{
							fMouseInside = true;
							transit = E_ENTERED_VIEW;
							win->fMouseInsideViews.AddItem(this);
						}
					}
					else if(!(fEventOptions & E_NO_POINTER_HISTORY) && IsCurrentMessage)
					{
						win->MessageQueue()->Lock();
						EMessage *aMsg = win->MessageQueue()->FindMessage((eint32)0);
						if(!(aMsg == NULL || aMsg->what != E_MOUSE_MOVED)) do
						{
							// TODO: check target
							EPoint aWhere;
							if(aMsg->FindPoint("where", &aWhere) == false) break;
							ConvertFromWindow(&aWhere);
							ConvertToParent(&aWhere);
							if(fVisibleRegion.Contains(aWhere) == false) break;
							aMsg = win->DetachCurrentMessage();
							if(aMsg) delete aMsg;
							ETK_DEBUG("[INTERFACE]: Ignore E_MOUSE_EVENT.");
						} while(false);
						win->MessageQueue()->Unlock();
					}

					// TODO: drag info
					if(!IsCurrentMessage || win->CurrentMessage()) MouseMoved(where, transit, NULL);
				}
				if(IsCurrentMessage && win->CurrentMessage() == NULL) break;

				EView *view;
				EMessage aMsg(*msg);

				for(eint32 i = 0; i < fViewsList.CountItems(); i++)
				{
					if((view = (EView*)fViewsList.ItemAt(i)) == NULL) continue;
					if(view->VisibleFrameRegion().Contains(where) == false) continue;

					if(!(view->EventMask() & E_POINTER_EVENTS))
					{
						EPoint pt = view->ConvertFromParent(where);
						aMsg.ReplacePoint("where", pt);
						view->MessageReceived(&aMsg);
					}

					break; // just one child can receive the message
				}
			}
			break;

		case E_UNMAPPED_KEY_DOWN:
		case E_UNMAPPED_KEY_UP:
		case E_KEY_DOWN:
		case E_KEY_UP:
			{
				EWindow *win = Window();
				if(!win) break;

				if(msg->what == E_UNMAPPED_KEY_UP || msg->what == E_KEY_UP)
				{
					if(fEventStored)
					{
						if(fKeyboardGrabbed)
						{
							win->_UngrabKeyboard();
							fKeyboardGrabbed = false;
						}

						euint32 eventMask = fEventMask & ~E_KEYBOARD_EVENTS;

						if(eventMask == 0)
						{
							_SetEventMask(fEventMaskStored, fEventOptionsStored);
							fEventStored = false;
						}
						else
						{
							_SetEventMask(eventMask, fEventOptions);
							if(!fMouseGrabbed) fEventOptions &= ~E_LOCK_WINDOW_FOCUS;
						}
					}
				}

				char _bytes[4];
				bzero(_bytes, 4);
				const char *bytes = NULL;

				if(msg->FindString("bytes", &bytes) != true || bytes == NULL)
				{
					bytes = _bytes;
					if((msg->what == E_KEY_DOWN || msg->what == E_KEY_UP))
					{
						for(eint32 i = 0; i < 3; i++)
						{
							eint8 val;
							if(!msg->FindInt8("byte", i, &val)) break;
							_bytes[i] = val;
						}
					}
				}

				if(msg->what == E_KEY_DOWN || msg->what == E_UNMAPPED_KEY_DOWN)
					KeyDown(bytes, strlen(bytes));
				else
					KeyUp(bytes, strlen(bytes));
			}
			break;

		default:
			EHandler::MessageReceived(msg);
	}
}


void
EView::Show()
{
	if(!fHidden) return;

	fHidden = false;

	EWindow *win = Window();
	if(win)
	{
		_UpdateOriginAndVisibleRegion(true);
		EView *nextSibling = fNextSibling;
		while(nextSibling != NULL)
		{
			if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
			nextSibling = nextSibling->fNextSibling;
		}
		Invalidate();

		win->fNeededToPulseViews.AddItem(this);
		if(win->fPulseRunner)
			win->fPulseRunner->SetCount((win->fPulseRate > E_INT64_CONSTANT(0) &&
						     win->fNeededToPulseViews.CountItems() > 0 &&
						     win->IsHidden() == false) ? -1 : 0);
	}

	if(!e_is_kind_of(this, EScrollBar) ||
	   !e_is_kind_of(fParent, EScrollView) ||
	   e_cast_as(fParent, EScrollView)->fTarget == NULL) return;

	if(e_cast_as(fParent, EScrollView)->fHSB == this ||
	   e_cast_as(fParent, EScrollView)->fVSB == this)
		e_cast_as(fParent, EScrollView)->fTarget->_UpdateOriginAndVisibleRegion(true);
}


void
EView::Hide()
{
	if(fHidden) return;

	EView::MakeFocus(false);
	Invalidate();

	fHidden = true;

	EWindow *win = Window();
	if(win)
	{
		_UpdateOriginAndVisibleRegion(true);
		EView *nextSibling = fNextSibling;
		while(nextSibling != NULL)
		{
			if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
			nextSibling = nextSibling->fNextSibling;
		}

		win->fNeededToPulseViews.RemoveItem(this);
		if(win->fPulseRunner)
			win->fPulseRunner->SetCount((win->fPulseRate > E_INT64_CONSTANT(0) &&
						     win->fNeededToPulseViews.CountItems() > 0 &&
						     win->IsHidden() == false) ? -1 : 0);
	}

	if(!e_is_kind_of(this, EScrollBar) ||
	   !e_is_kind_of(fParent, EScrollView) ||
	   e_cast_as(fParent, EScrollView)->fTarget == NULL) return;

	if(e_cast_as(fParent, EScrollView)->fHSB == this ||
	   e_cast_as(fParent, EScrollView)->fVSB == this)
		e_cast_as(fParent, EScrollView)->fTarget->_UpdateOriginAndVisibleRegion(true);
}


bool
EView::IsHidden() const
{
	if(fHidden || Window() == NULL) return true;
	if(fParent) return fParent->IsHidden();

	return false;
}


void
EView::AttachedToWindow()
{
}


void
EView::AllAttached()
{
}


void
EView::DetachedFromWindow()
{
}


void
EView::AllDetached()
{
}


void
EView::ChildRemoving(EView *child)
{
}


void
EView::AddChild(EView *child, EView *nextSibling)
{
	eint32 indexNext = -1;

	if(child == NULL || child->Looper() != NULL || child->Parent() != NULL ||
	   (nextSibling == NULL ? false : (nextSibling->Parent() != this || (indexNext = fViewsList.IndexOf(nextSibling)) < 0)))
	{
		ETK_WARNING("[INTERFACE]: %s --- Unable to add child.", __PRETTY_FUNCTION__);
		return;
	}

	EView *prevSibling = (indexNext >= 0 ? nextSibling->fPrevSibling : (EView*)fViewsList.LastItem());
	if((indexNext >= 0 ? fViewsList.AddItem(child, indexNext) : fViewsList.AddItem(child)) == false)
	{
		ETK_WARNING("[INTERFACE]: %s --- Unable to add child to views list.", __PRETTY_FUNCTION__);
		return;
	}

	if(prevSibling != NULL) prevSibling->fNextSibling = child;
	if(nextSibling != NULL) nextSibling->fPrevSibling = child;

	child->fParent = this;
	child->fPrevSibling = prevSibling;
	child->fNextSibling = nextSibling;

	EWindow *win = Window();
	if(win)
	{
		win->AddHandler(child);
		if(child->Looper() == win)
		{
			child->AttachToWindow();
			child->AttachedToWindow();

			win->AddViewChildrenToHandlersList(win, child);
			child->AllAttached();
		}
		else
		{
			ETK_OUTPUT("Warning: [INTERFACE]: %s --- Unable to attch child to window, but child added.\n", __PRETTY_FUNCTION__);
		}
	}

	if(child->IsHidden()) return;

	while(nextSibling != NULL)
	{
		if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
		nextSibling = nextSibling->fNextSibling;
	}
}


bool
EView::IsSibling(const EView *sibling) const
{
	if(sibling == NULL || sibling == this) return false;

	if(fParent != sibling->fParent) return false;
	if(fParent != NULL) return true;

	if(Window() != sibling->Window()) return false;
	if(Window() == NULL) return false;

	return(Window()->fViewsList.IndexOf((void*)this) >= 0 && Window()->fViewsList.IndexOf((void*)sibling) >= 0);
}


bool
EView::RemoveChild(EView *child)
{
	if(!child || child->Parent() != this) return false;

	bool childIsHidden = child->IsHidden();

	if(child->fScrollBar.IsEmpty() == false)
	{
		for(eint32 i = 0; i < child->fScrollBar.CountItems(); i++)
		{
			EScrollBar *scrollbar = (EScrollBar*)child->fScrollBar.ItemAt(i);
			scrollbar->fTarget = NULL;
		}
		child->fScrollBar.MakeEmpty();
	}

	if(e_is_kind_of(child, EScrollBar))
	{
		EScrollBar *scrollbar = e_cast_as(child, EScrollBar);
		if(scrollbar->fTarget != NULL)
		{
			scrollbar->fTarget->fScrollBar.RemoveItem(scrollbar);
			scrollbar->fTarget = NULL;
		}
	}

	if(child->Parent()) child->Parent()->ChildRemoving(child);

	EWindow *win = Window();
	if(win)
	{
		win->RemoveViewChildrenFromHandlersList(win, child);
		child->AllDetached();

		child->DetachedFromWindow();

		child->DetachFromWindow();
		win->RemoveHandler(child);
	}

	fViewsList.RemoveItem(child);

	EView *nextSibling = child->fNextSibling;

	child->fParent = NULL;
	if(child->fPrevSibling != NULL) child->fPrevSibling->fNextSibling = child->fNextSibling;
	if(child->fNextSibling != NULL) child->fNextSibling->fPrevSibling = child->fPrevSibling;
	child->fPrevSibling = NULL;
	child->fNextSibling = NULL;

	while(childIsHidden == false && nextSibling != NULL)
	{
		if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
		nextSibling = nextSibling->fNextSibling;
	}

	return true;
}


bool
EView::RemoveSelf()
{
	if(fParent == NULL)
	{
		if(Window() == NULL) return false;
		return Window()->RemoveChild(this);
	}

	return fParent->RemoveChild(this);
}


eint32
EView::CountChildren() const
{
	return fViewsList.CountItems();
}


EView*
EView::ChildAt(eint32 index) const
{
	return((EView*)fViewsList.ItemAt(index));
}


EView*
EView::NextSibling() const
{
	return fNextSibling;
}


EView*
EView::PreviousSibling() const
{
	return fPrevSibling;
}


EWindow*
EView::Window() const
{
	return e_cast_as(Looper(), EWindow);
}


EView*
EView::Parent() const
{
	return fParent;
}


EView*
EView::Ancestor() const
{
	if(fParent == NULL) return (EView*)this;

	EView *ancestor = fParent;
	while(ancestor->fParent != NULL) ancestor = ancestor->fParent;

	return ancestor;
}


ERect
EView::Bounds() const
{
	return ConvertFromParent(fFrame);
}


EPoint
EView::LeftTop() const
{
	return Bounds().LeftTop();
}


ERect
EView::Frame() const
{
	return fFrame;
}


ERect
EView::VisibleBounds() const
{
	if(IsHidden()) return ERect();
	return ConvertFromParent(fVisibleRegion.Frame());
}


ERect
EView::VisibleFrame() const
{
	if(IsHidden()) return ERect();
	return fVisibleRegion.Frame();
}


ERegion
EView::VisibleBoundsRegion() const
{
	if(IsHidden()) return ERegion();
	return ConvertFromParent(fVisibleRegion);
}


ERegion
EView::VisibleFrameRegion() const
{
	if(IsHidden()) return ERegion();
	return fVisibleRegion;
}


bool
EView::IsVisible() const
{
	if(IsHidden()) return false;
	return(fVisibleRegion.CountRects() > 0);
}


void
EView::MouseDown(EPoint where)
{
}


void
EView::MouseUp(EPoint where)
{
}


void
EView::MouseMoved(EPoint where, euint32 code, const EMessage *a_message)
{
}


void
EView::_WindowActivated(bool state)
{
	WindowActivated(state);

	EView *view;
	EList viewsList(fViewsList);
	for(eint32 i = 0; i < viewsList.CountItems(); i++)
	{
		if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Parent() != this) continue;
		view->_WindowActivated(state);
	}
}


void
EView::WindowActivated(bool state)
{
}


EView*
EView::FindView(const char *name) const
{
	EString srcStr(name);

	EView *view;
	for(eint32 i = 0; i < fViewsList.CountItems(); i++)
	{
		if((view = (EView*)fViewsList.ItemAt(i)) == NULL) continue;

		EString destStr(view->Name());

		if(srcStr == destStr) return view;

		EView *cView = view->FindView(name);
		if(cView) return cView;
	}

	return NULL;
}


void
EView::KeyDown(const char *bytes, eint32 numBytes)
{
}


void
EView::KeyUp(const char *bytes, eint32 numBytes)
{
}


void
EView::Pulse()
{
}


void
EView::FrameMoved(EPoint new_position)
{
}


void
EView::FrameResized(float new_width, float new_height)
{
}


void
EView::TargetedByScrollView(EScrollView *scroll_view)
{
}


void
EView::Draw(ERect updateRect)
{
}


void
EView::DrawAfterChildren(ERect updateRect)
{
}


void
EView::ConvertToScreen(EPoint* pt) const
{
	if(!pt) return;

	EWindow *win = Window();
	if(!win) return;

	*pt -= fLocalOrigin;
	*pt += fOrigin;

	win->ConvertToScreen(pt);
}


EPoint
EView::ConvertToScreen(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertToScreen(&pt1);
	return pt1;
}


void
EView::ConvertFromScreen(EPoint* pt) const
{
	if(!pt) return;

	EWindow *win = Window();
	if(!win) return;

	*pt -= fOrigin;
	*pt += fLocalOrigin;

	win->ConvertFromScreen(pt);
}


EPoint
EView::ConvertFromScreen(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertFromScreen(&pt1);
	return pt1;
}


void
EView::ConvertToScreen(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertToScreen(r->LeftTop());
	r->OffsetTo(pt);
}


ERect
EView::ConvertToScreen(ERect r) const
{
	ERect rect = r;
	ConvertToScreen(&rect);
	return rect;
}


void
EView::ConvertFromScreen(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertFromScreen(E_ORIGIN);
	r->OffsetBy(pt);
}


ERect
EView::ConvertFromScreen(ERect r) const
{
	ERect rect = r;
	ConvertFromScreen(&rect);
	return rect;
}


void
EView::ConvertToScreen(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertToScreen(region->Frame().LeftTop());
	region->OffsetBy(pt - region->Frame().LeftTop());
}


ERegion
EView::ConvertToScreen(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertToScreen(&aRegion);
	return aRegion;
}


void
EView::ConvertFromScreen(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertFromScreen(E_ORIGIN);
	region->OffsetBy(pt);
}


ERegion
EView::ConvertFromScreen(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertFromScreen(&aRegion);
	return aRegion;
}


void
EView::ConvertToParent(EPoint *pt) const
{
	if(!pt) return;

	*pt -= fLocalOrigin;
	*pt += fFrame.LeftTop();
}


EPoint
EView::ConvertToParent(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertToParent(&pt1);
	return pt1;
}


void
EView::ConvertFromParent(EPoint *pt) const
{
	if(!pt) return;

	*pt -= fFrame.LeftTop();
	*pt += fLocalOrigin;
}


EPoint
EView::ConvertFromParent(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertFromParent(&pt1);
	return pt1;
}


void
EView::ConvertToParent(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertToParent(r->LeftTop());
	r->OffsetTo(pt);
}


ERect
EView::ConvertToParent(ERect r) const
{
	ERect rect = r;
	ConvertToParent(&rect);
	return rect;
}


void
EView::ConvertFromParent(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertFromParent(E_ORIGIN);
	r->OffsetBy(pt);
}


ERect
EView::ConvertFromParent(ERect r) const
{
	ERect rect = r;
	ConvertFromParent(&rect);
	return rect;
}


void
EView::ConvertToParent(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertToParent(region->Frame().LeftTop());
	region->OffsetBy(pt - region->Frame().LeftTop());
}


ERegion
EView::ConvertToParent(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertToParent(&aRegion);
	return aRegion;
}


void
EView::ConvertFromParent(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertFromParent(E_ORIGIN);
	region->OffsetBy(pt);
}


ERegion
EView::ConvertFromParent(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertFromParent(&aRegion);
	return aRegion;
}


void
EView::ConvertToWindow(EPoint* pt) const
{
	if(!pt) return;

	*pt -= fLocalOrigin;
	*pt += fOrigin;
}


EPoint
EView::ConvertToWindow(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertToWindow(&pt1);
	return pt1;
}


void
EView::ConvertFromWindow(EPoint* pt) const
{
	if(!pt) return;

	*pt -= fOrigin;
	*pt += fLocalOrigin;
}


EPoint
EView::ConvertFromWindow(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertFromWindow(&pt1);
	return pt1;
}


void
EView::ConvertToWindow(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertToWindow(r->LeftTop());
	r->OffsetTo(pt);
}


ERect
EView::ConvertToWindow(ERect r) const
{
	ERect rect = r;
	ConvertToWindow(&rect);
	return rect;
}


void
EView::ConvertFromWindow(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertFromWindow(E_ORIGIN);
	r->OffsetBy(pt);
}


ERect
EView::ConvertFromWindow(ERect r) const
{
	ERect rect = r;
	ConvertFromWindow(&rect);
	return rect;
}


void
EView::ConvertToWindow(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertToWindow(region->Frame().LeftTop());
	region->OffsetBy(pt - region->Frame().LeftTop());
}


ERegion
EView::ConvertToWindow(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertToWindow(&aRegion);
	return aRegion;
}


void
EView::ConvertFromWindow(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertFromWindow(E_ORIGIN);
	region->OffsetBy(pt);
}


ERegion
EView::ConvertFromWindow(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertFromWindow(&aRegion);
	return aRegion;
}


void
EView::SetFlags(euint32 flags)
{
	EWindow *win = Window();
	if(win && !fHidden)
	{
		euint32 oldPulseNeeded = fViewFlags & E_PULSE_NEEDED;
		euint32 newPulseNeeded = flags & E_PULSE_NEEDED;

		if(oldPulseNeeded != newPulseNeeded)
		{
			if(newPulseNeeded == E_PULSE_NEEDED)
				win->fNeededToPulseViews.AddItem(this);
			else
				win->fNeededToPulseViews.RemoveItem(this);

			if(win->fPulseRunner)
				win->fPulseRunner->SetCount((win->fPulseRate > E_INT64_CONSTANT(0) &&
							     win->fNeededToPulseViews.CountItems() > 0 &&
							     win->IsHidden() == false) ? -1 : 0);
		}
	}

	fViewFlags = flags;
}


euint32
EView::Flags() const
{
	return fViewFlags;
}


void
EView::SetResizingMode(euint32 mode)
{
	fViewResizingMode = mode;
}


euint32
EView::ResizingMode() const
{
	return fViewResizingMode;
}


void
EView::_SetFrame(ERect newFrame, bool parent_changed)
{
	bool doMoved = false, doResized = false;
	EWindow *win = Window();

	if(newFrame == fFrame)
	{
		if(parent_changed) _UpdateOriginAndVisibleRegion(true);
		return;
	}

	if(fFrame.LeftTop() != newFrame.LeftTop()) doMoved = true;
	if(fFrame.Width() != newFrame.Width() || fFrame.Height() != newFrame.Height()) doResized = true;

	ERect updateRect1;
	ERect updateRect2;

	updateRect1 = fFrame;

	if(doMoved)
	{
		fFrame.OffsetTo(newFrame.LeftTop());
		updateRect2 = fFrame;
	}

	ERect oldFrame = fFrame;

	if(doResized)
	{
		fFrame.right = fFrame.left + newFrame.Width();
		fFrame.bottom = fFrame.top + newFrame.Height();
		updateRect2 = fFrame;

		_UpdateOriginAndVisibleRegion(false);

		EView *view;
		for(eint32 i = 0; (view = (EView*)fViewsList.ItemAt(i)) != NULL; i++)
		{
			euint32 vMode = view->ResizingMode();
			ERect vFrame = view->fFrame;

			if(vMode == E_FOLLOW_NONE || vMode == (E_FOLLOW_LEFT | E_FOLLOW_TOP))
			{
				view->_UpdateOriginAndVisibleRegion(true);
				continue;
			}

			float width_offset = fFrame.Width() - oldFrame.Width();
			float height_offset = fFrame.Height() - oldFrame.Height();

			if((vMode & E_FOLLOW_H_CENTER) && !((vMode & E_FOLLOW_LEFT) && (vMode & E_FOLLOW_RIGHT)))
			{
				float newCenter = fFrame.Center().x - (oldFrame.Center().x - vFrame.Center().x);
				if(vMode & E_FOLLOW_RIGHT)
				{
					vFrame.right += width_offset;
					vFrame.left = vFrame.right - 2.f * (vFrame.right - newCenter);
				}
				else if(vMode & E_FOLLOW_LEFT)
				{
					vFrame.right = vFrame.left + 2.f * (newCenter - vFrame.left);
				}
				else
				{
					float vWidth = vFrame.Width();
					vFrame.left = newCenter - vWidth / 2.f;
					vFrame.right = newCenter + vWidth / 2.f;
				}
			}
			else if(vMode & E_FOLLOW_RIGHT)
			{
				vFrame.right += width_offset;
				if(!(vMode & E_FOLLOW_LEFT))
					vFrame.left += width_offset;
			}

			if((vMode & E_FOLLOW_V_CENTER) && !((vMode & E_FOLLOW_TOP) && (vMode & E_FOLLOW_BOTTOM)))
			{
				float newCenter = fFrame.Center().y - (oldFrame.Center().y - vFrame.Center().y);
				if(vMode & E_FOLLOW_TOP_BOTTOM)
				{
					vFrame.bottom += height_offset;
					vFrame.top = vFrame.bottom - 2.f * (vFrame.bottom - newCenter);
				}
				else if(vMode & E_FOLLOW_TOP)
				{
					vFrame.bottom = vFrame.top + 2.f * (newCenter - vFrame.top);
				}
				else
				{
					float vHeight = vFrame.Height();
					vFrame.top = newCenter - vHeight / 2.f;
					vFrame.bottom = newCenter + vHeight / 2.f;
				}
			}
			else if(vMode & E_FOLLOW_BOTTOM)
			{
				vFrame.bottom += height_offset;
				if(!(vMode & E_FOLLOW_TOP))
					vFrame.top += height_offset;
			}

			view->_SetFrame(vFrame, true);
		}
	}
	else
	{
		_UpdateOriginAndVisibleRegion(true);
	}

	if(!IsHidden())
	{
		ConvertFromParent(&updateRect1);
		ConvertFromParent(&updateRect2);
		ConvertToWindow(&updateRect1);
		ConvertToWindow(&updateRect2);

		if(updateRect1.IsValid() || updateRect2.IsValid())
		{
			if(updateRect1.IsValid())
				win->fExposeRect = win->fExposeRect.IsValid() ? win->fExposeRect | updateRect1 : updateRect1;

			if(updateRect2.IsValid())
				win->fExposeRect = win->fExposeRect.IsValid() ? win->fExposeRect | updateRect2 : updateRect2;

			if(win->InUpdate() == false)
			{
				EMessage aMsg(_UPDATE_IF_NEEDED_);
				aMsg.AddInt64("when", e_real_time_clock_usecs());
				win->PostMessage(&aMsg, win);
			}
		}
	}

	if(fViewFlags & E_FRAME_EVENTS)
	{
		EMessage aMsg(E_VIEW_MOVED);
		aMsg.AddInt64("when", e_real_time_clock_usecs());
		aMsg.AddFloat("width", fFrame.Width());
		aMsg.AddFloat("height", fFrame.Height());
		aMsg.AddPoint("where", fFrame.LeftTop());

		if(doMoved)
		{
			if(win)
				win->PostMessage(&aMsg, this);
			else
				MessageReceived(&aMsg);
		}

		if(doResized)
		{
			aMsg.what = E_VIEW_RESIZED;
			if(win)
				win->PostMessage(&aMsg, this);
			else
				MessageReceived(&aMsg);
		}
	}
}


void
EView::MoveBy(float dh, float dv)
{
	MoveTo(fFrame.LeftTop() + EPoint(dh, dv));
}


void
EView::MoveTo(EPoint where)
{
	if(where == fFrame.LeftTop()) return;

	ERect rect = fFrame.OffsetToCopy(where);
	_SetFrame(rect, false);

	if(IsHidden()) return;
	EView *nextSibling = fNextSibling;
	while(nextSibling != NULL)
	{
		if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
		nextSibling = nextSibling->fNextSibling;
	}
}


void
EView::MoveTo(float x, float y)
{
	MoveTo(EPoint(x, y));
}


void
EView::ResizeBy(float dh, float dv)
{
	ResizeTo(fFrame.Width() + dh, fFrame.Height() + dv);
}


void
EView::ResizeTo(float width, float height)
{
	if(fFrame.Width() == width && fFrame.Height() == height) return;

	ERect rect = fFrame;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;
	_SetFrame(rect, false);

	if(IsHidden()) return;
	EView *nextSibling = fNextSibling;
	while(nextSibling != NULL)
	{
		if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
		nextSibling = nextSibling->fNextSibling;
	}
}


void
EView::AttachToWindow()
{
	_UpdateOriginAndVisibleRegion(false);

	EWindow *win = Window();
	if(!win) return;

	if(fEventMask & E_POINTER_EVENTS)
		win->fMouseInterestedViews.AddItem(this);

	if(fEventMask & E_KEYBOARD_EVENTS)
		win->fKeyboardInterestedViews.AddItem(this);

	Invalidate();

	if((Flags() & E_PULSE_NEEDED) && !fHidden)
	{
		win->fNeededToPulseViews.AddItem(this);
		if(win->fPulseRunner)
			win->fPulseRunner->SetCount((win->fPulseRate > E_INT64_CONSTANT(0) &&
						     win->fNeededToPulseViews.CountItems() > 0 &&
						     win->IsHidden() == false) ? -1 : 0);
	}
}


void
EView::DetachFromWindow()
{
	EWindow *win = Window();
	if(win)
	{
		if(fEventStored)
		{
			if(fMouseGrabbed)
			{
				win->_UngrabMouse();
				fMouseGrabbed = false;
			}

			if(fKeyboardGrabbed)
			{
				win->_UngrabKeyboard();
				fKeyboardGrabbed = false;
			}

			_SetEventMask(fEventMaskStored, fEventOptionsStored);

			fEventStored = false;
		}

		if(fMouseInside)
			win->fMouseInsideViews.RemoveItem(this);

		if(fEventMask & E_POINTER_EVENTS)
			win->fMouseInterestedViews.RemoveItem(this);

		if(fEventMask & E_KEYBOARD_EVENTS)
			win->fKeyboardInterestedViews.RemoveItem(this);
	}

	EView::MakeFocus(false);

	Invalidate();

	fOrigin = EPoint(0, 0);
	fVisibleRegion.MakeEmpty();

	if(win != NULL && (Flags() & E_PULSE_NEEDED) && !fHidden)
	{
		win->fNeededToPulseViews.RemoveItem(this);
		if(win->fPulseRunner)
			win->fPulseRunner->SetCount((win->fPulseRate > E_INT64_CONSTANT(0) &&
						     win->fNeededToPulseViews.CountItems() > 0 &&
						     win->IsHidden() == false) ? -1 : 0);
	}
}


void
EView::MovePenTo(EPoint pt)
{
	fPen = pt;
}


void
EView::MovePenTo(float x, float y)
{
	fPen.x = x;
	fPen.y = y;
}


void
EView::MovePenBy(float dx, float dy)
{
	fPen.x += dx;
	fPen.y += dy;
}


EPoint
EView::PenLocation() const
{
	return fPen;
}


void
EView::SetPenSize(float size)
{
	if(size < 0) return;
	if(fPenSize != size)
	{
		fPenSize = size;
		if(IsPrinting()) size /= UnitsPerPixel();
		fDC->SetPenSize((euint32)max_c(etk_round((double)size), 0));
	}
}


float
EView::PenSize() const
{
	return fPenSize;
}


void
EView::SetViewColor(e_rgb_color c)
{
	fIsCustomViewColor = true;

	if(fViewColor != c)
	{
		fViewColor.set_to(c.red, c.green, c.blue, c.alpha);
		Invalidate();
	}
}


void
EView::SetViewColor(euint8 r, euint8 g, euint8 b, euint8 a)
{
	e_rgb_color c;
	c.set_to(r, g, b, a);
	SetViewColor(c);
}


e_rgb_color
EView::ViewColor() const
{
	if(fIsCustomViewColor)
		return fViewColor;
	else
	{
		if(fParent)
			return fParent->ViewColor();
		else
		{
			EWindow *win = Window();
			if(win)
			{
				return win->BackgroundColor();
			}
			else
			{
				return e_ui_color(E_PANEL_BACKGROUND_COLOR);
			}
		}
	}
}


void
EView::SetHighColor(e_rgb_color c)
{
	fIsCustomHighColor = true;

	if(fHighColor != c)
	{
		// TODO
		fHighColor.set_to(c.red, c.green, c.blue, c.alpha);
	}
}


void
EView::SetHighColor(euint8 r, euint8 g, euint8 b, euint8 a)
{
	e_rgb_color c;
	c.set_to(r, g, b, a);
	SetHighColor(c);
}


e_rgb_color
EView::HighColor() const
{
	if(fIsCustomHighColor)
		return fHighColor;
	else
	{
		e_rgb_color c = ViewColor();
		c.red = 255 - c.red;
		c.green = 255 - c.green;
		c.blue = 255 - c.blue;
		return c;
	}
}


void
EView::SetLowColor(e_rgb_color c)
{
	fIsCustomLowColor = true;

	if(fLowColor != c)
	{
		// TODO
		fLowColor.set_to(c.red, c.green, c.blue, c.alpha);
	}
}


void
EView::SetLowColor(euint8 r, euint8 g, euint8 b, euint8 a)
{
	e_rgb_color c;
	c.set_to(r, g, b, a);
	SetLowColor(c);
}


e_rgb_color
EView::LowColor() const
{
	if(fIsCustomLowColor)
		return fLowColor;
	else
		return ViewColor();
}


void
EView::StrokePoint(EPoint _pt, e_pattern p)
{
	if(IsVisible() == false) return;

	EWindow *win = Window();
	if(!win) return;

	EPoint pt = ConvertToWindow(_pt);

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	EPoint _pt_ = EPoint(PenSize() / 2.f, PenSize() / 2.f);
	ERect updateRect(pt - _pt_, pt + _pt_);

	if(IsPrinting())
	{
		pt.x /= UnitsPerPixel();
		pt.y /= UnitsPerPixel();
	}

	pt.Floor();
	if(win->fPixmap->StrokePoint(fDC, (eint32)pt.x, (eint32)pt.y) == E_OK) win->_Update(updateRect, false);

	MovePenTo(_pt);
}


void
EView::StrokePoints(const EPoint *_pts, eint32 count, const euint8 *alpha, e_pattern p)
{
	if(IsVisible() == false || _pts == NULL || count <= 0) return;

	EWindow *win = Window();
	if(!win) return;

	eint32 *pts = new eint32[2 * count];
	if(!pts) return;
	eint32 *tmp = pts;

	EPoint pmin, pmax;

	for(eint32 i = 0; i < count; i++)
	{
		EPoint pt = ConvertToWindow(_pts[i]);

		if(i == 0)
			pmin = pmax = pt;
		else
		{
			pmin.x = min_c(pmin.x, pt.x);
			pmin.y = min_c(pmin.y, pt.y);
			pmax.x = max_c(pmax.x, pt.x);
			pmax.y = max_c(pmax.y, pt.y);
		}

		if(IsPrinting())
		{
			pt.x /= UnitsPerPixel();
			pt.y /= UnitsPerPixel();
		}

		pt.Floor();

		*tmp++ = (eint32)pt.x;
		*tmp++ = (eint32)pt.y;
	}

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	e_status_t status = E_ERROR;
	if(alpha) status = win->fPixmap->StrokePoints_Alphas(fDC, pts, alpha, count);
	else status = win->fPixmap->StrokePoints(fDC, pts, count);

	delete[] pts;

	if(status == E_OK)
	{
		ERect updateRect(ERect(pmin, pmax).InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));
		win->_Update(updateRect, false);
	}

	MovePenTo(_pts[count - 1]);
}


void
EView::StrokeLine(EPoint pt, e_pattern p)
{
	StrokeLine(PenLocation(), pt, p);
}


void
EView::StrokeLine(EPoint _pt0, EPoint _pt1, e_pattern p)
{
	if(IsVisible() == false) return;

	EWindow *win = Window();
	if(!win) return;

	EPoint start = ConvertToWindow(_pt0);
	EPoint end = ConvertToWindow(_pt1);

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	ERect updateRect;
	updateRect.left = min_c(start.x, end.x) - PenSize() / 2.f;
	updateRect.top = min_c(start.y, end.y) - PenSize() / 2.f;
	updateRect.right = max_c(start.x, end.x) + PenSize() / 2.f;
	updateRect.bottom = max_c(start.y, end.y) + PenSize() / 2.f;

	if(IsPrinting())
	{
		start.x /= UnitsPerPixel();
		start.y /= UnitsPerPixel();
		end.x /= UnitsPerPixel();
		end.y /= UnitsPerPixel();
	}

	start.Floor();
	end.Floor();
	if(win->fPixmap->StrokeLine(fDC,
				    (eint32)start.x, (eint32)start.y,
				    (eint32)end.x, (eint32)end.y) == E_OK) win->_Update(updateRect, false);

	MovePenTo(_pt1);
}


void
EView::StrokeRect(ERect rect, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false) return;

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->StrokeRect(fDC,
				    (eint32)r.left, (eint32)r.top,
				    (euint32)r.Width(), (euint32)r.Height()) == E_OK) win->_Update(updateRect, false);
}


void
EView::StrokePolygon(const EPolygon *aPolygon, bool closed, e_pattern p)
{
	if(IsVisible() == false || !aPolygon || aPolygon->CountPoints() <= 0) return;

	EWindow *win = Window();
	if(!win) return;

	eint32 *pts = new eint32[2 * aPolygon->CountPoints()];
	if(!pts) return;
	eint32 *tmp = pts;
	const EPoint *polyPts = aPolygon->Points();

	for(eint32 i = 0; i < aPolygon->CountPoints(); i++)
	{
		EPoint pt = ConvertToWindow(*polyPts++);

		if(IsPrinting())
		{
			pt.x /= UnitsPerPixel();
			pt.y /= UnitsPerPixel();
		}

		pt.Floor();

		*tmp++ = (eint32)pt.x;
		*tmp++ = (eint32)pt.y;
	}

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(win->fPixmap->StrokePolygon(fDC, pts, aPolygon->CountPoints(), closed) == E_OK)
	{
		ERect r = ConvertToWindow(aPolygon->Frame());
		ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));
		win->_Update(updateRect, false);
	}

	delete[] pts;
}


void
EView::StrokePolygon(const EPoint *ptArray, eint32 numPts, bool closed, e_pattern p)
{
	EPolygon aPolygon(ptArray, numPts);
	if(aPolygon.CountPoints() <= 0) return;
	StrokePolygon(&aPolygon, closed, p);
}


void
EView::FillPolygon(const EPolygon *aPolygon, e_pattern p)
{
	if(IsVisible() == false || !aPolygon || aPolygon->CountPoints() <= 0) return;

	EWindow *win = Window();
	if(!win) return;

	eint32 *pts = new eint32[2 * aPolygon->CountPoints()];
	if(!pts) return;
	eint32 *tmp = pts;
	const EPoint *polyPts = aPolygon->Points();

	for(eint32 i = 0; i < aPolygon->CountPoints(); i++)
	{
		EPoint pt = ConvertToWindow(*polyPts++);

		if(IsPrinting())
		{
			pt.x /= UnitsPerPixel();
			pt.y /= UnitsPerPixel();
		}

		pt.Floor();

		*tmp++ = (eint32)pt.x;
		*tmp++ = (eint32)pt.y;
	}

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(win->fPixmap->FillPolygon(fDC, pts, aPolygon->CountPoints()) == E_OK)
	{
		ERect r = ConvertToWindow(aPolygon->Frame());
		ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));
		win->_Update(updateRect, false);
	}

	delete[] pts;
}


void
EView::FillPolygon(const EPoint *ptArray, eint32 numPts, e_pattern p)
{
	EPolygon aPolygon(ptArray, numPts);
	if(aPolygon.CountPoints() <= 0) return;
	FillPolygon(&aPolygon, p);
}


void
EView::StrokeTriangle(EPoint pt1, EPoint pt2, EPoint pt3, e_pattern p)
{
	EPoint pts[3] = {pt1, pt2, pt3};
	StrokePolygon(pts, 3, true, p);
}


void
EView::FillTriangle(EPoint pt1, EPoint pt2, EPoint pt3, e_pattern p)
{
	EPoint pts[3] = {pt1, pt2, pt3};
	FillPolygon(pts, 3, p);
}


void
EView::StrokeRects(const ERect *rs, eint32 count, e_pattern p)
{
	if(IsVisible() == false || rs == NULL || count <= 0) return;

	EWindow *win = Window();
	if(!win) return;

	eint32 *rects = new eint32[4 * count];
	if(!rects) return;

	ERect updateRect;
	eint32 _count_ = 0;

	eint32 *tRects = rects;
	for(eint32 i = 0; i < count; i++)
	{
		ERect r = *rs++;
		ConvertToWindow(&r);
		if(!r.IsValid()) continue;
		if(!updateRect.IsValid())
			updateRect = r;
		else
			updateRect |= r;

		if(IsPrinting())
		{
			r.left /= UnitsPerPixel();
			r.top /= UnitsPerPixel();
			r.right /= UnitsPerPixel();
			r.bottom /= UnitsPerPixel();
		}

		r.Floor();
		*tRects++ = (eint32)r.left; *tRects++ = (eint32)r.top; *tRects++ = (eint32)r.Width(); *tRects++ = (eint32)r.Height();
		_count_++;
	}

	if(_count_ > 0)
	{
		fDC->SetHighColor(HighColor());
		fDC->SetLowColor(LowColor());
		fDC->SetPattern(p);

		if(win->fPixmap->StrokeRects(fDC, rects, _count_) == E_OK)
		{
			updateRect.InsetBy(PenSize() / -2.f, PenSize() / -2.f);
			win->_Update(updateRect, false);
		}
	}

	delete[] rects;
}


void
EView::FillRect(ERect rect, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false) return;

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->FillRect(fDC,
				  (eint32)r.left, (eint32)r.top,
				  (euint32)r.Width(), (euint32)r.Height()) == E_OK) win->_Update(updateRect, false);
}


void
EView::FillRects(const ERect *rs, eint32 count, e_pattern p)
{
	if(IsVisible() == false || rs == NULL || count <= 0) return;

	EWindow *win = Window();
	if(!win) return;

	eint32 *rects = new eint32[4 * count];
	if(!rects) return;

	ERect updateRect;
	eint32 _count_ = 0;

	eint32 *tRects = rects;
	for(eint32 i = 0; i < count; i++)
	{
		ERect r = *rs++;
		ConvertToWindow(&r);
		if(!r.IsValid()) continue;
		if(!updateRect.IsValid())
			updateRect = r;
		else
			updateRect |= r;

		if(IsPrinting())
		{
			r.left /= UnitsPerPixel();
			r.top /= UnitsPerPixel();
			r.right /= UnitsPerPixel();
			r.bottom /= UnitsPerPixel();
		}

		r.Floor();
		*tRects++ = (eint32)r.left; *tRects++ = (eint32)r.top; *tRects++ = (eint32)r.Width(); *tRects++ = (eint32)r.Height();
		_count_++;
	}

	if(_count_ > 0)
	{
		fDC->SetHighColor(HighColor());
		fDC->SetLowColor(LowColor());
		fDC->SetPattern(p);

		if(win->fPixmap->FillRects(fDC, rects, _count_) == E_OK)
		{
			updateRect.InsetBy(PenSize() / -2.f, PenSize() / -2.f);
			win->_Update(updateRect, false);
		}
	}

	delete[] rects;
}


void
EView::FillRegion(const ERegion *region, e_pattern p)
{
	if(IsVisible() == false || region == NULL) return;

	EWindow *win = Window();
	if(!win) return;

	ERect updateRect = region->Frame().InsetByCopy(PenSize() / -2.f, PenSize() / -2.f);

	ERegion aRegion(*region);
	ConvertToWindow(&aRegion);
	if(aRegion.CountRects() <= 0) return;

	if(IsPrinting()) aRegion.Scale(1.f / UnitsPerPixel());

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	e_status_t status = win->fPixmap->FillRegion(fDC, aRegion);
	if(status == E_OK) win->_Update(updateRect, false);
}


void
EView::StrokeRoundRect(ERect rect, float xRadius, float yRadius, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false || xRadius < 0 || yRadius < 0) return;
	if(rect.Width() == 0 || rect.Height() == 0 || (xRadius == 0 && yRadius == 0)) return StrokeRect(rect, p);

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
		xRadius /= UnitsPerPixel();
		yRadius /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->StrokeRoundRect(fDC,
					 (eint32)r.left, (eint32)r.top,
					 (euint32)r.Width(), (euint32)r.Height(),
					 (euint32)etk_round((double)xRadius),
					 (euint32)etk_round((double)yRadius)) == E_OK) win->_Update(updateRect, false);
}


void
EView::FillRoundRect(ERect rect, float xRadius, float yRadius, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false || xRadius < 0 || yRadius < 0) return;
	if(rect.Width() == 0 || rect.Height() == 0 || (xRadius == 0 && yRadius == 0)) return FillRect(rect, p);

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
		xRadius /= UnitsPerPixel();
		yRadius /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->FillRoundRect(fDC,
				       (eint32)r.left, (eint32)r.top,
				       (euint32)r.Width(), (euint32)r.Height(),
				       (euint32)etk_round((double)xRadius),
				       (euint32)etk_round((double)yRadius)) == E_OK) win->_Update(updateRect, false);
}


void
EView::StrokeArc(EPoint center, float xRadius, float yRadius, float start_angle, float arc_angle, e_pattern p)
{
	if(IsVisible() == false || xRadius <= 0 || yRadius <= 0) return;

	ERect r;
	r.left = center.x - xRadius;
	r.top = center.y - yRadius;
	r.right = center.x + xRadius;
	r.bottom = center.y + yRadius;

	StrokeArc(r, start_angle, arc_angle, p);
}


void
EView::StrokeArc(ERect rect, float start_angle, float arc_angle, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false) return;

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->StrokeArc(fDC,
				   (eint32)r.left, (eint32)r.top,
				   (euint32)r.Width(), (euint32)r.Height(),
				   start_angle, start_angle + arc_angle) == E_OK) win->_Update(updateRect, false);
}


void
EView::FillArc(EPoint center, float xRadius, float yRadius, float start_angle, float arc_angle, e_pattern p)
{
	if(IsVisible() == false || xRadius <= 0 || yRadius <= 0) return;

	ERect r;
	r.left = center.x - xRadius;
	r.top = center.y - yRadius;
	r.right = center.x + xRadius;
	r.bottom = center.y + yRadius;

	FillArc(r, start_angle, arc_angle, p);
}


void
EView::FillArc(ERect rect, float start_angle, float arc_angle, e_pattern p)
{
	if(IsVisible() == false || rect.IsValid() == false) return;

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(rect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(p);

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
	}

	r.Floor();
	if(win->fPixmap->FillArc(fDC,
				 (eint32)r.left, (eint32)r.top,
				 (euint32)r.Width(), (euint32)r.Height(),
				 start_angle, start_angle + arc_angle) == E_OK) win->_Update(updateRect, false);
}


void
EView::StrokeEllipse(EPoint center, float xRadius, float yRadius, e_pattern p)
{
	return StrokeArc(center, xRadius, yRadius, 0, 360, p);
}


void
EView::StrokeEllipse(ERect rect, e_pattern p)
{
	return StrokeArc(rect, 0, 360, p);
}


void
EView::FillEllipse(EPoint center, float xRadius, float yRadius, e_pattern p)
{
	return FillArc(center, xRadius, yRadius, 0, 360, p);
}


void
EView::FillEllipse(ERect rect, e_pattern p)
{
	return FillArc(rect, 0, 360, p);
}


void
EView::PushState()
{
	if(Window() == NULL)
		ETK_ERROR("[INTERFACE]: %s --- View isn't attached to a window!", __PRETTY_FUNCTION__);

	_EViewState_ *attr = new _EViewState_;
	if(attr == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to alloc memory for view state!", __PRETTY_FUNCTION__);

	attr->DrawingMode = fDrawingMode;
	attr->PenLocation = fPen;
	attr->PenSize = fPenSize;
	attr->HighColor = fHighColor;
	attr->LowColor = fLowColor;
	attr->CustomHighColor = fIsCustomHighColor;
	attr->CustomLowColor = fIsCustomLowColor;
	attr->Font = fFont;
	attr->Clipping = fClipping;
	attr->HasClipping = fHasClipping;
	attr->SquarePointStyle = fSquarePointStyle;

	_EViewState_ *prev = (_EViewState_*)fStatesList;
	attr->prev = prev;
	fStatesList = (void*)attr;
}


void
EView::PopState()
{
	if(Window() == NULL)
		ETK_ERROR("[INTERFACE]: %s --- View isn't attached to a window!", __PRETTY_FUNCTION__);

	_EViewState_ *attr = (_EViewState_*)fStatesList;
	if(attr == NULL)
	{
		ETK_WARNING("[INTERFACE]: %s --- Maybe you don't call \"PushState\" and \"PopState\" at same times.", __PRETTY_FUNCTION__);
		return;
	}
	fStatesList = (void*)(attr->prev);

	// TODO
	fDrawingMode = attr->DrawingMode;
	fPen = attr->PenLocation;
	fPenSize = attr->PenSize;
	fHighColor = attr->HighColor;
	fLowColor = attr->LowColor;
	fIsCustomHighColor = attr->CustomHighColor;
	fIsCustomLowColor = attr->CustomLowColor;
	fFont = attr->Font;
	fClipping = attr->Clipping;
	fHasClipping = attr->HasClipping;
	fSquarePointStyle = attr->SquarePointStyle;

	fDC->SetDrawingMode(fDrawingMode);
	fDC->SetSquarePointStyle(fSquarePointStyle);

	if(IsPrinting()) attr->PenSize /= UnitsPerPixel();
	fDC->SetPenSize((euint32)max_c(etk_round((double)attr->PenSize), 0));

	ERegion clipping(fVisibleRegion);
	ConvertFromParent(&clipping);
	if(fHasClipping) clipping &= fClipping;
	if(fClippingTemp.CountRects() > 0) clipping &= fClippingTemp;
	ConvertToWindow(&clipping);
	if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
	fDC->SetClipping(clipping);

	delete attr;
}


void
EView::_Invalidate(ERect invalRect, bool redraw, e_bigtime_t when)
{
	if(!IsVisible()) return;

	invalRect &= ConvertFromParent(fVisibleRegion.Frame());
	ConvertToWindow(&invalRect);
	if(invalRect.IsValid() == false) return;

	EWindow *win = Window();
	if(redraw)
		win->fExposeRect = win->fExposeRect.IsValid() ? win->fExposeRect | invalRect : invalRect;
	else
		win->fUpdateRect = win->fUpdateRect.IsValid() ? win->fUpdateRect | invalRect : invalRect;

	if(win->InUpdate() == false)
	{
		EMessage msg(_UPDATE_IF_NEEDED_);
		msg.AddInt64("when", when);
		win->PostMessage(&msg, win);
	}
}


void
EView::Invalidate(ERect invalRect, bool redraw)
{
	_Invalidate(invalRect, redraw, e_real_time_clock_usecs());
}


void
EView::Invalidate(bool redraw)
{
	Invalidate(Bounds(), redraw);
}


void
EView::_Expose(ERegion region, e_bigtime_t when)
{
	EWindow *win = Window();
	if(win == NULL || fFrame.IsValid() == false) return;
	if(when < fScrollTimestamp) {win->fBrokeOnExpose = true; return;}

	if(!(fViewFlags & E_UPDATE_WITH_REGION)) region.Set(region.Frame());

	ERegion clipping(fVisibleRegion);
	ConvertFromParent(&clipping);

	region &= clipping;
	if(region.CountRects() <= 0) return;

	fDC->SetHighColor(ViewColor());
	fDC->SetLowColor(ViewColor());
	fDC->SetPattern(E_SOLID_HIGH);

	fDC->SetPenSize(0);
	fDC->SetDrawingMode(E_OP_COPY);

	ERegion tmpClipping(region);
	ConvertToWindow(&tmpClipping);
	if(IsPrinting()) tmpClipping.Scale(1.f / UnitsPerPixel());
	if(!(fViewFlags & E_UPDATE_WITH_REGION))
		fDC->SetClipping(tmpClipping.Frame());
	else
		fDC->SetClipping(tmpClipping);

	ERect rect = ConvertToWindow(region.Frame());
	if(IsPrinting())
	{
		rect.left /= UnitsPerPixel();
		rect.top /= UnitsPerPixel();
		rect.right /= UnitsPerPixel();
		rect.bottom /= UnitsPerPixel();
	}
	rect.Floor();
	win->fPixmap->FillRect(fDC, (eint32)rect.left, (eint32)rect.top, (euint32)rect.Width(), (euint32)rect.Height());

	if(fHasClipping) clipping &= fClipping;
	if(!(fViewFlags & E_UPDATE_WITH_REGION)) fClippingTemp = region.Frame(); else fClippingTemp = region;
	clipping &= fClippingTemp;
	ConvertToWindow(&clipping);
	if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
	fDC->SetClipping(clipping);

	fDC->SetDrawingMode(fDrawingMode);

	float penSize = fPenSize;
	if(IsPrinting()) penSize /= UnitsPerPixel();
	fDC->SetPenSize((euint32)max_c(etk_round((double)penSize), 0));

	if(fViewFlags & E_WILL_DRAW)
	{
		if(fViewFlags & E_UPDATE_WITH_REGION)
			for(eint32 i = 0; i < region.CountRects(); i++) Draw(region.RectAt(i));
		else
			Draw(region.Frame());
	}

	bool doQuit = false;
	EView *child;
	for(eint32 i = 0; (child = (EView*)fViewsList.ItemAt(i)) != NULL; i++)
	{
		if(win->fBrokeOnExpose || win->_HasResizeMessage(true)) {doQuit = true; break;}
		if(region.Intersects(&(child->fVisibleRegion)) == false) continue;
		child->_Expose(child->ConvertFromParent(region), when);
	}

	if(!doQuit && (fViewFlags & E_DRAW_ON_CHILDREN))
	{
		if(fViewFlags & E_UPDATE_WITH_REGION)
			for(eint32 i = 0; i < region.CountRects(); i++) DrawAfterChildren(region.RectAt(i));
		else
			DrawAfterChildren(region.Frame());
	}

	fClippingTemp.MakeEmpty();

	clipping = fVisibleRegion;
	ConvertFromParent(&clipping);
	if(fHasClipping) clipping &= fClipping;
	ConvertToWindow(&clipping);
	if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
	fDC->SetClipping(clipping);
}


void
EView::SetDrawingMode(e_drawing_mode mode)
{
	if(fDrawingMode != mode)
	{
		fDrawingMode = mode;
		fDC->SetDrawingMode(fDrawingMode);
	}
}


e_drawing_mode
EView::DrawingMode() const
{
	return fDrawingMode;
}


void
EView::SetFont(const EFont *font, euint8 mask)
{
	if(!font) return;

	if(mask & E_FONT_ALL)
	{
		fFont = *font;
	}
	else
	{
		if(mask & E_FONT_FAMILY_AND_STYLE)
			fFont.SetFamilyAndStyle(font->FamilyAndStyle());

		if(mask & E_FONT_SIZE)
			fFont.SetSize(font->Size());

		if(mask & E_FONT_SHEAR)
			fFont.SetShear(font->Shear());

		if(mask & E_FONT_SPACING)
			fFont.SetSpacing(font->Spacing());
	}
}


void
EView::SetFont(const e_font_desc *fontDesc, euint8 mask)
{
	if(!fontDesc) return;
	EFont font(*fontDesc);
	SetFont(&font, mask);
}


void
EView::GetFont(EFont *font) const
{
	if(!font) return;
	*font = fFont;
}


void
EView::SetFontSize(float size)
{
	if(size <= 0) return;

	EFont font(fFont);
	font.SetSize(size);

	SetFont(&font, E_FONT_SIZE);
}


void
EView::GetFontHeight(e_font_height *height) const
{
	if(!height) return;

	fFont.GetHeight(height);
}


void
EView::ForceFontAliasing(bool enable)
{
	if(fForceFontAliasing != enable)
	{
		fForceFontAliasing = enable;
		// TODO
	}
}


void
EView::DrawString(const char *aString, eint32 length, float tabWidth)
{
	DrawString(aString, PenLocation(), length, tabWidth);
}


void
EView::DrawString(const char *aString, EPoint location, eint32 length, float tabWidth)
{
	if(aString == NULL || *aString == 0 || length == 0 || Window() == NULL) return;

	EFontEngine *engine = fFont.Engine();
	if(engine == NULL) return;

	float size = fFont.Size();
	float spacing = fFont.Spacing();
	float shear = fFont.Shear();
	bool bold = fFont.IsBoldStyle();

	engine->Lock();
	e_font_render_mode renderMode = engine->RenderMode();
	float spaceWidth = engine->StringWidth(" ", size, spacing, shear, bold, 1);
	engine->Unlock();

	if(!(renderMode & (E_FONT_RENDER_DIRECTLY | E_FONT_RENDER_PIXMAP))) return;

	if(tabWidth == 0)
	{
		if(renderMode & E_FONT_RENDER_DIRECTLY)
			DrawStringInDirectlyMode(aString, location, length);
		else // E_FONT_RENDER_PIXMAP
			DrawStringInPixmapMode(aString, location, length);
	}
	else
	{
		EString aStr(aString, length);
		float spacing_width = spacing * size;

		if(tabWidth < 0)
		{
			if(tabWidth < E_FONT_MIN_TAB_WIDTH) tabWidth = -E_FONT_MIN_TAB_WIDTH;
			else tabWidth = (float)(ceil((double)(-tabWidth)));
			tabWidth = (tabWidth * spaceWidth) + (tabWidth - 1.f) * spacing_width;
		}

		MovePenTo(location);

		for(eint32 aOffset = 0; aOffset < aStr.Length(); aOffset++)
		{
			eint32 oldOffset = aOffset, len;
			aOffset = aStr.FindFirst("\t", aOffset);

			len = (aOffset < 0 ? aStr.Length() : aOffset) - oldOffset;
			if(len > 0)
			{
				if(renderMode & E_FONT_RENDER_DIRECTLY)
					DrawStringInDirectlyMode(aStr.String() + oldOffset, PenLocation(), len);
				else // E_FONT_RENDER_PIXMAP
					DrawStringInPixmapMode(aStr.String() + oldOffset, PenLocation(), len);
			}

			if(aOffset < 0) break;

			MovePenBy((aOffset > 0 ? spacing_width : 0) + tabWidth + UnitsPerPixel(), 0);
		}
	}
}


void
EView::DrawStringInDirectlyMode(const char *aString, EPoint location, eint32 length)
{
	EWindow *win = Window();
	if(win == NULL || aString == NULL || *aString == 0 || length == 0 || !IsVisible()) return;

	EFontEngine *engine = fFont.Engine();
	if(engine == NULL) return;

	fDC->SetHighColor(HighColor());
	fDC->SetLowColor(LowColor());
	fDC->SetPattern(E_SOLID_HIGH);

	float size = fFont.Size();
	float spacing = fFont.Spacing();
	float shear = fFont.Shear();
	bool bold = fFont.IsBoldStyle();
	bool force_mono = fForceFontAliasing;

	EPoint oldLocation = PenLocation();
	MovePenTo(location);

	engine->Lock();
	engine->ForceFontAliasing(force_mono);
	float width = engine->StringWidth(aString, size, spacing, shear, bold, length);
	ERect updateRect = engine->RenderString(this, aString, size, spacing, shear, bold, length);
	engine->Unlock();

	if(updateRect.IsValid())
	{
		ConvertToWindow(&updateRect);
		win->_Update(updateRect, false);
	}

	location.x += width + UnitsPerPixel();
	MovePenTo(location);
}


void
EView::DrawStringInPixmapMode(const char *aString, EPoint location, eint32 length)
{
	EWindow *win = Window();
	if(win == NULL || aString == NULL || *aString == 0 || length == 0 || !IsVisible()) return;

	ERect rect = VisibleBounds();
	if(rect.IsValid() == false) return;

	EFontEngine *engine = fFont.Engine();
	if(engine == NULL) return;

	float size = fFont.Size();
	float spacing = fFont.Spacing();
	float shear = fFont.Shear();
	bool bold = fFont.IsBoldStyle();
	bool is_mono = fForceFontAliasing;

	eint32 w = 0, h = 0;

	engine->Lock();

	e_font_height fheight;
	engine->GetHeight(&fheight, size);

	EPoint startPoint = location;
	startPoint.y -= fheight.ascent + 1;

	engine->ForceFontAliasing(is_mono);

	float width = engine->StringWidth(aString, size, spacing, shear, bold, length);
	euint8 *bitmap = engine->RenderString(aString, &w, &h, &is_mono, size, spacing, shear, bold, length);

	engine->Unlock();

	rect &= ERect(startPoint, startPoint + EPoint((float)w, (float)h));
	if(!rect.IsValid() || startPoint.x > rect.right || startPoint.y > rect.bottom ||
	   startPoint.x + (float)w < rect.left || startPoint.y + (float)h < rect.top)
	{
		if(bitmap) delete[] bitmap;
		return;
	}

	eint32 _w_ = w;
	eint32 _h_ = h;
	w = min_c(w, (eint32)(ceil(rect.Width())));
	h = min_c(h, (eint32)(ceil(rect.Height())));

	if(!bitmap || w <= 0 || h <= 0)
	{
		if(bitmap) delete[] bitmap;
		return;
	}

	eint32 *pts = new eint32[2 * w * h];
	euint8 *alpha = is_mono ? NULL : new euint8[w * h];
	eint32 pointCount = 0;

	if(!pts)
	{
		if(alpha) delete[] alpha;
		delete[] bitmap;
		return;
	}

	eint32 i_start = max_c((eint32)(ceil(rect.top) - ceil(startPoint.y)), 0);
	eint32 j_start = max_c((eint32)(ceil(rect.left) - ceil(startPoint.x)), 0);

	ConvertToWindow(&startPoint);
	EPoint pmin, pmax;

	eint32 *tmp = pts;
	for(eint32 i = i_start; i < min_c(i_start + h, _h_); i++)
	{
		for(eint32 j = j_start; j < min_c(j_start + w, _w_); j++)
		{
			euint8 c = bitmap[i * _w_ + j];
			if(c != 0)
			{
				EPoint pt = startPoint + EPoint((float)j, (float)i);
				if(alpha) alpha[pointCount] = c;

				if(pointCount == 0)
					pmin = pmax = pt;
				else
				{
					pmin.x = min_c(pmin.x, pt.x);
					pmin.y = min_c(pmin.y, pt.y);
					pmax.x = max_c(pmax.x, pt.x);
					pmax.y = max_c(pmax.y, pt.y);
				}

				if(IsPrinting())
				{
					pt.x /= UnitsPerPixel();
					pt.y /= UnitsPerPixel();
				}

				pt.Floor();

				*tmp++ = (eint32)pt.x;
				*tmp++ = (eint32)pt.y;

				pointCount++;
			}
		}
	}

	e_status_t status = E_ERROR;

	if(pointCount > 0)
	{
		fDC->SetHighColor(HighColor());
		fDC->SetLowColor(LowColor());
		fDC->SetPattern(E_SOLID_HIGH);

		if(alpha && DrawingMode() == E_OP_ALPHA)
		{
			status = win->fPixmap->StrokePoints_Alphas(fDC, pts, alpha, pointCount);
		}
		else
		{
			if(alpha)
			{
				// here we just generate 16 gray
				const euint8 *_alpha_ = alpha;
				EList ptsLists[16];
				for(eint32 i = 0; i < pointCount; i++)
				{
					eint32 a = (eint32)(*_alpha_++);
					ptsLists[a / 16].AddItem((void*)&pts[i * 2]);
				}

				e_rgb_color ptColors[16];
				e_rgb_color lcolor = LowColor();
				for(eint32 i = 0; i < 16; i++)
				{
					ptColors[i].set_to(HighColor());
					lcolor.alpha = 255 - (euint8)i * 17;
					ptColors[i].mix(lcolor);
				}

				status = win->fPixmap->StrokePoints_Colors(fDC, ptsLists, 16, ptColors);
			}
			else
			{
				status = win->fPixmap->StrokePoints(fDC, pts, pointCount);
			}
		}
	}

	delete[] pts;
	if(alpha) delete[] alpha;

	delete[] bitmap;

	if(status == E_OK)
	{
		EPoint _pt_ = EPoint(PenSize() / 2.f, PenSize() / 2.f);
		ERect updateRect(pmin - _pt_, pmax + _pt_);
		win->_Update(updateRect, false);
	}

	location.x += width + UnitsPerPixel();
	MovePenTo(location);
}


void
EView::DrawString(const char *aString, eint32 length, EPoint location, float tabWidth)
{
	DrawString(aString, location, length, tabWidth);
}


void
EView::MakeFocus(bool focusState)
{
	EWindow *win = Window();
	if(!win) return;

	if(focusState)
	{
		if(win->fFocus != this)
		{
			if(win->fFocus) win->fFocus->MakeFocus(false);
			win->fFocus = this;
			win->SetPreferredHandler(this);
		}
	}
	else
	{
		if(win->fFocus == this)
		{
			win->fFocus = NULL;
			if(win->PreferredHandler() == this) win->SetPreferredHandler(NULL);
		}
	}
}


bool
EView::IsFocus() const
{
	EWindow *win = Window();
	if(!win) return false;

	return(win->fFocus == this);
}


e_status_t
EView::SetEventMask(euint32 mask, euint32 options)
{
	if(!fEventStored) return _SetEventMask(mask, options);

	fEventMaskStored = mask;
	fEventOptionsStored = options;

	return E_OK;
}


e_status_t
EView::_SetEventMask(euint32 mask, euint32 options)
{
	EWindow *win = Window();

	if(fEventMask != mask)
	{
		if(!win)
		{
			fEventMask = mask;
		}
		else if(fEventMask != 0)
		{
			euint32 mask1 = fEventMask & ~mask;
			euint32 mask2 = mask & ~fEventMask;

			if(mask2 & E_POINTER_EVENTS)
			{
				if(win->fMouseInterestedViews.AddItem(this) == false) return E_ERROR;
			}
			else if(mask2 & E_KEYBOARD_EVENTS)
			{
				if(win->fKeyboardInterestedViews.AddItem(this) == false) return E_ERROR;
			}

			if(mask1 & E_POINTER_EVENTS)
				win->fMouseInterestedViews.RemoveItem(this);
			else if(mask1 & E_KEYBOARD_EVENTS)
				win->fKeyboardInterestedViews.RemoveItem(this);

			fEventMask = mask;
		}
		else
		{
			if(mask & E_POINTER_EVENTS)
			{
				if(win->fMouseInterestedViews.AddItem(this) == false) return E_ERROR;
			}

			if(mask & E_KEYBOARD_EVENTS)
			{
				if(win->fKeyboardInterestedViews.AddItem(this) == false)
				{
					if(mask & E_POINTER_EVENTS)
						win->fMouseInterestedViews.RemoveItem(this);
					return E_ERROR;
				}
			}

			fEventMask = mask;
		}
	}

	// TODO: E_NO_POINTER_HISTORY and action implement
	fEventOptions = (options & E_NO_POINTER_HISTORY ? E_NO_POINTER_HISTORY : 0);

	return E_OK;
}


euint32
EView::EventMask() const
{
	return fEventMask;
}


e_status_t
EView::SetPrivateEventMask(euint32 mask, euint32 options)
{
	// TODO: suspend etc...
	if(mask == 0) return E_ERROR;

	EWindow *win = Window();
	if(!win) return E_ERROR;

	EMessage *msg = win->CurrentMessage();
	if(!msg) return E_ERROR;

	if(mask == E_KEYBOARD_EVENTS)
	{
		if(!(msg->what != E_KEY_DOWN || msg->what != E_UNMAPPED_KEY_DOWN)) return E_ERROR;
	}
	else if(mask == E_POINTER_EVENTS)
	{
		if(msg->what != E_MOUSE_DOWN) return E_ERROR;
	}
	else return E_ERROR;

	euint32 eventMask, eventNewMask;
	euint32 eventOptions, eventNewOptions;

	eventMask = eventNewMask = fEventMask;
	eventOptions = eventNewOptions = fEventOptions;

	if(!fEventStored)
	{
		fEventStored = true;

		fEventMaskStored = fEventMask;
		fEventOptionsStored = fEventOptions;

		eventNewMask = mask;
		eventNewOptions = options;
	}
	else
	{
		euint32 _mask_ = mask & ~eventNewMask;
		euint32 _options_ = options & ~eventNewOptions;

		if(_mask_ != 0)
		{
			if(_mask_ & E_POINTER_EVENTS) eventNewMask &= E_POINTER_EVENTS;
			if(_mask_ & E_KEYBOARD_EVENTS) eventNewMask &= E_KEYBOARD_EVENTS;
		}

		if(_options_ != 0)
		{
			if(_options_ & E_LOCK_WINDOW_FOCUS) eventNewOptions &= E_LOCK_WINDOW_FOCUS;
			if(_options_ & E_SUSPEND_VIEW_FOCUS) eventNewOptions &= E_SUSPEND_VIEW_FOCUS;
			if(_options_ & E_NO_POINTER_HISTORY) eventNewOptions &= E_NO_POINTER_HISTORY;
		}
	}

	if(eventMask != eventNewMask || eventOptions != eventNewOptions)
	{
		e_status_t status = _SetEventMask(eventNewMask, eventNewOptions);
		if(status != E_OK) return status;
		fEventOptions = eventNewOptions;
	}

	if(options & E_LOCK_WINDOW_FOCUS)
	{
		if(mask == E_POINTER_EVENTS)
		{
			if(!fMouseGrabbed)
			{
				if(win->_GrabMouse() == false)
				{
					_SetEventMask(eventMask, eventOptions);
					fEventOptions = eventOptions;
					return E_ERROR;
				}
				fMouseGrabbed = true;
			}
		}
		else if(mask == E_KEYBOARD_EVENTS)
		{
			if(!fKeyboardGrabbed)
			{
				if(win->_GrabKeyboard() == false)
				{
					_SetEventMask(eventMask, eventOptions);
					fEventOptions = eventOptions;
					return E_ERROR;
				}
				fKeyboardGrabbed = true;
			}
		}
	}

	return E_OK;
}


void
EView::GetPreferredSize(float *width, float *height)
{
	float w = 0, h = 0;
	EView *child;

	ERect rect = fFrame.OffsetToCopy(E_ORIGIN);

	for(eint32 i = 0; (child = ChildAt(i)) != NULL; i++)
	{
		float cW = 0, cH = 0;
		euint32 cMode = child->ResizingMode();

		if(child->IsHidden()) continue;
		child->GetPreferredSize(&cW, &cH);

		if((cMode & E_FOLLOW_LEFT) || cMode == E_FOLLOW_NONE) cW += child->fFrame.left;
		if(cMode & E_FOLLOW_RIGHT) cW += rect.right - child->fFrame.right;

		if((cMode & E_FOLLOW_TOP) || cMode == E_FOLLOW_NONE) cH += child->fFrame.top;
		if(cMode & E_FOLLOW_BOTTOM) cH += rect.bottom - child->fFrame.bottom;

		w = max_c(w, cW);
		h = max_c(h, cH);
	}

	if(width) *width = w;
	if(height) *height = h;
}


void
EView::ResizeToPreferred()
{
	if(!fFrame.IsValid()) return;

	float vWidth = -1, vHeight = -1;
	GetPreferredSize(&vWidth, &vHeight);
	if(vWidth < 0) vWidth = fFrame.Width();
	if(vHeight < 0) vHeight = fFrame.Height();
	if(vWidth == fFrame.Width() && vHeight == fFrame.Height()) return;

	ERect vFrame = fFrame;
	euint32 vMode = fViewResizingMode;

	if((vMode & E_FOLLOW_H_CENTER) && !((vMode & E_FOLLOW_LEFT) || (vMode & E_FOLLOW_RIGHT)))
	{
		float centerX = fFrame.Center().x;
		vFrame.left = centerX - vWidth / 2.f;
		vFrame.right = centerX + vWidth / 2.f;
	}
	else if(!((vMode & E_FOLLOW_LEFT) && (vMode & E_FOLLOW_RIGHT)))
	{
		if(vMode & E_FOLLOW_RIGHT)
			vFrame.left = vFrame.right - vWidth;
		else
			vFrame.right = vFrame.left + vWidth;
	}

	if((vMode & E_FOLLOW_V_CENTER) && !((vMode & E_FOLLOW_TOP) || (vMode & E_FOLLOW_BOTTOM)))
	{
		float centerY = fFrame.Center().y;
		vFrame.top = centerY - vHeight / 2.f;
		vFrame.bottom = centerY + vHeight / 2.f;
	}
	else if(!((vMode & E_FOLLOW_TOP) && (vMode & E_FOLLOW_BOTTOM)))
	{
		if(vMode & E_FOLLOW_BOTTOM)
			vFrame.top = vFrame.bottom - vHeight;
		else
			vFrame.bottom = vFrame.top + vHeight;
	}

	if(!vFrame.IsValid()) return;

	if(vFrame != fFrame)
	{
		if(vFrame.Width() != fFrame.Width() || vFrame.Height() != fFrame.Height())
			ResizeTo(vFrame.Width(), vFrame.Height());
		if(vFrame.LeftTop() != fFrame.LeftTop())
			MoveTo(vFrame.LeftTop());
	}
}


void
EView::GetClippingRegion(ERegion *clipping) const
{
	if(clipping == NULL) return;

	if(fHasClipping)
	{
		*clipping = fClipping;
	}
	else
	{
		*clipping = fVisibleRegion;
		ConvertFromParent(clipping);
	}
}


void
EView::ConstrainClippingRegion(const ERegion *_clipping)
{
	if(_clipping == NULL)
	{
		if(fHasClipping == false) return;

		fHasClipping = false;
		fClipping.MakeEmpty();

		ERegion clipping(fVisibleRegion);
		ConvertFromParent(&clipping);
		if(fClippingTemp.CountRects() > 0) clipping &= fClippingTemp;
		ConvertToWindow(&clipping);
		if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
		fDC->SetClipping(clipping);
	}
	else
	{
		fHasClipping = true;
		fClipping = *_clipping;

		ERegion clipping(fVisibleRegion);
		ConvertFromParent(&clipping);
		clipping &= fClipping;
		if(fClippingTemp.CountRects() > 0) clipping &= fClippingTemp;
		ConvertToWindow(&clipping);
		if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
		fDC->SetClipping(clipping);
	}
}


void
EView::ConstrainClippingRegion(ERect clipping)
{
	ERegion aRegion(clipping);
	ConstrainClippingRegion(&aRegion);
}


e_status_t
EView::GetMouse(EPoint *location, eint32 *buttons, bool checkMessageQueue)
{
	EWindow *win = Window();
	if(!win || !location || !buttons) return E_ERROR;

	if(checkMessageQueue)
	{
		win->MessageQueue()->Lock();
		EMessage *msg = win->MessageQueue()->FindMessage(E_MOUSE_MOVED, 0);
		if(msg == NULL) msg = win->MessageQueue()->FindMessage(E_MOUSE_UP, 0);
		if(msg != NULL)
		{
			EPoint where;
			eint32 btns;
			if(msg->FindPoint("where", &where) && msg->FindInt32("buttons", &btns))
			{
				*location = ConvertFromWindow(where);
				*buttons = btns;
				win->MessageQueue()->RemoveMessage(msg);
				win->MessageQueue()->Unlock();
				return E_OK;
			}
		}
		win->MessageQueue()->Unlock();
	}

	eint32 mx, my;
	if(win->fWindow->QueryMouse(&mx, &my, buttons) != E_OK) return E_ERROR;
	location->x = (float)mx;
	location->y = (float)my;
	ConvertFromWindow(location);

	return E_OK;
}


bool
EView::QueryCurrentMouse(bool pushed, eint32 buttons, bool btnsAlone, eint32 *clicks) const
{
	EWindow *win = Window();
	if(!win) return false;

	EMessage *msg = win->CurrentMessage();
	if(msg == NULL) return false;
	if(pushed && msg->what != E_MOUSE_DOWN) return false;
	if(!pushed && msg->what != E_MOUSE_UP) return false;

	eint32 btns;
	if(msg->FindInt32("buttons", &btns) == false) return false;

	if(clicks)
	{
		if(msg->FindInt32("clicks", clicks) == false) *clicks = 1;
	}

	return(btnsAlone ? (buttons == btns) : (btns >= buttons));
}


void
EView::_UpdateOriginAndVisibleRegion(bool deep)
{
	EWindow *win = Window();

	if(win)
	{
		if(fParent)
			fOrigin = fFrame.LeftTop() + fParent->fOrigin - fParent->fLocalOrigin;
		else
			fOrigin = fFrame.LeftTop();

		if(!IsHidden())
		{
			if(fParent)
			{
				fVisibleRegion = fParent->fVisibleRegion;
				if(!(!e_is_kind_of(fParent, EScrollView) || e_cast_as(fParent, EScrollView)->fTarget != this))
				{
					ERect targetRect = e_cast_as(fParent, EScrollView)->TargetValidFrame(false);
					fParent->ConvertToParent(&targetRect);
					fVisibleRegion &= targetRect;
				}
				fParent->ConvertFromParent(&fVisibleRegion);
			}
			else
			{
				fVisibleRegion.Set(win->Bounds());
			}

			EView *sibling = (fParent ? fParent->ChildAt(0) : win->ChildAt(0));
			while(fVisibleRegion.CountRects() > 0)
			{
				if(sibling == this)
				{
					fVisibleRegion &= fFrame;
					break;
				}
				if(sibling->fHidden == false)
				{
					float unitsPerPixel = (IsPrinting() ? sibling->UnitsPerPixel() : 1);
					fVisibleRegion.Exclude(sibling->fFrame.InsetByCopy(-unitsPerPixel, -unitsPerPixel));
				}
				sibling = sibling->fNextSibling;
			}
		}
		else
		{
			fVisibleRegion.MakeEmpty();
		}
	}
	else
	{
		fOrigin = EPoint(0, 0);
		fVisibleRegion.MakeEmpty();
	}

	ERegion clipping(fVisibleRegion);
	ConvertFromParent(&clipping);
	if(fHasClipping) clipping &= fClipping;
	if(fClippingTemp.CountRects() > 0) clipping &= fClippingTemp;
	ConvertToWindow(&clipping);
	if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());
	fDC->SetClipping(clipping);

	if(deep)
	{
		EView *child = ChildAt(0);
		while(child != NULL)
		{
			child->_UpdateOriginAndVisibleRegion(true);
			child = child->fNextSibling;
		}
	}
}


bool
EView::IsPrinting() const
{
	// TODO
	return false;
}


float
EView::UnitsPerPixel() const
{
	if(!IsPrinting()) return 1;

	// TODO
	return 1;
}


void
EView::SetEnabled(bool state)
{
	if(fEnabled != state)
	{
		fEnabled = state;
		if(Flags() & E_WILL_DRAW) Invalidate();
	}
}


bool
EView::IsEnabled() const
{
	return fEnabled;
}


void
EView::ScrollBy(float dh, float dv)
{
	ScrollTo(EPoint(dh, dv) + fLocalOrigin);
}


void
EView::ScrollTo(float x, float y)
{
	ScrollTo(EPoint(x, y));
}


void
EView::ScrollTo(EPoint where)
{
	if(where.x < 0) where.x = 0;
	if(where.y < 0) where.y = 0;

	if(fLocalOrigin != where)
	{
		fLocalOrigin = where;

		_UpdateOriginAndVisibleRegion(true);

		fScrollTimestamp = e_real_time_clock_usecs();
		_Invalidate(Bounds(), true, fScrollTimestamp);

		for(eint32 i = 0; i < fScrollBar.CountItems(); i++)
		{
			EScrollBar *scrollbar = (EScrollBar*)fScrollBar.ItemAt(i);
			scrollbar->_SetValue(scrollbar->Orientation() == E_HORIZONTAL ? fLocalOrigin.x : fLocalOrigin.y, false);
		}
	}
}


void
EView::SetSquarePointStyle(bool state)
{
	if(fSquarePointStyle != state)
	{
		if(fDC->SetSquarePointStyle(state) == E_OK) fSquarePointStyle = state;
	}
}


bool
EView::IsSquarePointStyle() const
{
	return fSquarePointStyle;
}


void
EView::DrawBitmap(const EBitmap *bitmap)
{
	DrawBitmap(bitmap, PenLocation());
}


void
EView::DrawBitmap(const EBitmap *bitmap, EPoint where)
{
	if(bitmap == NULL) return;

	ERect r = bitmap->Bounds();
	DrawBitmap(bitmap, r, r.OffsetToCopy(where));
}


void
EView::DrawBitmap(const EBitmap *bitmap, ERect srcRect, ERect destRect)
{
	if(IsVisible() == false || bitmap == NULL || bitmap->fPixmap == NULL ||
	   srcRect.IsValid() == false || bitmap->Bounds().Intersects(srcRect) == false || destRect.IsValid() == false) return;

	EWindow *win = Window();
	if(!win) return;

	ERect r = ConvertToWindow(destRect);
	ERect updateRect(r.InsetByCopy(PenSize() / -2.f, PenSize() / -2.f));

	if(IsPrinting())
	{
		r.left /= UnitsPerPixel();
		r.top /= UnitsPerPixel();
		r.right /= UnitsPerPixel();
		r.bottom /= UnitsPerPixel();
	}

	r.Floor();
	srcRect.Floor();

	euint32 alpha = (fDrawingMode == E_OP_ALPHA ? HighColor().alpha : 255);
	ERegion clipping(fVisibleRegion);
	ConvertFromParent(&clipping);
	if(fHasClipping) clipping &= fClipping;
	ConvertToWindow(&clipping);
	if(IsPrinting()) clipping.Scale(1.f / UnitsPerPixel());

	if(bitmap->fPixmap->CopyTo(win->fPixmap,
				   (eint32)srcRect.left, (eint32)srcRect.top, (euint32)srcRect.Width(), (euint32)srcRect.Height(),
				   (eint32)r.left, (eint32)r.top, (euint32)r.Width(), (euint32)r.Height(),
				   alpha, &clipping) == E_OK) win->_Update(updateRect, false);

	MovePenTo(destRect.LeftTop());
}

