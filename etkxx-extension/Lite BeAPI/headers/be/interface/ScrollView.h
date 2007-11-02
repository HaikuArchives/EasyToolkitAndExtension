#include <be/interface/ScrollBar.h>

#ifndef __LITE_BEAPI_SCROLL_VIEW_H__
#define __LITE_BEAPI_SCROLL_VIEW_H__

#ifdef __cplusplus

// class
class BScrollView : public EScrollView {
public:
	BScrollView(const char *name,
		    BView *target,
		    uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		    uint32 flags = 0,
		    bool horizontal = false,
		    bool vertical = false,
		    border_style border = B_FANCY_BORDER);
	virtual ~BScrollView();
};


inline BScrollView::BScrollView(const char *name,
				BView *target,
				uint32 resizeMask,
				uint32 flags,
				bool horizontal,
				bool vertical,
				border_style border)
	: EScrollView(target == NULL ? BRect(0, 0, 1, 1) : target->Frame(),
		      name,
		      target,
		      resizeMask, flags,
		      horizontal, vertical,
		      border)
{
	if(target != NULL) target->MoveTo(B_ORIGIN);
}


inline BScrollView::~BScrollView()
{
}


#endif /* __cplusplus */

#endif /* __LITE_BEAPI_SCROLL_VIEW_H__ */

