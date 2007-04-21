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
 * File: Bitmap.cpp
 * Description: EBitmap --- a rectangular image for drawing
 * Warning: Unfinished.
 *
 * --------------------------------------------------------------------------*/


#include <etk/add-ons/graphics/GraphicsEngine.h>
#include <etk/app/Application.h>
#include <etk/render/Pixmap.h>

#include "Bitmap.h"


EBitmap::EBitmap(ERect bounds, bool acceptsViews)
	: EArchivable(), fRows(0), fColumns(0), fPixmap(NULL)
{
	if(bounds.IsValid() == false) return;

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
	{
		ETK_WARNING("[INTERFACE]: %s --- You should create EBitmap within graphics engine.", __PRETTY_FUNCTION__);
		return;
	}

	if(acceptsViews)
	{
		// TODO
		ETK_WARNING("[INTERFACE]: %s --- AcceptsViews not supported yet.", __PRETTY_FUNCTION__);
		return;
	}

	fColumns = (euint32)bounds.IntegerWidth() + 1;
	fRows = (euint32)bounds.IntegerHeight() + 1;

	fPixmap = etk_app->fGraphicsEngine->CreatePixmap(fColumns - 1, fRows - 1);
}


EBitmap::EBitmap(const EBitmap *bitmap, bool acceptsViews)
	: EArchivable(), fRows(0), fColumns(0), fPixmap(NULL)
{
	if(bitmap == NULL || bitmap->fPixmap == NULL) return;

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
	{
		ETK_WARNING("[INTERFACE]: %s --- You should create EBitmap within graphics engine.", __PRETTY_FUNCTION__);
		return;
	}

	if(acceptsViews)
	{
		// TODO
		ETK_WARNING("[INTERFACE]: %s --- AcceptsViews not supported yet.", __PRETTY_FUNCTION__);
		return;
	}

	fColumns = bitmap->fColumns;
	fRows = bitmap->fRows;

	if((fPixmap = etk_app->fGraphicsEngine->CreatePixmap(fColumns - 1, fRows - 1)) != NULL)
		bitmap->fPixmap->CopyTo(fPixmap, 0, 0, fColumns - 1, fRows - 1, 0, 0, fColumns - 1, fRows - 1, 255);
}


EBitmap::EBitmap(const EPixmap *pixmap, bool acceptsViews)
	: EArchivable(), fRows(0), fColumns(0), fPixmap(NULL)
{
	if(pixmap == NULL || pixmap->IsValid() == false || pixmap->Bounds().IsValid() == false) return;

	if(etk_app == NULL || etk_app->fGraphicsEngine == NULL)
	{
		ETK_WARNING("[INTERFACE]: %s --- You should create EBitmap within graphics engine.", __PRETTY_FUNCTION__);
		return;
	}

	if(acceptsViews)
	{
		// TODO
		ETK_WARNING("[INTERFACE]: %s --- AcceptsViews not supported yet.", __PRETTY_FUNCTION__);
		return;
	}

	fColumns = (euint32)pixmap->Bounds().IntegerWidth() + 1;
	fRows = (euint32)pixmap->Bounds().IntegerHeight() + 1;

	if((fPixmap = etk_app->fGraphicsEngine->CreatePixmap(fColumns - 1, fRows - 1)) != NULL)
	{
		EGraphicsContext *dc = etk_app->fGraphicsEngine->CreateContext();
		if(dc)
		{
			dc->SetDrawingMode(E_OP_COPY);
			dc->SetHighColor(0, 0, 0, 255);
			dc->SetClipping(ERegion(pixmap->Bounds()));
			fPixmap->DrawPixmap(dc, pixmap, 0, 0, fColumns - 1, fRows - 1, 0, 0, fColumns - 1, fRows - 1);
			delete dc;
		}
	}
}


EBitmap::~EBitmap()
{
	if(fPixmap) delete fPixmap;
}


e_status_t
EBitmap::InitCheck() const
{
	return(fPixmap != NULL ? E_OK : E_ERROR);
}


bool
EBitmap::IsValid() const
{
	return(fPixmap != NULL);
}


ERect
EBitmap::Bounds() const
{
	return ERect(0.f, 0.f, (float)fColumns - 1.f, (float)fRows - 1.f);
}

