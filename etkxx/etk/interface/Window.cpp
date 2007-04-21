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
 * File: Window.cpp
 *
 * --------------------------------------------------------------------------*/

#include <math.h>

#include <etk/config.h>
#include <etk/support/ClassInfo.h>
#include <etk/support/String.h>
#include <etk/kernel/Kernel.h>
#include <etk/app/Application.h>
#include <etk/support/Autolock.h>
#include <etk/add-ons/graphics/GraphicsEngine.h>

#include "InterfaceDefs.h"
#include "View.h"
#include "ScrollBar.h"
#include "Window.h"

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


EWindow::EWindow(ERect frame, const char *title,
		 e_window_type type,
		 euint32 flags, euint32 workspace)
	: ELooper(NULL, E_DISPLAY_PRIORITY), fWindow(NULL), fPixmap(NULL), fDC(NULL), fWindowTitle(NULL), fFocus(NULL), fUpdateHolderThreadId(E_INT64_CONSTANT(0)), fUpdateHolderCount(E_INT64_CONSTANT(-1)), fInUpdate(false), fHidden(true), fMinimized(false), fActivated(false), fActivatedTimemap(E_INT64_CONSTANT(0)), fPositionChangedTimemap(E_INT64_CONSTANT(0)), fSizeChangedTimemap(E_INT64_CONSTANT(0)), fMouseGrabCount(0), fKeyboardGrabCount(0), fBrokeOnExpose(false), fPulseRate(E_INT64_CONSTANT(500000)), fPulseRunner(NULL)
{
	EString winLooperName;
	winLooperName << "Window " << etk_get_handler_token(this);
	SetName(winLooperName.String());

	fBackgroundColor.set_to(e_ui_color(E_PANEL_BACKGROUND_COLOR));

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Window must created within a application which has graphics-engine!", __PRETTY_FUNCTION__);

	switch(type)
	{
		case E_TITLED_WINDOW:
			fWindowLook = E_TITLED_WINDOW_LOOK;
			fWindowFeel = E_NORMAL_WINDOW_FEEL;
			break;

		case E_MODAL_WINDOW:
			fWindowLook = E_MODAL_WINDOW_LOOK;
			fWindowFeel = E_MODAL_APP_WINDOW_FEEL;
			break;

		case E_DOCUMENT_WINDOW:
			fWindowLook = E_DOCUMENT_WINDOW_LOOK;
			fWindowFeel = E_NORMAL_WINDOW_FEEL;
			break;

		case E_BORDERED_WINDOW:
			fWindowLook = E_BORDERED_WINDOW_LOOK;
			fWindowFeel = E_NORMAL_WINDOW_FEEL;
			break;

		case E_FLOATING_WINDOW:
			fWindowLook = E_FLOATING_WINDOW_LOOK;
			fWindowFeel = E_FLOATING_APP_WINDOW_FEEL;
			break;

		default:
			fWindowLook = E_TITLED_WINDOW_LOOK;
			fWindowFeel = E_NORMAL_WINDOW_FEEL;
	}

	fFrame = frame;
	fWindowFlags = flags;

	EMessenger msgrSelf(this);
	frame.Floor();
	if((fWindow = etk_app->fGraphicsEngine->CreateWindow((eint32)frame.left, (eint32)frame.top,
							     (euint32)max_c(frame.Width(), 0),
							     (euint32)max_c(frame.Height(), 0))) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create window!", __PRETTY_FUNCTION__);
	else if((fPixmap = etk_app->fGraphicsEngine->CreatePixmap((euint32)max_c(frame.Width(), 0),
								  (euint32)max_c(frame.Height(), 0))) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create pixmap!", __PRETTY_FUNCTION__);
	else if((fDC = etk_app->fGraphicsEngine->CreateContext()) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create graphics context!", __PRETTY_FUNCTION__);

	fDC->SetClipping(ERegion(frame.OffsetToCopy(E_ORIGIN)));
	fDC->SetDrawingMode(E_OP_COPY);
	fDC->SetPattern(E_SOLID_HIGH);
	fDC->SetHighColor(fBackgroundColor);
	fDC->SetPenSize(0);

	if(title) fWindowTitle = EStrdup(title);
	fWindowWorkspaces = 0;

	fWindow->SetFlags(fWindowFlags);
	fWindow->SetLook(fWindowLook);
	fWindow->SetFeel(fWindowFeel);
	fWindow->SetBackgroundColor(fBackgroundColor);
	fWindow->SetTitle(fWindowTitle);
	fWindow->ContactTo(&msgrSelf);

	SetWorkspaces(workspace);

	EMessenger msgr(this);
	EMessage pulseMsg(E_PULSE);
	fPulseRunner = new EMessageRunner(msgr, &pulseMsg, fPulseRate, 0);
	if(!(fPulseRunner == NULL || fPulseRunner->IsValid())) {delete fPulseRunner; fPulseRunner = NULL;}
}


EWindow::EWindow(ERect frame, const char *title,
		 e_window_look look, e_window_feel feel,
		 euint32 flags, euint32 workspace)
	: ELooper(NULL, E_DISPLAY_PRIORITY), fWindow(NULL), fPixmap(NULL), fDC(NULL), fWindowTitle(NULL), fFocus(NULL), fUpdateHolderThreadId(E_INT64_CONSTANT(0)), fUpdateHolderCount(E_INT64_CONSTANT(-1)), fInUpdate(false), fHidden(true), fMinimized(false), fActivated(false), fActivatedTimemap(E_INT64_CONSTANT(0)), fPositionChangedTimemap(E_INT64_CONSTANT(0)), fSizeChangedTimemap(E_INT64_CONSTANT(0)), fMouseGrabCount(0), fKeyboardGrabCount(0), fBrokeOnExpose(false), fPulseRate(E_INT64_CONSTANT(500000)), fPulseRunner(NULL)
{
	EString winLooperName;
	winLooperName << "Window " << etk_get_handler_token(this);
	SetName(winLooperName.String());

	fBackgroundColor.set_to(e_ui_color(E_PANEL_BACKGROUND_COLOR));

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Window must created within a application which has graphics-engine!", __PRETTY_FUNCTION__);

	fFrame = frame;
	fWindowLook = look;
	fWindowFlags = flags;

	EMessenger msgrSelf(this);
	frame.Floor();
	if((fWindow = etk_app->fGraphicsEngine->CreateWindow((eint32)frame.left, (eint32)frame.top,
							     (euint32)max_c(frame.Width(), 0),
							     (euint32)max_c(frame.Height(), 0))) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create window!", __PRETTY_FUNCTION__);
	else if((fPixmap = etk_app->fGraphicsEngine->CreatePixmap((euint32)max_c(frame.Width(), 0),
								  (euint32)max_c(frame.Height(), 0))) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create pixmap!", __PRETTY_FUNCTION__);
	else if((fDC = etk_app->fGraphicsEngine->CreateContext()) == NULL)
		ETK_ERROR("[INTERFACE]: %s --- Unable to create graphics context!", __PRETTY_FUNCTION__);

	fDC->SetClipping(ERegion(frame.OffsetToCopy(E_ORIGIN)));
	fDC->SetDrawingMode(E_OP_COPY);
	fDC->SetPattern(E_SOLID_HIGH);
	fDC->SetHighColor(fBackgroundColor);
	fDC->SetPenSize(0);

	if(title) fWindowTitle = EStrdup(title);
	fWindowWorkspaces = 0;
	fWindowFeel = feel;

	fWindow->SetFlags(fWindowFlags);
	fWindow->SetLook(fWindowLook);
	fWindow->SetFeel(fWindowFeel);
	fWindow->SetBackgroundColor(fBackgroundColor);
	fWindow->SetTitle(fWindowTitle);
	fWindow->ContactTo(&msgrSelf);

	SetWorkspaces(workspace);

	EMessenger msgr(this);
	EMessage pulseMsg(E_PULSE);
	fPulseRunner = new EMessageRunner(msgr, &pulseMsg, fPulseRate, 0);
	if(!(fPulseRunner == NULL || fPulseRunner->IsValid())) {delete fPulseRunner; fPulseRunner = NULL;}
}


EWindow::~EWindow()
{
	Hide();

	EView *child = NULL;
	while((child = (EView*)fViewsList.LastItem()) != NULL)
	{
		RemoveChild(child);
		delete child;
	}

	if(fWindow) delete fWindow;

	if(fPixmap) delete fPixmap;
	if(fDC) delete fDC;
	if(fWindowTitle) delete[] fWindowTitle;
	if(fPulseRunner) delete fPulseRunner;
}


EWindow::EWindow(EMessage *from)
	: ELooper(NULL, E_DISPLAY_PRIORITY), fWindow(NULL), fPixmap(NULL), fDC(NULL), fWindowTitle(NULL), fFocus(NULL), fUpdateHolderThreadId(E_INT64_CONSTANT(0)), fUpdateHolderCount(E_INT64_CONSTANT(-1)), fInUpdate(false), fHidden(true), fMinimized(false), fActivated(false), fActivatedTimemap(E_INT64_CONSTANT(0)), fPositionChangedTimemap(E_INT64_CONSTANT(0)), fSizeChangedTimemap(E_INT64_CONSTANT(0)), fMouseGrabCount(0), fKeyboardGrabCount(0), fBrokeOnExpose(false), fPulseRate(E_INT64_CONSTANT(500000)), fPulseRunner(NULL)
{
	EString winLooperName;
	winLooperName << "Window " << etk_get_handler_token(this);
	SetName(winLooperName.String());

	// TODO

	EMessenger msgr(this);
	EMessage pulseMsg(E_PULSE);
	fPulseRunner = new EMessageRunner(msgr, &pulseMsg, fPulseRate, 0);
	if(!(fPulseRunner == NULL || fPulseRunner->IsValid())) {delete fPulseRunner; fPulseRunner = NULL;}
}


e_status_t
EWindow::Archive(EMessage *into, bool deep) const
{
	if(!into) return E_ERROR;

	ELooper::Archive(into, deep);
	into->AddString("class", "EWindow");

	// TODO

	return E_OK;
}


EArchivable*
EWindow::Instantiate(EMessage *from)
{
	if(e_validate_instantiation(from, "EWindow"))
		return new EWindow(from);
	return NULL;
}


void
EWindow::DispatchMessage(EMessage *msg, EHandler *target)
{
	if(target == NULL) target = PreferredHandler();
	if(!target || target->Looper() != this) return;

	if(target != this)
	{
		ELooper::DispatchMessage(msg, target);
		return;
	}

	bool IsCurrentMessage = (CurrentMessage() == msg ? true : false);
	bool sendNotices = true;

	msg->RemoveBool("etk:msg_from_gui");
	switch(msg->what)
	{
		case E_PULSE:
			for(eint32 i = 0; i < fNeededToPulseViews.CountItems(); i++)
			{
				EView *view = (EView*)fNeededToPulseViews.ItemAt(i);
				view->MessageReceived(msg);
			}
			break;

		case E_MOUSE_DOWN:
		case E_MOUSE_UP:
		case E_MOUSE_MOVED:
		case E_MOUSE_WHEEL_CHANGED:
			{
				EView *view;
				EPoint where;
				if(msg->FindPoint("where", &where) == false && msg->what != E_MOUSE_WHEEL_CHANGED)
				{
					if(msg->FindPoint("screen_where", &where) == false)
					{
						ETK_DEBUG("[INTERFACE]: %s --- Invalid message.", __PRETTY_FUNCTION__);
						break;
					}
					ConvertFromScreen(&where);
					msg->AddPoint("where", where);
				}

				if(msg->what != E_MOUSE_WHEEL_CHANGED)
				{
					EList Views;

					for(eint32 i = 0; i < fMouseInsideViews.CountItems(); i++)
					{
						if((view = (EView*)fMouseInsideViews.ItemAt(i)) == NULL) continue;
						if(view->EventMask() & E_POINTER_EVENTS) continue;
						if(view->VisibleBoundsRegion().Contains(view->ConvertFromWindow(where))) continue;
						Views.AddItem(view);
					}

					if(!Views.IsEmpty())
					{
						EMessage aMsg(*msg);
						aMsg.what = E_MOUSE_MOVED;

						for(eint32 i = 0; i < Views.CountItems(); i++)
						{
							if((view = (EView*)Views.ItemAt(i)) == NULL || view->Window() != this) continue;
							EPoint pt = view->ConvertFromWindow(where);
							aMsg.ReplacePoint("where", pt);
							view->MessageReceived(&aMsg);
						}

						if(IsCurrentMessage && !CurrentMessage()) break;
					}
				}

				EMessage aMsg(*msg);

				for(eint32 i = 0; i < fViewsList.CountItems(); i++)
				{
					if((view = (EView*)fViewsList.ItemAt(i)) == NULL || view->Window() != this) continue;
					if(view->VisibleFrameRegion().Contains(where) == false) continue;

					if(!(view->EventMask() & E_POINTER_EVENTS))
					{
						EPoint pt = view->ConvertFromWindow(where);
						aMsg.ReplacePoint("where", pt);

						view->MessageReceived(&aMsg);
					}

					break; // just one child can receive the message
				}

				EList viewsList(fMouseInterestedViews);
				for(eint32 i = 0; i < viewsList.CountItems(); i++)
				{
					if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Window() != this) continue;

					EPoint pt = view->ConvertFromWindow(where);
					aMsg.ReplacePoint("where", pt);

					view->MessageReceived(&aMsg);

					if(IsCurrentMessage && !CurrentMessage()) break;
				}
			}
			break;

		case E_UNMAPPED_KEY_DOWN:
		case E_UNMAPPED_KEY_UP:
		case E_KEY_DOWN:
		case E_KEY_UP:
		case E_MODIFIERS_CHANGED:
			{
				// TODO: shortcuts
				EView *view;

				if((view = CurrentFocus()) != NULL) view->MessageReceived(msg);
				if(IsCurrentMessage && !CurrentMessage()) break;

				EList viewsList(fKeyboardInterestedViews);
				for(eint32 i = 0; i < viewsList.CountItems(); i++)
				{
					if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Window() != this) continue;
					if(view != CurrentFocus()) view->MessageReceived(msg);

					if(IsCurrentMessage && !CurrentMessage()) break;
				}
			}
			break;

		case E_WORKSPACES_CHANGED:
			{
				euint32 curWorkspace;
				if(msg->FindInt32("new", (eint32*)&curWorkspace) == false) break;
				if(curWorkspace == 0) break;
				if(fWindowWorkspaces != curWorkspace)
				{
					euint32 oldWorkspace = fWindowWorkspaces;
					fWindowWorkspaces = curWorkspace;
					if(oldWorkspace != 0) WorkspacesChanged(oldWorkspace, curWorkspace);
				}
			}
			break;

		case E_WINDOW_ACTIVATED:
			{
				e_bigtime_t when;
				bool active;
				if(msg->FindInt64("when", (eint64*)&when) == false) break;
				if(fWindow->GetActivatedState(&active) != E_OK) break;

				if(fActivated != active && fActivatedTimemap <= when)
				{
					fActivated = active;
					fActivatedTimemap = when;
					if(active && !(fWindowFlags & E_AVOID_FRONT)) fWindow->Raise();
					WindowActivated(active);

					EView *view;
					EList viewsList(fViewsList);
					for(eint32 i = 0; i < viewsList.CountItems(); i++)
					{
						if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Window() != this) continue;
						view->_WindowActivated(active);
					}
				}
			}
			break;

		case E_WINDOW_MOVED:
		case E_WINDOW_RESIZED:
			{
				e_bigtime_t when;
				if(msg->FindInt64("when", &when) == false) break;
				if(msg->what == E_WINDOW_MOVED && when < fPositionChangedTimemap) break;
				if(msg->what == E_WINDOW_RESIZED && when < fSizeChangedTimemap) break;

				EPoint where = fFrame.LeftTop();
				float w = fFrame.Width();
				float h = fFrame.Height();

				bool doMoved = false, doResized = false;

				if(msg->what == E_WINDOW_RESIZED)
				{
					if(msg->FindFloat("width", &w) == false || msg->FindFloat("height", &h) == false) break;
					msg->FindPoint("where", &where);
				}
				else if(msg->what == E_WINDOW_MOVED)
				{
					if(msg->FindPoint("where", &where) == false) break;
				}

				doMoved = (msg->what == E_WINDOW_MOVED ? fFrame.LeftTop() != where : false);
				doResized = (msg->what == E_WINDOW_RESIZED ? (fFrame.Width() != w || fFrame.Height() != h) : false);

				if(CurrentMessage() == msg)
				{
					MessageQueue()->Lock();
					while(MessageQueue()->IsEmpty() == false)
					{
						EMessage *aMsg = MessageQueue()->FindMessage((eint32)0);
						if(!aMsg) break;

						if(!(aMsg->what == E_WINDOW_RESIZED || aMsg->what == E_WINDOW_MOVED))
						{
							if(aMsg->what == _UPDATE_ || aMsg->what == _UPDATE_IF_NEEDED_)
							{
								if(!doResized) break;
								MessageQueue()->RemoveMessage(aMsg);
								continue;
							}
							break;
						}

						if(aMsg->what == E_WINDOW_RESIZED)
						{
							float w1, h1;
							e_bigtime_t nextWhen;
							if(aMsg->FindFloat("width", &w1) == false ||
							   aMsg->FindFloat("height", &h1) == false ||
							   aMsg->FindInt64("when", &nextWhen) == false ||
							   nextWhen < when)
							{
								MessageQueue()->RemoveMessage(aMsg);
								continue;
							}
							w = w1; h = h1; when = nextWhen;
							aMsg->FindPoint("where", &where);
						}
						else // E_WINDOW_MOVED
						{
							e_bigtime_t nextWhen;
							if(aMsg->FindInt64("when", &nextWhen) == false ||
							   nextWhen < when ||
							   aMsg->FindPoint("where", &where) == false)
							{
								MessageQueue()->RemoveMessage(aMsg);
								continue;
							}
							when = nextWhen;
						}

						if(fFrame.LeftTop() != where && !doMoved) doMoved = true;
						if((fFrame.Width() != w || fFrame.Height() != h) && !doResized) doResized = true;

						MessageQueue()->RemoveMessage(aMsg);
					}
					MessageQueue()->Unlock();
				}

				if(doMoved)
				{
					fPositionChangedTimemap = when;

					fFrame.OffsetTo(where);
				}

				if(doResized)
				{
					fSizeChangedTimemap = when;

					ERect oldFrame = fFrame;
					fFrame.right = fFrame.left + w;
					fFrame.bottom = fFrame.top + h;

					ERect rFrame = fFrame.FloorCopy();
					fPixmap->ResizeTo((euint32)max_c(rFrame.Width(), 0), (euint32)max_c(rFrame.Height(), 0));
					fDC->SetClipping(ERegion(rFrame.OffsetToCopy(E_ORIGIN)));

					fExposeRect = Bounds();
					fBrokeOnExpose = false;
					PostMessage(_UPDATE_IF_NEEDED_, this);

					// for disable update
					bool oldInUpdate = fInUpdate;
					fInUpdate = true;

					EView *view;
					for(eint32 i = 0; (view = (EView*)fViewsList.ItemAt(i)) != NULL; i++)
					{
						euint32 vMode = view->ResizingMode();
						ERect vFrame = view->Frame();

						if(vMode == E_FOLLOW_NONE || vMode == (E_FOLLOW_LEFT | E_FOLLOW_TOP))
						{
							view->_SetFrame(vFrame, true);
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

					fInUpdate = oldInUpdate;
				}
				else if(fBrokeOnExpose)
				{
					fBrokeOnExpose = false;
					PostMessage(_UPDATE_IF_NEEDED_, this);
				}

				sendNotices = false;

				if(doMoved)
				{
					FrameMoved(where);
					if(IsWatched())
					{
						EMessage aMsg(E_WINDOW_MOVED);
						aMsg.AddInt64("when", when);
						aMsg.AddPoint("where", where);
						SendNotices(E_WINDOW_MOVED, &aMsg);
					}
				}
				if(doResized)
				{
					FrameResized(fFrame.Width(), fFrame.Height());
					if(IsWatched())
					{
						EMessage aMsg(E_WINDOW_RESIZED);
						aMsg.AddInt64("when", when);
						aMsg.AddFloat("width", fFrame.Width());
						aMsg.AddFloat("height", fFrame.Height());
						SendNotices(E_WINDOW_RESIZED, &aMsg);
					}
				}
			}
			break;

		case E_MINIMIZE:
			{
				bool minimize;
				if(msg->FindBool("minimize", &minimize) == false) break;

				Minimize(minimize);
			}
			break;

		case E_MINIMIZED:
			{
				bool minimize;
				if(msg->FindBool("minimize", &minimize) == false) break;

				if(fMinimized != minimize)
				{
					fMinimized = minimize;
					// in order call the virtual function
					if(!IsHidden()) Minimize(minimize);
				}
			}
			break;

		case _UPDATE_IF_NEEDED_:
			{
				sendNotices = false;

				e_bigtime_t when = e_real_time_clock_usecs();
				msg->FindInt64("when", (eint64*)&when);

				if(CurrentMessage() == msg)
				{
					bool noNeededToUpdate = false;
					EMessage *aMsg = NULL;

					MessageQueue()->Lock();
					if((aMsg = MessageQueue()->FindMessage((eint32)0)) != NULL)
					{
						if(aMsg->what == _UPDATE_IF_NEEDED_)
						{
							// Here we don't need to update until the next event
							noNeededToUpdate = true;
						}
						else if(aMsg->what == _UPDATE_)
						{
							// Here we don't need to update because of
							// that it's a expose event next to handle, and
							// probably within the short time for switching
							// another or more expose events will need to be handle.
							noNeededToUpdate = true;
						}
					}
					MessageQueue()->Unlock();
					if(noNeededToUpdate) break;
				}

				_UpdateIfNeeded(when);
			}
			break;

		case _UPDATE_: // TODO: speed up
			{
				sendNotices = false;

				ERect rect;
				if(msg->FindRect("etk:frame", &rect))
				{
					bool expose = false;
					msg->FindBool("etk:expose", &expose);

					rect &= Bounds();
					if(rect.IsValid())
					{
						if(expose)
							fExposeRect = fExposeRect.IsValid() ? (fExposeRect | rect) : rect;
						else
							fUpdateRect = fUpdateRect.IsValid() ? (fUpdateRect | rect) : rect;
					}
				}

				if(CurrentMessage() == msg)
				{
					bool noNeededToSendUpdate = false;
					EMessage *aMsg = NULL;
					MessageQueue()->Lock();
					if((aMsg = MessageQueue()->FindMessage((eint32)0)) != NULL)
					{
						if(aMsg->what == _UPDATE_IF_NEEDED_)
						{
							// Here we don't need to post _UPDATE_IF_NEEDED_
							noNeededToSendUpdate = true;
						}
						else if(aMsg->what == _UPDATE_)
						{
							// Here we don't post _UPDATE_IF_NEEDED_ because of
							// that it's a expose event next to handle, and
							// probably within the short time for switching
							// another or more expose events will need to be handle.
							noNeededToSendUpdate = true;
						}
					}
					MessageQueue()->Unlock();
					if(noNeededToSendUpdate) break;
				}

				PostMessage(_UPDATE_IF_NEEDED_, this);
			}
			break;

		default:
			sendNotices = false;
			ELooper::DispatchMessage(msg, target);
	}

	if(sendNotices && IsWatched()) SendNotices(msg->what, msg);
}


void
EWindow::Quit()
{
	if(!IsLockedByCurrentThread())
		ETK_ERROR("[INTERFACE]: %s --- Window must LOCKED before \"Quit()\" call!", __PRETTY_FUNCTION__);

	if(fWindow)
	{
		fWindow->Hide();
		fWindow->ContactTo(NULL);
	}

	if(fWindowFlags & E_QUIT_ON_WINDOW_CLOSE) etk_app->PostMessage(E_QUIT_REQUESTED);

	ELooper::Quit();
}


void
EWindow::Show()
{
	if(!fHidden) return;

	fHidden = false;
	fMinimized = false;

	fWindow->Show();

	if(fPulseRunner)
		fPulseRunner->SetCount((fPulseRate > E_INT64_CONSTANT(0) && fNeededToPulseViews.CountItems() > 0) ? -1 : 0);

	if(!(IsRunning() || Proxy() != this)) Run();

	EMessage msg(_UPDATE_);
	msg.AddInt64("when", e_real_time_clock_usecs());
	msg.AddRect("etk:frame", Bounds());
	msg.AddBool("etk:expose", true);

	PostMessage(&msg, this);

	if(fWindowFeel == E_MODAL_APP_WINDOW_FEEL)
	{
		EMessenger msgrSelf(this);
		etk_app->AddModalWindow(msgrSelf);
	}
}


void
EWindow::Hide()
{
	if(fHidden) return;

	if(fPulseRunner) fPulseRunner->SetCount(E_INT64_CONSTANT(0));

	if(fWindowFeel == E_MODAL_APP_WINDOW_FEEL)
	{
		EMessenger msgrSelf(this);
		etk_app->RemoveModalWindow(msgrSelf);
	}

	if(fMouseGrabCount > 0)
	{
		fWindow->UngrabMouse();
		fMouseGrabCount = 0;

		EView *view;
		for(eint32 i = 0; i < fMouseInterestedViews.CountItems(); i++)
		{
			if((view = (EView*)fMouseInterestedViews.ItemAt(i)) == NULL) continue;
			if(view->fMouseGrabbed) view->fMouseGrabbed = false;
		}
	}

	if(fKeyboardGrabCount > 0)
	{
		fWindow->UngrabKeyboard();
		fKeyboardGrabCount = 0;

		EView *view;
		for(eint32 i = 0; i < fKeyboardInterestedViews.CountItems(); i++)
		{
			if((view = (EView*)fKeyboardInterestedViews.ItemAt(i)) == NULL) continue;
			if(view->fKeyboardGrabbed) view->fKeyboardGrabbed = false;
		}
	}

	fWindow->Hide();

	fHidden = true;
	fMinimized = false;
	fBrokeOnExpose = false;

	if(IsWatched(E_MINIMIZED))
	{
		EMessage aMsg(E_MINIMIZED);
		aMsg.AddInt64("when", e_real_time_clock_usecs());
		aMsg.AddBool("minimize", false);
		SendNotices(E_MINIMIZED, &aMsg);
	}
}


bool
EWindow::IsHidden() const
{
	return fHidden;
}


bool
EWindow::IsMinimized() const
{
	if(IsHidden()) return false;
	if(fMinimized) return true;

	return false;
}


void
EWindow::AddViewChildrenToHandlersList(EWindow *win, EView *child)
{
	if(!win || !child) return;
	EView *view = NULL;
	for(eint32 i = 0; (view = child->ChildAt(i)) != NULL; i++)
	{
		win->AddHandler(view);

		if(view->Looper() != win)
		{
			ETK_WARNING("[INTERFACE]: %s --- Add child of the view added by \"AddChild()\" failed.", __PRETTY_FUNCTION__);
			continue;
		}

		view->AttachToWindow();
		view->AttachedToWindow();

		AddViewChildrenToHandlersList(win, view);
		view->AllAttached();
	}
}


void
EWindow::RemoveViewChildrenFromHandlersList(EWindow *win, EView *child)
{
	if(!win || !child || child->Looper() != win) return;
	EView *view = NULL;
	for(eint32 i = 0; (view = child->ChildAt(i)) != NULL; i++)
	{
		RemoveViewChildrenFromHandlersList(win, view);
		view->AllDetached();

		view->DetachedFromWindow();

		view->DetachFromWindow();
		win->RemoveHandler(view);
	}
}


void
EWindow::AddChild(EView *child, EView *nextSibling)
{
	eint32 indexNext = -1;

	if(child == NULL || child->Looper() != NULL || child->Parent() != NULL ||
	   (nextSibling == NULL ? false : (nextSibling->Looper() != this ||
					   nextSibling->Parent() != NULL ||
					   (indexNext = fViewsList.IndexOf(nextSibling)) < 0)))
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

	AddHandler(child);
	if(child->Looper() != this)
	{
		fViewsList.RemoveItem(child);
		ETK_WARNING("[INTERFACE]: %s --- Unable to attach child to window, abort to add child.", __PRETTY_FUNCTION__);
		return;
	}

	if(prevSibling != NULL) prevSibling->fNextSibling = child;
	if(nextSibling != NULL) nextSibling->fPrevSibling = child;

	child->fPrevSibling = prevSibling;
	child->fNextSibling = nextSibling;

	child->AttachToWindow();
	child->AttachedToWindow();

	AddViewChildrenToHandlersList(this, child);
	child->AllAttached();

	if(child->fHidden) return;

	while(nextSibling != NULL)
	{
		if(nextSibling->fHidden == false) nextSibling->_UpdateOriginAndVisibleRegion(true);
		nextSibling = nextSibling->fNextSibling;
	}
}


bool
EWindow::RemoveChild(EView *child)
{
	if(child == NULL || child->Looper() != this || child->Parent() != NULL) return false;

	eint32 childIndex = fViewsList.IndexOf(child);
	if(childIndex < 0) return false;

	bool childIsHidden = child->fHidden;

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
		if(scrollbar->fTarget == NULL)
		{
			scrollbar->fTarget->fScrollBar.RemoveItem(scrollbar);
			scrollbar->fTarget = NULL;
		}
	}

	if(child->Parent()) child->Parent()->ChildRemoving(child);

	RemoveViewChildrenFromHandlersList(this, child);
	child->AllDetached();

	child->DetachedFromWindow();

	child->DetachFromWindow();
	RemoveHandler(child);

	fViewsList.RemoveItem(childIndex);

	EView *nextSibling = child->fNextSibling;

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


eint32
EWindow::CountChildren() const
{
	return fViewsList.CountItems();
}


EView*
EWindow::ChildAt(eint32 index) const
{
	return((EView*)fViewsList.ItemAt(index));
}


void
EWindow::ConvertToScreen(EPoint* pt) const
{
	if(!pt) return;
	*pt += fFrame.LeftTop();
}


EPoint
EWindow::ConvertToScreen(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertToScreen(&pt1);
	return pt1;
}


void
EWindow::ConvertFromScreen(EPoint* pt) const
{
	if(!pt) return;
	*pt -= fFrame.LeftTop();
}


EPoint
EWindow::ConvertFromScreen(EPoint pt) const
{
	EPoint pt1 = pt;
	ConvertFromScreen(&pt1);
	return pt1;
}


void
EWindow::ConvertToScreen(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertToScreen(r->LeftTop());
	r->OffsetTo(pt);
}


ERect
EWindow::ConvertToScreen(ERect r) const
{
	ERect rect = r;
	ConvertToScreen(&rect);
	return rect;
}


void
EWindow::ConvertFromScreen(ERect *r) const
{
	if(!r) return;
	EPoint pt = ConvertFromScreen(E_ORIGIN);
	r->OffsetBy(pt);
}


ERect
EWindow::ConvertFromScreen(ERect r) const
{
	ERect rect = r;
	ConvertFromScreen(&rect);
	return rect;
}


void
EWindow::ConvertToScreen(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertToScreen(region->Frame().LeftTop());
	region->OffsetBy(pt - region->Frame().LeftTop());
}


ERegion
EWindow::ConvertToScreen(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertToScreen(&aRegion);
	return aRegion;
}


void
EWindow::ConvertFromScreen(ERegion *region) const
{
	if(!region || region->CountRects() <= 0) return;
	EPoint pt = ConvertFromScreen(E_ORIGIN);
	region->OffsetBy(pt);
}


ERegion
EWindow::ConvertFromScreen(const ERegion &region) const
{
	ERegion aRegion(region);
	ConvertFromScreen(&aRegion);
	return aRegion;
}


void
EWindow::FrameMoved(EPoint new_position)
{
}


void
EWindow::WorkspacesChanged(euint32 old_ws, euint32 new_ws)
{
}


void
EWindow::WorkspaceActivated(eint32 ws, bool state)
{
}


void
EWindow::FrameResized(float new_width, float new_height)
{
}


void
EWindow::Minimize(bool minimize)
{
	if(fMouseGrabCount > 0)
	{
		fWindow->UngrabMouse();
		fMouseGrabCount = 0;

		EView *view;
		for(eint32 i = 0; i < fMouseInterestedViews.CountItems(); i++)
		{
			if((view = (EView*)fMouseInterestedViews.ItemAt(i)) == NULL) continue;
			if(view->fMouseGrabbed) view->fMouseGrabbed = false;
		}
	}

	if(fKeyboardGrabCount > 0)
	{
		fWindow->UngrabKeyboard();
		fKeyboardGrabCount = 0;

		EView *view;
		for(eint32 i = 0; i < fKeyboardInterestedViews.CountItems(); i++)
		{
			if((view = (EView*)fKeyboardInterestedViews.ItemAt(i)) == NULL) continue;
			if(view->fKeyboardGrabbed) view->fKeyboardGrabbed = false;
		}
	}

	if(fMinimized == minimize) return;

	fMinimized = minimize;

	if(fHidden) return;

	if(fMinimized)
		fWindow->Iconify();
	else
		fWindow->Show();
}


ERect
EWindow::Bounds() const
{
	return fFrame.OffsetToCopy(E_ORIGIN);
}


ERect
EWindow::Frame() const
{
	return fFrame;
}


void
EWindow::DisableUpdates()
{
	eint64 currentThread = etk_get_current_thread_id();

	if(fUpdateHolderThreadId != E_INT64_CONSTANT(0) && fUpdateHolderThreadId != currentThread)
		ETK_ERROR("[INTERFACE]: %s --- Must call \"DisableUpdates()\" and \"EnableUpdates()\" in a same thread!", __PRETTY_FUNCTION__);
	else if(currentThread == E_INT64_CONSTANT(0))
		ETK_ERROR("[INTERFACE]: %s --- Thread not support!", __PRETTY_FUNCTION__);

	if(fUpdateHolderThreadId == E_INT64_CONSTANT(0))
	{
		fUpdateHolderThreadId = currentThread;
		fUpdateHolderCount = 1;
	}
	else
	{
		if(E_MAXINT64 - E_INT64_CONSTANT(1) < fUpdateHolderCount)
			ETK_ERROR("[INTERFACE]: %s --- Call \"DisableUpdates()\" more than limited times!", __PRETTY_FUNCTION__);

		fUpdateHolderCount++;
	}
}


void
EWindow::EnableUpdates()
{
	eint64 currentThread = etk_get_current_thread_id();

	if(fUpdateHolderThreadId != E_INT64_CONSTANT(0) && fUpdateHolderThreadId != currentThread)
		ETK_ERROR("[INTERFACE]: %s --- Must call \"DisableUpdates()\" and \"EnableUpdates()\" in a same thread!", __PRETTY_FUNCTION__);
	else if(currentThread == E_INT64_CONSTANT(0))
		ETK_ERROR("[INTERFACE]: %s --- Thread not support!", __PRETTY_FUNCTION__);
	else if(fUpdateHolderThreadId <= E_INT64_CONSTANT(0))
	{
		ETK_WARNING("[INTERFACE]: %s --- Must call \"DisableUpdates()\" before \"EnableUpdates()\"!", __PRETTY_FUNCTION__);
		return;
	}

	fUpdateHolderCount--;
	if(fUpdateHolderCount > E_INT64_CONSTANT(0)) return;

	fUpdateHolderCount = E_INT64_CONSTANT(0);
	fUpdateHolderThreadId = E_INT64_CONSTANT(0);

	if(fUpdateRect.IsValid() && !_HasResizeMessage(false))
	{
		fUpdateRect.Floor();
		fPixmap->CopyTo(fWindow,
				(eint32)fUpdateRect.left, (eint32)fUpdateRect.top,
				(euint32)fUpdateRect.Width(), (euint32)fUpdateRect.Height(),
				(eint32)fUpdateRect.left, (eint32)fUpdateRect.top,
				(euint32)fUpdateRect.Width(), (euint32)fUpdateRect.Height(),
				255, NULL);
		fUpdateRect = ERect();
	}
}


bool
EWindow::NeedsUpdate() const
{
	return(fExposeRect.IsValid() || fUpdateRect.IsValid());
}


void
EWindow::_UpdateIfNeeded(e_bigtime_t when)
{
	if(_HasResizeMessage(false) || !NeedsUpdate() || !fWindow) return;

	fBrokeOnExpose = false;
	ERect r = fExposeRect;
	if(r.IsValid())
	{
		bool oldInUpdate = fInUpdate;

		fExposeRect = ERect();

		fInUpdate = true;
		_Expose(r, when);
		if(fBrokeOnExpose)
		{
			fExposeRect = fExposeRect.IsValid() ? (fExposeRect | r) : r;
			fInUpdate = oldInUpdate;
			fBrokeOnExpose = false;
			PostMessage(_UPDATE_IF_NEEDED_, this);
			return;
		}
		else if(fExposeRect.IsValid())
		{
			fUpdateRect = fUpdateRect.IsValid() ? (fUpdateRect | r) : r;
			fInUpdate = oldInUpdate;
			_UpdateIfNeeded(e_real_time_clock_usecs());
			return;
		}

		if(fUpdateRect.IsValid()) r |= fUpdateRect;

		fInUpdate = oldInUpdate;
	}
	else
	{
		r = fUpdateRect;
	}

	fUpdateRect = ERect();

	r &= Bounds();

	if(r.IsValid() == false) return;

	r.Floor();
	fPixmap->CopyTo(fWindow,
			(eint32)r.left, (eint32)r.top, (euint32)r.Width(), (euint32)r.Height(),
			(eint32)r.left, (eint32)r.top, (euint32)r.Width(), (euint32)r.Height(),
			255, NULL);
}


void
EWindow::UpdateIfNeeded()
{
	_UpdateIfNeeded(e_real_time_clock_usecs());
}


void
EWindow::_Update(ERect rect, bool force_update)
{
	if(rect.IsValid() == false) return;
	fUpdateRect = fUpdateRect.IsValid() ? (fUpdateRect | rect) : rect;
	if(fInUpdate) return;
	if(fUpdateRect.IsValid() == false) return;
	if(force_update || fUpdateHolderThreadId == E_INT64_CONSTANT(0))
	{
		fUpdateRect.Floor();
		fPixmap->CopyTo(fWindow,
				(eint32)fUpdateRect.left, (eint32)fUpdateRect.top,
				(euint32)fUpdateRect.Width(), (euint32)fUpdateRect.Height(),
				(eint32)fUpdateRect.left, (eint32)fUpdateRect.top,
				(euint32)fUpdateRect.Width(), (euint32)fUpdateRect.Height(),
				255, NULL);
		fUpdateRect = ERect();
	}
}


void
EWindow::SetBackgroundColor(e_rgb_color c)
{
	if(fBackgroundColor != c)
	{
		fBackgroundColor.set_to(c.red, c.green, c.blue, c.alpha);
		fWindow->SetBackgroundColor(fBackgroundColor);
		fDC->SetHighColor(fBackgroundColor);

		fExposeRect = Bounds();
		PostMessage(_UPDATE_IF_NEEDED_, this);
	}
}


void
EWindow::SetBackgroundColor(euint8 r, euint8 g, euint8 b, euint8 a)
{
	e_rgb_color c;
	c.set_to(r, g, b, a);
	SetBackgroundColor(c);
}


e_rgb_color
EWindow::BackgroundColor() const
{
	return fBackgroundColor;
}


void
EWindow::_Expose(ERect rect, e_bigtime_t when)
{
	if(!fWindow) return;

	rect &= Bounds();
	if(rect.IsValid() == false) return;

	ERect r = rect.FloorCopy();
	fPixmap->FillRect(fDC, (eint32)r.left, (eint32)r.top, (euint32)r.Width(), (euint32)r.Height());

	ERegion region(rect);

	EView *child;
	for(eint32 i = 0; (child = (EView*)fViewsList.ItemAt(i)) != NULL; i++)
	{
		if(fBrokeOnExpose || _HasResizeMessage(true)) break;
		if(child->fVisibleRegion.Intersects(rect) == false) continue;

		child->_Expose(child->ConvertFromParent(region), when);
	}
}


bool
EWindow::InUpdate() const
{
	return fInUpdate;
}


bool
EWindow::_HasResizeMessage(bool setBrokeOnExpose)
{
	bool ret = false;

	MessageQueue()->Lock();

	EMessage *msg = MessageQueue()->FindMessage(E_WINDOW_RESIZED, 0, 20);
	if(msg)
	{
		float w, h;
		if(msg->FindFloat("width", &w) && msg->FindFloat("height", &h))
			ret = (fFrame.Width() != w || fFrame.Height() != h);
	}

	MessageQueue()->Unlock();

	if(ret && setBrokeOnExpose) fBrokeOnExpose = true;

	return ret;
}


void
EWindow::Activate(bool state)
{
	if(!(fHidden || fMinimized) || !state)
	{
		if(fWindow->Activate(state) != E_OK)
		{
			ETK_DEBUG("[INTERFACE]: %s --- Unable to %s window.", __PRETTY_FUNCTION__, (state ? "activate" : "inactivate"));
			return;
		}

		fActivatedTimemap = e_real_time_clock_usecs();
		fActivated = state;
		if(state && !(fWindowFlags & E_AVOID_FRONT)) fWindow->Raise();
		WindowActivated(state);

		EView *view;
		EList viewsList(fViewsList);
		for(eint32 i = 0; i < viewsList.CountItems(); i++)
		{
			if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Window() != this) continue;
			view->_WindowActivated(state);
		}
	}
}


bool
EWindow::IsActivate() const
{
	return fActivated;
}


void
EWindow::WindowActivated(bool state)
{
}


EView*
EWindow::FindView(const char *name) const
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


EView*
EWindow::FindView(EPoint where) const
{
	if(Bounds().Contains(where) == false) return NULL;

	EView *view;
	for(eint32 i = 0; i < fViewsList.CountItems(); i++)
	{
		if((view = (EView*)fViewsList.ItemAt(i)) == NULL) continue;
		if(view->VisibleFrameRegion().Contains(where)) return view;
	}

	return NULL;
}


EView*
EWindow::CurrentFocus() const
{
	return fFocus;
}


e_status_t
EWindow::SetType(e_window_type type)
{
	e_window_look look;
	e_window_feel feel;

	switch(type)
	{
		case E_MODAL_WINDOW:
			look = E_MODAL_WINDOW_LOOK;
			feel = E_MODAL_APP_WINDOW_FEEL;
			break;

		case E_DOCUMENT_WINDOW:
			look = E_DOCUMENT_WINDOW_LOOK;
			feel = E_NORMAL_WINDOW_FEEL;
			break;

		case E_BORDERED_WINDOW:
			look = E_BORDERED_WINDOW_LOOK;
			feel = E_NORMAL_WINDOW_FEEL;
			break;

		case E_FLOATING_WINDOW:
			look = E_FLOATING_WINDOW_LOOK;
			feel = E_FLOATING_APP_WINDOW_FEEL;
			break;

		default:
			return E_ERROR;
	}

	e_status_t status;

	e_window_look oldLook = fWindowLook;
	if((status = SetLook(look)) != E_OK) return status;
	if((status = SetFeel(feel)) != E_OK)
	{
		SetLook(oldLook);
		return status;
	}

	return E_OK;
}


e_window_type
EWindow::Type() const
{
	if(fWindowLook == E_TITLED_WINDOW_LOOK && fWindowFeel == E_NORMAL_WINDOW_FEEL)
		return E_TITLED_WINDOW;
	else if(fWindowLook == E_MODAL_WINDOW_LOOK && fWindowFeel == E_MODAL_APP_WINDOW_FEEL)
		return E_MODAL_WINDOW;
	else if(fWindowLook == E_DOCUMENT_WINDOW_LOOK && fWindowFeel == E_NORMAL_WINDOW_FEEL)
		return E_DOCUMENT_WINDOW;
	else if(fWindowLook == E_BORDERED_WINDOW_LOOK && fWindowFeel == E_NORMAL_WINDOW_FEEL)
		return E_BORDERED_WINDOW;
	else if(fWindowLook == E_FLOATING_WINDOW_LOOK && fWindowFeel == E_FLOATING_APP_WINDOW_FEEL)
		return E_FLOATING_WINDOW;
	else return E_UNTYPED_WINDOW;
}


e_status_t
EWindow::SetLook(e_window_look look)
{
	if(fWindowLook != look)
	{
		e_status_t status = fWindow->SetLook(look);
		if(status != E_OK) return status;
		fWindowLook = look;
	}

	return E_OK;
}


e_window_look
EWindow::Look() const
{
	return fWindowLook;
}


e_status_t
EWindow::SetFeel(e_window_feel feel)
{
	if(fWindowFeel != feel)
	{
		e_status_t status = fWindow->SetFeel(feel);
		if(status != E_OK) return status;

		e_window_feel oldFeel = fWindowFeel;
		fWindowFeel = feel;

		if((oldFeel == E_MODAL_APP_WINDOW_FEEL || feel == E_MODAL_APP_WINDOW_FEEL) && !IsHidden())
		{
			EMessenger msgrSelf(this);
			if(oldFeel == E_MODAL_APP_WINDOW_FEEL)
				etk_app->RemoveModalWindow(msgrSelf);
			else
				etk_app->AddModalWindow(msgrSelf);
		}
	}

	return E_OK;
}


e_window_feel
EWindow::Feel() const
{
	return fWindowFeel;
}


e_status_t
EWindow::SetFlags(euint32 flags)
{
	if(fWindowFlags != flags)
	{
		e_status_t status = fWindow->SetFlags(flags);
		if(status != E_OK) return status;
		fWindowFlags = flags;
		if(flags & E_AVOID_FOCUS)
		{
			if(fActivated != false)
			{
				if(fWindow->Activate(false) == E_OK)
				{
					fActivatedTimemap = e_real_time_clock_usecs();

					fActivated = false;
					WindowActivated(false);

					EView *view;
					EList viewsList(fViewsList);
					for(eint32 i = 0; i < viewsList.CountItems(); i++)
					{
						if((view = (EView*)viewsList.ItemAt(i)) == NULL || view->Window() != this) continue;
						view->_WindowActivated(false);
					}
				}
			}
		}
	}

	return E_OK;
}


euint32
EWindow::Flags() const
{
	return fWindowFlags;
}


void
EWindow::SetWorkspaces(euint32 workspace)
{
	if(workspace == 0)
	{
		if(etk_app->fGraphicsEngine->GetCurrentWorkspace(&workspace) != E_OK || workspace == 0) return;
	}

	if(fWindowWorkspaces != workspace || fWindowWorkspaces == 0)
	{
		if(fWindow->SetWorkspaces(workspace) == E_OK)
		{
			euint32 oldWorkspace = fWindowWorkspaces;
			fWindowWorkspaces = workspace;
			if(oldWorkspace != 0) WorkspacesChanged(oldWorkspace, workspace);
		}
	}
}


euint32
EWindow::Workspaces() const
{
	return fWindowWorkspaces;
}


void
EWindow::MoveBy(float dx, float dy)
{
	MoveTo(fFrame.LeftTop() + EPoint(dx, dy));
}


void
EWindow::ResizeBy(float dx, float dy)
{
	ResizeTo(fFrame.Width() + dx, fFrame.Height() + dy);
}


void
EWindow::MoveTo(EPoint where)
{
	if(fFrame.LeftTop() != where)
	{
		EPoint pt = where.FloorCopy();
		if(fWindow->MoveTo((eint32)pt.x, (eint32)pt.y) != E_OK) return;

		fPositionChangedTimemap = e_real_time_clock_usecs();
		fFrame.OffsetTo(where);
		FrameMoved(where);

		if(IsWatched(E_WINDOW_MOVED))
		{
			EMessage aMsg(E_WINDOW_MOVED);
			aMsg.AddInt64("when", fPositionChangedTimemap);
			aMsg.AddPoint("where", where);
			SendNotices(E_WINDOW_MOVED, &aMsg);
		}
	}
}


void
EWindow::ResizeTo(float w, float h)
{
	euint32 min_h = E_MAXUINT32, max_h = E_MAXUINT32, min_v = E_MAXUINT32, max_v = E_MAXUINT32;
	fWindow->GetSizeLimits(&min_h, &max_h, &min_v, &max_v);

	if(w < (float)min_h && min_h != E_MAXUINT32) w = (float)min_h;
	else if(w > (float)max_h && max_h != E_MAXUINT32) w = (float)max_h;
	if(h < (float)min_v && min_v != E_MAXUINT32) h = (float)min_v;
	else if(h > (float)max_v && max_v != E_MAXUINT32) h = (float)max_v;

	if(fFrame.Width() != w || fFrame.Height() != h)
	{
		ERect frame(fFrame);
		frame.right = frame.left + w;
		frame.bottom = frame.top + h;
		frame.Floor();
		if(fWindow->MoveAndResizeTo((eint32)frame.left, (eint32)frame.top,
					    (euint32)max_c(frame.Width(), 0),
					    (euint32)max_c(frame.Height(), 0)) != E_OK) return;

		fSizeChangedTimemap = e_real_time_clock_usecs();

		ERect oldFrame = fFrame;
		fFrame.right = fFrame.left + w;
		fFrame.bottom = fFrame.top + h;

		ERect rFrame = fFrame.FloorCopy();
		fPixmap->ResizeTo((euint32)max_c(rFrame.Width(), 0), (euint32)max_c(rFrame.Height(), 0));
		fDC->SetClipping(ERegion(rFrame.OffsetToCopy(E_ORIGIN)));

		fExposeRect = Bounds();
		PostMessage(_UPDATE_IF_NEEDED_, this);

		// for disable update
		bool oldInUpdate = fInUpdate;
		fInUpdate = true;

		EView *view;
		for(eint32 i = 0; (view = (EView*)fViewsList.ItemAt(i)) != NULL; i++)
		{
			euint32 vMode = view->ResizingMode();
			ERect vFrame = view->Frame();

			if(vMode == E_FOLLOW_NONE || vMode == (E_FOLLOW_LEFT | E_FOLLOW_TOP))
			{
				view->_SetFrame(vFrame, true);
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

		fInUpdate = oldInUpdate;

		FrameResized(fFrame.Width(), fFrame.Height());

		if(IsWatched(E_WINDOW_RESIZED))
		{
			EMessage aMsg(E_WINDOW_RESIZED);
			aMsg.AddInt64("when", fSizeChangedTimemap);
			aMsg.AddFloat("width", fFrame.Width());
			aMsg.AddFloat("height", fFrame.Height());
			SendNotices(E_WINDOW_RESIZED, &aMsg);
		}
	}
}


void
EWindow::SetSizeLimits(float min_h, float max_h, float min_v, float max_v)
{
	euint32 minH = E_MAXUINT32, maxH = E_MAXUINT32, minV = E_MAXUINT32, maxV = E_MAXUINT32;
	if(min_h >= 0) minH = (euint32)etk_round((double)min_h);
	if(max_h >= 0) maxH = (euint32)etk_round((double)max_h);
	if(min_v >= 0) minV = (euint32)etk_round((double)min_v);
	if(max_v >= 0) maxV = (euint32)etk_round((double)max_v);

	if(fWindow->SetSizeLimits(minH, maxH, minV, maxV) == E_OK)
	{
		if(min_h >= 0 || min_v >= 0)
		{
			ERect r = fFrame;
			if(r.Width() < min_h && min_h >= 0) r.right = r.left + min_h;
			if(r.Height() < min_v && min_v >= 0) r.bottom = r.top + min_v;

			if(r != fFrame) ResizeTo(r.Width(), r.Height());
		}
	}
}


void
EWindow::GetSizeLimits(float *min_h, float *max_h, float *min_v, float *max_v) const
{
	euint32 minH = E_MAXUINT32, maxH = E_MAXUINT32, minV = E_MAXUINT32, maxV = E_MAXUINT32;
	fWindow->GetSizeLimits(&minH, &maxH, &minV, &maxV);

	if(min_h) *min_h = (minH != E_MAXUINT32 ? (float)minH : -1.f);
	if(max_h) *max_h = (maxH != E_MAXUINT32 ? (float)maxH : -1.f);
	if(min_v) *min_v = (minV != E_MAXUINT32 ? (float)minV : -1.f);
	if(max_v) *max_v = (maxV != E_MAXUINT32 ? (float)maxV : -1.f);
}


e_status_t
EWindow::SendBehind(const EWindow *win)
{
	if(fWindow == NULL) return E_ERROR;
	if(win)
	{
		if(win->fWindow == NULL) return E_ERROR;
		return fWindow->Lower(win->fWindow);
	}

	return fWindow->Raise();
}


#if 0
e_status_t
EWindow::SendFront(const EWindow *win)
{
	if(fWindow == NULL) return E_ERROR;
	if(win)
	{
		if(win->fWindow == NULL) return E_ERROR;
		return win->fWindow->Lower(fWindow);
	}

	return fWindow->Lower(NULL);
}
#endif


bool
EWindow::_GrabMouse()
{
	if(fMouseGrabCount == 0)
	{
		if(fWindow->GrabMouse() != E_OK)
		{
			ETK_DEBUG("[INTERFACE]: Mouse grab failed.");
			return false;
		}
	}

	if(fMouseGrabCount < E_MAXUINT32)
	{
		fMouseGrabCount++;
//		ETK_DEBUG("[INTERFACE]: Mouse grabbed (%u).", fMouseGrabCount);
		return true;
	}

	return false;
}


bool
EWindow::_GrabKeyboard()
{
	if(fKeyboardGrabCount == 0)
	{
		if(fWindow->GrabKeyboard() != E_OK)
		{
			ETK_DEBUG("[INTERFACE]: Keyboard grab failed.");
			return false;
		}
	}

	if(fKeyboardGrabCount < E_MAXUINT32)
	{
		fKeyboardGrabCount++;
//		ETK_DEBUG("[INTERFACE]: Keyboard grabbed (%u).", fKeyboardGrabCount);
		return true;
	}

	return false;
}


void
EWindow::_UngrabMouse()
{
	if(fMouseGrabCount == 0) return;
	fMouseGrabCount--;
	if(fMouseGrabCount == 0) fWindow->UngrabMouse();
//	ETK_DEBUG("[INTERFACE]: Mouse ungrabbed (%u).", fMouseGrabCount);
}


void
EWindow::_UngrabKeyboard()
{
	if(fKeyboardGrabCount == 0) return;
	fKeyboardGrabCount--;
	if(fKeyboardGrabCount == 0) fWindow->UngrabKeyboard();
//	ETK_DEBUG("[INTERFACE]: Keyboard ungrabbed (%u).", fKeyboardGrabCount);
}


bool
EWindow::GrabMouse()
{
	return _GrabMouse();
}


bool
EWindow::GrabKeyboard()
{
	return _GrabKeyboard();
}


void
EWindow::UngrabMouse()
{
	_UngrabMouse();
}


void
EWindow::UngrabKeyboard()
{
	_UngrabKeyboard();
}


bool
EWindow::IsMouseGrabbed() const
{
	return(fMouseGrabCount > 0);
}


bool
EWindow::IsKeyboardGrabbed() const
{
	return(fKeyboardGrabCount > 0);
}


void
EWindow::SetPulseRate(e_bigtime_t rate)
{
	if(fPulseRunner == NULL)
	{
		ETK_DEBUG("[INTERFACE]: %s --- No message runner.", __PRETTY_FUNCTION__);
		return;
	}

	if(fPulseRunner->SetInterval(rate) == E_OK)
	{
		fPulseRate = rate;
		fPulseRunner->SetCount((rate > E_INT64_CONSTANT(0) && fNeededToPulseViews.CountItems() > 0 && !IsHidden()) ? -1 : 0);
	}
	else
	{
		ETK_DEBUG("[INTERFACE]: %s --- Unable to set pulse rate.", __PRETTY_FUNCTION__);
	}
}


e_bigtime_t
EWindow::PulseRate() const
{
	if(fPulseRunner == NULL) return E_INT64_CONSTANT(-1);
	return fPulseRate;
}


const char*
EWindow::Title() const
{
	return fWindowTitle;
}


void
EWindow::SetTitle(const char *title)
{
	EString str(title);

	if(str != fWindowTitle)
	{
		if(fWindowTitle) delete[] fWindowTitle;
		fWindowTitle = (str.String() == NULL ? NULL : EStrdup(str.String()));

		fWindow->SetTitle(fWindowTitle);
	}
}

