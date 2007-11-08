#ifndef __LITE_BEAPI_APP_DEFS_H__
#define __LITE_BEAPI_APP_DEFS_H__

#include <be/support/SupportDefs.h>
#include <be/app/Cursor.h>

#define B_ABOUT_REQUESTED		E_ABOUT_REQUESTED
#define B_QUIT_REQUESTED		E_QUIT_REQUESTED

#define B_CURSOR_SYSTEM_DEFAULT		E_CURSOR_SYSTEM_DEFAULT
#define B_CURSOR_I_BEAM			E_CURSOR_I_BEAM
#define B_HAND_CURSOR			((const uchar *)(E_CURSOR_HAND->Data()))
#define B_I_BEAM_CURSOR			((const uchar *)(E_CURSOR_I_BEAM->Data()))


#endif /* __LITE_BEAPI_APP_DEFS_H__ */

