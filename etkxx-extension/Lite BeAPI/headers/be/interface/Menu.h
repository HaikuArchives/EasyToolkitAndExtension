#ifndef __LITE_BEAPI_MENU_H__
#define __LITE_BEAPI_MENU_H__

#include <be/support/List.h>
#include <be/app/Invoker.h>
#include <be/interface/View.h>

#ifdef __cplusplus

// class
#define BMenu			EMenu
#define BMenuItem		EMenuItem
#define BMenuField		EMenuField
#define BMenuBar		EMenuBar
#define BPopUpMenu		EPopUpMenu

#endif /* __cplusplus */

/* others */
#define menu_layout		e_menu_layout
#define B_ITEMS_IN_ROW		E_ITEMS_IN_ROW
#define B_ITEMS_IN_COLUMN	E_ITEMS_IN_COLUMN
#define B_ITEMS_IN_MATRIX	E_ITEMS_IN_MATRIX

#define menu_bar_border		e_menu_bar_border
#define B_BORDER_FRAME		E_BORDER_FRAME
#define B_BORDER_CONTENTS	E_BORDER_CONTENTS
#define B_BORDER_EACH_ITEM	E_BORDER_EACH_ITEM

#endif /* __LITE_BEAPI_MENU_H__ */

