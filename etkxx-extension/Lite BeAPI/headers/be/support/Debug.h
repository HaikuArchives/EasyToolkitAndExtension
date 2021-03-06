#ifndef __LITE_BEAPI_DEBUG_H__
#define __LITE_BEAPI_DEBUG_H__

#include <be/support/SupportDefs.h>

#define _debugPrintf	ETK_DEBUG

namespace Lite_BeAPI {

inline void _debuggerAssert(const char *filename, int lineno, const char *msg)
{
	ETK_ERROR("File: %s, Line: %d --- %s", filename, lineno, msg);
}

} // namespace Lite_BeAPI

using namespace Lite_BeAPI;

#if DEBUG
	/* TODO */
#else /* !DEBUG */
	/* TODO */
#endif /* DEBUG */

#endif /* __LITE_BEAPI_DEBUG_H__ */

