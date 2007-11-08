#ifndef __LITE_BEAPI_GRAPHICS_DEFS_H__
#define __LITE_BEAPI_GRAPHICS_DEFS_H__

#include <be/support/SupportDefs.h>

#define pattern				e_pattern
#define rgb_color			e_rgb_color
#define color_space			e_color_space

#define B_SOLID_HIGH			E_SOLID_HIGH
#define B_SOLID_LOW			E_SOLID_LOW
#define B_MIXED_COLORS			E_MIXED_COLORS

#define B_TRANSPARENT_COLOR		E_TRANSPARENT_COLOR
#define B_TRANSPARENT_MAGIC_RGBA32	E_TRANSPARENT_MAGIC_RGBA32

/* drawing mode */
#define B_OP_COPY			E_OP_COPY
#define B_OP_OVER			E_OP_OVER
#define B_OP_ERASE			E_OP_ERASE
#define B_OP_INVERT			E_OP_INVERT
#define B_OP_ADD			E_OP_ADD
#define B_OP_SUBTRACT			E_OP_SUBTRACT
#define B_OP_BLEND			E_OP_BLEND
#define B_OP_MIN			E_OP_MIN
#define B_OP_MAX			E_OP_MAX
#define B_OP_SELECT			E_OP_SELECT
#define B_OP_ALPHA			E_OP_ALPHA

/* color space */
#define B_CMAP8				E_CMAP8
#define B_RGB32				E_RGB32
#define B_RGBA32			E_RGBA32
#define B_RGB24				E_RGB24
#define B_RGB24_BIG			E_RGB24_BIG
#define B_COLOR_8_BIT			E_CMAP8


#endif /* __LITE_BEAPI_GRAPHICS_DEFS_H__ */

