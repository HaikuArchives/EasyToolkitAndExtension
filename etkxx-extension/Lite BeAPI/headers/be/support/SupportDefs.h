#ifndef __LITE_BEAPI_SUPPORT_DEFS_H__
#define __LITE_BEAPI_SUPPORT_DEFS_H__

#include <etkxx.h>

#if !(ETK_MAJOR_VERSION > 0 || ETK_MINOR_VERSION > 3 || ETK_MICRO_VERSION >= 4)
	#error "Lite BeAPI requires ETK++ >= 0.3.4"
#endif

#ifdef ETK_LITTLE_ENDIAN
	#define __INTEL__
#else
	#define __POWERPC__
#endif

#define __LITE_BEAPI__

#include <be/support/Errors.h>


#ifndef ulong
#define ulong		unsigned long
#endif

#ifndef uint
#define uint		unsigned int
#endif

#ifndef ushort
#define ushort		unsigned short
#endif

#ifndef uchar
#define uchar		unsigned char
#endif

#define int8		eint8
#define uint8		euint8

#define int16		eint16
#define uint16		euint16

#define int32		eint32
#define uint32		euint32

#define int64		eint64
#define uint64		euint64

#define unichar		eunichar

#define status_t	e_status_t
#define bigtime_t	e_bigtime_t
#define type_code	e_type_code

#endif /* __LITE_BEAPI_SUPPORT_DEFS_H__ */

