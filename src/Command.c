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


DEC is not to blame for what you find here now. — DWF

******************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <stdio.h>

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
#include <X11/Xaw3dXft/AnyString.h>
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
static void PaintCommandWidget(Widget, XEvent *);
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

static void get_or_change_xorGC (CommandWidget cbw, Pixel fg, Pixel bg) {
  // Default fill_style FillSolid uses only foreground
  XGCValues values;
  values.foreground = fg ^ bg;
  values.function = GXxor;
  values.graphics_exposures = False;
  if (cbw->command.xorGC)
    XtReleaseGC((Widget)cbw, cbw->command.xorGC);
  cbw->command.xorGC = XtGetGC((Widget)cbw,
    GCForeground|GCFunction|GCGraphicsExposures, &values);
}

static void get_or_change_dashedGC (CommandWidget cbw, Pixel fg) {
  XGCValues values;
  values.foreground = fg;
  values.line_style = LineOnOffDash;
  // char dashes acts like a uint8_t.  0 gets you a BadValue error.
  if (cbw->command.highlight_thickness < 1)
    values.dashes = (char)4;
  else if (cbw->command.highlight_thickness < 64)
    values.dashes = (char)(cbw->command.highlight_thickness * 4);
  else
    values.dashes = (char)255;
  values.graphics_exposures = False;

  // Xlib special case
  // "The line-width is measured in pixels and either can be greater than or
  // equal to one (wide line) or can be the special value zero (thin line)."
  if (cbw->command.highlight_thickness > 1)
    values.line_width = cbw->command.highlight_thickness;
  else
    values.line_width = 0;

  if (cbw->command.dashedGC)
    XtReleaseGC((Widget)cbw, cbw->command.dashedGC);
  cbw->command.dashedGC = XtGetGC((Widget)cbw,
    GCForeground|GCLineStyle|GCLineWidth|GCDashList|GCGraphicsExposures,
				  &values);
}

static GC
Get_GC(CommandWidget cbw, Pixel fg, Pixel bg)
{
  XGCValues	values;

  values.foreground   = fg;
  values.background   = bg;
  values.font	      = cbw->label.font->fid;
  values.cap_style = CapProjecting;
  values.graphics_exposures = False;

  // Xlib special case
  // "The line-width is measured in pixels and either can be greater than or
  // equal to one (wide line) or can be the special value zero (thin line)."
  if (cbw->command.highlight_thickness > 1)
    values.line_width = cbw->command.highlight_thickness;
  else
    values.line_width = 0;

  static_assert(Got_XAW_defines);
#ifdef XAW_INTERNATIONALIZATION
  if ( cbw->simple.international == True )
      return XtAllocateGC((Widget)cbw, 0,
		 (GCForeground|GCBackground|GCLineWidth|GCCapStyle|GCGraphicsExposures),
		 &values, GCFont, 0 );
  else
#endif
      return XtGetGC((Widget)cbw,
		 (GCForeground|GCBackground|GCFont|GCLineWidth|GCCapStyle|GCGraphicsExposures),
		 &values);
}

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  CommandWidget cbw = (CommandWidget) new;
  int shape_event_base, shape_error_base;

  cbw->command.xorGC = cbw->command.dashedGC = NULL;

  /* Save values that are overridden when shape is not rectangle. */
  cbw->command.orig_shadow_width = cbw->threeD.shadow_width;
  cbw->command.orig_border_width = cbw->core.border_width;
  cbw->command.orig_highlight_thickness = cbw->command.highlight_thickness;

  if (cbw->command.shape_style != XawShapeRectangle
      && !XShapeQueryExtension(XtDisplay(new), &shape_event_base,
			       &shape_error_base))
      cbw->command.shape_style = XawShapeRectangle;
  if (cbw->command.highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT) {
      if (cbw->command.shape_style != XawShapeRectangle)
	  cbw->command.highlight_thickness = 0;
      else
	  cbw->command.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
  }
  if (cbw->command.shape_style != XawShapeRectangle) {
    cbw->threeD.shadow_width = 0;
    if (cbw->core.border_width == 0)
      cbw->core.border_width = 1;
  }

  const Pixel fg = cbw->label.foreground, bg = cbw->core.background_pixel;
  cbw->command.normal_GC = Get_GC(cbw, fg, bg);
  cbw->command.inverse_GC = Get_GC(cbw, bg, fg);
  cbw->command.inverse_stipple_GC = Xaw3dXftGetStippleGC(new, fg);
  get_or_change_xorGC(cbw, fg, bg);
  get_or_change_dashedGC(cbw, fg);

  cbw->command.set = False;
  cbw->command.highlighted = HighlightNone;
}

// Apply the xor effect for the Set state (needed by both the Set action
// procedure and PaintCommandWidget).
static void setEffect (Widget w) {
  assert(XtIsRealized(w));
  CommandWidget cbw = (CommandWidget)w;
  assert(cbw->command.set);
  const Dimension s = cbw->threeD.shadow_width;
  // Flip the entire contents
  if (!cbw->label.xorSet) {
    XFillRectangle(XtDisplay(w), XtWindow(w), cbw->command.xorGC,
      s, s, cbw->core.width - 2*s, cbw->core.height - 2*s);
    cbw->label.xorSet = True;
  }
  // Redraw anti-aliased text to fix edges.  Minor glitch:  if
  // highlightThickness exceeds internalHeight or internalWidth, this may
  // draw over the highlight.
  if (cbw->label.pixmap == None && cbw->label.label != None &&
      cbw->label.xftfont != None)
    Xaw3dXftDrawAnyString(XtDisplay(w), cbw->label.visual,
      cbw->core.colormap, XtWindow(w), cbw->label.font, labelFontSet(cbw),
      cbw->label.xftfont, XtIsSensitive(w), international(cbw),
      cbw->command.inverse_GC, cbw->command.inverse_stipple_GC,
      &cbw->label.xftbg, &cbw->label.xftfg, cbw->label.label_x,
      cbw->label.label_y, cbw->label.encoding, cbw->label.label);
  // Flip the shadow
  CommandWidgetClass cwclass = (CommandWidgetClass)XtClass(w);
  (*cwclass->threeD_class.shadowdraw) (w, NULL, NULL, cbw->threeD.relief,
    !cbw->command.set);
}

/***************************
*
*  Action Procedures
*
***************************/

static void
Set(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;
  if (cbw->command.set)
    return;
  cbw->command.set = True;
  if (XtIsRealized(w)) {
    if (cbw->command.highlighted == HighlightWhenUnset &&
	cbw->command.highlight_thickness > 0)
      PaintCommandWidget(w, event); // Clear the highlight
    else
      setEffect(w);
  }
  // Else set but not realized
}

static void
Unset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;
  if (!cbw->command.set)
    return;
  cbw->command.set = False;
  if (XtIsRealized(w)) {
    if (cbw->command.highlighted == HighlightWhenUnset &&
	cbw->command.highlight_thickness > 0)
      PaintCommandWidget(w, event); // Reapply the highlight
    else {
      const Dimension s = cbw->threeD.shadow_width;
      // Flip the entire contents
      if (cbw->label.xorSet) {
	XFillRectangle(XtDisplay(w), XtWindow(w), cbw->command.xorGC,
	  s, s, cbw->core.width - 2*s, cbw->core.height - 2*s);
	cbw->label.xorSet = False;
      }
      // Redraw anti-aliased text to fix edges.  Minor glitch:  if
      // highlightThickness exceeds internalHeight or internalWidth, this may
      // draw over the highlight.
      if (cbw->label.pixmap == None && cbw->label.label != None &&
	  cbw->label.xftfont != None)
	Xaw3dXftDrawAnyString(XtDisplay(w), cbw->label.visual,
	  cbw->core.colormap, XtWindow(w), cbw->label.font, labelFontSet(cbw),
	  cbw->label.xftfont, XtIsSensitive(w), international(cbw),
	  cbw->command.normal_GC, cbw->label.stipple_GC, &cbw->label.xftfg,
	  &cbw->label.xftbg, cbw->label.label_x, cbw->label.label_y,
	  cbw->label.encoding, cbw->label.label);
      // Flip the shadow
      CommandWidgetClass cwclass = (CommandWidgetClass)XtClass(w);
      (*cwclass->threeD_class.shadowdraw) (w, NULL, NULL, cbw->threeD.relief,
	!cbw->command.set);
    }
  }
  // Else unset but not realized
}

static void
Reset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;
  if (cbw->command.set) {
    // This doesn't get called by Toggle, so assume HighlightWhenUnset, and
    // we don't have to clear the highlight.
    cbw->command.highlighted = HighlightNone;
    Unset(w, event, params, num_params);
  } else
    Unhighlight(w, event, params, num_params);
}

static void
Highlight(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;
  const Boolean isHighlighted =
    cbw->command.highlighted == HighlightWhenUnset && !cbw->command.set ||
    cbw->command.highlighted == HighlightAlways;
  if ( *num_params == (Cardinal) 0)
    cbw->command.highlighted = HighlightWhenUnset;
  else {
    if ( *num_params != (Cardinal) 1)
      XtWarning("Too many parameters passed to highlight action table.");
    switch (params[0][0]) {
    case 'A':
    case 'a':
      cbw->command.highlighted = HighlightAlways;
      break;
    default:
      cbw->command.highlighted = HighlightWhenUnset;
      break;
    }
  }
  if (XtIsRealized(w) &&
      cbw->command.highlight_thickness > 0 &&
      !isHighlighted)
    PaintCommandWidget(w, event);
}

static void
Unhighlight(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;
  const Boolean isHighlighted =
    cbw->command.highlighted == HighlightWhenUnset && !cbw->command.set ||
    cbw->command.highlighted == HighlightAlways;
  cbw->command.highlighted = HighlightNone;
  if (XtIsRealized(w) &&
      cbw->command.highlight_thickness > 0 &&
      isHighlighted)
    PaintCommandWidget(w, event);
}

static void
Notify(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  CommandWidget cbw = (CommandWidget)w;

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (cbw->command.set)
    XtCallCallbackList(w, cbw->command.callbacks, (XtPointer) NULL);
}

/*
 * Repaint the widget window
 */

/************************
*
*  REDISPLAY (DRAW)
*
************************/

static void
Redisplay(Widget w, XEvent *event, Region region)
{
  PaintCommandWidget(w, event);
}

/*	Function Name: PaintCommandWidget
 *	Description: Paints the command widget.
 *	Arguments: w - the command widget.
 *	Returns: none
 */
static void
PaintCommandWidget(Widget w, XEvent *event)
{
  CommandWidget cbw = (CommandWidget) w;
  CommandWidgetClass cwclass = (CommandWidgetClass) XtClass (w);
  Dimension	s = cbw->threeD.shadow_width;
  Boolean very_thick = cbw->command.highlight_thickness >
    Min(cbw->core.width, cbw->core.height)/2;

  // Clean slate.
  XClearWindow(XtDisplay(w), XtWindow(w));
  cbw->label.xorSet = False;

  // The following line causes Label Redisplay.
  (*SuperClass->core_class.expose) (w, event, NULL);

  // Draw highlight rectangle on top of that.
  if (cbw->command.highlight_thickness > 0) {
    if (cbw->command.highlighted == HighlightWhenUnset && !cbw->command.set ||
        cbw->command.highlighted == HighlightAlways) {
      GC gc;
      if (cbw->command.highlight_dashed)
	gc = cbw->command.dashedGC;
      else
	gc = cbw->command.normal_GC;
      if (very_thick)
	XFillRectangle(XtDisplay(w), XtWindow(w), gc,
	  s, s, cbw->core.width - 2 * s, cbw->core.height - 2 * s);
      else {
	/* wide lines are centered on the path, so indent it */
	Position offset = cbw->command.highlight_thickness/2;
	// Rectangle line thickness is line_width of the GC
	XDrawRectangle(XtDisplay(w),XtWindow(w), gc,
	  s + offset, s + offset,
	  cbw->core.width - cbw->command.highlight_thickness - 2 * s,
	  cbw->core.height - cbw->command.highlight_thickness - 2 * s);
      }
    }
  }

  // Reapply xor if we are supposed to be set.  In the Xft case, the Label
  // text is unfortunately being drawn twice if we are set.
  if (cbw->command.set)
    setEffect(w); // This takes care of the shadows.
  else
    (*cwclass->threeD_class.shadowdraw) (w, event, NULL, cbw->threeD.relief, !cbw->command.set);
}

static void
Destroy(Widget w)
{
  CommandWidget cbw = (CommandWidget) w;
  XtReleaseGC(w, cbw->command.normal_GC);
  XtReleaseGC(w, cbw->command.inverse_GC);
  XtReleaseGC(w, cbw->command.inverse_stipple_GC);
  if (cbw->command.xorGC)
    XtReleaseGC(w, cbw->command.xorGC);
  if (cbw->command.dashedGC)
    XtReleaseGC(w, cbw->command.dashedGC);
}

/*
 * Set specified arguments into widget
 */

static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  CommandWidget oldcbw = (CommandWidget) current;
  CommandWidget cbw = (CommandWidget) new;
  Boolean redisplay = False;

  // Label has already noticed if XtIsSensitive(current) !=
  // XtIsSensitive(new), but we have additional cleanup if we are becoming
  // insensitive.  XtIsSensitive returns False if core.sensitive and
  // core.ancestor_sensitive disagree, but ancestor_sensitive should not be
  // changing at the moment.
  if (oldcbw->core.sensitive != cbw->core.sensitive && !cbw->core.sensitive) {
    /* about to become insensitive */
    cbw->command.set = False;
    cbw->command.highlighted = HighlightNone;
    redisplay = True;
  }

  // These widths are messed with depending on shape (see Initialize).  For
  // now we pretend that the new shape is the same as the old shape.  If it
  // is not, things get fixed in the next block.
  if (cbw->threeD.shadow_width != oldcbw->threeD.shadow_width) {
    cbw->command.orig_shadow_width = cbw->threeD.shadow_width;
    if (cbw->command.shape_style != XawShapeRectangle)
      cbw->threeD.shadow_width = 0;
    if (cbw->threeD.shadow_width != oldcbw->threeD.shadow_width)
      redisplay = True;
  }
  if (cbw->core.border_width != oldcbw->core.border_width) {
    cbw->command.orig_border_width = cbw->core.border_width;
    if (cbw->command.shape_style != XawShapeRectangle &&
        cbw->core.border_width == 0)
      cbw->core.border_width = 1;
    if (cbw->core.border_width != oldcbw->core.border_width)
      redisplay = True;
  }
  if (cbw->command.highlight_thickness != oldcbw->command.highlight_thickness)
    cbw->command.orig_highlight_thickness = cbw->command.highlight_thickness;

  // Shape changes
  Boolean shape_changed = False;
  if (oldcbw->command.shape_style != cbw->command.shape_style) {
    shape_changed = True;
    if (XtIsRealized(new) && !ShapeButton(cbw, True)) {
      // Requested shape change was rejected by XmuReshapeWidget.
      // ShapeButton sets shape_style to rectangle when it fails.
      // Just keep the old shape and act casual.
      cbw->command.shape_style = oldcbw->command.shape_style;
      shape_changed = False;
    }
    // If not realized, the shape is deemed changed; but ShapeButton could
    // fail later in Realize(), and then we'll be stuck with the wrong
    // widths.
    if (shape_changed) {
      redisplay = True;
      if (cbw->command.shape_style == XawShapeRectangle) {
	cbw->threeD.shadow_width = cbw->command.orig_shadow_width;
	cbw->core.border_width = cbw->command.orig_border_width;
	if (cbw->command.orig_highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT)
	  cbw->command.highlight_thickness = DEFAULT_HIGHLIGHT_THICKNESS;
      } else {
	cbw->threeD.shadow_width = 0;
	if (cbw->core.border_width == 0)
	  cbw->core.border_width = 1;
	if (cbw->command.orig_highlight_thickness == DEFAULT_SHAPE_HIGHLIGHT)
	  cbw->command.highlight_thickness = 0;
      }
    }
  }

  // Change our GCs if necessary.
  if (oldcbw->label.foreground != cbw->label.foreground           ||
      oldcbw->core.background_pixel != cbw->core.background_pixel ||
      oldcbw->label.font->fid != cbw->label.font->fid             ||
      oldcbw->command.highlight_thickness != cbw->command.highlight_thickness) {
    const Pixel fg = cbw->label.foreground, bg = cbw->core.background_pixel;
    XtReleaseGC(new, cbw->command.normal_GC);
    XtReleaseGC(new, cbw->command.inverse_GC);
    XtReleaseGC(new, cbw->command.inverse_stipple_GC);
    cbw->command.normal_GC = Get_GC(cbw, fg, bg);
    cbw->command.inverse_GC = Get_GC(cbw, bg, fg);
    cbw->command.inverse_stipple_GC = Xaw3dXftGetStippleGC(new, fg);
    get_or_change_xorGC(cbw, fg, bg);
    get_or_change_dashedGC(cbw, fg);
    redisplay = True;
  }

  // Changing from 0 to 1 border to make a new shape show up falls through
  // the cracks and we end up with no border until the next cycle.  I'm
  // guessing this is because Label's SetValues ran before we made the
  // change.  Calling Resize here might be a bad thing to do, but it fixes
  // the problem.
  if (shape_changed && cbw->core.border_width != oldcbw->core.border_width)
    Resize(new);

  // Unfortunately, changes to border_width can be rejected by the geometry
  // manager.  Any recovery has to be done in SetValuesAlmost.

  /*
    "After calling all the set_values procedures, XtSetValues forces a
    redisplay by calling XClearArea if any of the set_values procedures
    returned True." - libXt docs, Widget State: The set_values Procedure

    That fill with bg color clears the Xor state from Set/Unset.
  */
  if (redisplay) cbw->label.xorSet = False;

  return (redisplay);
}

static void
ClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtSetTypeConverter( XtRString, XtRShapeStyle, XmuCvtStringToShapeStyle,
		        (XtConvertArgList)NULL, 0, XtCacheNone, (XtDestructor)NULL );
}


static Boolean
ShapeButton(CommandWidget cbw, Boolean checkRectangular)
{
    Dimension corner_size = 0;

    if (cbw->command.shape_style == XawShapeRoundedRectangle) {
	corner_size = (cbw->core.width < cbw->core.height) ? cbw->core.width
	                                                   : cbw->core.height;
	corner_size = (int) (corner_size * cbw->command.corner_round) / 100;
    }

    if (checkRectangular || cbw->command.shape_style != XawShapeRectangle) {
	if (!XmuReshapeWidget((Widget) cbw, cbw->command.shape_style,
			      corner_size, corner_size)) {
	    fprintf(stderr, "Command ShapeButton:  failed to reshape widget\n");
	    // Assuming that it was rectangle to begin with?
	    cbw->command.shape_style = XawShapeRectangle;
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
    fprintf(stderr, "Xaw3dXft Command widget:  geometry manager said no\n");
    // Recovery steps?
  } else if (request->border_width != reply->border_width) {
    fprintf(stderr, "Xaw3dXft Command widget:  geometry manager said almost, rejected change to border_width\n");
    // Recovery steps?
  } else {
    fprintf(stderr, "Xaw3dXft Command widget:  geometry manager said almost\n");
  }
  Resize(new);
  (*commandWidgetClass->core_class.superclass->core_class.set_values_almost)(old, new, request, reply);
}
