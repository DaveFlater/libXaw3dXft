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
*/


/*
 * ListP.h - Private definitions for List widget
 *
 * This is a List widget.  It allows the user to select an item in a list and
 * notifies the application through a callback function.
 *
 *	Created: 	8/13/88
 *	By:		Chris D. Peterson
 *                      MIT - Project Athena
 */

#ifndef _XawListP_h
#define _XawListP_h

/***********************************************************************
 *
 * List Widget Private Data
 *
 ***********************************************************************/

#include "Xaw3dP.h"
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dXft/SimpleP.h>
#include <X11/Xaw3dXft/List.h>

#define NO_HIGHLIGHT            XAW_LIST_NONE
#define OUT_OF_RANGE            -1
#define OKAY                     0

/* New fields for the List widget class record */

typedef struct {int foo;} ListClassPart;

/* Full class record declaration */
typedef struct _ListClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    ListClassPart	list_class;
} ListClassRec;

extern ListClassRec listClassRec;

/* New fields for the List widget record */
typedef struct {
    /* resources */
    // The ints here want to be Cardinal or Dimension, but the types are
    // kept consistent with Xaw.
    Pixel	foreground;
    Pixel       highlight;      /* highlight background color */
    Dimension	internal_width, /* if not 3d, user sets directly. */
        	internal_height,
                column_space,	/* half of *_space is add on top/bot/left of*/
                row_space;	/* each item's text bounding box. half added to longest for right */
    int         default_cols;   // Number of columns, Cardinal
    Boolean     force_cols,
                paste,
                vertical_cols;
    int         longest;	/* in pixels, Dimension */
    int         nitems;		/* number of items in the list, Cardinal */
    XFontStruct	*font;
    XFontSet 	fontset;	/* Sheeran, Omron KK, 93/03/05 */
    String *    list;		/* for i18n, always in multibyte format */
    XtCallbackList callback;
    char *	xftfontname;
    unsigned char highlightStyle;
    unsigned char encoding;

    /* private state */
    GC normal_GC;               // fg, font
    GC rev_GC;                  // foreground = bg, font
    GC stipple_GC;              // FillStippled with bg
    GC xor_fgbg_GC;             // function = GXxor by fg ^ bg
    GC xor_bghl_GC;             // function = GXxor by bg ^ hl
    XftColor xftfg;
    XftColor xftbg;
    Dimension   col_width,	/* width of each column. */
                row_height;	/* height of each row. */
    Cardinal    nrows,		/* number of rows in the list. */
                ncols;		/* number of columns in the list. */
    int is_highlighted,	   /* index of the item currently highlighted or -1 */
        want_highlighted;  /* item that should be highlighted or -1 */

    int         freedoms;       /* flags for resizing height and width */
    XftFont *	xftfont;
    Visual *    visual;
} ListPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _ListRec {
    CorePart	core;
    SimplePart	simple;
    ListPart	list;
} ListRec;

#endif /* _XawListP_h */
