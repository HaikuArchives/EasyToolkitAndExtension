#ifndef __LITE_BEAPI_BITMAP_H__
#define __LITE_BEAPI_BITMAP_H__

#include <be/interface/GraphicsDefs.h>
#include <be/interface/Window.h>

#ifdef __cplusplus

namespace Lite_BeAPI {

class BBitmap : public EBitmap {
public:
	BBitmap(BRect bounds,
		color_space depth,
		bool accepts_views = false,
		bool need_contiguous = false);
	virtual ~BBitmap();

	void	SetBits(const void *data,
			int32 length,
			int32 offset,
			color_space cs);
};


inline BBitmap::BBitmap(BRect bounds,
			color_space depth,
			bool accepts_views,
			bool need_contiguous)
	: EBitmap(bounds, true)
{
	bounds.OffsetTo(B_ORIGIN);
	Lock();
	AddChild(new BView(bounds, NULL, B_FOLLOW_NONE, 0));
	ChildAt(0)->SetHighColor(255, 255, 255);
	ChildAt(0)->FillRect(bounds);
	Unlock();
}


inline BBitmap::~BBitmap()
{
}


inline void
BBitmap::SetBits(const void *data, int32 length, int32 offset, color_space cs)
{
	// TODO
	ETK_WARNING("[LITE_BEAPI]: %s --- TODO", __PRETTY_FUNCTION__);
}

} // namespace Lite_BeAPI

using namespace Lite_BeAPI;

#endif /* __cplusplus */

#endif /* __LITE_BEAPI_BITMAP_H__ */

