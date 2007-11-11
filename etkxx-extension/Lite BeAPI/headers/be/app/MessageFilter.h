#ifndef __LITE_BEAPI_MESSAGE_FILTER_H__
#define __LITE_BEAPI_MESSAGE_FILTER_H__

#include <be/app/Handler.h>

#ifdef __cplusplus

// class
#define BMessageFilter			EMessageFilter

#define filter_hook			e_filter_hook

#endif /* __cplusplus */

/* others */

#define filter_result			e_filter_result
#define B_SKIP_MESSAGE			E_SKIP_MESSAGE
#define B_DISPATCH_MESSAGE		E_DISPATCH_MESSAGE
#define B_ANY_DELIVERY			E_ANY_DELIVERY

#define message_delivery		e_message_delivery
#define B_DROPPED_DELIVERY		E_DROPPED_DELIVERY
#define B_PROGRAMMED_DELIVERY		E_PROGRAMMED_DELIVERY

#define message_source			e_message_source
#define B_ANY_SOURCE			E_ANY_SOURCE
#define B_REMOTE_SOURCE			E_REMOTE_SOURCE
#define B_LOCAL_SOURCE			E_LOCAL_SOURCE

#endif /* __LITE_BEAPI_MESSAGE_FILTER_H__ */

