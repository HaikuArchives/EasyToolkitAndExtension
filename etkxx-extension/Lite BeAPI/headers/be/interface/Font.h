#ifndef __LITE_BEAPI_FONT_H__
#define __LITE_BEAPI_FONT_H__

#include <be/interface/InterfaceDefs.h>

/* others */

#define B_FONT_FAMILY_LENGTH			E_FONT_FAMILY_LENGTH
#define B_FONT_STYLE_LENGTH			E_FONT_STYLE_LENGTH
#define font_family				e_font_family
#define font_style				e_font_style
#define font_height				e_font_height

#define count_font_families()			etk_count_font_families()
#define count_font_styles(family)		etk_count_font_styles((const char*)&family[0])

#ifdef __cplusplus

#define be_plain_font				(reinterpret_cast<const BFont*>((void*)etk_plain_font))
#define be_bold_font				(reinterpret_cast<const BFont*>((void*)etk_bold_font))
#define be_fixed_font				(reinterpret_cast<const BFont*>((void*)etk_fixed_font))

namespace Lite_BeAPI {

class BFont : public EFont {
public:
	BFont();
	BFont(const BFont *font);
	BFont(const BFont &font);
	virtual ~BFont();

	BFont	&operator=(const BFont &font);
	bool	operator==(const BFont &font);
	bool	operator!=(const BFont &font);

	void	GetStringWidths(const char *stringArray[],
				const int32 lengthArray[],
				int32 numStrings,
				float widthArray[]) const;
};


inline BFont::BFont()
	: EFont()
{
}


inline BFont::BFont(const BFont *font)
	: EFont(font)
{
}


inline BFont::BFont(const BFont &font)
	: EFont(font)
{
}


inline BFont::~BFont()
{
}


inline BFont&
BFont::operator=(const BFont &font)
{
	EFont::operator=(font);
	return *this;
}


inline bool
BFont::operator==(const BFont &font)
{
	return EFont::operator==(font);
}


inline bool
BFont::operator!=(const BFont &font)
{
	return EFont::operator!=(font);
}


inline void
BFont::GetStringWidths(const char *stringArray[],
		       const int32 lengthArray[],
		       int32 numStrings,
		       float widthArray[]) const
{
	for(int32 i = 0; i < numStrings; i++)
		widthArray[i] = StringWidth(*(stringArray++), lengthArray[i]);
}


inline status_t get_font_family(int32 index, font_family *name, uint32 *flags = NULL)
{
	const char *tmp = NULL;

	if(etk_get_font_family(index, &tmp) != B_OK) return B_ERROR;

	if(flags != NULL) *flags = 0;
	if(name != NULL)
	{
		if(tmp == NULL)
			*((char *)(*name)) = 0;
		else
			memcpy(*name, tmp, strlen(tmp) + 1);
	}
	return B_OK;
}


inline status_t get_font_style(font_family family, int32 index, font_style *name, uint32 *flags = NULL)
{
	const char *tmp = NULL;

	if(etk_get_font_style((const char*)&family[0], index, &tmp) != B_OK) return B_ERROR;
	if(flags != NULL) *flags = 0;
	if(name != NULL)
	{
		if(tmp == NULL)
			*((char *)(*name)) = 0;
		else
			memcpy(*name, tmp, strlen(tmp) + 1);
	}
	return B_OK;
}


inline status_t get_font_style(font_family family, int32 index, font_style *name, uint16 *face, uint32 *flags = NULL)
{
	if(get_font_style(family, index, name, flags) != B_OK) return B_ERROR;
	if(face != NULL) *face = 0;
	return B_OK;
}

} // namespace Lite_BeAPI

using namespace Lite_BeAPI;

#endif /* __cplusplus */

#endif /* __LITE_BEAPI_FONT_H__ */

