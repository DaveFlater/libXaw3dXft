/*
Copyright (c) 1989, 1994  X Consortium

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

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 * Author:  Chris D. Peterson, MIT X Consortium
 */

/*
 * SmeP.h - Private definitions for Sme object
 *
 */

#ifndef _XawSmeBSBP_h
#define _XawSmeBSBP_h

/***********************************************************************
 *
 * Sme Object Private Data
 *
 ***********************************************************************/

#include "Xaw3dP.h"
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dXft/SmeThreeDP.h>
#include <X11/Xaw3dXft/SmeBSB.h>

/************************************************************
 *
 * New fields for the Sme Object class record.
 *
 ************************************************************/

typedef struct _SmeBSBClassPart {
  XtPointer extension;
} SmeBSBClassPart;

/* Full class record declaration */
typedef struct _SmeBSBClassRec {
    RectObjClassPart	rect_class;
    SmeClassPart	sme_class;
    SmeThreeDClassPart	sme_threeD_class;
    SmeBSBClassPart	sme_bsb_class;
} SmeBSBClassRec;

extern SmeBSBClassRec smeBSBClassRec;

/* New fields for the Sme Object record */
static_assert(Got_XAW_defines);
typedef struct {
    /* resources */
    String label;		/* The entry label. */
    int vert_space;		/* extra vert space to leave, as a percentage
				   of the font height of the label. */
    Pixmap left_bitmap, right_bitmap; /* pixmaps to show. */
    Dimension left_margin, right_margin; /* left and right margins. */
    Pixel foreground;		/* foreground color. */
    Pixel highlight;            /* highlight background color */
    XFontStruct * font;		/* The font to show label in. */
#ifdef XAW_INTERNATIONALIZATION
    XFontSet fontset;		/* or fontset */
#endif
    XtJustify justify;		/* Justification for the label. */
    int underline;		/* index of letter to underline in label. */
    char *xftfontname;
    unsigned char highlightStyle;
    unsigned char encoding;

    /* private resources. */
    // Unlike Command/Toggle, we never have to deal with reverse color &
    // insensitive at the same time.
    GC normal_GC;               // fg, font
    GC rev_GC;                  // foreground = bg, font
    GC stipple_GC;              // FillStippled with bg
    GC xor_fgbg_GC;             // function = GXxor by fg ^ bg
    GC xor_bghl_GC;             // function = GXxor by bg ^ hl
    XftColor xftfg;
    XftColor xftbg;
    XftColor xfthl;
    Boolean xorSet;
    Dimension label_width;
    Dimension label_height;
    Dimension left_bitmap_width; /* size and depth of each pixmap. */
    Dimension left_bitmap_height;
    Dimension right_bitmap_width;
    Dimension right_bitmap_height;
    Cardinal left_depth;
    Cardinal right_depth;
    String menu_name;		/* name of nested sub-menu or NULL */
    XftFont *xftfont;
    Visual *visual;
} SmeBSBPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _SmeBSBRec {
    ObjectPart		object;
    RectObjPart		rectangle;
    SmePart		sme;
    SmeThreeDPart	sme_threeD;
    SmeBSBPart		sme_bsb;
} SmeBSBRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

#endif /* _XawSmeBSBP_h */
