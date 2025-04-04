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

******************************************************************/

#ifndef _XawTextSink_h
#define _XawTextSink_h

#include <X11/Xaw3dxft/Text.h>

/***********************************************************************
 *
 * TextSink Object
 *
 ***********************************************************************/

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 foreground          Foreground      Pixel              XtDefaultForeground
 background          Background      Pixel              XtDefaultBackground

*/

/* Class record constants */

extern WidgetClass textSinkObjectClass;

typedef struct _TextSinkClassRec *TextSinkObjectClass;
typedef struct _TextSinkRec      *TextSinkObject;

typedef enum {XawisOn, XawisOff} XawTextInsertState;

/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/

_XFUNCPROTOBEGIN

/*	Function Name: XawTextSinkDisplayText
 *	Description: Stub function that in subclasses will display text.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location to start drawing text.
 *                 pos1, pos2 - location of starting and ending points
 *                              in the text buffer.
 *                 highlight - highlight this text?
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

extern void XawTextSinkDisplayText(
    Widget		/* w */,
#if NeedWidePrototypes
    /* Position */ int	/* x */,
    /* Position	*/ int	/* y */,
#else
    Position		/* x */,
    Position		/* y */,
#endif
    XawTextPosition	/* pos1 */,
    XawTextPosition	/* pos2 */,
#if NeedWidePrototypes
    /* Boolean */ int	/* highlight */
#else
    Boolean		/* highlight */
#endif
);

/*	Function Name: XawTextSinkInsertCursor
 *	Description: Places the InsertCursor.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location for the cursor.
 *                 staye - whether to turn the cursor on, or off.
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

extern void XawTextSinkInsertCursor(
    Widget		/* w */,
#if NeedWidePrototypes
    /* Position */ int	/* x */,
    /* Position	*/ int	/* y */,
    /* XawTextInsertState */ int /* state */
#else
    Position		/* x */,
    Position		/* y */,
    XawTextInsertState	/* state */
#endif
);

/*	Function Name: XawTextSinkClearToBackground
 *	Description: Clears a region of the sink to the background color.
 *	Arguments: w - the TextSink Object.
 *                 x, y  - location of area to clear.
 *                 width, height - size of area to clear
 *	Returns: void.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

extern void XawTextSinkClearToBackground(
    Widget		/* w */,
#if NeedWidePrototypes
    /* Position */ int	/* x */,
    /* Position */ int	/* y */,
    /* Dimension */ int	/* width */,
    /* Dimension */ int	/* height */
#else
    Position		/* x */,
    Position		/* y */,
    Dimension		/* width */,
    Dimension		/* height */
#endif
);

/*	Function Name: XawTextSinkFindPosition
 *	Description: Finds a position in the text.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - reference position.
 *                 fromX   - reference location.
 *                 width,  - width of section to paint text.
 *                 stopAtWordBreak - returned position is a word break?
 *                 resPos - Position to return.      *** RETURNED ***
 *                 resWidth - Width actually used.   *** RETURNED ***
 *                 resHeight - Height actually used. *** RETURNED ***
 *	Returns: none (see above).
 */

extern void XawTextSinkFindPosition(
    Widget		/* w */,
    XawTextPosition	/* fromPos */,
    int			/* fromX */,
    int			/* width */,
#if NeedWidePrototypes
    /* Boolean */ int	/* stopAtWordBreak */,
#else
    Boolean		/* stopAtWordBreak */,
#endif
    XawTextPosition*	/* pos_return */,
    int*		/* width_return */,
    int*		/* height_return */
);

/*	Function Name: XawTextSinkFindDistance
 *	Description: Find the Pixel Distance between two text Positions.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - starting Position.
 *                 fromX   - x location of starting Position.
 *                 toPos   - end Position.
 *                 resWidth - Distance between fromPos and toPos.
 *                 resPos   - Actual toPos used.
 *                 resHeight - Height required by this text.
 *	Returns: none.
 */

extern void XawTextSinkFindDistance (
    Widget		/* w */,
    XawTextPosition	/* fromPos */,
    int			/* fromX */,
    XawTextPosition     /* toPos */,
    int*		/* width_return */,
    XawTextPosition*	/* pos_return */,
    int*		/* height_return */
);

/*	Function Name: XawTextSinkResolve
 *	Description: Resolves a location to a position.
 *	Arguments: w - the TextSink Object.
 *                 pos - a reference Position.
 *                 fromx - a reference Location.
 *                 width - width to move.
 *                 resPos - the resulting position.
 *	Returns: none
 */

extern void XawTextSinkResolve(
    Widget		/* w */,
    XawTextPosition	/* fromPos */,
    int			/* fromX */,
    int			/* width */,
    XawTextPosition*	/* pos_return */
);

/*	Function Name: XawTextSinkMaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *                   a given height.
 *	Arguments: w - the TextSink Object.
 *                 height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */

extern int XawTextSinkMaxLines(
    Widget		/* w */,
#if NeedWidePrototypes
    /* Dimension */ int	/* height */
#else
    Dimension		/* height */
#endif
);

/*	Function Name: XawTextSinkMaxHeight
 *	Description: Finds the Minimum height that will contain a given number
 *                   lines.
 *	Arguments: w - the TextSink Object.
 *                 lines - the number of lines.
 *	Returns: the height.
 */

extern int XawTextSinkMaxHeight(
    Widget		/* w */,
    int			/* lines */
);

/*	Function Name: XawTextSinkSetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the TextSink Object.
 *                 tab_count - the number of tabs in the list.
 *                 tabs - the text positions of the tabs.
 *	Returns: none
 */

extern void XawTextSinkSetTabs(
    Widget		/* w */,
    int			/* tab_count */,
    int*		/* tabs */
);

/*	Function Name: XawTextSinkGetCursorBounds
 *	Description: Finds the bounding box for the insert curor (caret).
 *	Arguments: w - the TextSinkObject.
 *                 rect - an X rectance containing the cursor bounds.
 *	Returns: none (fills in rect).
 */

extern void XawTextSinkGetCursorBounds(
    Widget		/* w */,
    XRectangle*		/* rect_return */
);

_XFUNCPROTOEND

#endif /* _XawTextSrc_h */
