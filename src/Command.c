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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>

/*
 * Command.c - Command button widget
 */

#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/CommandP.h>
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/CommonP.h>
#include <X11/Xmu/Converters.h>
#include <X11/extensions/shape.h>

#define DEFAULT_HIGHLIGHT_THICKNESS 2
#define DEFAULT_SHAPE_HIGHLIGHT 32767

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

static char defaultTranslations[] =
    "<EnterWindow>:	highlight()		\n\
     <LeaveWindow>:	reset()			\n\
     <Btn1Down>:	set()			\n\
     <Btn1Up>:		notify() unset()	";

#define offset(field) XtOffsetOf(CommandRec, field)
static XtResource resources[] = {
   {XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
      offset(command.callbacks), XtRCallback, (XtPointer)NULL},
   {XtNhighlightDashed, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(command.highlight_dashed), XtRImmediate, (XtPointer)FALSE},
   {XtNhighlightThickness, XtCThickness, XtRDimension, sizeof(Dimension),
      offset(command.highlight_thickness), XtRImmediate,
      (XtPointer)DEFAULT_SHAPE_HIGHLIGHT},
   {XtNshapeStyle, XtCShapeStyle, XtRShapeStyle, sizeof(int),
      offset(command.shape_style), XtRImmediate, (XtPointer)XawShapeRectangle},
   {XtNcornerRoundPercent, XtCCornerRoundPercent, XtRDimension,
        sizeof(Dimension), offset(command.corner_round), XtRImmediate,
	(XtPointer) 25},
   {XtNshadowWidth, XtCShadowWidth, XtRDimension, sizeof(Dimension),
	offset(threeD.shadow_width), XtRImmediate, (XtPointer) 2},
   {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
      XtOffsetOf(RectObjRec,rectangle.border_width), XtRImmediate,
      (XtPointer) 0}
};
#undef offset

static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Redisplay(Widget, XEvent *, Region);
static void Set(Widget, XEvent *, String *, Cardinal *);
static void Reset(Widget, XEvent *, String *, Cardinal *);
static void Notify(Widget, XEvent *, String *, Cardinal *);
static void Unset(Widget, XEvent *, String *, Cardinal *);
static void Highlight(Widget, XEvent *, String *, Cardinal *);
static void Unhighlight(Widget, XEvent *, String *, Cardinal *);
static void Destroy(Widget);
static void ClassInitialize(void);
static Boolean ShapeButton(CommandWidget, Boolean);
static void Realize(Widget, Mask *, XSetWindowAttributes *);
static void Resize(Widget);
static void SetValuesAlmost(Widget, Widget, XtWidgetGeometry *, XtWidgetGeometry *);

static XtActionsRec actionsList[] = {
  {"set",		Set},
  {"notify",		Notify},
  {"highlight",		Highlight},
  {"reset",		Reset},
  {"unset",		Unset},
  {"unhighlight",	Unhighlight}
};

#define SuperClass ((LabelWidgetClass)&labelClassRec)

CommandClassRec commandClassRec = {
  {
    (WidgetClass) SuperClass,		/* superclass		  */
    "Command",				/* class_name		  */
    sizeof(CommandRec),			/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    Realize,				/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    Resize,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    SetValuesAlmost,		        /* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    XtInheritChangeSensitive		/* change_sensitive	*/
  },  /* SimpleClass fields initialization */
  {
    XtInheritXaw3dShadowDraw,           /* shadowdraw           */
  },  /* ThreeD Class fields initialization */
  {
    0,                                     /* field not used    */
  },  /* LabelClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* CommandClass fields initialization */
};

  /* for public consumption */
WidgetClass commandWidgetClass = (WidgetClass) &commandClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void get_or_change_GCs (CommandWidget cw) {
  const Pixel fg = cw->label.foreground, bg = cw->core.background_pixel;

  // rev_GC
  if (cw->command.rev_GC)
    XtReleaseGC((Widget)cw, cw->command.rev_GC);
  cw->command.rev_GC = Xaw3dXftGetTextGC((Widget)cw, bg, cw->label.font,
					  cw->simple.international);

  // inverse_stipple_GC
  if (cw->command.inverse_stipple_GC)
    XtReleaseGC((Widget)cw, cw->command.inverse_stipple_GC);
  cw->command.inverse_stipple_GC = Xaw3dXftGetStippleGC((Widget)cw, fg);

  XGCValues values;
  values.graphics_exposures = False;

  // xor_GC
  // Default fill_style FillSolid uses only foreground
  values.foreground = fg ^ bg;
  values.function = GXxor;
  if (cw->command.xor_GC)
    XtReleaseGC((Widget)cw, cw->command.xor_GC);
  cw->command.xor_GC = XtGetGC((Widget)cw,
    GCForeground|GCFunction|GCGraphicsExposures, &values);

  // hl_solid_GC
  values.foreground = fg;
  // Xlib special case
  // "The line-width is measured in pixels and either can be greater than or
  // equal to one (wide line) or can be the special value zero (thin line)."
  if (cw->command.highlight_thickness > 1)
    values.line_width = cw->command.highlight_thickness;
  else
    values.line_width = 0;
  if (cw->command.hl_solid_GC)
    XtReleaseGC((Widget)cw, cw->command.hl_solid_GC);
  cw->command.hl_solid_GC = XtGetGC((Widget)cw,
    GCForeground|GCLineWidth|GCGraphicsExposures, &values);

  // hl_dashed_GC
  values.line_style = LineOnOffDash;
  // char dashes acts like a uint8_t.  0 gets you a BadValue error.
  if (cw->command.highlight_thickness < 1)
    values.dashes = (char)4;
  else if (cw->command.highlight_thickness < 64)
    values.dashes = (char)(cw->command.highlight_thickness * 4);
  else
    values.dashes = (char)255;
  if (cw->command.hl_dashed_GC)
    XtReleaseGC((Widget)cw, cw->command.hl_dashed_GC);
  cw->command.hl_dashed_GC = XtGetGC((Widget)cw,
    GCForeground|GCLineStyle|GCLineWidth|GCDashList|GCGraphicsExposures,
				  &values);

  // XftColors
  Display *display = XtDisplay(cw);
  Visual *visual = cw->label.visual;
  Colormap cmap = cw->core.colormap;
  Xaw3dXftGetXftColor(display, visual, cmap, bg, &cw->command.xftbg);
}

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  CommandWidget cw = (CommandWidget) new;
  int shape_event_base, shape_error_base;

  cw->command.rev_GC =
    cw->command.hl_solid_GC =
    cw->command.hl_dashed_GC =
    cw->command.inverse_stipple_GC =
    cw->command.xor_GC = NULL;

  /* Save values that are overridden when shape is not rectangle. */
  cw->command.orig_shadow_width = cw->threeD.shadow_width;
  cw->command.orig_border_width = cw->core.border_width;
  cw->command.orig_highlight_thickness = cw->command.highlight_thickness;

  if (cw->command.shape_style != XawShapeRectangle
      && !XShapeQueryExtension(XtDisplay(new), &shape_event_base,
			       &shape_error_base))
      cw->command.shape_style = XawShapeRectangle;
  if (cw->command.highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT) {
      if (cw->command.shape_style != XawShapeRectangle)
	  cw->command.highlight_thickness = 0;
      else
	  cw->command.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
  }
  if (cw->command.shape_style != XawShapeRectangle) {
    cw->threeD.shadow_width = 0;
    if (cw->core.border_width == 0)
      cw->core.border_width = 1;
  }

  get_or_change_GCs(cw);
  cw->command.set = False;
  cw->command.highlighted = HighlightNone;
}

/***************************
*
*  Action Procedures
*
***************************/

/*
  The HighlightWhenUnset protocol, which is used by everything except Toggle,
  means that every set/unset comes with a highlight/unhighlight (unless
  highlights are disabled).

  Highlight/unhighlight currently require a redisplay.  Highlight could be
  done with two extra GCs, but to avoid a redisplay on unhighlight, we would
  need to stop allowing the highlight rectangle to overlap label contents.

  If it's not worth implementing a fast path for highlight/unhighlight, it's
  definitely not worth it for set/unset, which are complicated by the need to
  redraw Xft text on a clean background.
*/

static void Set (Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;
  if (!cw->command.set) {
    cw->command.set = True;
    if (XtIsRealized(w))
      Redisplay(w, event, NULL);
  }
}

static void Unset (Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;
  if (cw->command.set) {
    cw->command.set = False;
    if (XtIsRealized(w))
      Redisplay(w, event, NULL);
  }
}

static void
Reset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;
  // This doesn't get called by Toggle.
  if (cw->command.set) {
    cw->command.highlighted = HighlightNone;
    Unset(w, event, params, num_params);
  } else
    Unhighlight(w, event, params, num_params);
}

static void
Highlight(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;
  const Boolean isHighlighted =
    cw->command.highlighted == HighlightWhenUnset && !cw->command.set ||
    cw->command.highlighted == HighlightAlways;
  if ( *num_params == (Cardinal) 0)
    cw->command.highlighted = HighlightWhenUnset;
  else {
    if ( *num_params != (Cardinal) 1)
      XtWarning("Too many parameters passed to highlight action table.");
    switch (params[0][0]) {
    case 'A':
    case 'a':
      cw->command.highlighted = HighlightAlways;
      break;
    default:
      cw->command.highlighted = HighlightWhenUnset;
      break;
    }
  }
  if (XtIsRealized(w) &&
      cw->command.highlight_thickness > 0 &&
      !isHighlighted)
    Redisplay(w, event, NULL);
}

static void
Unhighlight(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;
  const Boolean isHighlighted =
    cw->command.highlighted == HighlightWhenUnset && !cw->command.set ||
    cw->command.highlighted == HighlightAlways;
  cw->command.highlighted = HighlightNone;
  if (XtIsRealized(w) &&
      cw->command.highlight_thickness > 0 &&
      isHighlighted)
    Redisplay(w, event, NULL);
}

static void
Notify(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cw = (CommandWidget)w;

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (cw->command.set)
    XtCallCallbackList(w, cw->command.callbacks, (XtPointer) NULL);
}

/*
 * Repaint the widget window
 */

/************************
*
*  REDISPLAY (DRAW)
*
************************/

// This duplicates/replaces code from Label Redisplay so that effects can be
// applied in the right order.
static void Redisplay(Widget w, XEvent *event, Region region) {
  CommandWidget cw = (CommandWidget)w;
  Display *display = XtDisplay(w);
  Window window = XtWindow(w);
  Dimension s = cw->threeD.shadow_width,
           ht = cw->command.highlight_thickness;
  const Boolean very_thick = (ht > Min(cw->core.width, cw->core.height)/2),
                 sensitive = XtIsSensitive(w),
                       set = cw->command.set;

  // Clean slate
  XClearWindow(display, window);

  // Draw bitmaps
  if (cw->label.pixmap == None) {
    /* draw left bitmap */
    if (cw->label.left_bitmap && cw->label.lbm_width && cw->label.lbm_height)
      Xaw3dXftCopy(w, cw->label.left_bitmap, window, cw->label.normal_GC,
		   cw->label.lbm_width, cw->label.lbm_height, cw->label.depth,
		   cw->label.internal_width + cw->threeD.shadow_width,
		   ((Position)cw->core.height -
		    (Position)cw->label.lbm_height)/2);
  } else // cw->label.pixmap != None
    Xaw3dXftCopy(w, cw->label.pixmap, window, cw->label.normal_GC,
		 cw->label.label_width, cw->label.label_height, cw->label.depth,
		 cw->label.label_x, cw->label.label_y);

  // Draw highlight rectangle
  if (ht > 0) {
    if (cw->command.highlighted == HighlightWhenUnset && !set ||
        cw->command.highlighted == HighlightAlways) {
      GC gc;
      if (cw->command.highlight_dashed)
	gc = cw->command.hl_dashed_GC;
      else
	gc = cw->command.hl_solid_GC;
      if (very_thick)
	XFillRectangle(display, window, gc,
	  s, s, cw->core.width - 2*s, cw->core.height - 2*s);
      else {
	/* wide lines are centered on the path, so indent it */
	Position offset = ht/2;
	// Rectangle line thickness is line_width of the GC
	XDrawRectangle(display, window, gc, s + offset, s + offset,
	  cw->core.width - ht - 2*s, cw->core.height - ht - 2*s);
      }
    }
  }

  // Apply reverse color to bg, bitmaps, and highlight rectangle
  if (set)
    XFillRectangle(display, window, cw->command.xor_GC, s, s,
		   cw->core.width - 2*s, cw->core.height - 2*s);

  // Draw label text
  if (cw->label.pixmap == None && cw->label.label) {
    GC gc;
    XftColor *xfg;
    if (set) {
      gc = cw->command.rev_GC;
      xfg = &cw->command.xftbg;
    } else {
      gc = cw->label.normal_GC;
      xfg = &cw->label.xftfg;
    }
    Xaw3dXftDrawAnyString(display, cw->label.visual, cw->core.colormap, window,
      cw->label.font, cw->label.fontset, cw->label.xftfont,
      cw->simple.international, gc, xfg, cw->label.label_x, cw->label.label_y,
      NULL, cw->label.encoding, cw->label.label);
  }

  // Apply insensitive stipple
  if (!sensitive) {
    GC gc = (set ? cw->command.inverse_stipple_GC : cw->label.stipple_GC);
    XFillRectangle(display, window, gc, s, s,
		   cw->core.width - 2*s, cw->core.height - 2*s);
  }

  // Draw shadows if applicable
  if (s > 0) {
    CommandWidgetClass cwclass = (CommandWidgetClass)XtClass(w);
    (*cwclass->threeD_class.shadowdraw) (w, event, NULL, cw->threeD.relief,
					 !set);
  }
}

static void
Destroy(Widget w)
{
  CommandWidget cw = (CommandWidget)w;
  if (cw->command.rev_GC)
    XtReleaseGC(w, cw->command.rev_GC);
  if (cw->command.hl_solid_GC)
    XtReleaseGC(w, cw->command.hl_solid_GC);
  if (cw->command.hl_dashed_GC)
    XtReleaseGC(w, cw->command.hl_dashed_GC);
  if (cw->command.xor_GC)
    XtReleaseGC(w, cw->command.xor_GC);
  if (cw->command.inverse_stipple_GC)
    XtReleaseGC(w, cw->command.inverse_stipple_GC);
}

/*
 * Set specified arguments into widget
 */

static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  CommandWidget oldcw = (CommandWidget) current;
  CommandWidget cw = (CommandWidget) new;
  Boolean redisplay = False;

  // Label has already noticed if XtIsSensitive(current) !=
  // XtIsSensitive(new), but we have additional cleanup if we are becoming
  // insensitive.  XtIsSensitive returns False if core.sensitive and
  // core.ancestor_sensitive disagree, but ancestor_sensitive should not be
  // changing at the moment.
  if (oldcw->core.sensitive != cw->core.sensitive && !cw->core.sensitive) {
    /* about to become insensitive */
    cw->command.set = False;
    cw->command.highlighted = HighlightNone;
    redisplay = True;
  }

  // These widths are messed with depending on shape (see Initialize).  For
  // now we pretend that the new shape is the same as the old shape.  If it
  // is not, things get fixed in the next block.
  if (cw->threeD.shadow_width != oldcw->threeD.shadow_width) {
    cw->command.orig_shadow_width = cw->threeD.shadow_width;
    if (cw->command.shape_style != XawShapeRectangle)
      cw->threeD.shadow_width = 0;
    if (cw->threeD.shadow_width != oldcw->threeD.shadow_width)
      redisplay = True;
  }
  if (cw->core.border_width != oldcw->core.border_width) {
    cw->command.orig_border_width = cw->core.border_width;
    if (cw->command.shape_style != XawShapeRectangle &&
        cw->core.border_width == 0)
      cw->core.border_width = 1;
    if (cw->core.border_width != oldcw->core.border_width)
      redisplay = True;
  }
  if (cw->command.highlight_thickness != oldcw->command.highlight_thickness)
    cw->command.orig_highlight_thickness = cw->command.highlight_thickness;

  // Shape changes
  Boolean shape_changed = False;
  if (oldcw->command.shape_style != cw->command.shape_style) {
    shape_changed = True;
    if (XtIsRealized(new) && !ShapeButton(cw, True)) {
      // Requested shape change was rejected by XmuReshapeWidget.
      // ShapeButton sets shape_style to rectangle when it fails.
      // Just keep the old shape and act casual.
      cw->command.shape_style = oldcw->command.shape_style;
      shape_changed = False;
    }
    // If not realized, the shape is deemed changed; but ShapeButton could
    // fail later in Realize(), and then we'll be stuck with the wrong
    // widths.
    if (shape_changed) {
      redisplay = True;
      if (cw->command.shape_style == XawShapeRectangle) {
	cw->threeD.shadow_width = cw->command.orig_shadow_width;
	cw->core.border_width = cw->command.orig_border_width;
	if (cw->command.orig_highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT)
	  cw->command.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
      } else {
	cw->threeD.shadow_width = 0;
	if (cw->core.border_width == 0)
	  cw->core.border_width = 1;
	if (cw->command.orig_highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT)
	  cw->command.highlight_thickness = 0;
      }
    }
  }

  // Notice if the colors or plain old font changed.  We need
  // get_or_change_GCs when international changes because the GCs need to be
  // writable if true and they need to have their font set if false.  Xaw
  // does not allow international to change.
  if (oldcw->label.foreground != cw->label.foreground           ||
      oldcw->core.background_pixel != cw->core.background_pixel ||
      oldcw->label.font->fid != cw->label.font->fid ||
      oldcw->simple.international != cw->simple.international ||
    oldcw->command.highlight_thickness != cw->command.highlight_thickness) {
    get_or_change_GCs(cw);
    redisplay = True;
  }

  // Changing from 0 to 1 border to make a new shape show up falls through
  // the cracks and we end up with no border until the next cycle.  I'm
  // guessing this is because Label's SetValues ran before we made the
  // change.  Calling Resize here might be a bad thing to do, but it fixes
  // the problem.
  if (shape_changed && cw->core.border_width != oldcw->core.border_width)
    Resize(new);

  // Unfortunately, changes to border_width can be rejected by the geometry
  // manager.  Any recovery has to be done in SetValuesAlmost.

  return redisplay;
}

static void
ClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtSetTypeConverter( XtRString, XtRShapeStyle, XmuCvtStringToShapeStyle,
		        (XtConvertArgList)NULL, 0, XtCacheNone, (XtDestructor)NULL );
}


static Boolean
ShapeButton(CommandWidget cw, Boolean checkRectangular)
{
    Dimension corner_size = 0;

    if (cw->command.shape_style == XawShapeRoundedRectangle) {
	corner_size = (cw->core.width < cw->core.height) ? cw->core.width
	                                                   : cw->core.height;
	corner_size = (int) (corner_size * cw->command.corner_round) / 100;
    }

    if (checkRectangular || cw->command.shape_style != XawShapeRectangle) {
	if (!XmuReshapeWidget((Widget) cw, cw->command.shape_style,
			      corner_size, corner_size)) {
	    XtWarning("Command ShapeButton:  failed to reshape widget");
	    // Assuming that it was rectangle to begin with?
	    cw->command.shape_style = XawShapeRectangle;
	    return(False);
	}
    }
    return(True);
}

static void
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
  (*commandWidgetClass->core_class.superclass->core_class.realize)
    (w, valueMask, attributes);
  // No recovery if shape change fails.  Things will be wrong.
  ShapeButton((CommandWidget)w, False);
}

static void
Resize(Widget w)
{
  // No recovery if shape change fails.  Things will be wrong.
  if (XtIsRealized(w))
    ShapeButton((CommandWidget)w, False);
  (*commandWidgetClass->core_class.superclass->core_class.resize)(w);
}

// If the attempt to change border_width in SetValues is rejected, we end up
// here.
static void
SetValuesAlmost (Widget old, Widget new, XtWidgetGeometry *request,
  XtWidgetGeometry *reply) {
  if (reply->request_mode == 0) {
    XtWarning("libXaw3dXft Command widget:  geometry manager said no");
    // Recovery steps?
  } else if (request->border_width != reply->border_width) {
    XtWarning("libXaw3dXft Command widget:  geometry manager said almost, rejected change to border_width");
    // Recovery steps?
  } else {
    XtWarning("libXaw3dXft Command widget:  geometry manager said almost");
  }
  Resize(new);
  (*commandWidgetClass->core_class.superclass->core_class.set_values_almost)(old, new, request, reply);
}
