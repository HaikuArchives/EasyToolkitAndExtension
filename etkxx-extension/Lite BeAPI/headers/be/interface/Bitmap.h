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

	void		*Bits() const;
	void		SetBits(const void *data,
				int32 length,
				int32 offset,
				color_space cs);
	int32		BitsLength() const;
	int32		BytesPerRow() const;
	color_space	ColorSpace() const;
};


inline BBitmap::BBitmap(BRect bounds,
			color_space depth,
			bool accepts_views,
			bool need_contiguous)
	: EBitmap(bounds, true)
{
#if 0
	bounds.OffsetTo(B_ORIGIN);
	Lock();
	AddChild(new BView(bounds, NULL, B_FOLLOW_NONE, 0));
	ChildAt(0)->SetHighColor(255, 255, 255);
	ChildAt(0)->FillRect(bounds);
	Unlock();
#endif
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


inline void*
BBitmap::Bits() const
{
	// TODO
	ETK_WARNING("[LITE_BEAPI]: %s --- TODO", __PRETTY_FUNCTION__);
	return NULL;
}


inline int32
BBitmap::BitsLength() const
{
	// TODO
	ETK_WARNING("[LITE_BEAPI]: %s --- TODO", __PRETTY_FUNCTION__);
	return 0;
}


inline int32
BBitmap::BytesPerRow() const
{
	// TODO
	ETK_WARNING("[LITE_BEAPI]: %s --- TODO", __PRETTY_FUNCTION__);
	return 0;
}


inline color_space
BBitmap::ColorSpace() const
{
	// TODO
	ETK_WARNING("[LITE_BEAPI]: %s --- TODO", __PRETTY_FUNCTION__);
	return B_RGB32;
}

} // namespace Lite_BeAPI

using namespace Lite_BeAPI;

#endif /* __cplusplus */

#endif /* __LITE_BEAPI_BITMAP_H__ */

