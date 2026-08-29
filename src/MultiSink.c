/*
 * Copyright 1991 by OMRON Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OMRON not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  OMRON makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *      Author: Li Yuhong	 OMRON Corporation
 */

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

// MultiSink.c is AsciiSink.c with unsigned char replaced by wchar_t, plus
// one function _XawMultiSinkPosToXY.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <wchar.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/CommonP.h>
#include <X11/Xaw3dXft/Encoding.h>
#include <X11/Xaw3dXft/MultiSinkP.h>
#include <X11/Xaw3dXft/MultiSrcP.h>
#include <X11/Xaw3dXft/TextP.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/XawInit.h>

// The competing definition comes from TextP.h
#ifdef GETLASTPOS
#undef GETLASTPOS		/* We will use our own GETLASTPOS. */
#endif

#define GETLASTPOS XawTextSourceScan(source, (XawTextPosition) 0, XawstAll, XawsdRight, 1, TRUE)

#define internalEncoding XawTextEncodingWc

static void DisplayText(Widget, Position, Position, XawTextPosition,
                        XawTextPosition, Boolean);
static void FindPosition(Widget, XawTextPosition, int, int, Boolean,
                         XawTextPosition *, int *, int *);
static void FindDistance(Widget, XawTextPosition, int, XawTextPosition,
                         int *, XawTextPosition *, int *);

#define SuperClass		(&textSinkClassRec)
MultiSinkClassRec multiSinkClassRec = {
  { /* core_class fields */
    /* superclass	  	*/	(WidgetClass) SuperClass,
    /* class_name	  	*/	"MultiSink",
    /* widget_size	  	*/	sizeof(MultiSinkRec),
    /* class_initialize   	*/	XawInitializeWidgetSet,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	NULL,
    /* initialize_hook		*/	NULL,
    /* obj1		  	*/	NULL,
    /* obj2		  	*/	NULL,
    /* obj3		  	*/	0,
    /* resources	  	*/	NULL,
    /* num_resources	  	*/	0,
    /* xrm_class	  	*/	NULLQUARK,
    /* obj4		  	*/	FALSE,
    /* obj5		  	*/	FALSE,
    /* obj6			*/	FALSE,
    /* obj7		  	*/	FALSE,
    /* destroy		  	*/	NULL,
    /* obj8		  	*/	NULL,
    /* obj9		  	*/	NULL,
    /* set_values	  	*/	NULL,
    /* set_values_hook		*/	NULL,
    /* obj10			*/	NULL,
    /* get_values_hook		*/	NULL,
    /* obj11		 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* obj12		   	*/	NULL,
    /* obj13			*/	NULL,
    /* obj14			*/	NULL,
    /* extension		*/	NULL
  },
  { /* text_sink_class fields */
    // public:
      /* DisplayText              */      DisplayText,
      /* InsertCursor             */      XtInheritInsertCursor,
      /* ClearToBackground        */      XtInheritClearToBackground,
      /* FindPosition             */      FindPosition,
      /* FindDistance             */      FindDistance,
      /* Resolve                  */      XtInheritResolve,
      /* MaxLines                 */      XtInheritMaxLines,
      /* MaxHeight                */      XtInheritMaxHeight,
      /* SetTabs                  */      XtInheritSetTabs,
      /* GetCursorBounds          */      XtInheritGetCursorBounds,
    // protected:
      /* PaintText */                     XtInheritPaintText
  },
  { /* multi_sink_class fields */
    /* unused			*/	0
  }
};

WidgetClass multiSinkObjectClass = (WidgetClass)&multiSinkClassRec;

/* Utilities */

// Translation for non-printing control characters — C0, DEL, C1.
// C0 and DEL get the ^ prefix to mean Control-something.
// C1 would be Escape-something but there's no standard expression.
// Current Xaw behavior is to insert an octal code for C1.
// Copied the Xaw behavior.
static constexpr uint8_t maxCharExprLen = 4;
static void CharExpr (MultiSinkObject sink, wchar_t c,
wchar_t expr[static maxCharExprLen+1]) {
  if (c < 0x20 || c >= 0x7f && c < 0xa0) {
    if (sink->text_sink.display_nonprinting) {
      if (c > 0x7f) {
	expr[0] = L'\\';
	expr[1] = (c >> 6 & 7) + L'0';
	expr[2] = (c >> 3 & 7) + L'0';
	expr[3] = (c & 7) + L'0';
	expr[4] = 0;
      } else {
	expr[0] = L'^';
	expr[1] = (c == 0x7f ? L'?' : c + L'@');
	expr[2] = 0;
      }
    } else {
      expr[0] = L' ';
      expr[1] = 0;
    }
  } else {
    expr[0] = c;
    expr[1] = 0;
  }
}

static Dimension CharWidth (Widget w, int x, wchar_t c) {
  MultiSinkObject sink = (MultiSinkObject)w;

  // LF should not happen here.
  // if (c == L'\n') return 0;
  assert(c != L'\n');

  // Tabs
  if (c == L'\t') {
    x -= ((TextWidget)XtParent(w))->text.margin.left;
    unsigned i;
    Position *tab;
    for (i=0, tab = sink->text_sink.tabs; i < sink->text_sink.tab_count;
    ++i, ++tab)
      if (x < *tab)
	return *tab - x;
    // We ran out of tab stops, so it really is 0.
    return 0;
  }

  // Everything else
  wchar_t expr[maxCharExprLen+1];
  CharExpr(sink, c, expr);
  Dimension width;
  Display *display = XtDisplayOfObject(w);
  Xaw3dXftSizeAnyString(display, sink->text_sink.font,
    sink->text_sink.fontset, sink->text_sink.xftfont,
    sink->text_sink.international, internalEncoding, expr,
    &width, NULL);
  return width;
}

/* Sink Object Functions */

/*
 * This function does not know about drawing more than one line of text.
 */

// Coordinates here are upper left corner.
static void DisplayText (Widget w, Position x, Position y,
XawTextPosition pos1, XawTextPosition pos2, Boolean highlight) {
  MultiSinkObject sink = (MultiSinkObject)w;
  TextWidget ctx = (TextWidget)XtParent(w);
  Widget source = XawTextGetSource((Widget)ctx);

  if (!sink->text_sink.echo || !ctx->text.lt.lines) return;

  // Get "protected" function pointer
  Dimension (* const PaintText) (Widget w, Position x, Position y,
    XawTextEncoding encoding, const void *buf, Cardinal num_chars,
    Boolean highlight) = multiSinkClassRec.text_sink_class.PaintText;

  // FIXME margins handling remains unclear
  Position max_x = XtWidth(ctx) - ctx->text.margin.right - 1;

  wchar_t buf[BUFSIZ+1]; // Leave room to terminate
  Cardinal j, k;
  XawTextBlock blk;
  for (j=0; pos1 < pos2 && x <= max_x; ) {
    pos1 = XawTextSourceRead(source, pos1, &blk, (int) pos2 - pos1);
    for (k=0; k < blk.length && x <= max_x; ++k) {
      if (j+maxCharExprLen >= BUFSIZ) {
	// Buffer is full:  flush buffer (1)
	buf[j] = 0;
	x += (*PaintText)(w, x, y, internalEncoding, buf, j, highlight);
	if (x > max_x) return;
	j = 0;
      }
      wchar_t c = ((wchar_t *)blk.ptr)[k];

      // Tolerate a linefeed in the final position.
      if (c == L'\n' && k == blk.length - 1 && pos1 == pos2) continue;
      assert(c != L'\n');

      if (c == L'\t') {
	// Tab:  flush buffer (2)
	if (j) {
	  buf[j] = 0;
	  x += (*PaintText)(w, x, y, internalEncoding, buf, j, highlight);
	  if (x > max_x) return;
	  j = 0;
	}

	// Handle tab
        const Dimension width = CharWidth(w, x, L'\t'),
	               height = sink->text_sink.fontHeight;
	// Possibly restore a background pixmap before mangling it.
	Window window = XtWindow(ctx);
	Display *display = XtDisplay(ctx);
	XClearArea(display, window, x, y, width, height, False);
	if (highlight) {
	  GC fillgc = (sink->text_sink.highlightStyle == TextHighlightReverse ?
	    sink->text_sink.xor_fgbg_GC : sink->text_sink.xor_bghl_GC);
	  XFillRectangle(display, window, fillgc, x, y, width, height);
	}
	x += width;
      } else {
	// Everything else
	wchar_t expr[maxCharExprLen+1];
	CharExpr(sink, c, expr);
	Cardinal n = wcslen(expr);
	wcsncpy(buf+j, expr, n);
	j += n;
      }
    }
  }

  if (j && x <= max_x) { // No more input:  flush buffer (3)
    buf[j] = 0;
    (void) (*PaintText)(w, x, y, internalEncoding, buf, j, highlight);
  }
}

/*
 * Given two positions, find the distance between them.
 */

// The height returned from here is always thrown away.
static void
FindDistance (Widget w, XawTextPosition fromPos, int fromx, XawTextPosition toPos,
              int *resWidth, XawTextPosition *resPos, int *resHeight)
{
    MultiSinkObject sink = (MultiSinkObject) w;
    Widget source = XawTextGetSource(XtParent(w));

    XawTextPosition index, lastPos;
    XawTextBlock blk;

    /* we may not need this */
    lastPos = GETLASTPOS;
    XawTextSourceRead(source, fromPos, &blk, (int) toPos - fromPos);
    *resWidth = 0;
    for (index = fromPos; index != toPos && index < lastPos; index++) {
	if (index - blk.firstPos >= blk.length)
	    XawTextSourceRead(source, index, &blk, (int) toPos - fromPos);
        wchar_t c = *((wchar_t *)blk.ptr + index - blk.firstPos);
	if (c == L'\n') {
	    index++;
	    break;
	}
	*resWidth += CharWidth(w, fromx + *resWidth, c);
    }
    *resPos = index;
    if (resHeight)
      *resHeight = sink->text_sink.fontHeight;
}

// resHeight is apparently a way to smuggle the font height to
// _BuildLineTable in Text.c.  Everyone else throws it away.
static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width,
             Boolean stopAtWordBreak, XawTextPosition *resPos, int *resWidth,
             int *resHeight)
{
    MultiSinkObject sink = (MultiSinkObject) w;
    Widget source = XawTextGetSource(XtParent(w));

    XawTextPosition lastPos, index, whiteSpacePosition = 0;
    int     lastWidth = 0, whiteSpaceWidth = 0;
    Boolean whiteSpaceSeen;
    XawTextBlock blk;

    lastPos = GETLASTPOS;

    XawTextSourceRead(source, fromPos, &blk, BUFSIZ);
    *resWidth = 0;
    whiteSpaceSeen = FALSE;

    wchar_t c = 0;
    for (index = fromPos; *resWidth <= width && index < lastPos; index++) {
	lastWidth = *resWidth;
	if (index - blk.firstPos >= blk.length)
	    XawTextSourceRead(source, index, &blk, BUFSIZ);
	c = *((wchar_t *)blk.ptr + index - blk.firstPos);
	if (c == L'\n') {
	    index++;
	    break;
	}
        *resWidth += CharWidth(w, fromx + *resWidth, c);
        if ((c == L' ' || c == L'\t') && *resWidth <= width) {
	    whiteSpaceSeen = TRUE;
	    whiteSpacePosition = index;
	    whiteSpaceWidth = *resWidth;
	}
    }
    if (*resWidth > width && index > fromPos) {
	*resWidth = lastWidth;
	index--;
	if (stopAtWordBreak && whiteSpaceSeen) {
	    index = whiteSpacePosition + 1;
	    *resWidth = whiteSpaceWidth;
	}
    }
    if (index == lastPos && c != L'\n')
      index = lastPos + 1;
    *resPos = index;
    if (resHeight)
      *resHeight = sink->text_sink.fontHeight;
}

/***** Public routines *****/

// This is for XawIm.  w is the TextWidget.
void _XawMultiSinkPosToXY (Widget w, XawTextPosition pos, Position *x,
Position *y) {
  MultiSinkObject sink = (MultiSinkObject) ((TextWidget)w)->text.sink;
  _XawTextPosToXY(w, pos, x, y);
  *y += sink->text_sink.fontAscent;
}
