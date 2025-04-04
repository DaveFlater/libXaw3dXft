/*
 * Copyright (c) 1999 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

#ifndef _XawTipP_h
#define _XawTipP_h

#include "Xaw3dP.h"
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dxft/Tip.h>

typedef struct {
    XtPointer extension;
} TipClassPart;

typedef struct _TipClassRec {
    CoreClassPart core_class;
    TipClassPart tip_class;
} TipClassRec;

extern TipClassRec tipClassRec;

typedef struct _TipPart {
    /* resources */
    Pixel foreground;
    XFontStruct	*font;
#ifdef XAW_INTERNATIONALIZATION
    XFontSet fontset;
#endif
    Dimension internal_width;
    Dimension internal_height;
    String label;
    int backing_store;
    int timeout;
    char *xftfontname;

    /* private */
    GC gc;
    XtIntervalId timer;
#ifdef XAW_INTERNATIONALIZATION
    Boolean international;
#endif
    unsigned char encoding;
    XftFont *xftfont;
} TipPart;

typedef struct _TipRec {
    CorePart core;
    TipPart tip;
} TipRec;

#endif /* _XawTipP_h */
