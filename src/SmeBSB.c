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
 * Portions Copyright (c) 2003 Brian V. Smith
 * Rights, permissions, and disclaimer per the above X Consortium license.
 */

/*
 * SmeBSB.c - Source code file for BSB Menu Entry object.
 *
 * Date:    September 26, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium
 *          kit@expo.lcs.mit.edu
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/Cardinals.h>
#include <X11/Xaw3dXft/CommonP.h>
#include <X11/Xaw3dXft/Encoding.h>
#include <X11/Xaw3dXft/SimpleMenP.h>
#include <X11/Xaw3dXft/SmeBSBP.h>
#include <X11/Xaw3dXft/ThreeDP.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <X11/Xmu/Drawing.h>
#include <X11/Xos.h>
#include <assert.h>
#include <stdio.h>

#define offset(field) XtOffsetOf(SmeBSBRec, sme_bsb.field)

static_assert(Got_XAW_defines);
static XtResource resources[] = {
  {XtNlabel,  XtCLabel, XtRString, sizeof(String),
     offset(label), XtRString, NULL},
  {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
     offset(encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
  {XtNvertSpace,  XtCVertSpace, XtRInt, sizeof(int),
     offset(vert_space), XtRImmediate, (XtPointer)25},
  {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
     offset(left_bitmap), XtRImmediate, (XtPointer)None},
  {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
     offset(justify), XtRImmediate, (XtPointer)XtJustifyLeft},
  {XtNrightBitmap, XtCRightBitmap, XtRBitmap, sizeof(Pixmap),
     offset(right_bitmap), XtRImmediate, (XtPointer)None},
  {XtNleftMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(left_margin), XtRImmediate, (XtPointer)4},
  {XtNrightMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(right_margin), XtRImmediate, (XtPointer)4},
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     offset(foreground), XtRString, (XtPointer)XtDefaultForeground},
  {XtNhighlight, XtCBackground, XtRPixel, sizeof(Pixel),
     offset(highlight), XtRString, (XtPointer)XtDefaultBackground},
  {XtNhighlightStyle, XtCMenuHighlightStyle, XtRUnsignedChar,
     sizeof(unsigned char), offset(highlightStyle), XtRImmediate,
     (XtPointer)MenuHighlightReverse},
  {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     offset(font), XtRString, (XtPointer)XtDefaultFont},
  {XtNxftFont,  XtCXftFont, XtRString, sizeof(String),
     offset(xftfontname), XtRString, NULL},
#ifdef XAW_INTERNATIONALIZATION
  {XtNfontSet,  XtCFontSet, XtRFontSet, sizeof(XFontSet),
     offset(fontset), XtRString, (XtPointer)XtDefaultFontSet},
#endif
  {XtNmenuName, XtCMenuName, XtRString, sizeof(String),
     offset(menu_name), XtRImmediate, NULL},
  {XtNunderline,  XtCIndex, XtRInt, sizeof(int),
     offset(underline), XtRImmediate, (XtPointer)-1}
};
#undef offset

/*
 * Semi public function definitions.
 */

static void Redisplay(Widget, XEvent *, Region);
static void Destroy(Widget);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Highlight(Widget);
static void Unhighlight(Widget);
static void ClassInitialize(void);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);
static XtGeometryResult QueryGeometry(Widget, XtWidgetGeometry *, XtWidgetGeometry *);

#define superclass (&smeThreeDClassRec)
SmeBSBClassRec smeBSBClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "SmeBSB",
    /* size               */    sizeof(SmeBSBRec),
    /* class_initializer  */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed      */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */	NULL,
    /* realize            */    NULL,
    /* actions            */    NULL,
    /* num_actions        */    ZERO,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE,
    /* compress_exposure  */    FALSE,
    /* compress_enterleave*/ 	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    NULL,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus       */    NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table		  */    NULL,
    /* query_geometry	  */    QueryGeometry,
    /* display_accelerator*/    NULL,
    /* extension	  */    NULL
  },{
    /* SimpleMenuClass Fields */
    /* highlight          */	Highlight,
    /* unhighlight        */	Unhighlight,
    /* notify             */	XtInheritNotify,
    /* extension	  */	NULL
  }, {
    /* ThreeDClass Fields */
    /* shadowdraw         */    XtInheritXawSme3dShadowDraw
  }, {
    /* BSBClass Fields */
    /* extension	  */    NULL
  }
};

WidgetClass smeBSBObjectClass = (WidgetClass) &smeBSBClassRec;

/****************************************************************
 *
 * Private functions
 *
 ****************************************************************/

#define VisualOf(w) (w->sme_bsb.visual)
#define ObjParent(w) (w->object.parent)

static void get_or_change_GCs (SmeBSBObject ent) {
  const Pixel fg = ent->sme_bsb.foreground,
              bg = ObjParent(ent)->core.background_pixel,
              hl = ent->sme_bsb.highlight;

  // normal_GC
  if (ent->sme_bsb.normal_GC)
    XtReleaseGC((Widget)ent, ent->sme_bsb.normal_GC);
  ent->sme_bsb.normal_GC = Xaw3dXftGetTextGC((Widget)ent, fg, ent->sme_bsb.font,
					     menuIntl(ent));

  // rev_GC
  if (ent->sme_bsb.rev_GC)
    XtReleaseGC((Widget)ent, ent->sme_bsb.rev_GC);
  ent->sme_bsb.rev_GC = Xaw3dXftGetTextGC((Widget)ent, bg, ent->sme_bsb.font,
					  menuIntl(ent));

  // stipple_GC
  if (ent->sme_bsb.stipple_GC)
    XtReleaseGC((Widget)ent, ent->sme_bsb.stipple_GC);
  ent->sme_bsb.stipple_GC = Xaw3dXftGetStippleGC((Widget)ent, bg);

  XGCValues values;
  values.graphics_exposures = False;

  // xor_fgbg_GC
  values.foreground = fg ^ bg;
  values.function = GXxor;
  if (ent->sme_bsb.xor_fgbg_GC)
    XtReleaseGC((Widget)ent, ent->sme_bsb.xor_fgbg_GC);
  ent->sme_bsb.xor_fgbg_GC = XtGetGC((Widget)ent,
    GCForeground|GCFunction|GCGraphicsExposures, &values);

  // xor_bghl_GC
  values.foreground = bg ^ hl;
  if (ent->sme_bsb.xor_bghl_GC)
    XtReleaseGC((Widget)ent, ent->sme_bsb.xor_bghl_GC);
  ent->sme_bsb.xor_bghl_GC = XtGetGC((Widget)ent,
    GCForeground|GCFunction|GCGraphicsExposures, &values);

  // XftColors
  Display *display = XtDisplayOfObject((Widget)ent);
  Visual *visual = VisualOf(ent);
  Colormap cmap = ObjParent(ent)->core.colormap;
  Xaw3dXftGetXftColor(display, visual, cmap, fg, &ent->sme_bsb.xftfg);
  Xaw3dXftGetXftColor(display, visual, cmap, bg, &ent->sme_bsb.xftbg);
  Xaw3dXftGetXftColor(display, visual, cmap, hl, &ent->sme_bsb.xfthl);
}

/*
 * Calculate width and height of displayed text in pixels
 */

static void GetTextDimensions (SmeBSBObject ent) {
  Display *display = XtDisplayOfObject((Widget)ent);
  if (ent->sme_bsb.label == NULL)
    ent->sme_bsb.label_height = ent->sme_bsb.label_width = 0;
  else
    Xaw3dXftSizeAnyString(display, ent->sme_bsb.font, menuFontSet(ent),
			  ent->sme_bsb.xftfont, menuIntl(ent),
			  ent->sme_bsb.encoding, ent->sme_bsb.label,
			  &ent->sme_bsb.label_width,
			  &ent->sme_bsb.label_height);
}

/*	Function Name: GetDefaultSize
 *	Description: Calculates the Default (preferred) size of
 *                   this menu entry.
 *	Arguments: ent - the menu entry widget.
 *                 width, height - default sizes (RETURNED).
 *	Returns: none.
 *
 * GetTextDimensions and GetBitmapDimensions must already have been called as
 * necessary to update the respective saved dimensions.
 */

static void GetDefaultSize (SmeBSBObject ent, Dimension *width,
			    Dimension *height) {
  Dimension ww=ent->sme_bsb.label_width, hh=ent->sme_bsb.label_height;

  ww += ent->sme_bsb.left_margin + ent->sme_bsb.right_margin +
        2 * ent->sme_threeD.shadow_width;
  if (ent->sme_bsb.left_bitmap_height > hh)
    hh = ent->sme_bsb.left_bitmap_height;
  if (ent->sme_bsb.right_bitmap_height > hh)
    hh = ent->sme_bsb.right_bitmap_height;
  hh *= (100.0f + ent->sme_bsb.vert_space)/100.0f;
  hh += 2 * ent->sme_threeD.shadow_width;

  if (width) *width = ww;
  if (height) *height = hh;
}

// Given:
// - All dimensions have been set
// - Widget is realized (so has rectangle.x and rectangle.y)
// Calculate position of text label
static void GetTextPosition (SmeBSBObject ent, Position *text_x,
			     Position *text_y) {
  assert(XtIsRealized((Widget)ent));
  if (text_x) {
    const Dimension s = ent->sme_threeD.shadow_width;
    // enum XtJustify is defined in... X11/Xmu/Converters.h
    switch (ent->sme_bsb.justify) {
    case XtJustifyLeft:
      *text_x = ent->rectangle.x + s + ent->sme_bsb.left_margin;
      break;
    case XtJustifyCenter:
      const Position middle_width = (Position)ent->rectangle.width -
	ent->sme_bsb.left_margin -
	ent->sme_bsb.right_margin -
	s*2;
      *text_x = ent->rectangle.x + s + ent->sme_bsb.left_margin +
	(middle_width - ent->sme_bsb.label_width)/2;
      break;
    case XtJustifyRight:
      *text_x = ent->rectangle.x + ent->rectangle.width -
	ent->sme_bsb.right_margin - s - ent->sme_bsb.label_width;
    }
  }
  if (text_y)
    *text_y = (Position)ent->rectangle.y +
      ((Position)ent->rectangle.height - (Position)ent->sme_bsb.label_height)/2;
}

/*      Function Name: DrawBitmaps
 *      Description: Draws left and right bitmaps.
 *      Arguments: w - the simple menu widget.
 *      Returns: none
 */

static void DrawBitmaps (Widget w) {
  Position x_loc, y_loc;
  SmeBSBObject ent = (SmeBSBObject)w;
  GC gc = ent->sme_bsb.normal_GC;

  /*
   * Draw left bitmap
   */

  if (ent->sme_bsb.left_bitmap) {
    x_loc = (Position)ent->rectangle.x +
      (Position)ent->sme_threeD.shadow_width +
      ((Position)ent->sme_bsb.left_margin -
       (Position)ent->sme_bsb.left_bitmap_width) / 2;
    y_loc = (Position)ent->rectangle.y +
      ((Position)ent->rectangle.height -
       (Position)ent->sme_bsb.left_bitmap_height) / 2;
    Xaw3dXftCopy(XtDisplayOfObject(w), ent->sme_bsb.left_bitmap,
		 XtWindowOfObject(w), gc,
		 ent->sme_bsb.left_bitmap_width,
		 ent->sme_bsb.left_bitmap_height,
		 ent->sme_bsb.left_depth,
		 x_loc, y_loc);
  }

  /*
   * Draw right bitmap
   */

  if (ent->sme_bsb.right_bitmap) {
    x_loc = (Position)ent->rectangle.x +
      (Position)ent->rectangle.width -
      (Position)ent->sme_threeD.shadow_width -
      (Position)(ent->sme_bsb.right_margin +
		 ent->sme_bsb.right_bitmap_width) / 2;
    y_loc = (Position)ent->rectangle.y +
      ((Position)ent->rectangle.height -
       (Position)ent->sme_bsb.right_bitmap_height) / 2;
    Xaw3dXftCopy(XtDisplayOfObject(w), ent->sme_bsb.right_bitmap,
		 XtWindowOfObject(w), gc,
		 ent->sme_bsb.right_bitmap_width,
		 ent->sme_bsb.right_bitmap_height,
		 ent->sme_bsb.right_depth,
		 x_loc, y_loc);
  }
}

/*      Function Name: GetBitmapDimensions
 *      Description: Gets the dimensions of either of the bitmaps.
 *      Arguments: w - the bsb menu entry widget.
 *                 is_left - TRUE if we are testing left bitmap,
 *                           FALSE if we are testing the right bitmap.
 *      Returns: none
 */

static void GetBitmapDimensions (Widget w, Boolean is_left) {
  SmeBSBObject entry = (SmeBSBObject)w;
  if (is_left) {
    entry->sme_bsb.left_bitmap_width = entry->sme_bsb.left_bitmap_height = 0;
    if (entry->sme_bsb.left_bitmap != None) {
      if (!Xaw3dXftGetDrawableDimensions(XtDisplayOfObject(w),
      entry->sme_bsb.left_bitmap, &entry->sme_bsb.left_bitmap_width,
      &entry->sme_bsb.left_bitmap_height, &entry->sme_bsb.left_depth)) {
	char buf[BUFSIZ];
	(void) sprintf(buf, "Xaw SmeBSB Object: %s %s \"%s\".",
		       "Could not get Left Bitmap",
		       "geometry information for menu entry",
		       XtName(w));
	XtAppError(XtWidgetToApplicationContext(w), buf);
      }
    }
  } else {
    entry->sme_bsb.right_bitmap_width = entry->sme_bsb.right_bitmap_height = 0;
    if (entry->sme_bsb.right_bitmap != None) {
      if (!Xaw3dXftGetDrawableDimensions(XtDisplayOfObject(w),
      entry->sme_bsb.right_bitmap, &entry->sme_bsb.right_bitmap_width,
      &entry->sme_bsb.right_bitmap_height, &entry->sme_bsb.right_depth)) {
	char buf[BUFSIZ];
	(void) sprintf(buf, "Xaw SmeBSB Object: %s %s \"%s\".",
		       "Could not get Right Bitmap",
		       "geometry information for menu entry",
		       XtName(w));
	XtAppError(XtWidgetToApplicationContext(w), buf);
      }
    }
  }
}

// Used in Redisplay, Highlight, and Unhighlight
static void DrawTextAndUnderline (Widget w, GC gc, XftColor *xfg,
				  XftColor *xbg) {
  SmeBSBObject ent = (SmeBSBObject)w;
  Display *display = XtDisplayOfObject(w);
  Window window = XtWindowOfObject(w);
  Visual *visual = VisualOf(ent);
  Widget parent = ObjParent(ent);
  Colormap cmap = parent->core.colormap;
  Position text_x, text_y;
  GetTextPosition(ent, &text_x, &text_y);
  Xaw3dXftDrawAnyString(display, visual, cmap, window, ent->sme_bsb.font,
    menuFontSet(ent), ent->sme_bsb.xftfont, True, menuIntl(ent), gc, None,
    xfg, xbg, text_x, text_y, ent->sme_bsb.encoding, ent->sme_bsb.label);
  if (ent->sme_bsb.underline >= 0) {
    Position x1, x2, y;
    if (Xaw3dXftLocateUnderline(display, ent->sme_bsb.font, menuFontSet(ent),
        ent->sme_bsb.xftfont, menuIntl(ent), ent->sme_bsb.encoding,
	ent->sme_bsb.label, ent->sme_bsb.underline, &x1, &x2, &y)) {
      x1 += text_x;
      x2 += text_x;
      y  += text_y;
      XDrawLine(display, window, gc, x1, y, x2, y);
    }
  }
}

/************************************************************
 *
 * Semi-public functions
 *
 ************************************************************/

/*	Function Name: ClassInitialize
 *	Description: Initializes the SmeBSBObject.
 *	Arguments: none.
 *	Returns: none.
 */

static void
ClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRJustify, XmuCvtStringToJustify,
		    (XtConvertArgList)NULL, (Cardinal)0 );
}

/*      Function Name: Initialize
 *      Description: Initializes the simple menu widget
 *      Arguments: request - the widget requested by the argument list.
 *                 new     - the new widget with both resource and non
 *                           resource values.
 *      Returns: none.
 */

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  SmeBSBObject entry = (SmeBSBObject)new;

  Xaw3dXftGetVisualInfo(new, &VisualOf(entry), NULL, NULL);
  entry->sme_bsb.normal_GC =
    entry->sme_bsb.rev_GC =
    entry->sme_bsb.stipple_GC =
    entry->sme_bsb.xor_fgbg_GC =
    entry->sme_bsb.xor_bghl_GC =
    NULL;
  entry->sme_bsb.xorSet = False;

  if (entry->sme_bsb.xftfontname)
    entry->sme_bsb.xftfont = Xaw3dXftGetFont(new, entry->sme_bsb.xftfontname);
  else {
    entry->sme_bsb.xftfont = NULL;
#ifdef XAW_INTERNATIONALIZATION
    if (entry->sme.international && !entry->sme_bsb.fontset)
      XtError("SmeBSB initialized with international true but no fontset\n");
    else
#endif
      if (!entry->sme_bsb.font) XtError("SmeBSB initialized with no font\n");
  }

  // Avoid surprises:  just always dup the string.
  if (entry->sme_bsb.label == NULL && XtName(new) != NULL)
    // Same encoding question as in Label.
    entry->sme_bsb.label = Xaw3dXftAnyStrdup(entry->sme_bsb.encoding,
					     XtName(new));
  else if (entry->sme_bsb.label != NULL)
    entry->sme_bsb.label = Xaw3dXftAnyStrdup(entry->sme_bsb.encoding,
					     entry->sme_bsb.label);

  get_or_change_GCs(entry);
  GetBitmapDimensions(new, True);	/* Left bitmap Info */
  GetBitmapDimensions(new, False);	/* Right bitmap Info */
  GetTextDimensions(entry);
  GetDefaultSize(entry, &(entry->rectangle.width), &(entry->rectangle.height));
}

/*      Function Name: Destroy
 *      Description: Called at destroy time, cleans up.
 *      Arguments: w - the SmeBSB object.
 *      Returns: none.
 */

static void Destroy(Widget w) {
  SmeBSBObject ent = (SmeBSBObject)w;
  // As per Initialize, we always dup label.
  free(ent->sme_bsb.label);
  if (ent->sme_bsb.normal_GC)
    XtReleaseGC(w, ent->sme_bsb.normal_GC);
  if (ent->sme_bsb.rev_GC)
    XtReleaseGC(w, ent->sme_bsb.rev_GC);
  if (ent->sme_bsb.stipple_GC)
    XtReleaseGC(w, ent->sme_bsb.stipple_GC);
  if (ent->sme_bsb.xor_fgbg_GC)
    XtReleaseGC(w, ent->sme_bsb.xor_fgbg_GC);
  if (ent->sme_bsb.xor_bghl_GC)
    XtReleaseGC(w, ent->sme_bsb.xor_bghl_GC);
}

/*      Function Name: Redisplay
 *      Description: Redisplays the contents of the widget.
 *      Arguments: w - the SmeBSB object.
 *                 event - the X event that caused this redisplay.
 *                 region - the region the needs to be repainted.
 *      Returns: none.
 */

// This supports redisplaying in a highlighted state, but I don't know
// whether this code is actually reachable while highlighted.
static void Redisplay (Widget w, XEvent *event, Region region) {
  SmeBSBObject ent = (SmeBSBObject)w;
  Display *display = XtDisplayOfObject(w);
  Window window = XtWindowOfObject(w);
  Widget parent = ObjParent(ent);
  const Boolean sensitive = XtIsSensitive(w),
                mouseover = (w == XawSimpleMenuGetActiveEntry(parent));

  // Clean slate
  XClearArea(display, window, ent->rectangle.x, ent->rectangle.y,
	     ent->rectangle.width, ent->rectangle.height, False);
  ent->sme_bsb.xorSet = False;

  // Draw bitmaps
  DrawBitmaps(w);

  // Apply reverse color or highlight to bg and bitmaps
  if (sensitive && mouseover &&
      ent->sme_bsb.highlightStyle != MenuHighlightShadow) {
    GC gc = (ent->sme_bsb.highlightStyle == MenuHighlightReverse ?
	     ent->sme_bsb.xor_fgbg_GC : ent->sme_bsb.xor_bghl_GC);
    XFillRectangle(display, window, gc,
		   ent->rectangle.x, ent->rectangle.y,
		   ent->rectangle.width, ent->rectangle.height);
    ent->sme_bsb.xorSet = True;
  }

  // Draw label text
  if (ent->sme_bsb.label) {
    GC gc;
    XftColor *xfg, *xbg;
    if (sensitive && mouseover &&
	ent->sme_bsb.highlightStyle != MenuHighlightShadow) {
      if (ent->sme_bsb.highlightStyle == MenuHighlightReverse) {
	gc = ent->sme_bsb.rev_GC;
	xfg = &ent->sme_bsb.xftbg;
	xbg = &ent->sme_bsb.xftfg;
      } else {     // MenuHighlightBackground
	gc = ent->sme_bsb.normal_GC;
	xfg = &ent->sme_bsb.xftfg;
	xbg = &ent->sme_bsb.xfthl;
      }
    } else {
      gc = ent->sme_bsb.normal_GC;
      xfg = &ent->sme_bsb.xftfg;
      xbg = &ent->sme_bsb.xftbg;
    }
    DrawTextAndUnderline(w, gc, xfg, xbg);
  }

  // Apply insensitive stipple (never on top of reverse color or highlight)
  if (!sensitive)
    XFillRectangle(display, window, ent->sme_bsb.stipple_GC,
		   ent->rectangle.x, ent->rectangle.y,
		   ent->rectangle.width, ent->rectangle.height);

  // Draw shadows if applicable
  ent->sme_threeD.shadowed = (sensitive && mouseover &&
    ent->sme_bsb.highlightStyle == MenuHighlightShadow &&
    ent->sme_threeD.shadow_width > 0);
  if (ent->sme_threeD.shadowed) {
    SmeBSBObjectClass oclass = (SmeBSBObjectClass)XtClass(w);
    (*oclass->sme_threeD_class.shadowdraw) (w);
  }
}


/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  SmeBSBObject cur_ent = (SmeBSBObject)current,
               new_ent = (SmeBSBObject)new;
  Boolean ret_val = False;

  // Code as incoming from Xaw ignored changes to width and height.  ?

  // The default lazy policy is to recalculate everything and repaint.
  // rectangle.sensitive:  ancestor_sensitive can't change here.
  if (cur_ent->sme_bsb.underline != new_ent->sme_bsb.underline ||
#ifdef XAW_INTERNATIONALIZATION
      cur_ent->sme.international != new_ent->sme.international ||
#endif
      cur_ent->rectangle.sensitive != new_ent->rectangle.sensitive ||
      cur_ent->sme_bsb.left_margin != new_ent->sme_bsb.left_margin ||
      cur_ent->sme_bsb.right_margin != new_ent->sme_bsb.right_margin ||
      cur_ent->sme_bsb.vert_space != new_ent->sme_bsb.vert_space ||
      cur_ent->sme_bsb.encoding != new_ent->sme_bsb.encoding ||
      cur_ent->sme_bsb.highlightStyle != new_ent->sme_bsb.highlightStyle)
    ret_val = True;

  // Changes to fontset are irrelevant unless international is true.
#ifdef XAW_INTERNATIONALIZATION
  if (cur_ent->sme_bsb.fontset != new_ent->sme_bsb.fontset &&
      new_ent->sme.international)
    ret_val = True;
#endif

  // Notice if the label text changed.
  if (cur_ent->sme_bsb.label != new_ent->sme_bsb.label) {
    // As per Initialize, we always dup label.
    if (cur_ent->sme_bsb.label)
      free(cur_ent->sme_bsb.label);
    if (new_ent->sme_bsb.label == NULL && XtName(new) != NULL)
      new_ent->sme_bsb.label = Xaw3dXftAnyStrdup(new_ent->sme_bsb.encoding,
						 XtName(new));
    else if (new_ent->sme_bsb.label != NULL)
      new_ent->sme_bsb.label = Xaw3dXftAnyStrdup(new_ent->sme_bsb.encoding,
						 new_ent->sme_bsb.label);
    ret_val = True;
  }

  // Notice if colors or plain old font changed.
  // We have no idea whether Parent->core.background_pixel changed.
  if (cur_ent->sme_bsb.foreground != new_ent->sme_bsb.foreground ||
      cur_ent->sme_bsb.highlight != new_ent->sme_bsb.highlight ||
      cur_ent->sme_bsb.font->fid != new_ent->sme_bsb.font->fid) {
    get_or_change_GCs(new_ent);
    ret_val = True;
  }

  // Notice if xftfont changed.
  if (cur_ent->sme_bsb.xftfontname != new_ent->sme_bsb.xftfontname) {
    if (new_ent->sme_bsb.xftfontname)
      new_ent->sme_bsb.xftfont = Xaw3dXftGetFont(new,
						 new_ent->sme_bsb.xftfontname);
    else
      new_ent->sme_bsb.xftfont = NULL;
    ret_val = True;
  }

  // Notice if bitmaps changed.
  if (cur_ent->sme_bsb.left_bitmap != new_ent->sme_bsb.left_bitmap) {
    GetBitmapDimensions(new, True);
    ret_val = True;
  }
  if (cur_ent->sme_bsb.right_bitmap != new_ent->sme_bsb.right_bitmap) {
    GetBitmapDimensions(new, False);
    ret_val = True;
  }

  // Recalculate and redraw as needed.
  if (ret_val) {
    new_ent->sme_bsb.xorSet = False;
    GetTextDimensions(new_ent);
    GetDefaultSize(new_ent, &new_ent->rectangle.width,
		   &new_ent->rectangle.height);
    (XtParent(new)->core.widget_class->core_class.resize)(new);
  }
  return ret_val;
}

/*	Function Name: QueryGeometry.
 *	Description: Returns the preferred geometry for this widget.
 *	Arguments: w - the menu entry object.
 *                 itended, return_val - the intended and return geometry info.
 *	Returns: A Geometry Result.
 */

static XtGeometryResult
QueryGeometry(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *return_val)
{
  XtGeometryMask mode = intended->request_mode;
  Dimension width, height;
  XtGeometryResult ret_val = XtGeometryYes;
  SmeBSBObject ent = (SmeBSBObject)w;

  // Not assuming that our current rectangle dimensions are the ones that we
  // calculated and set earlier
  GetDefaultSize(ent, &width, &height);

    if ( ((mode & CWWidth) && (intended->width != width)) ||
	 !(mode & CWWidth) ) {
	return_val->request_mode |= CWWidth;
	return_val->width = width;
	ret_val = XtGeometryAlmost;
    }

    if ( ((mode & CWHeight) && (intended->height != height)) ||
	 !(mode & CWHeight) ) {
	return_val->request_mode |= CWHeight;
	return_val->height = height;
	ret_val = XtGeometryAlmost;
    }

    if (ret_val == XtGeometryAlmost) {
	mode = return_val->request_mode;

	if ( ((mode & CWWidth) && (width == ent->rectangle.width)) &&
	     ((mode & CWHeight) && (height == ent->rectangle.height)) )
	    return(XtGeometryNo);
    }

  // This *definitely* should not be happening here.  QueryGeometry is not
  // Resize.
  // ent->rectangle.width = width;
  // ent->rectangle.height = height;

  return ret_val;
}

// The following are triggered by SimpleMenu when it thinks that the mouse
// has entered or exited a particular menu entry.

static void Highlight (Widget w) {
  SmeBSBObject ent = (SmeBSBObject)w;
  Widget parent = ObjParent(ent);
  const Boolean sensitive = XtIsSensitive(w),
                mouseover = (w == XawSimpleMenuGetActiveEntry(parent));
  assert(sensitive && mouseover);
  if (ent->sme_bsb.highlightStyle == MenuHighlightShadow) {
    if (!ent->sme_threeD.shadowed && ent->sme_threeD.shadow_width > 0) {
      ent->sme_threeD.shadowed = True;
      SmeBSBObjectClass oclass = (SmeBSBObjectClass)XtClass(w);
      (*oclass->sme_threeD_class.shadowdraw) (w);
    }
  } else if (!ent->sme_bsb.xorSet) {
    Display *display = XtDisplayOfObject(w);
    Window window = XtWindowOfObject(w);
    GC gc = (ent->sme_bsb.highlightStyle == MenuHighlightReverse ?
	     ent->sme_bsb.xor_fgbg_GC : ent->sme_bsb.xor_bghl_GC);
    XFillRectangle(display, window, gc,
		   ent->rectangle.x, ent->rectangle.y,
		   ent->rectangle.width, ent->rectangle.height);
    ent->sme_bsb.xorSet = True;
    // If highlightStyle == MenuHighlightReverse, text redraw is needed only
    // if anti-aliased.
    if (ent->sme_bsb.highlightStyle == MenuHighlightBackground ||
	ent->sme_bsb.xftfont) {
      XftColor *xfg, *xbg;
      if (ent->sme_bsb.highlightStyle == MenuHighlightReverse) {
	gc = ent->sme_bsb.rev_GC;   // unused
	xfg = &ent->sme_bsb.xftbg;
	xbg = &ent->sme_bsb.xftfg;
      } else {     // MenuHighlightBackground
	gc = ent->sme_bsb.normal_GC;
	xfg = &ent->sme_bsb.xftfg;
	xbg = &ent->sme_bsb.xfthl;
      }
      DrawTextAndUnderline(w, gc, xfg, xbg);
    }
  }
}

static void Unhighlight (Widget w) {
  SmeBSBObject ent = (SmeBSBObject)w;
  Widget parent = ObjParent(ent);
  const Boolean sensitive = XtIsSensitive(w),
                mouseover = (w == XawSimpleMenuGetActiveEntry(parent));
  assert(sensitive && !mouseover);
  if (ent->sme_bsb.highlightStyle == MenuHighlightShadow) {
    if (ent->sme_threeD.shadowed && ent->sme_threeD.shadow_width > 0) {
      ent->sme_threeD.shadowed = False;
      SmeBSBObjectClass oclass = (SmeBSBObjectClass)XtClass(w);
      (*oclass->sme_threeD_class.shadowdraw) (w);
    } else ent->sme_threeD.shadowed = False; // no point leaving this on
  } else if (ent->sme_bsb.xorSet) {
    Display *display = XtDisplayOfObject(w);
    Window window = XtWindowOfObject(w);
    GC gc = (ent->sme_bsb.highlightStyle == MenuHighlightReverse ?
	     ent->sme_bsb.xor_fgbg_GC : ent->sme_bsb.xor_bghl_GC);
    XFillRectangle(display, window, gc,
		   ent->rectangle.x, ent->rectangle.y,
		   ent->rectangle.width, ent->rectangle.height);
    ent->sme_bsb.xorSet = False;
    // If highlightStyle == MenuHighlightReverse, text redraw is needed only
    // if anti-aliased.
    if (ent->sme_bsb.highlightStyle == MenuHighlightBackground ||
	ent->sme_bsb.xftfont)
      DrawTextAndUnderline(w, ent->sme_bsb.normal_GC, &ent->sme_bsb.xftfg,
			   &ent->sme_bsb.xftbg);
  }
}
