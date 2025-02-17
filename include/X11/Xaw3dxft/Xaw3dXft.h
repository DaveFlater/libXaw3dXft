/* Xaw3dXft.h,v 1.0 10/11/07 08:07:27 demailly Exp $ */

/***********************************************************

Copyright (c) 2010  J.-P. Demailly

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef _Xaw3dXft_h
#define _Xaw3dXft_h

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dxft/XawInit.h>

#define XtN3dxftData "3dxftData"
#define XtN3dxftProc "3dxftProc"

#define GET_XAW3DXFT_DATA(x) XawInitializeWidgetSet(); \
    x = (Xaw3dXftData *)XrmQuarkToString(XrmStringToQuark("Xaw3dXftData")+1); \
    if (x) x = (Xaw3dXftData *) strtoul((char *)((char *)x+4), NULL, 10);

typedef struct _Xaw3dXftProc {
    void (* set_default_hilit_color)(void);
    void (* set_hilit_color)(char * value);
    void (* set_default_fontname)(char *name);
    void (* set_insensitive_twist)(char *value);
    XftFont * (* get_font)(Display *dpy, char *name);
    int (* text_width)(Widget w, XftFont *font, char *str, int len);
    void (* draw_string)(Widget w, XftFont *font, int x, int y, char *str, int len);
#ifdef XAW_ARROW_SCROLLBARS
    Widget (* get_scrollbar)(Widget w, XtPointer ptr);
    void (* handle_mousewheel)(Widget w, XtPointer ptr, XEvent *event, Boolean * flg);
    void (* set_mousewheel_handler)(Widget w, XtPointer ptr);
    void (* set_mousewheel_steps )(Widget w, XtPointer ptr, int n);
#endif  
} Xaw3dXftProc;

typedef struct _Xaw3dXftData {
    char encoding;
    char string_hilight;
    char text_bg_hilight;
    char string_use_pixmap;
    char menu_spacing;
    char show_tips;
    char tip_do_grab;
    char border_hack;
    char no_hilit_reverse;
    char button_inverse;
    char button_dashed;
    char multi_column_menu;
    char edit_delete_alternative;
    char text_sb_right;  
    unsigned short insensitive_twist[4];
    Xaw3dXftProc * proc;
    Pixel tip_background_color;
    Pixel text_bg_hilight_color;
    Pixel text_fg_alternate_color;
    char * hilit_color;
    char * default_fontname;
    XftFont * default_font;
} Xaw3dXftData;

#ifdef __cplusplus
}
#endif

#endif
