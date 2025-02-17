/* Xaw3dXftP.h,v 1.0 10/11/07 08:07:27 demailly Exp $ */

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

#include <X11/Xaw3dxft/Xaw3dXft.h>

#ifndef _Xaw3dXftP_h
#define _Xaw3dXftP_h

#ifdef __cplusplus
extern "C" {
#endif

extern Xaw3dXftData * _Xaw3dXft;

extern void Xaw3dXftSetDefaultHilitColor(void);
extern void Xaw3dXftSetHilitColor(char * value);
extern void Xaw3dXftSetDefaultFontName(char *name);
extern void Xaw3dXftSetInsensitiveTwist(char *value);
extern XftFont * Xaw3dXftGetFont(Display *dpy, char *name);

extern int Xaw3dXftTextWidth(Widget w, XftFont *font, char *str, int len);
extern void Xaw3dXftDrawString(Widget w, XftFont *font, int x, int y, char *str, int len);

#ifdef XAW_ARROW_SCROLLBARS
extern Widget Xaw3dXftGetScrollbar(Widget w, XtPointer ptr);
extern void Xaw3dXftHandleMouseWheel (Widget w, XtPointer ptr, XEvent *event, Boolean * flg);
extern void Xaw3dXftSetMouseWheelHandler(Widget w, XtPointer ptr);
extern void Xaw3dXftSetMouseWheelSteps (Widget w, XtPointer ptr, int n);
#endif

extern void Xaw3dXftInitialize(void);

#ifdef __cplusplus
}
#endif

#endif
