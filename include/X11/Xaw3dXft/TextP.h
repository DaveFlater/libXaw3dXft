/***********************************************************

Copyright (c) 1987, 1988, 1994  X Consortium

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


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

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


Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)

******************************************************************/

#ifndef _XawTextP_h
#define _XawTextP_h

#include <X11/Xaw3dXft/Text.h>
#include <X11/Xaw3dXft/SimpleP.h>
#include <X11/Xaw3dXft/Xaw3dXft.h>

/****************************************************************
 *
 * Text widget private
 *
 ****************************************************************/
#define MAXCUT	30000	/* Maximum number of characters that can be cut. */

#define GETLASTPOS  XawTextSourceScan(ctx->text.source, 0, \
				      XawstAll, XawsdRight, 1, TRUE)

#define zeroPosition ((XawTextPosition) 0)

extern XtActionsRec _XawTextActionsTable[];
extern Cardinal _XawTextActionsTableCount;

/* constants that subclasses may want to know */
#define DEFAULT_TEXT_HEIGHT ((Dimension)~0)

/* displayable text management data structures */

typedef struct {
  XawTextPosition position;
  Position y;
  Dimension textWidth;
} XawTextLineTableEntry, *XawTextLineTableEntryPtr;

typedef struct {
    XawTextPosition   left, right;
    XawTextSelectType type;
    Atom*	     selections;
    int		     atom_count;
    int		     array_size;
} XawTextSelection;

// Contents is a NUL-terminated string in the internal encoding coming from
// _XawTextSaltAwaySelection or _DeleteOrKill.
typedef struct _XawTextSelectionSalt {
  struct _XawTextSelectionSalt *next;
  XawTextSelection s;
  void *contents;
} XawTextSelectionSalt;

/* Line Tables are n+1 long - last position displayed is in last lt entry */
typedef struct {
  XawTextPosition	 top;	/* Top of the displayed text.		*/
  int			 lines;	/* How many lines in this table.	*/
  XawTextLineTableEntry *info;  /* A dynamic array, one entry per line  */
} XawTextLineTable, *XawTextLineTablePtr;

// These should have been Dimensions, but now the signedness is baked in.
typedef struct _XawTextMargin {
  Position left, right, top, bottom;
} XawTextMargin;

#define VMargins(ctx) ((ctx)->text.margins.top + (ctx)->text.margins.bottom)
#define HMargins(ctx) ((ctx)->text.margins.left + (ctx)->text.margins.right)
#define HMarginsOffset(ctx) (HMargins(ctx) - (Position)(ctx)->text.hscroll_offset)

#define IsPositionVisible(ctx, pos) \
		(pos >= ctx->text.lt.info[0].position && \
		 pos < ctx->text.lt.info[ctx->text.lt.lines].position)

/*
 * Search & Replace data structure.
 */

struct SearchAndReplace {
  Boolean selection_changed;	/* flag so that the selection cannot be
				   changed out from underneath query-replace.*/
  Widget search_popup;		/* The popup widget that allows searches.*/
  Widget label1;		/* The label widgets for the search window. */
  Widget label2;
  Widget left_toggle;		/* The left search toggle radioGroup. */
  Widget right_toggle;		/* The right search toggle radioGroup. */
  Widget rep_label;		/* The Replace label string. */
  Widget rep_text;		/* The Replace text field. */
  Widget search_text;		/* The Search text field. */
  Widget rep_one;		/* The Replace one button. */
  Widget rep_all;		/* The Replace all button. */
};

/* Private Text Definitions */

/* New fields for the Text widget class record */

typedef struct {int empty;} TextClassPart;

struct text_move {
    int h, v;
    struct text_move * next;
};

/* Full class record declaration */
typedef struct _TextClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    TextClassPart	text_class;
} TextClassRec;

extern TextClassRec textClassRec;

/* New fields for the Text widget record */
typedef struct _TextPart {
    /* resources */

    Widget              source, sink;
    XawTextPosition	insertPos;
    XawTextSelection	s;
    XawTextSelectType	*sarray;	   /* Array to cycle for selections. */
    XawTextSelectionSalt    *salt;	     /* salted away selections */
    int			options;	     /* wordbreak, scroll, etc. */
    int			dialog_horiz_offset; /* position for popup dialog */
    int			dialog_vert_offset;  /* position for popup dialog */
    Boolean		display_caret;	     /* insertion pt visible iff T */
    Boolean             auto_fill;           /* Auto fill mode? */
    Boolean             useright; /* Place vertical scrollbar on right side? */
    XawTextScrollMode   scroll_vert, scroll_horiz; /*what type of scrollbars.*/
    XawTextWrapMode     wrap;            /* The type of wrapping. */
    XawTextResizeMode   resize;	             /* what to resize */
    XawTextMargin       res_margins;         /* Original resource margins */
    XtCallbackList	unrealize_callbacks; /* used for scrollbars */

    /* private state */
    XawTextMargin       margins;    /* Includes shadow and scrollbar widths */
    Dimension       hscroll_offset; /* Horizontal scrolling amount in pixels */
    XawTextLineTable	lt;
    XawTextScanDirection extendDir;
    XawTextSelection	origSel;    /* the selection being modified */
    Time	    lasttime;	    /* timestamp of last processed action */
    Time	    time;	    /* time of last key or button action */
    Position	    ev_x, ev_y;	    /* x, y coords for key or button action */
    Widget          vbar, hbar;	    /* The scroll bars (none = NULL). */
    struct SearchAndReplace * search;/* Search and replace structure. */
    Widget          file_insert;    /* The file insert popup widget. */
    XawTextPosition  *updateFrom;   /* Array of start positions for update. */
    XawTextPosition  *updateTo;	    /* Array of end positions for update. */
    int		    numranges;	    /* How many update ranges there are. */
    int		    maxranges;	    /* How many ranges we have space for */
    XawTextPosition  lastPos;	    /* Last position of source. */
    GC              gc;
    Boolean	    showposition;   /* True if we need to show the position. */
    Boolean         hasfocus;       /* TRUE if we currently have input focus.*/
    Boolean	    update_disabled; /* TRUE if display updating turned off */
    Boolean         single_char;    /* Single character replaced. */
    XawTextPosition  old_insert;    /* Last insertPos for batched updates */
    short           mult;	    /* Multiplier. */
    struct text_move * copy_area_offsets; /* Text offset area (linked list) */
    Widget          threeD;	    /* shadow drawing */

    /* The purpose of from_left is to remember the original pixel offset as
       you use the up or down arrows to move between lines.  Without it, if
       you have a variable-width font, the cursor drifts leftward.  Outside
       of MoveLine, it just gets reset to -1 whenever insertPos is changed in
       a way that invalidates the scrolling context.  Invalidating it too
       much causes the leftward drift.  Invalidating it not enough causes the
       cursor to warp back to a previous spot. */
    int from_left;

    /* private state, shared w/Source and Sink */
    Boolean	    redisplay_needed; /* signal used only in SetValues */
    XawTextSelectionSalt    *salt2;   /* other salted away selections */
} TextPart;

/*************************************************************
 *
 * Resource types private to Text widget.
 *
 *************************************************************/

#define XtRScrollMode "ScrollMode"
#define XtRWrapMode "WrapMode"
#define XtRResizeMode "ResizeMode"

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _TextRec {
    CorePart	core;
    SimplePart	simple;
    TextPart	text;
} TextRec;

/********************************************
 *
 * Semi-private functions
 * for use by other Xaw modules only
 *
 *******************************************/

extern void _XawTextBuildLineTable (
    TextWidget /*ctx*/,
    XawTextPosition /*top pos*/,
    _XtBoolean /* force_rebuild */
);

extern char* _XawTextGetSTRING(
    TextWidget /*ctx*/,
    XawTextPosition /*left*/,
    XawTextPosition /*right*/
);

extern void _XawTextSaltAwaySelection(
    TextWidget /*ctx*/,
    Atom* /*selections*/,
    int /*num_atoms*/
);

extern void _XawTextPosToXY(
    Widget			/* w */,
    XawTextPosition		/* pos */,
    Position *			/* x */,
    Position *			/*y */
);

/*
  Vs. the signature required by XtOwnSelection, this function has one extra
  parameter at the end, SelectionSelect.
     True = do the Text version of the MatchSelection block
    False = do the TextAction version
  This used to be two very long, nearly identical functions that differed
  only in that one place.
*/
extern Boolean _XawTextConvertSelection (Widget w, Atom *selection,
  Atom *target, Atom *type, XtPointer *value, unsigned long *length,
  int *format, Boolean SelectionSelect);

#endif /* _XawTextP_h */
