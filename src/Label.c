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

#include <X11/Xaw3dxft/Xaw3dP.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xaw3dxft/XawInit.h>
#include <X11/Xaw3dxft/Xaw3dXftP.h>
#include <X11/Xaw3dxft/Command.h>
#include <X11/Xaw3dxft/LabelP.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Drawing.h>
#include <stdio.h>
#include <ctype.h>

/* needed for abs() */
#include <stdlib.h>

#define streq(a,b) (strcmp( (a), (b) ) == 0)

#define MULTI_LINE_LABEL 32767


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(LabelRec, field)
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
    char *nl;

    if (lw->label.pixmap != None) {
	Window root;
	int x, y;
	unsigned int width, height, bw, depth;

	if (XGetGeometry(XtDisplay(lw), lw->label.pixmap, &root, &x, &y,
			 &width, &height, &bw, &depth)) {
	    lw->label.label_height = height;
	    lw->label.label_width = width;
	    lw->label.depth = depth;
	    return;
	}
    }

    if (_Xaw3dXft->encoding) {
        XftFont *xftfont = lw->label.xftfont;
        lw->label.label_height = xftfont->height;
        if (lw->label.label == NULL) {
            lw->label.label_len = 0;
            lw->label.label_width = 0;
        }
        else if ((nl = strchr(lw->label.label, '\n')) != NULL) {
	    char *label;
            lw->label.label_len = MULTI_LINE_LABEL;
            lw->label.label_width = 0;
            for (label = lw->label.label; nl != NULL; nl = strchr(label, '\n')) {
	        int width;

                width = Xaw3dXftTextWidth((Widget)lw, xftfont, label, (int)(nl - label));
	        if (width > (int)lw->label.label_width)
		    lw->label.label_width = width;
	        label = nl + 1;
	        if (*label)
		    lw->label.label_height += xftfont->height;
	    }
	    if (*label) {
	        int width;
                width = Xaw3dXftTextWidth((Widget)lw, xftfont, label, strlen(label));
	        if (width > (int) lw->label.label_width)
		    lw->label.label_width = width;
	    }
        } else {
	    lw->label.label_len = strlen(lw->label.label);
	    lw->label.label_width =
	        Xaw3dXftTextWidth((Widget)lw, xftfont,
                                  lw->label.label, (int) lw->label.label_len);
        }
    } else
#ifdef XAW_INTERNATIONALIZATION
    if ( lw->simple.international == True ) {
      XFontSet	fset = lw->label.fontset;
      XFontSetExtents *ext = XExtentsOfFontSet(fset);

      lw->label.label_height = ext->max_ink_extent.height;
      if (lw->label.label == NULL) {
	  lw->label.label_len = 0;
	  lw->label.label_width = 0;
      }
      else if ((nl = strchr(lw->label.label, '\n')) != NULL) {
	  char *label;
	  lw->label.label_len = MULTI_LINE_LABEL;
	  lw->label.label_width = 0;
	  for (label = lw->label.label; nl != NULL; nl = strchr(label, '\n')) {
	      int width = XmbTextEscapement(fset, label, (int)(nl - label));

	      if (width > (int)lw->label.label_width)
		  lw->label.label_width = width;
	      label = nl + 1;
	      if (*label)
		  lw->label.label_height +=
		      ext->max_ink_extent.height;
	  }
	  if (*label) {
	      int width = XmbTextEscapement(fset, label, strlen(label));

	      if (width > (int) lw->label.label_width)
		  lw->label.label_width = width;
	  }
      } else {
	  lw->label.label_len = strlen(lw->label.label);
	  lw->label.label_width =
	      XmbTextEscapement(fset, lw->label.label, (int) lw->label.label_len);
      }

    } else
#endif
    {
        XFontStruct *font = lw->label.font;
        lw->label.label_height = font->max_bounds.ascent + font->max_bounds.descent;
        if (lw->label.label == NULL) {
            lw->label.label_len = 0;
            lw->label.label_width = 0;
        }
        else if ((nl = strchr(lw->label.label, '\n')) != NULL) {
	    char *label;
            lw->label.label_len = MULTI_LINE_LABEL;
            lw->label.label_width = 0;
            for (label = lw->label.label; nl != NULL; nl = strchr(label, '\n')) {
	        int width;

	        if (lw->label.encoding)
		    width = XTextWidth16(font, (XChar2b *)label, (int)(nl - label)/2);
	        else
		    width = XTextWidth(font, label, (int)(nl - label));
	        if (width > (int)lw->label.label_width)
		    lw->label.label_width = width;
	        label = nl + 1;
	        if (*label)
		    lw->label.label_height +=
		        font->max_bounds.ascent + font->max_bounds.descent;
	    }
	    if (*label) {
	        int width;

	        if (lw->label.encoding)
		    width = XTextWidth16(font, (XChar2b *)label, (int)strlen(label)/2);
	        else
		    width = XTextWidth(font, label, strlen(label));
	        if (width > (int) lw->label.label_width)
		    lw->label.label_width = width;
	    }
        } else {
	    lw->label.label_len = strlen(lw->label.label);
	    if (lw->label.encoding)
	        lw->label.label_width =
		    XTextWidth16(font, (XChar2b *)lw->label.label,
			         (int) lw->label.label_len/2);
	    else
	        lw->label.label_width =
		    XTextWidth(font, lw->label.label, (int) lw->label.label_len);
        }
    }
}

static void
GetnormalGC(LabelWidget lw)
{
    XGCValues	values;

    values.foreground	= lw->label.foreground;
    values.background	= lw->core.background_pixel;
    values.font		= lw->label.font->fid;
    values.graphics_exposures = False;

#ifdef XAW_INTERNATIONALIZATION
    if ( lw->simple.international == True )
        /* Since Xmb/wcDrawString eats the font, I must use XtAllocateGC. */
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

static void
compute_bitmap_offsets (LabelWidget lw)
{
    if (lw->label.lbm_height != 0)
	lw->label.lbm_y = (lw->core.height - lw->label.lbm_height) / 2;
    else
	lw->label.lbm_y = 0;
}

static void
set_bitmap_info (LabelWidget lw)
{
    Window root;
    int x, y;
    unsigned int bw;

    if (lw->label.pixmap || !(lw->label.left_bitmap &&
	  XGetGeometry (XtDisplay(lw), lw->label.left_bitmap, &root, &x, &y,
			&lw->label.lbm_width, &lw->label.lbm_height,
			&bw, &lw->label.depth))) {
	lw->label.lbm_width = lw->label.lbm_height = 0;
    }
    compute_bitmap_offsets (lw);
}

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    LabelWidget lw = (LabelWidget) new;

    if (_Xaw3dXft->encoding)
	lw->label.xftfont = Xaw3dXftGetFont(XtDisplayOfObject(new), lw->label.xftfontname);
    else {
	lw->label.xftfont = NULL;
	if (!lw->label.font) XtError("Aborting: no font found\n");
	if (lw->simple.international && !lw->label.fontset)
	  XtError("Aborting: no fontset found\n");
    }

    /* disable shadows if we're not a subclass of Command */
    if (!XtIsSubclass(new, commandWidgetClass))
	lw->threeD.shadow_width = 0;

    if (lw->label.label == NULL)
        lw->label.label = XtNewString(lw->core.name);
    else
        lw->label.label = XtNewString(lw->label.label);

    GetnormalGC(lw);
    GetgrayGC(lw);

    SetTextWidthAndHeight(lw);  /* label.label or label.pixmap */

    if (lw->core.height == 0)
	lw->core.height = lw->label.label_height +
				2 * lw->label.internal_height;

    set_bitmap_info(lw);  /* req's core.height, sets label.lbm_* */

    if (lw->label.lbm_height > lw->label.label_height)
	lw->core.height = lw->label.lbm_height +
				2 * lw->label.internal_height;

    if (lw->core.width == 0)
        lw->core.width = lw->label.label_width +
				2 * lw->label.internal_width +
				LEFT_OFFSET(lw);  /* req's label.lbm_width */

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
    GC gc;

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

    gc = XtIsSensitive(gw) ? w->label.normal_GC : w->label.gray_GC;
#ifdef notdef
    if (region != NULL)
	XSetRegion(XtDisplay(gw), gc, region);
#endif /*notdef*/

    if (w->label.pixmap == None) {
	int len = w->label.label_len;
	char *label = w->label.label;
#ifdef XAW_INTERNATIONALIZATION
        Position ksy = w->label.label_y;
#endif
	Position y;
	if (_Xaw3dXft->encoding)
            y = w->label.label_y + w->label.xftfont->ascent;
	else
            y = w->label.label_y + w->label.font->max_bounds.ascent;

	/* display left bitmap */
	if (w->label.left_bitmap && w->label.lbm_width != 0) {
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
			   (int) w->label.internal_width,
			   (int) w->label.lbm_y,
			   (unsigned long) 1L);
	    else
		XCopyArea(XtDisplay(gw), pm, XtWindow(gw), gc, 0, 0,
			  w->label.lbm_width, w->label.lbm_height,
			  (int) w->label.internal_width,
			  (int) w->label.lbm_y);
	}

	if (_Xaw3dXft->encoding) {
	    if (len == MULTI_LINE_LABEL) {
	        char *nl;
	        while ((nl = strchr(label, '\n')) != NULL) {
		    Xaw3dXftDrawString(gw, w->label.xftfont,
				       w->label.label_x, y,
				       label, (int)(nl - label));
		    y += w->label.xftfont->height;
		    label = nl + 1;
	        }
	        len = strlen(label);
	    }
	    if (len) {
		Xaw3dXftDrawString(gw, w->label.xftfont,
				   w->label.label_x, y,
				   label, len);
	    }
	} else
#ifdef XAW_INTERNATIONALIZATION
        if ( w->simple.international == True) {

	    XFontSetExtents *ext = XExtentsOfFontSet(w->label.fontset);

	    ksy += abs(ext->max_ink_extent.y);

            if (len == MULTI_LINE_LABEL) {
	        char *nl;
	        while ((nl = strchr(label, '\n')) != NULL) {
	            XmbDrawString(XtDisplay(w), XtWindow(w), w->label.fontset, gc,
	  		        w->label.label_x, ksy, label, (int)(nl - label));
	            ksy += ext->max_ink_extent.height;
	            label = nl + 1;
	        }
	        len = strlen(label);
            }
            if (len)
	        XmbDrawString(XtDisplay(w), XtWindow(w), w->label.fontset, gc,
			      w->label.label_x, ksy, label, len);

        } else
#endif
	{ /* international false, so use R5 routine */

	    if (len == MULTI_LINE_LABEL) {
	        char *nl;
	        while ((nl = strchr(label, '\n')) != NULL) {
		    if (w->label.encoding)
		        XDrawString16(XtDisplay(gw), XtWindow(gw), gc,
				 		w->label.label_x, y,
                                      (XChar2b *)label, (int)(nl - label)/2);
		    else
		        XDrawString(XtDisplay(gw), XtWindow(gw), gc,
			       		w->label.label_x, y, label, (int)(nl - label));
		    y += w->label.font->max_bounds.ascent +
		                        w->label.font->max_bounds.descent;
		    label = nl + 1;
	        }
	        len = strlen(label);
	    }
	    if (len) {
	        if (w->label.encoding)
		    XDrawString16(XtDisplay(gw), XtWindow(gw), gc,
			     w->label.label_x, y, (XChar2b *)label, len/2);
	        else
		    XDrawString(XtDisplay(gw), XtWindow(gw), gc,
			   w->label.label_x, y, label, len);
	    }

        } /* endif international */

    } else {
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

#ifdef notdef
    if (region != NULL)
	XSetClipMask(XtDisplay(gw), gc, (Pixmap)None);
#endif /* notdef */
}

static void
_Reposition(LabelWidget lw, Dimension width, Dimension height,
            Position *dx, Position *dy)
{
    Position newPos;
    Position leftedge = lw->label.internal_width + LEFT_OFFSET(lw);

    switch (lw->label.justify) {
	case XtJustifyLeft:
	    newPos = leftedge;
	    break;
	case XtJustifyRight:
	    newPos = width - lw->label.label_width - lw->label.internal_width;
	    break;
	case XtJustifyCenter:
	default:
	    newPos = (int)(width - lw->label.label_width) / 2;
	    break;
    }

    if (newPos < (Position)leftedge)
	newPos = leftedge;
    *dx = newPos - lw->label.label_x;
    lw->label.label_x = newPos;

    *dy = (newPos = (int)(height - lw->label.label_height) / 2)
	  - lw->label.label_y;
    lw->label.label_y = newPos;

    lw->label.lbm_y = (height - lw->label.lbm_height) / 2;

    return;
}

static void
Resize(Widget w)
{
    LabelWidget lw = (LabelWidget)w;
    Position dx, dy;

    _Reposition(lw, w->core.width, w->core.height, &dx, &dy);
    compute_bitmap_offsets (lw);
}

/*
 * Set specified arguments into widget
 */

#define PIXMAP		0
#define WIDTH		1
#define HEIGHT		2
#define NUM_CHECKS	3

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    LabelWidget curlw = (LabelWidget) current;
    LabelWidget reqlw = (LabelWidget) request;
    LabelWidget newlw = (LabelWidget) new;
    int i;
    Boolean was_resized = False, redisplay = False, checks[NUM_CHECKS];

    for (i = 0; i < NUM_CHECKS; i++)
	checks[i] = FALSE;
    for (i = 0; i < *num_args; i++) {
	if (streq(XtNbitmap, args[i].name))
	    checks[PIXMAP] = TRUE;
	if (streq(XtNwidth, args[i].name))
	    checks[WIDTH] = TRUE;
	if (streq(XtNheight, args[i].name))
	    checks[HEIGHT] = TRUE;
    }

    if (newlw->label.label == NULL)
	newlw->label.label = newlw->core.name;
    if (curlw->label.label != newlw->label.label) {
        if (curlw->label.label != curlw->core.name)
	    XtFree((char *)curlw->label.label);
	if (newlw->label.label != newlw->core.name)
	    newlw->label.label = XtNewString(newlw->label.label);
	was_resized = True;
    }

    if (was_resized || checks[PIXMAP] ||
		curlw->label.font != newlw->label.font ||
#ifdef XAW_INTERNATIONALIZATION
		(curlw->simple.international &&
			curlw->label.fontset != newlw->label.fontset) ||
#endif
		curlw->label.encoding != newlw->label.encoding ||
		curlw->label.justify != newlw->label.justify) {
	SetTextWidthAndHeight(newlw);   /* label.label or label.pixmap */
	was_resized = True;
    }

    if (curlw->label.left_bitmap != newlw->label.left_bitmap ||
		curlw->label.internal_width != newlw->label.internal_width ||
		curlw->label.internal_height != newlw->label.internal_height)
	was_resized = True;

    /* recalculate the window size if something has changed. */
    if (newlw->label.resize && was_resized) {
	if (curlw->core.height == reqlw->core.height && !checks[HEIGHT])
	    newlw->core.height = newlw->label.label_height +
				2 * newlw->label.internal_height;

	set_bitmap_info (newlw);  /* req's core.height, sets label.lbm_* */

	if (newlw->label.lbm_height > newlw->label.label_height)
	    newlw->core.height = newlw->label.lbm_height +
					2 * newlw->label.internal_height;

	if (curlw->core.width == reqlw->core.width && !checks[WIDTH])
	    newlw->core.width = newlw->label.label_width +
				2 * newlw->label.internal_width +
				LEFT_OFFSET(newlw);  /* req's label.lbm_width */
    }

    /* enforce minimum dimensions */
    if (newlw->label.resize) {
	if (checks[HEIGHT]) {
	    if (newlw->label.label_height > newlw->label.lbm_height)
		i = newlw->label.label_height +
			2 * newlw->label.internal_height;
	    else
		i = newlw->label.lbm_height + 2 * newlw->label.internal_height;
	    if (i > newlw->core.height)
		newlw->core.height = i;
	}
	if (checks[WIDTH]) {
	    i = newlw->label.label_width + 2 * newlw->label.internal_width +
			LEFT_OFFSET(newlw);  /* req's label.lbm_width */
	    if (i > newlw->core.width)
		newlw->core.width = i;
	}
    }

    if (curlw->core.background_pixel != newlw->core.background_pixel ||
		curlw->label.foreground != newlw->label.foreground ||
		curlw->label.font->fid != newlw->label.font->fid) {
        /* the fontset is not in the GC - no new GC if fontset changes */
	XtReleaseGC(new, curlw->label.normal_GC);
	XtReleaseGC(new, curlw->label.gray_GC);
	XmuReleaseStippledPixmap( XtScreen(current), curlw->label.stipple );
	GetnormalGC(newlw);
	GetgrayGC(newlw);
	redisplay = True;
    }

#ifdef XAW_MULTIPLANE_PIXMAPS
    if (curlw->label.pixmap != newlw->label.pixmap)
    {
	newlw->label.stippled = None;
	if (curlw->label.stippled != None)
	    XFreePixmap(XtDisplay(current), curlw->label.stippled);
    }
    if (curlw->label.left_bitmap != newlw->label.left_bitmap)
    {
	newlw->label.left_stippled = None;
	if (curlw->label.left_stippled != None)
	    XFreePixmap(XtDisplay(current), curlw->label.left_stippled);
    }
#endif

    if (was_resized) {
	Position dx, dy;

	/* Resize() will be called if geometry changes succeed */
	_Reposition(newlw, curlw->core.width, curlw->core.height, &dx, &dy);
    }

    return was_resized || redisplay ||
	   XtIsSensitive(current) != XtIsSensitive(new);
}

static void
Destroy(Widget w)
{
    LabelWidget lw = (LabelWidget)w;

    if ( lw->label.label != lw->core.name )
	XtFree( lw->label.label );
    if (lw->label.xftfont && lw->label.xftfont != _Xaw3dXft->default_font)
        XftFontClose(XtDisplayOfObject(w), lw->label.xftfont);
    XtReleaseGC( w, lw->label.normal_GC );
    XtReleaseGC( w, lw->label.gray_GC);
#ifdef XAW_MULTIPLANE_PIXMAPS
    if (lw->label.stippled != None)
	XFreePixmap(XtDisplay(w), lw->label.stippled);
    if (lw->label.left_stippled != None)
	XFreePixmap(XtDisplay(w), lw->label.left_stippled);
#endif
    XmuReleaseStippledPixmap( XtScreen(w), lw->label.stipple );
}


static XtGeometryResult
QueryGeometry(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *preferred)
{
    LabelWidget lw = (LabelWidget)w;

    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = (lw->label.label_width +
			    2 * lw->label.internal_width +
			    LEFT_OFFSET(lw));
    preferred->height = lw->label.label_height +
			    2 * lw->label.internal_height;
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
