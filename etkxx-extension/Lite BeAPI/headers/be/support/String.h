#ifndef __LITE_BEAPI_STRING_H__
#define __LITE_BEAPI_STRING_H__

#include <be/support/SupportDefs.h>

#ifdef __cplusplus

// class
#define BString			EString

#endif /* __cplusplus */

#endif /* __LITE_BEAPI_STRING_H__ */

#if defined(_WIN32) && defined(__GNUC__)
#include_next <string.h>
#endif

