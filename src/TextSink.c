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


Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)

*/

/*
 * Author:  Chris Peterson, MIT X Consortium.
 *
 * Much code taken from X11R3 AsciiSink.
 */

/*
 * TextSink.c - TextSink object. (For use with the text widget).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/CommonP.h>
#include <X11/Xaw3dXft/TextSinkP.h>
#include <X11/Xaw3dXft/TextP.h>

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static void ClassPartInitialize(WidgetClass);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Destroy(Widget);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

static int MaxHeight(Widget, int);
static int MaxLines(Widget, Dimension);
static void DisplayText(Widget, Position, Position, XawTextPosition,
                        XawTextPosition, Boolean);
static void ClearToBackground(Widget, Position, Position, Dimension, Dimension);
static void FindPosition(Widget, XawTextPosition, int, int, Boolean,
                         XawTextPosition *, int *, int *);
static void FindDistance(Widget, XawTextPosition, int, XawTextPosition,
                         int *, XawTextPosition *, int *);
static void Resolve(Widget, XawTextPosition, int, int, XawTextPosition *);
static void SetTabs(Widget, int, short *);
static void GetCursorBounds(Widget, XRectangle *);
static void InsertCursor(Widget, Position, Position, XawTextInsertState);
static Dimension PaintText (Widget w, Position x, Position y,
  XawTextEncoding encoding, const void *buf, Cardinal num_chars,
  Boolean highlight);

#define offset(field) XtOffsetOf(TextSinkRec, text_sink.field)
static XtResource resources[] = {
  {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     offset(foreground), XtRString, XtDefaultForeground},
  {XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
     offset(background), XtRString, XtDefaultBackground},
  {XtNecho, XtCOutput, XtRBoolean, sizeof(Boolean),
     offset(echo), XtRImmediate, (XtPointer) True},
  {XtNdisplayNonprinting, XtCOutput, XtRBoolean, sizeof(Boolean),
     offset(display_nonprinting), XtRImmediate, (XtPointer) True},
  {XtNfont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
     offset(font), XtRString, XtDefaultFont},
  {XtNfontSet, XtCFontSet, XtRFontSet, sizeof (XFontSet),
     offset(fontset), XtRString, XtDefaultFontSet},
  {XtNinternational, XtCInternational, XtRBoolean, sizeof(Boolean),
     offset(international), XtRImmediate, (XtPointer) FALSE},
  {XtNxftFont, XtCXftFont, XtRString, sizeof(String),
     offset(xftfontname), XtRString, NULL},
  {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
     offset(encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
  {XtNhighlight, XtCBackground, XtRPixel, sizeof(Pixel),
     offset(highlight), XtRString, (XtPointer)XtDefaultBackground},
  {XtNhighlightStyle, XtCTextHighlightStyle, XtRUnsignedChar,
     sizeof(unsigned char), offset(highlightStyle), XtRImmediate,
     (XtPointer)TextHighlightReverse}
};
#undef offset

#define SuperClass		(&objectClassRec)
TextSinkClassRec textSinkClassRec = {
  {
/* core_class fields */
    /* superclass	  	*/	(WidgetClass) SuperClass,
    /* class_name	  	*/	"TextSink",
    /* widget_size	  	*/	sizeof(TextSinkRec),
    /* class_initialize   	*/	XawInitializeWidgetSet,
    /* class_part_initialize	*/	ClassPartInitialize,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* obj1		  	*/	NULL,
    /* obj2		  	*/	NULL,
    /* obj3	  		*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* obj4		  	*/	FALSE,
    /* obj5	  		*/	FALSE,
    /* obj6			*/	FALSE,
    /* obj7	  	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* obj8		  	*/	NULL,
    /* obj9		  	*/	NULL,
    /* set_values	  	*/	SetValues,
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
/* text_sink_class fields */
  {
    // public:
      /* DisplayText              */      DisplayText,
      /* InsertCursor             */      InsertCursor,
      /* ClearToBackground        */      ClearToBackground,
      /* FindPosition             */      FindPosition,
      /* FindDistance             */      FindDistance,
      /* Resolve                  */      Resolve,
      /* MaxLines                 */      MaxLines,
      /* MaxHeight                */      MaxHeight,
      /* SetTabs                  */      SetTabs,
      /* GetCursorBounds          */      GetCursorBounds,
    // protected:
      /* PaintText                */      PaintText
  }
};

WidgetClass textSinkObjectClass = (WidgetClass)&textSinkClassRec;

#define VisualOf(w) (w->text_sink.visual)
#define ColormapOf(w) (w->object.parent->core.colormap)

static void get_or_change_GCs (TextSinkObject ts) {
  const Pixel fg = ts->text_sink.foreground,
              bg = ts->text_sink.background,
              hl = ts->text_sink.highlight;

  // normal_GC
  if (ts->text_sink.normal_GC)
    XtReleaseGC((Widget)ts, ts->text_sink.normal_GC);
  ts->text_sink.normal_GC = Xaw3dXftGetTextGC((Widget)ts, fg, ts->text_sink.font,
					 ts->text_sink.international);

  // rev_GC
  if (ts->text_sink.rev_GC)
    XtReleaseGC((Widget)ts, ts->text_sink.rev_GC);
  ts->text_sink.rev_GC = Xaw3dXftGetTextGC((Widget)ts, bg, ts->text_sink.font,
				      ts->text_sink.international);

  // stipple_GC
  if (ts->text_sink.stipple_GC)
    XtReleaseGC((Widget)ts, ts->text_sink.stipple_GC);
  ts->text_sink.stipple_GC = Xaw3dXftGetStippleGC((Widget)ts, bg);

  XGCValues values;
  values.graphics_exposures = False;

  // xor_fgbg_GC
  values.foreground = fg ^ bg;
  values.function = GXxor;
  if (ts->text_sink.xor_fgbg_GC)
    XtReleaseGC((Widget)ts, ts->text_sink.xor_fgbg_GC);
  ts->text_sink.xor_fgbg_GC = XtGetGC((Widget)ts,
    GCForeground|GCFunction|GCGraphicsExposures, &values);

  // xor_bghl_GC
  values.foreground = bg ^ hl;
  if (ts->text_sink.xor_bghl_GC)
    XtReleaseGC((Widget)ts, ts->text_sink.xor_bghl_GC);
  ts->text_sink.xor_bghl_GC = XtGetGC((Widget)ts,
    GCForeground|GCFunction|GCGraphicsExposures, &values);

  // XftColors
  Display *display = XtDisplayOfObject((Widget)ts);
  Visual *visual = VisualOf(ts);
  Colormap cmap = ColormapOf(ts);
  Xaw3dXftGetXftColor(display, visual, cmap, fg, &ts->text_sink.xftfg);
  Xaw3dXftGetXftColor(display, visual, cmap, bg, &ts->text_sink.xftbg);
}

#define insertCursor_width 6
#define insertCursor_height 3
static char insertCursor_bits[] = {0x0c, 0x1e, 0x33};

static Pixmap CreateInsertCursor (Screen *s) {
  return XCreateBitmapFromData(DisplayOfScreen(s), RootWindowOfScreen(s),
    insertCursor_bits, insertCursor_width, insertCursor_height);
}

/*	Function Name: GetCursorBounds
 *	Description: Finds the bounding box for the insert cursor (caret).
 *	Arguments: w - the TextSinkObject.
 *                 rect - an X rectangle to return the cursor bounds in.
 *	Returns: none (fills in rect).
 */

static void GetCursorBounds (Widget w, XRectangle *rect) {
  TextSinkObject sink = (TextSinkObject)w;
  rect->width = (unsigned short) insertCursor_width;
  rect->height = (unsigned short) insertCursor_height;
  rect->x = sink->text_sink.cursor_x - (short) (rect->width / 2);
  rect->y = sink->text_sink.cursor_y - (short) rect->height;
}

/*	Function Name: InsertCursor
 *	Description: Places the InsertCursor.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location for the cursor.
 *                 state - whether to turn the cursor on or off.
 *	Returns: none.
 */

static void InsertCursor (Widget w, Position x, Position y,
XawTextInsertState state) {
  TextSinkObject sink = (TextSinkObject)w;
  Widget text_widget = XtParent(w);
  XRectangle rect;

  sink->text_sink.cursor_x = x;
  sink->text_sink.cursor_y = y;
  GetCursorBounds(w, &rect);
  if (state != sink->text_sink.laststate && XtIsRealized(text_widget))
    XCopyPlane(XtDisplay(text_widget),
      sink->text_sink.insertCursorOn,
      XtWindow(text_widget), sink->text_sink.xor_fgbg_GC,
      0, 0, (unsigned int) rect.width, (unsigned int) rect.height,
      (int) rect.x, (int) rect.y, 1);
  sink->text_sink.laststate = state;
}

static void ClassPartInitialize (WidgetClass wc) {
  TextSinkObjectClass t_src = (TextSinkObjectClass)wc,
    superC = (TextSinkObjectClass)t_src->object_class.superclass;

  /*
   * We don't need to check for null super since we'll get to TextSink
   * eventually.
   */
  // Always checking for XtInherit* stops us from dereferencing
  // superC->text_sink_class.* if we're already in TextSink.

  // public:
  if (t_src->text_sink_class.DisplayText == XtInheritDisplayText)
    t_src->text_sink_class.DisplayText = superC->text_sink_class.DisplayText;

  if (t_src->text_sink_class.InsertCursor == XtInheritInsertCursor)
    t_src->text_sink_class.InsertCursor =
					superC->text_sink_class.InsertCursor;

  if (t_src->text_sink_class.ClearToBackground == XtInheritClearToBackground)
    t_src->text_sink_class.ClearToBackground =
				   superC->text_sink_class.ClearToBackground;

  if (t_src->text_sink_class.FindPosition == XtInheritFindPosition)
    t_src->text_sink_class.FindPosition =
					superC->text_sink_class.FindPosition;

  if (t_src->text_sink_class.FindDistance == XtInheritFindDistance)
    t_src->text_sink_class.FindDistance =
				       superC->text_sink_class.FindDistance;

  if (t_src->text_sink_class.Resolve == XtInheritResolve)
    t_src->text_sink_class.Resolve = superC->text_sink_class.Resolve;

  if (t_src->text_sink_class.MaxLines == XtInheritMaxLines)
    t_src->text_sink_class.MaxLines = superC->text_sink_class.MaxLines;

  if (t_src->text_sink_class.MaxHeight == XtInheritMaxHeight)
    t_src->text_sink_class.MaxHeight = superC->text_sink_class.MaxHeight;

  if (t_src->text_sink_class.SetTabs == XtInheritSetTabs)
    t_src->text_sink_class.SetTabs = superC->text_sink_class.SetTabs;

  if (t_src->text_sink_class.GetCursorBounds == XtInheritGetCursorBounds)
    t_src->text_sink_class.GetCursorBounds =
				     superC->text_sink_class.GetCursorBounds;

  // protected:
  if (t_src->text_sink_class.PaintText == XtInheritPaintText)
    t_src->text_sink_class.PaintText = superC->text_sink_class.PaintText;
}

/*	Function Name: Initialize
 *	Description: Initializes the TextSink Object.
 *	Arguments: request, new - the requested and new values for the object
 *                                instance.
 *	Returns: none.
 *
 */

static void Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  TextSinkObject sink = (TextSinkObject)new;
  Display *display = XtDisplayOfObject(new);

  Xaw3dXftGetVisualInfo(new, &VisualOf(sink), NULL, NULL);
  sink->text_sink.normal_GC =
    sink->text_sink.rev_GC =
    sink->text_sink.stipple_GC =
    sink->text_sink.xor_fgbg_GC =
    sink->text_sink.xor_bghl_GC =
    NULL;
  sink->text_sink.insertCursorOn = CreateInsertCursor(XtScreenOfObject(new));
  sink->text_sink.laststate = XawisOff;
  sink->text_sink.cursor_x = sink->text_sink.cursor_y = 0;

  Xaw3dXftFixDefaultEncoding(args, *num_args, sink->text_sink.international,
    &sink->text_sink.encoding);
  if (sink->text_sink.xftfontname)
    sink->text_sink.xftfont = Xaw3dXftGetFont(new, sink->text_sink.xftfontname);
  else
    sink->text_sink.xftfont = NULL;
  if (sink->text_sink.international && !sink->text_sink.fontset)
    XtError("TextSink initialized with international true but no fontset");
  if (!sink->text_sink.font) XtError("TextSink initialized with no font");

  Xaw3dXftAnyFontMetrics(display, sink->text_sink.font, sink->text_sink.fontset,
    sink->text_sink.xftfont, sink->text_sink.international,
    &sink->text_sink.fontHeight, &sink->text_sink.fontAscent,
    &sink->text_sink.fontWidth);

  // AsciiText's Initialize calls XawTextSinkSetTabs after we are done
  // setting up here.
  sink->text_sink.tab_count = 0;
  sink->text_sink.tabs = sink->text_sink.char_tabs = NULL;

  get_or_change_GCs(sink);
  // FIXME tempted to default size to fit initial text
}

/*	Function Name: Destroy
 *	Description: This function cleans up when the object is
 *                   destroyed.
 *	Arguments: w - the TextSink Object.
 *	Returns: none.
 */

static void Destroy (Widget w) {
  TextSinkObject sink = (TextSinkObject)w;
  if (sink->text_sink.normal_GC)
    XtReleaseGC(w, sink->text_sink.normal_GC);
  if (sink->text_sink.rev_GC)
    XtReleaseGC(w, sink->text_sink.rev_GC);
  if (sink->text_sink.stipple_GC)
    XtReleaseGC(w, sink->text_sink.stipple_GC);
  if (sink->text_sink.xor_fgbg_GC)
    XtReleaseGC(w, sink->text_sink.xor_fgbg_GC);
  if (sink->text_sink.xor_bghl_GC)
    XtReleaseGC(w, sink->text_sink.xor_bghl_GC);
  if (sink->text_sink.tabs)
    XtFree((char*)sink->text_sink.tabs);
  if (sink->text_sink.char_tabs)
    XtFree((char*)sink->text_sink.char_tabs);
  XFreePixmap(XtDisplayOfObject(w), sink->text_sink.insertCursorOn);
}

/*	Function Name: SetValues
 *	Description: Sets the values for the TextSink
 *	Arguments: current - current state of the object.
 *                 request - what was requested.
 *                 new - what the object will become.
 *
 * The SetValues functions of the source and sink always return False, but
 * they set text.redisplay_needed, which triggers a redisplay in Text.
 */

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  TextSinkObject curts = (TextSinkObject)current;
  TextSinkObject newts = (TextSinkObject)new;
  Boolean redraw = False;

  // Notice if the colors or plain old font changed.  We need
  // get_or_change_GCs when international changes because the GCs need to be
  // writable if true and they need to have their font set if false.  Xaw
  // does not allow international to change.
  if (curts->text_sink.foreground    != newts->text_sink.foreground ||
      curts->text_sink.background    != newts->text_sink.background ||
      curts->text_sink.highlight     != newts->text_sink.highlight  ||
      curts->text_sink.font->fid     != newts->text_sink.font->fid  ||
      curts->text_sink.international != newts->text_sink.international) {
    get_or_change_GCs(newts);
    redraw = True;
  }

  // Notice if the Xft font changed
  if (curts->text_sink.xftfontname != newts->text_sink.xftfontname) {
    if (newts->text_sink.xftfontname)
      newts->text_sink.xftfont = Xaw3dXftGetFont(new,
	newts->text_sink.xftfontname);
    else
      newts->text_sink.xftfont = NULL;
    redraw = True;
  }

  // Notice if other things changed.
  // If encoding changes on a MultiSrc, it'll export to the new encoding.  If
  // encoding changes on an AsciiSrc, it won't notice.
  if (curts->text_sink.echo != newts->text_sink.echo ||
      curts->text_sink.fontset != newts->text_sink.fontset ||
      curts->text_sink.display_nonprinting !=
        newts->text_sink.display_nonprinting ||
      curts->text_sink.highlightStyle != newts->text_sink.highlightStyle)
    redraw = True;

  // FIXME SetTabs needs to be done in TextSink whenever font changes
  #if 0
    if ( w->text_sink.fontset != old_w->text_sink.fontset ) {
	((TextWidget)XtParent(new))->text.redisplay_needed = True;
	SetTabs((Widget)w, w->text_sink.tab_count, w->text_sink.char_tabs);
    }
  #endif

  // Then pass the buck
  if (redraw)
    ((TextWidget)XtParent(new))->text.redisplay_needed = True;
  return False;
}

/************************************************************
 *
 * Class specific methods.
 *
 ************************************************************/

/*	Function Name: PaintText
 *	Description: Actually paints the text into the window.
 *	Arguments: w - the text widget.
 *                 x, y - location to paint the text (upper left)
 *                 encoding - 8bit or wc internal encoding
 *                 buf, num_chars - buffer and length of text to paint.
 *                            buf string must be terminated!
 *                 highlight - whether to highlight
 *	Returns: the width of the text painted.
 */

static Dimension PaintText (Widget w, Position x, Position y,
XawTextEncoding encoding, const void *buf, Cardinal num_chars,
Boolean highlight) {
  TextSinkObject sink = (TextSinkObject)w;
  TextWidget ctx = (TextWidget)XtParent(w);

  Cardinal num_bytes;
  switch (encoding) {
  case XawTextEncoding8bit:
    num_bytes = num_chars;
    break;
  case XawTextEncodingWc:
    num_bytes = num_chars * sizeof(wchar_t);
    break;
  default:
    XtError("libXaw3dXft: unsupported internal encoding in PaintText");
  }

  // FIXME margins handling remains unclear.
  const Dimension corewidth = ctx->core.width,
                    rmargin = ctx->text.margin.right;
  Position max_x = (Position)corewidth - (Position)rmargin - 1;
  // DrawText shouldn't call us if x is off the right edge.
  assert(x <= max_x);

  Dimension width, height;
  Display *display = XtDisplay(ctx);
  Xaw3dXftSizeAnyStringN(display, sink->text_sink.font,
    sink->text_sink.fontset, sink->text_sink.xftfont,
    sink->text_sink.international, encoding, buf, num_bytes, &width, &height);

  if ((Position)width <= -x)
    return width; // Off the left side

  // The text arrives here in little pieces, and every little piece has to
  // clear its own background and apply its own highlighting or stipple.
  // Groan.
  // XtIsSensitive(w) returns a false negative.
  const Boolean sensitive = XtIsSensitive((Widget)ctx);
  if (highlight && !sensitive) {
    XtWarning("libXaw3dXft: insensitive Text widget has highlighted text");
    highlight = False;
  }

  // Possibly restore a background pixmap before mangling it.
  Window window = XtWindow(ctx);
  XClearArea(display, window, x, y, width, height, False);
  if (highlight) {
    GC fillgc = (sink->text_sink.highlightStyle == TextHighlightReverse ?
	         sink->text_sink.xor_fgbg_GC : sink->text_sink.xor_bghl_GC);
    XFillRectangle(display, window, fillgc, x, y, width, height);
  }

  GC gc;
  XftColor *xfg;
  if (highlight && sink->text_sink.highlightStyle == TextHighlightReverse) {
    gc = sink->text_sink.rev_GC;
    xfg = &sink->text_sink.xftbg;
  } else {
    gc = sink->text_sink.normal_GC;
    xfg = &sink->text_sink.xftfg;
  }
  Xaw3dXftDrawAnyStringN(display, VisualOf(sink), ctx->core.colormap, window,
    sink->text_sink.font, sink->text_sink.fontset, sink->text_sink.xftfont,
    sink->text_sink.international, gc, xfg, x, y, NULL, encoding, buf,
    num_bytes);

  // Insensitive stipple
  if (!sensitive)
    XFillRectangle(display, window, sink->text_sink.stipple_GC, x, y, width,
      height);

  // Draw the box at the right margin when text runs off the right side.
  // This uses normal_GC regardless of the gc used for text.
  if (x + (Position)width > max_x && rmargin != 0) {
    x = max_x + 1;
    XFillRectangle(display, window, sink->text_sink.normal_GC, x, y, rmargin,
      height);
  }

  return width;
}

/*	Function Name: DisplayText
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

static void
DisplayText(Widget w, Position x, Position y, XawTextPosition pos1,
            XawTextPosition pos2, Boolean highlight)
{
  return;
}

/*	Function Name: ClearToBackground
 *	Description: Clears a region of the sink to the background color.
 *	Arguments: w - the TextSink Object.
 *                 x, y  - location of area to clear.
 *                 width, height - size of area to clear
 *	Returns: void.
 *
 */

static void
ClearToBackground(Widget w, Position x, Position y, Dimension width, Dimension height)
{
/*
 * Don't clear in height or width are zero.
 * XClearArea() has special semantic for these values.
 */

    if ( (height == 0) || (width == 0) ) return;
    XClearArea(XtDisplayOfObject(w), XtWindowOfObject(w),
	       x, y, width, height, False);
}

/*	Function Name: FindPosition
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

static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width, Boolean stopAtWordBreak,
             XawTextPosition *resPos, int *resWidth, int *resHeight)
{
  *resPos = fromPos;
  *resHeight = *resWidth = 0;
}

/*	Function Name: FindDistance
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

static void
FindDistance(Widget w, XawTextPosition fromPos, int fromx, XawTextPosition toPos,
             int *resWidth, XawTextPosition *resPos, int *resHeight)
{
  *resWidth = *resHeight = 0;
  *resPos = fromPos;
}

/*	Function Name: Resolve
 *	Description: Resolves a location to a position.
 *	Arguments: w - the TextSink Object.
 *                 pos - a reference Position.
 *                 fromx - a reference Location.
 *                 width - width to move.
 *                 resPos - the resulting position.
 *	Returns: none
 */

static void Resolve (Widget w, XawTextPosition pos, int fromx, int width,
XawTextPosition *resPos) {
  TextSinkObjectClass class = (TextSinkObjectClass)w->core.widget_class;
  Widget source = XawTextGetSource(XtParent(w));
  int discardedWidth;
  (*class->text_sink_class.FindPosition)(w, pos, fromx, width, False, resPos,
    &discardedWidth, NULL);
  const XawTextPosition lastpos = XawTextSourceScan(source, 0, XawstAll,
    XawsdRight, 1, True);
  if (*resPos > lastpos)
    *resPos = lastpos;
}

/*	Function Name: MaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *                   a given height.
 *	Arguments: w - the TextSink Object.
 *                 height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */

static int MaxLines (Widget w, Dimension height) {
  TextSinkObject sink = (TextSinkObject)w;
  return height / sink->text_sink.fontHeight;
}

/*	Function Name: MaxHeight
 *	Description: Finds the Minimum height that will contain a given number
 *                   lines.
 *	Arguments: w - the TextSink Object.
 *                 lines - the number of lines.
 *	Returns: the height.
 */

static int MaxHeight (Widget w, int lines) {
  TextSinkObject sink = (TextSinkObject)w;
  return lines * sink->text_sink.fontHeight;
}

/*	Function Name: SetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the TextSink Object.
 *                 tab_count - the number of tabs in the list.
 *                 tabs - the text positions of the tabs.
 *	Returns: none
 */

static void SetTabs (Widget w, int tab_count, short *tabs) {
  TextSinkObject sink = (TextSinkObject)w;

  if (tab_count > sink->text_sink.tab_count) {
    sink->text_sink.tabs = (Position *)
	XtRealloc((char *) sink->text_sink.tabs,
		  (Cardinal) (tab_count * sizeof(Position)));
    sink->text_sink.char_tabs = (short *)
	XtRealloc((char *) sink->text_sink.char_tabs,
		  (Cardinal) (tab_count * sizeof(short)));
  }

  for (unsigned i=0; i < tab_count; ++i) {
    sink->text_sink.tabs[i] = tabs[i] * sink->text_sink.fontWidth;
    sink->text_sink.char_tabs[i] = tabs[i];
  }

  sink->text_sink.tab_count = tab_count;
  // FIXME:  rebuilding the line table here might have been because it includes textWidth for each line....
  ((TextWidget)XtParent(w))->text.redisplay_needed = True;
}

/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/


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

void
XawTextSinkDisplayText(Widget w,
		       Position x, Position y,
		       XawTextPosition pos1, XawTextPosition pos2,
		       Boolean highlight)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.DisplayText)(w, x, y, pos1, pos2, highlight);
}

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

void
XawTextSinkInsertCursor(Widget w,
			Position x, Position y, XawTextInsertState state)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.InsertCursor)(w, x, y, state);
}


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

void
XawTextSinkClearToBackground (Widget w,
			      Position x, Position y,
			      Dimension width, Dimension height)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.ClearToBackground)(w, x, y, width, height);
}

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

void
XawTextSinkFindPosition(Widget w, XawTextPosition fromPos, int fromx,
			int width,
			Boolean stopAtWordBreak,
			XawTextPosition *resPos, int *resWidth, int *resHeight)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.FindPosition)(w, fromPos, fromx, width,
					 stopAtWordBreak,
					 resPos, resWidth, resHeight);
}

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

void
XawTextSinkFindDistance (Widget w, XawTextPosition fromPos, int fromx,
			 XawTextPosition toPos, int *resWidth,
			 XawTextPosition *resPos, int *resHeight)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.FindDistance)(w, fromPos, fromx, toPos,
					 resWidth, resPos, resHeight);
}

/*	Function Name: XawTextSinkResolve
 *	Description: Resolves a location to a position.
 *	Arguments: w - the TextSink Object.
 *                 pos - a reference Position.
 *                 fromx - a reference Location.
 *                 width - width to move.
 *                 resPos - the resulting position.
 *	Returns: none
 */

void
XawTextSinkResolve(Widget w, XawTextPosition pos, int fromx, int width,
		   XawTextPosition *resPos)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.Resolve)(w, pos, fromx, width, resPos);
}

/*	Function Name: XawTextSinkMaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *                   a given height.
 *	Arguments: w - the TextSink Object.
 *                 height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */

int
XawTextSinkMaxLines(Widget w,
		    Dimension height)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  return((*class->text_sink_class.MaxLines)(w, height));
}

/*	Function Name: XawTextSinkMaxHeight
 *	Description: Finds the Minimum height that will contain a given number
 *                   lines.
 *	Arguments: w - the TextSink Object.
 *                 lines - the number of lines.
 *	Returns: the height.
 */

int
XawTextSinkMaxHeight(Widget w, int lines)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  return((*class->text_sink_class.MaxHeight)(w, lines));
}

/*	Function Name: XawTextSinkSetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the TextSink Object.
 *                 tab_count - the number of tabs in the list.
 *                 tabs - the text positions of the tabs.
 *	Returns: none
 */

void
XawTextSinkSetTabs(Widget w, int tab_count, int *tabs)
{
  if (tab_count > 0) {
    TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;
    short *char_tabs = (short*)XtMalloc( (unsigned)tab_count*sizeof(short) );
    short *tab;
    int i;

    for (i = tab_count, tab = char_tabs; i; i--) *tab++ = (short)*tabs++;

    (*class->text_sink_class.SetTabs)(w, tab_count, char_tabs);
    XtFree((char *)char_tabs);
  }
}

/*	Function Name: XawTextSinkGetCursorBounds
 *	Description: Finds the bounding box for the insert curor (caret).
 *	Arguments: w - the TextSinkObject.
 *                 rect - an X rectance containing the cursor bounds.
 *	Returns: none (fills in rect).
 */

void
XawTextSinkGetCursorBounds(Widget w, XRectangle *rect)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.GetCursorBounds)(w, rect);
}
