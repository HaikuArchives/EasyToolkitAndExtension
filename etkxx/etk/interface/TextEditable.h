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
 * File: TextEditable.h
 * Description: ETextEditable --- a single-line editable field
 * 
 * --------------------------------------------------------------------------*/

#ifndef __ETK_TEXT_EDITABLE_H__
#define __ETK_TEXT_EDITABLE_H__

#include <etk/interface/InterfaceDefs.h>
#include <etk/interface/Control.h>

#ifdef __cplusplus /* Just for C++ */

class _IMPEXP_ETK ETextEditable : public EControl {
public:
	ETextEditable(ERect frame,
		      const char *name,
		      const char *initial_text,
		      EMessage *message,
		      euint32 resizeMode = E_FOLLOW_LEFT | E_FOLLOW_TOP,
		      euint32 flags = E_WILL_DRAW | E_FRAME_EVENTS);
	virtual ~ETextEditable();

	void		MakeEditable(bool editable);
	bool		IsEditable() const;

	void		SetPosition(eint32 pos); // UTF-8
	eint32		Position() const; // UTF-8

	virtual void	SetText(const char *text);
	void		SetText(const EString &text);
	const char	*Text() const;
	char		*DuplicateText(eint32 startPos, eint32 endPos); // UTF-8, return value must free by "free"

	void		SetMaxChars(eint32 max);
	eint32		MaxChars() const;

	// HideTyping(): flag
	// 	0x00(false)	--- show everything
	// 	0x20 ~ 0x7e	--- replace the characters with this
	// 	others(true)	--- invisible
	void		HideTyping(euint8 flag = 0x2a);
	euint8		IsTypingHidden() const;

	void		InsertText(const char *text, eint32 nChars = -1, eint32 position = -1); // UTF-8
	void		RemoveText(eint32 startPos, eint32 endPos); // UTF-8, include endPos
	void		MakeEmpty();

	void		MakeSelectable(bool selectable);
	bool		IsSelectable() const;

	virtual void	Select(eint32 startPos, eint32 endPos); // UTF-8, include endPos
	bool		GetSelection(eint32 *startPos, eint32 *endPos) const; // UTF-8, include endPos
	void		SelectAll();
	void		Deselect();
	bool		IsSelected() const;

	void		SetTextAlignment(e_alignment alignment);
	e_alignment	TextAlignment() const;

	virtual void	SetMargins(float left, float top, float right, float bottom);
	void		GetMargins(float *left, float *top, float *right, float *bottom) const;

	virtual void	Draw(ERect updateRect);
	virtual void	FrameResized(float new_width, float new_height);
	virtual void	MouseDown(EPoint where);
	virtual void	MouseUp(EPoint where);
	virtual void	MouseMoved(EPoint where, euint32 code, const EMessage *a_message);
	virtual void	WindowActivated(bool state);
	virtual void	KeyDown(const char *bytes, eint32 numBytes);
	virtual void	KeyUp(const char *bytes, eint32 numBytes);
	virtual void	SetFont(const EFont *font, euint8 mask = E_FONT_ALL);
	virtual void	GetPreferredSize(float *width, float *height);
	virtual void	MakeFocus(bool focusState = true);
	virtual void	MessageReceived(EMessage *msg);

private:
	char *fText;
	bool fEditable;
	bool fSelectable;
	e_alignment fAlignment;
	eint32 fPosition;
	eint32 fSelectStart;
	eint32 fSelectEnd;
	ERect fMargins;
	float *fCharWidths;
	eint32 fCount;
	float locationOffset;
	eint32 fSelectTracking;
	eint32 fMaxChars;
	euint8 fTypingHidden;

	void DrawSelectedBackground(ERect updateRect);
	void DrawCursor();

	bool GetCharLocation(eint32 pos, float *x, float *y, EFont *font = NULL);
	float _StringWidth(const EFont &font, const char *str) const;
	float *_CharWidths(const EFont &font, const char *str, eint32 *count) const;
	void _DrawString(const char *str, EPoint location);
};


inline void ETextEditable::MakeEmpty()
{
	SetText(NULL);
}


inline void ETextEditable::SelectAll()
{
	Select(0, -1);
}


inline void ETextEditable::Deselect()
{
	Select(-1, 0);
}


inline bool ETextEditable::IsSelected() const
{
	return GetSelection(NULL, NULL);
}


#endif /* __cplusplus */

#endif /* __ETK_TEXT_EDITABLE_H__ */

