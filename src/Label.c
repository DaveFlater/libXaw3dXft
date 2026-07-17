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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
/*
 * Label.c - Label widget
 *
 */

#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/Command.h>
#include <X11/Xaw3dXft/LabelP.h>
#include <X11/Xaw3dXft/AnyString.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Drawing.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define streq(a,b) (strcmp( (a), (b) ) == 0)


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(LabelRec, field)
static_assert(Got_XAW_defines);
static XtResource resources[] = {
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	offset(label.foreground), XtRString, XtDefaultForeground},
    {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	offset(label.font),XtRString, XtDefaultFont},
    {XtNxftFont,  XtCXftFont, XtRString, sizeof(String),
	offset(label.xftfontname),XtRString, NULL},
#ifdef XAW_INTERNATIONALIZATION
    {XtNfontSet,  XtCFontSet, XtRFontSet, sizeof(XFontSet ),
        offset(label.fontset),XtRString, XtDefaultFontSet},
#endif
    {XtNlabel,  XtCLabel, XtRString, sizeof(String),
	offset(label.label), XtRString, NULL},
    {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
	offset(label.encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
    {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
	offset(label.justify), XtRImmediate, (XtPointer)XtJustifyCenter},
    {XtNinternalWidth, XtCWidth, XtRDimension,  sizeof(Dimension),
	offset(label.internal_width), XtRImmediate, (XtPointer)4},
    {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
	offset(label.internal_height), XtRImmediate, (XtPointer)2},
    {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
       offset(label.left_bitmap), XtRImmediate, (XtPointer) None},
    {XtNbitmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset(label.pixmap), XtRImmediate, (XtPointer)None},
    {XtNresize, XtCResize, XtRBoolean, sizeof(Boolean),
	offset(label.resize), XtRImmediate, (XtPointer)True},
    {XtNshadowWidth, XtCShadowWidth, XtRDimension, sizeof(Dimension),
	offset(threeD.shadow_width), XtRImmediate, (XtPointer) 0},
    {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
         XtOffsetOf(RectObjRec,rectangle.border_width), XtRImmediate,
         (XtPointer)1}
};
#undef offset

static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Resize(Widget);
static void Redisplay(Widget, XEvent *, Region);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);
static void ClassInitialize(void);
static void Destroy(Widget);
static XtGeometryResult QueryGeometry(Widget, XtWidgetGeometry *, XtWidgetGeometry *);

LabelClassRec labelClassRec = {
  {
/* core_class fields */
    /* superclass	  	*/	(WidgetClass) &threeDClassRec,
    /* class_name	  	*/	"Label",
    /* widget_size	  	*/	sizeof(LabelRec),
    /* class_initialize   	*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize		  	*/	XtInheritRealize,
    /* actions		  	*/	NULL,
    /* num_actions	  	*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* compress_motion	  	*/	TRUE,
    /* compress_exposure  	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* resize		  	*/	Resize,
    /* expose		  	*/	Redisplay,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus	 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* tm_table		   	*/	NULL,
    /* query_geometry		*/	QueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
/* Simple class fields initialization */
  {
    /* change_sensitive		*/	XtInheritChangeSensitive
  },
/* ThreeD class fields initialization */
  {
    /* shadowdraw 		*/	XtInheritXaw3dShadowDraw
  },
/* Label class fields initialization */
  {
    /* ignore 			*/	0
  }
};

WidgetClass labelWidgetClass = (WidgetClass)&labelClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#define VisualOf(w) (w->label.visual)

static void
ClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRJustify, XmuCvtStringToJustify,
		    (XtConvertArgList)NULL, 0 );
}

/*
 * Calculate width and height of displayed text in pixels
 */

static void
SetTextWidthAndHeight(LabelWidget lw)
{
    if (lw->label.pixmap != None) {
	Window root;
	int x, y;
	unsigned int width, height, bw;

        // label.depth is not core.depth
	if (XGetGeometry(XtDisplay(lw), lw->label.pixmap, &root, &x, &y,
			 &width, &height, &bw, &lw->label.depth)) {
	    lw->label.label_height = height; // uint32 to uint16
	    lw->label.label_width = width;   // uint32 to uint16
	    return;
	} else fprintf(stderr,
"libXaw3dXft:  XGetGeometry failed in SetTextWidthAndHeight of Label.  There\n"
"might be something wrong with the bitmap resource of a Label widget.\n");
        // Then we just fall through and use the text dimensions‽
    }

    if (lw->label.label == NULL)
      lw->label.label_height = lw->label.label_width = 0;
    else
      Xaw3dXftSizeAnyString(XtDisplay(lw), lw->label.font, labelFontSet(lw),
	lw->label.xftfont, international(lw), lw->label.encoding,
	lw->label.label, &lw->label.label_width, &lw->label.label_height);
}

static void
GetnormalGC(LabelWidget lw)
{
    XGCValues	values;

    // There's no slack given here for the plain old font to be null when
    // you're using a fontSet or xftFont.  It won't *be* null unless someone
    // explicitly nulls it out.
    assert(lw->label.font);

    values.foreground	= lw->label.foreground;
    values.background	= lw->core.background_pixel;
    values.font		= lw->label.font->fid;
    values.graphics_exposures = False;

#ifdef XAW_INTERNATIONALIZATION
    if ( lw->simple.international == True )
        /* Since Xmb/wcDrawString eats the font, I must use XtAllocateGC. */
        // That means:  Xmb/wcDrawString does XSetFont on the GC.
        lw->label.normal_GC = XtAllocateGC(
                (Widget)lw, 0,
	(unsigned) GCForeground | GCBackground | GCGraphicsExposures,
	&values, GCFont, 0 );
    else
#endif
        lw->label.normal_GC = XtGetGC(
	(Widget)lw,
	(unsigned) GCForeground | GCBackground | GCFont | GCGraphicsExposures,
	&values);
}

static void
GetgrayGC(LabelWidget lw)
{
    XGCValues	values;

    values.foreground = lw->label.foreground;
    values.background = lw->core.background_pixel;
    values.font	      = lw->label.font->fid;
    values.fill_style = FillTiled;
    values.tile       = XmuCreateStippledPixmap(XtScreen((Widget)lw),
						lw->label.foreground,
						lw->core.background_pixel,
						lw->core.depth);
    values.graphics_exposures = False;

    lw->label.stipple = values.tile;
#ifdef XAW_INTERNATIONALIZATION
    if ( lw->simple.international == True )
        /* Since Xmb/wcDrawString eats the font, I must use XtAllocateGC. */
        lw->label.gray_GC = XtAllocateGC((Widget)lw,  0,
				(unsigned) GCForeground | GCBackground |
					   GCTile | GCFillStyle |
					   GCGraphicsExposures,
				&values, GCFont, 0);
    else
#endif
        lw->label.gray_GC = XtGetGC((Widget)lw,
				(unsigned) GCForeground | GCBackground |
					   GCFont | GCTile | GCFillStyle |
					   GCGraphicsExposures,
				&values);
}

// Given:    left_bitmap
// Compute:  lbm_width, lbm_height
// left_bitmap is ignored when pixmap is present.
static void
get_lbm_dimensions (LabelWidget lw)
{
    Window root;
    int x, y;
    unsigned int bw;

    lw->label.lbm_width = lw->label.lbm_height = 0;
    if (!lw->label.pixmap && lw->label.left_bitmap) {
        // label.depth is not core.depth
	Status status = XGetGeometry(XtDisplay(lw),
				     lw->label.left_bitmap,
				     &root, &x, &y,
				     &lw->label.lbm_width,
				     &lw->label.lbm_height,
				     &bw, &lw->label.depth);
	if (!status) {
	    fprintf(stderr,
"libXaw3dXft:  XGetGeometry failed in get_lbm_dimensions of Label.  There\n"
"might be something wrong with the leftBitmap resource of a Label widget.\n");
	    // The following suppresses the left bitmap.
	    lw->label.lbm_width = lw->label.lbm_height = 0;
	}
    }
}

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    LabelWidget lw = (LabelWidget) new;

    Xaw3dXftGetVisualInfo(new, &VisualOf(lw), NULL, NULL);
    lw->label.xorSet = False;

    if (lw->label.xftfontname)
	lw->label.xftfont = Xaw3dXftGetFont(new, lw->label.xftfontname);
    else {
	lw->label.xftfont = NULL;
#ifdef XAW_INTERNATIONALIZATION
	if (lw->simple.international && !lw->label.fontset)
	  XtError("Label initialized with international true but no fontset\n");
	else
#endif
	if (!lw->label.font) XtError("Label initialized with no font\n");
    }

    // Avoid surprises:  just always dup the string.
    if (lw->label.label == NULL && lw->core.name != NULL)
      // Unsure what to do about encoding when defaulting to core.name.
      // It's required to be const char *, but could be UTF8.
      lw->label.label = Xaw3dXftAnyStrdup(lw->label.encoding, lw->core.name);
    else if (lw->label.label != NULL)
      lw->label.label = Xaw3dXftAnyStrdup(lw->label.encoding, lw->label.label);

    GetnormalGC(lw);
    GetgrayGC(lw);
    lw->label.stipple_GC = Xaw3dXftGetStippleGC(new, lw->core.background_pixel);
    Xaw3dXftGetXftColor(XtDisplay(lw), VisualOf(lw), lw->core.colormap,
      lw->label.foreground, &lw->label.xftfg);
    Xaw3dXftGetXftColor(XtDisplay(lw), VisualOf(lw), lw->core.colormap,
      lw->core.background_pixel, &lw->label.xftbg);

    SetTextWidthAndHeight(lw);  /* label.label or label.pixmap */

    get_lbm_dimensions(lw);
    if (lw->core.height == 0)
	lw->core.height = (Dimension) (
	  (lw->label.lbm_height > lw->label.label_height ?
	   lw->label.lbm_height : lw->label.label_height) +
				       2 * lw->label.internal_height +
				       2 * lw->threeD.shadow_width);

    if (lw->core.width == 0)
	lw->core.width = (Dimension) (lw->label.label_width +
				      (2 * lw->label.internal_width) +
				      (2 * lw->threeD.shadow_width) +
				      (Dimension) LEFT_OFFSET(lw)); /* req's label.lbm_width */

    lw->label.label_x = lw->label.label_y = 0;
    (*XtClass(new)->core_class.resize) ((Widget)lw);

    lw->label.stippled = lw->label.left_stippled = None;
} /* Initialize */

/*
 * Repaint the widget window
 */

/* ARGSUSED */
static void
Redisplay(Widget gw, XEvent *event, Region region)
{
    LabelWidget w = (LabelWidget) gw;
    LabelWidgetClass lwclass = (LabelWidgetClass) XtClass (gw);
    Pixmap pm;

    /*
     * Don't draw shadows if Command is going to redraw them.
     * The shadow draw method is region aware, but since 99% of
     * all labels don't have shadows, we'll check for a shadow
     * before we incur the function call overhead.
     */
    if (!XtIsSubclass (gw, commandWidgetClass) && w->threeD.shadow_width > 0)
	(*lwclass->threeD_class.shadowdraw) (gw, event, region,
					     w->threeD.relief, True);

    /*
     * now we'll see if we need to draw the rest of the label
     */
    if (region != NULL) {
	int x = w->label.label_x;
	unsigned int width = w->label.label_width;
	if (w->label.lbm_width) {
	    if (w->label.label_x > (x = w->label.internal_width))
		width += w->label.label_x - x;
	}
	if (XRectInRegion(region, x, w->label.label_y,
			 width, w->label.label_height) == RectangleOut){
	    return;
	}
    }

    GC gc;
    if (XtIsSensitive(gw))
      gc = w->label.normal_GC;
    else
      gc = w->label.gray_GC;

    if (w->label.pixmap == None) {
	/* draw left bitmap */
	if (w->label.left_bitmap && w->label.lbm_width &&
	    w->label.lbm_height) {
	    pm = w->label.left_bitmap;
#ifdef XAW_MULTIPLANE_PIXMAPS
	    if (!XtIsSensitive(gw)) {
		if (w->label.left_stippled == None)
		    w->label.left_stippled = stipplePixmap(gw,
				w->label.left_bitmap, w->core.colormap,
				w->core.background_pixel, w->label.depth);
		if (w->label.left_stippled != None)
		    pm = w->label.left_stippled;
	    }
#endif

	    if (w->label.depth == 1)
		XCopyPlane(XtDisplay(gw), pm, XtWindow(gw), gc, 0, 0,
			   w->label.lbm_width, w->label.lbm_height,
			   (int) w->label.internal_width +
				 w->threeD.shadow_width,
			   ((int)w->core.height - (int)w->label.lbm_height)/2,
			   (unsigned long) 1L);
	    else
		XCopyArea(XtDisplay(gw), pm, XtWindow(gw), gc, 0, 0,
			  w->label.lbm_width, w->label.lbm_height,
			  (int) w->label.internal_width +
				w->threeD.shadow_width,
			  ((int)w->core.height - (int)w->label.lbm_height)/2);
	}

	/* draw label text */
	if (w->label.label)
	  Xaw3dXftDrawAnyString(XtDisplay(w), VisualOf(w),
	    w->core.colormap, XtWindow(w), w->label.font, labelFontSet(w),
	    w->label.xftfont, XtIsSensitive(gw), international(w), gc,
	    w->label.stipple_GC, &w->label.xftfg, &w->label.xftbg,
	    w->label.label_x, w->label.label_y, w->label.encoding,
	    w->label.label);

    } else { // w->label.pixmap != None
	pm = w->label.pixmap;
#ifdef XAW_MULTIPLANE_PIXMAPS
	if (!XtIsSensitive(gw)) {
	    if (w->label.stippled == None)
		w->label.stippled = stipplePixmap(gw,
				w->label.pixmap, w->core.colormap,
				w->core.background_pixel, w->label.depth);
	    if (w->label.stippled != None)
		pm = w->label.stippled;
	}
#endif

	if (w->label.depth == 1)
	    XCopyPlane(XtDisplay(gw), pm, XtWindow(gw), gc, 0, 0,
		       w->label.label_width, w->label.label_height,
		       w->label.label_x, w->label.label_y,
		       1L);
	else
	    XCopyArea(XtDisplay(gw), pm, XtWindow(gw), gc, 0, 0,
		      w->label.label_width, w->label.label_height,
		      w->label.label_x, w->label.label_y);
    }

    /*
      Of unknown utility:
      XSetRegion(XtDisplay(gw), gc, region);
      XSetClipMask(XtDisplay(gw), gc, (Pixmap)None);
    */
}

static void
_Reposition(LabelWidget lw, Dimension width, Dimension height,
            Position *dx, Position *dy)
{
    Position newPos;
    Position leftedge = (Position)(lw->label.internal_width +
			           LEFT_OFFSET(lw) +
                                   lw->threeD.shadow_width);

    switch (lw->label.justify) {
	case XtJustifyLeft:
	    newPos = leftedge;
	    break;
	case XtJustifyRight:
	    newPos = (Position)width -
		     (Position)(lw->label.label_width +
				lw->label.internal_width +
				lw->threeD.shadow_width);
	    break;
	case XtJustifyCenter:
	default:
	    newPos = ((Position)width - (Position)lw->label.label_width) / 2;
	    break;
    }

    if (newPos < leftedge)
	newPos = leftedge;
    *dx = newPos - lw->label.label_x;
    lw->label.label_x = newPos;

    *dy = (newPos = ((Position)height - (Position)lw->label.label_height) / 2)
	  - lw->label.label_y;
    lw->label.label_y = newPos;
}

static void
Resize(Widget w)
{
    LabelWidget lw = (LabelWidget)w;
    Position dx, dy;

    _Reposition(lw, w->core.width, w->core.height, &dx, &dy);
}

/*
 * Set specified arguments into widget
 */

#define WIDTH		0
#define HEIGHT		1
#define NUM_CHECKS	2

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    LabelWidget curlw = (LabelWidget)current;
    LabelWidget reqlw = (LabelWidget)request;
    LabelWidget newlw = (LabelWidget)new;
    Boolean was_resized = False, redisplay = False, checks[NUM_CHECKS];

    // was_resized incurs a repaint but furthermore causes label position to
    // be recalculated.  If redisplay is set but was_resized is not, you get
    // repaint but not the recalculation.

    // Flags to indicate when something appeared in the arglist regardless
    // whether its value changed.
    {
      Cardinal c;
      for (c = 0; c < NUM_CHECKS; c++)
	checks[c] = False;
      for (c = 0; c < *num_args; c++) {
	if (streq(XtNwidth, args[c].name))
	  checks[WIDTH] = True;
	if (streq(XtNheight, args[c].name))
	  checks[HEIGHT] = True;
      }
    }

    // Notice if the label text changed.
    if (curlw->label.label != newlw->label.label) {
        // As per Initialize, we always dup label.
        if (curlw->label.label)
	  free(curlw->label.label);
	if (newlw->label.label == NULL && newlw->core.name != NULL)
	  newlw->label.label = Xaw3dXftAnyStrdup(newlw->label.encoding, newlw->core.name);
	else if (newlw->label.label != NULL)
	  newlw->label.label = Xaw3dXftAnyStrdup(newlw->label.encoding, newlw->label.label);
	was_resized = True;
    }
    // If *only* the encoding changed, we gain nothing by repeating the
    // strdup, but it might display differently.
    if (curlw->label.encoding != newlw->label.encoding)
      was_resized = True;

    // Notice if the font changed.
    if (curlw->label.xftfontname != newlw->label.xftfontname) {
      if (newlw->label.xftfontname)
	newlw->label.xftfont = Xaw3dXftGetFont(new, newlw->label.xftfontname);
      else
	newlw->label.xftfont = NULL;
      was_resized = True;
    }
    if (curlw->label.font->fid != newlw->label.font->fid)
      was_resized = True;
#ifdef XAW_INTERNATIONALIZATION
    if (curlw->simple.international &&
      curlw->label.fontset != newlw->label.fontset)
      was_resized = True;
#endif

    // Notice if the pixmap changed.
    if (curlw->label.pixmap != newlw->label.pixmap) {
      newlw->label.stippled = None;
      if (curlw->label.stippled != None)
	XFreePixmap(XtDisplay(current), curlw->label.stippled);
      was_resized = True;
    }

    // If any of that happened, recalculate label dimensions.  (Does not
    // depend on left_bitmap.)
    if (was_resized)
      SetTextWidthAndHeight(newlw);   /* label.label or label.pixmap */

    // Notice if the left pixmap changed.
    if (curlw->label.left_bitmap != newlw->label.left_bitmap) {
      newlw->label.left_stippled = None;
      if (curlw->label.left_stippled != None)
	XFreePixmap(XtDisplay(current), curlw->label.left_stippled);
      was_resized = True;
    }

    // Notice other reasons to recalculate label position.
    if (curlw->label.justify != newlw->label.justify ||
      curlw->label.internal_width != newlw->label.internal_width ||
      curlw->label.internal_height != newlw->label.internal_height ||
      curlw->threeD.shadow_width != newlw->threeD.shadow_width)
      was_resized = True;

    /* recalculate the window size if something has changed. */
    if (newlw->label.resize && was_resized) {
      get_lbm_dimensions(newlw);
      if (curlw->core.height == reqlw->core.height && !checks[HEIGHT])
	newlw->core.height =
	  (newlw->label.lbm_height > newlw->label.label_height ?
	   newlw->label.lbm_height : newlw->label.label_height) +
	  2 * newlw->label.internal_height +
	  2 * newlw->threeD.shadow_width;
      if (curlw->core.width == reqlw->core.width && !checks[WIDTH])
	newlw->core.width = newlw->label.label_width +
	  2 * newlw->label.internal_width +
	  2 * newlw->threeD.shadow_width +
	  LEFT_OFFSET(newlw); /* req's label.lbm_width */
    }

    /* enforce minimum dimensions */
    if (newlw->label.resize) {
      int i;
      if (checks[HEIGHT]) {
	if (newlw->label.label_height > newlw->label.lbm_height)
	  i = newlw->label.label_height
	    + 2 * newlw->label.internal_height
	    + 2 * newlw->threeD.shadow_width;
	else
	  i = newlw->label.lbm_height
	    + 2 * newlw->label.internal_height
	    + 2 * newlw->threeD.shadow_width;
	if (i > newlw->core.height)
	  newlw->core.height = i;
      }
      if (checks[WIDTH]) {
	i = newlw->label.label_width
	  + 2 * newlw->label.internal_width
	  + 2 * newlw->threeD.shadow_width
	  + LEFT_OFFSET(newlw); /* req's label.lbm_width */
	if (i > newlw->core.width)
	  newlw->core.width = i;
      }
    }

    // Notice if colors changed.  (GCs also need updating if plain old font
    // changed.)
    if (curlw->core.background_pixel != newlw->core.background_pixel ||
	     curlw->label.foreground != newlw->label.foreground ||
              curlw->label.font->fid != newlw->label.font->fid) {
        /* the fontset is not in the GC - no new GC if fontset changes */
	XtReleaseGC(current, curlw->label.normal_GC);
	XtReleaseGC(current, curlw->label.gray_GC);
	XmuReleaseStippledPixmap(XtScreen(current), curlw->label.stipple);
	GetnormalGC(newlw);
	GetgrayGC(newlw);
	if (curlw->label.foreground != newlw->label.foreground)
	  Xaw3dXftGetXftColor(XtDisplay(newlw), VisualOf(newlw),
	    newlw->core.colormap, newlw->label.foreground,
	    &newlw->label.xftfg);
	if (curlw->core.background_pixel != newlw->core.background_pixel) {
	  Xaw3dXftGetXftColor(XtDisplay(newlw), VisualOf(newlw),
	    newlw->core.colormap, newlw->core.background_pixel,
	    &newlw->label.xftbg);
	  XtReleaseGC(current, curlw->label.stipple_GC);
	  newlw->label.stipple_GC = Xaw3dXftGetStippleGC(new,
	    newlw->core.background_pixel);
	}
	redisplay = True;
    }

    // Notice if sensitive changed.
    if (XtIsSensitive(current) != XtIsSensitive(new))
      redisplay = True;

    // Finally, act on was_resized and redisplay.
    if (was_resized) {
      redisplay = True;
      Position dx, dy;
      /* Resize() will be called if geometry changes succeed */
      _Reposition(newlw, curlw->core.width, curlw->core.height, &dx, &dy);
    }
    /*
      "After calling all the set_values procedures, XtSetValues forces a
      redisplay by calling XClearArea if any of the set_values procedures
      returned True." - libXt docs, Widget State: The set_values Procedure

      That fill with bg color clears the Xor state from Set/Unset in Command.
    */
    if (redisplay) newlw->label.xorSet = False;
    return redisplay;
}

static void
Destroy(Widget w)
{
    LabelWidget lw = (LabelWidget)w;

    // As per Initialize, we always dup label.
    free(lw->label.label);
    // Xft fonts are cached; never call XftFontClose.
    XtReleaseGC(w, lw->label.normal_GC );
    XtReleaseGC(w, lw->label.gray_GC);
    XtReleaseGC(w, lw->label.stipple_GC);
    if (lw->label.stippled != None)
	XFreePixmap(XtDisplay(w), lw->label.stippled);
    if (lw->label.left_stippled != None)
	XFreePixmap(XtDisplay(w), lw->label.left_stippled);
    XmuReleaseStippledPixmap( XtScreen(w), lw->label.stipple );
}


static XtGeometryResult
QueryGeometry(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *preferred)
{
    LabelWidget lw = (LabelWidget)w;

    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = (Dimension) (lw->label.label_width
				    + (2 * lw->label.internal_width)
				    + (2 * lw->threeD.shadow_width)
				    + (int) LEFT_OFFSET(lw));
    preferred->height = (Dimension) (
	  (lw->label.lbm_height > lw->label.label_height ?
	   lw->label.lbm_height : lw->label.label_height)
				     + (2 * lw->label.internal_height)
				     + (2 * lw->threeD.shadow_width));
    if (  ((intended->request_mode & (CWWidth | CWHeight))
	   	== (CWWidth | CWHeight)) &&
	  intended->width == preferred->width &&
	  intended->height == preferred->height)
	return XtGeometryYes;
    else if (preferred->width == w->core.width &&
	     preferred->height == w->core.height)
	return XtGeometryNo;
    else
	return XtGeometryAlmost;
}
