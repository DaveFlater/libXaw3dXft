/*
 * Copyright (c) 1999 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/*
 * Portions Copyright (c) 2003 David J. Hawkey Jr.
 * Rights, permissions, and disclaimer per the above XFree86 license.
 */

/* Copyright © 2026 David Flater
 * X11 license (as per the historical licenses that the package inherits)
 */

/*
  Notes 2026-10-26 DWF:

  In sources as incoming, XCreateWindow was called with XtBorderWidth(w)
  for border_width.  That somehow resulted in blurry text.  It was blurry
  regardless whether the text rendering was done by Xft or by Xlib.  The
  problem reproduced with both Xaw and Xaw3d with the default font.

  The border_hack workaround that was added in Xaw3dXft 1.6.2h set the
  window's border width to 0 after the fact and then artificially drew a
  black border.  In Xaw3dXft v2 I removed that hack and fixed the border
  width at 0.

  The only two widgets with border_hack code were Tip and SimpleMenu.
  The common factor is they both pop up independent windows instead of
  fitting within a composite parent.
*/

#include <stdlib.h>

#include <X11/Xmu/Converters.h>
#include <X11/Xos.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/CommonP.h>
#include <X11/Xaw3dXft/TipP.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/XawInit.h>

// TipEventHandler is added to the parent widget with TIP_EVENT_MASK.
// TipShellEventHandler is added to the shell with KeyPressMask.
#define	TIP_EVENT_MASK (ButtonPressMask	  |	\
			ButtonReleaseMask |	\
			ButtonMotionMask  |	\
/* 			PointerMotionMask |*/	\
			KeyPressMask	  |	\
			KeyReleaseMask	  |	\
			EnterWindowMask	  |	\
			LeaveWindowMask)

/*
 * Class Methods
 */
static void XawTipClassInitialize(void);
static void XawTipInitialize(Widget, Widget, ArgList, Cardinal *);
static void XawTipDestroy(Widget);
static void XawTipExpose(Widget, XEvent *, Region);
static void XawTipRealize(Widget, Mask *, XSetWindowAttributes *);
static Boolean XawTipSetValues(Widget, Widget, Widget, ArgList, Cardinal *);

/*
 * Prototypes
 */
static void TipEventHandler(Widget, XtPointer, XEvent *, Boolean *);
static void TipShellEventHandler(Widget, XtPointer, XEvent *, Boolean *);
static void TipTimeoutCallback(XtPointer, XtIntervalId *);
static void ResetTip (TipWidget tip, Bool add_timeout);

/*
 * Initialization
 */
#define offset(field) XtOffsetOf(TipRec, tip.field)
static XtResource resources[] = {
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    offset(foreground), XtRString, XtDefaultForeground},
  {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct*),
    offset(font), XtRString, XtDefaultFont},
  {XtNxftFont, XtCXftFont, XtRString, sizeof(String),
    offset(xftfontname), XtRString, NULL},
  {XtNfontSet, XtCFontSet, XtRFontSet, sizeof(XFontSet),
    offset(fontset), XtRString, XtDefaultFontSet},
  {XtNinternational, XtCInternational, XtRBoolean, sizeof(Boolean),
    offset(international), XtRImmediate, (XtPointer)False},
  {XtNlabel, XtCLabel, XtRString, sizeof(String),
    offset(label), XtRString, NULL},
  {XtNencoding, XtCEncoding, XtREnum, sizeof(XtEnum),
    offset(encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
  {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    offset(internal_height), XtRImmediate, (XtPointer)2},
  {XtNinternalWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    offset(internal_width), XtRImmediate, (XtPointer)2},
  // X.h defines NotUseful, WhenMapped, and Always to be 0, 1, and 2
  // respectively.  The default value of 3 results in the backing store
  // attribute being left alone, which is probably what we want.
  {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof(int),
    offset(backing_store), XtRImmediate,
    (XtPointer)(Always + WhenMapped + NotUseful)},
  {XtNtimeout, XtCTimeout, XtRInt, sizeof(int),
    offset(timeout), XtRImmediate, (XtPointer)1200},
  {XtNenable, XtCEnable, XtRBoolean, sizeof(Boolean),
    offset(enable), XtRImmediate, (XtPointer)True}
};
#undef offset

TipClassRec tipClassRec = {
  /* core */
  {
    (WidgetClass)&widgetClassRec,	/* superclass */
    "Tip",				/* class_name */
    sizeof(TipRec),			/* widget_size */
    XawTipClassInitialize,		/* class_initialize */
    NULL,				/* class_part_initialize */
    False,				/* class_inited */
    XawTipInitialize,			/* initialize */
    NULL,				/* initialize_hook */
    XawTipRealize,			/* realize */
    NULL,				/* actions */
    0,					/* num_actions */
    resources,				/* resources */
    XtNumber(resources),		/* num_resources */
    NULLQUARK,				/* xrm_class */
    True,				/* compress_motion */
    True,				/* compress_exposure */
    True,				/* compress_enterleave */
    False,				/* visible_interest */
    XawTipDestroy,			/* destroy */
    NULL,				/* resize */
    XawTipExpose,			/* expose */
    XawTipSetValues,			/* set_values */
    NULL,				/* set_values_hook */
    XtInheritSetValuesAlmost,		/* set_values_almost */
    NULL,				/* get_values_hook */
    NULL,				/* accept_focus */
    XtVersion,				/* version */
    NULL,				/* callback_private */
    NULL,				/* tm_table */
    XtInheritQueryGeometry,		/* query_geometry */
    XtInheritDisplayAccelerator,	/* display_accelerator */
    NULL,				/* extension */
  },
  /* tip */
  {
    NULL,				/* extension */
  },
};

WidgetClass tipWidgetClass = (WidgetClass)&tipClassRec;

#define VisualOf(w)    (w->tip.visual)
#define VisualDepth(w) (w->core.depth)

/*
 * Implementation
 */

/*
 * XmuCvtBackingStoreToString() from XFree86's distribution, because
 * X.Org's distribution doesn't have it.
 */

static Boolean
XawCvtBackingStoreToString(Display *dpy, XrmValuePtr args, Cardinal *num_args,
                           XrmValuePtr fromVal, XrmValuePtr toVal, XtPointer *data)
{
  static String buffer;
  Cardinal size;

  switch (*(int *)fromVal->addr)
  {
    case NotUseful:
      buffer = XtEnotUseful;
      break;
    case WhenMapped:
      buffer = XtEwhenMapped;
      break;
    case Always:
      buffer = XtEalways;
      break;
    case (Always + WhenMapped + NotUseful):
      buffer = XtEdefault;
      break;
    default:
      XtWarning("Cannot convert BackingStore to String");
      toVal->addr = NULL;
      toVal->size = 0;
      return (False);
  }

  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
  {
      if (toVal->size < size)
      {
	  toVal->size = size;
	  return (False);
      }
      strcpy((char *)toVal->addr, buffer);
  }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}

static void get_or_change_GCs (TipWidget tip) {
  const Pixel fg = tip->tip.foreground;

  // normal_GC
  if (tip->tip.normal_GC)
    XtReleaseGC((Widget)tip, tip->tip.normal_GC);
  tip->tip.normal_GC = Xaw3dXftGetTextGC((Widget)tip, fg, tip->tip.font,
					  tip->tip.international);

  // XftColors
  Display *display = XtDisplay(tip);
  Visual *visual = VisualOf(tip);
  Colormap cmap = tip->core.colormap;
  Xaw3dXftGetXftColor(display, visual, cmap, fg, &tip->tip.xftfg);
}

static void TipEnable (TipWidget tip) {
  if (tip->tip.enable)
    XtAddEventHandler(XtParent(tip), TIP_EVENT_MASK, False, TipEventHandler,
      (XtPointer)tip);
}

static void TipDisable (TipWidget tip) {
  XtRemoveEventHandler(XtParent(tip), TIP_EVENT_MASK, False, TipEventHandler,
    (XtPointer)tip);
  ResetTip(tip, False);
}

static void XawTipClassInitialize () {
  XawInitializeWidgetSet();
  XtAddConverter(XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
    NULL, 0);
  XtSetTypeConverter(XtRBackingStore, XtRString, XawCvtBackingStoreToString,
    NULL, 0, XtCacheNone, NULL);
}

static void XawTipInitialize (Widget req, Widget w, ArgList args,
Cardinal *num_args) {
  TipWidget tip = (TipWidget)w;

  Xaw3dXftGetVisualInfo(w, &VisualOf(tip), NULL, NULL);
  tip->tip.normal_GC = NULL;

  Xaw3dXftFixDefaultEncoding(args, *num_args, tip->tip.international,
    &tip->tip.encoding);
  if (tip->tip.xftfontname)
    tip->tip.xftfont = Xaw3dXftGetFont(w, tip->tip.xftfontname);
  else
    tip->tip.xftfont = NULL;
  if (tip->tip.international && !tip->tip.fontset)
    XtError("Tip initialized with international true but no fontset");
  if (!tip->tip.font) XtError("Tip initialized with no font");

  // Avoid surprises:  just always dup the string.
  if (tip->tip.label == NULL && tip->core.name != NULL)
    // Unsure what to do about encoding when defaulting to core.name.
    // It's required to be const char *, but could be UTF8.
    tip->tip.label = Xaw3dXftAnyStrdup(tip->tip.encoding, tip->core.name);
  else if (tip->tip.label != NULL)
    tip->tip.label = Xaw3dXftAnyStrdup(tip->tip.encoding, tip->tip.label);

  get_or_change_GCs(tip);

  tip->tip.mapped = False;
  tip->tip.timer = 0;
  tip->core.border_width = 0; // See Notes 2026-10-26 DWF at top

  tip->tip.shell = w;
  while (XtParent(tip->tip.shell))
    tip->tip.shell = XtParent(tip->tip.shell);

  TipEnable(tip);
  XtAddEventHandler(tip->tip.shell, KeyPressMask, False, TipShellEventHandler,
    tip);
}

static void XawTipDestroy (Widget w) {
  TipWidget tip = (TipWidget)w;

  // As per Initialize, we always dup label.
  free(tip->tip.label);
  if (tip->tip.timer)
    XtRemoveTimeOut(tip->tip.timer);
  if (tip->tip.normal_GC)
    XtReleaseGC(w, tip->tip.normal_GC);
  XtRemoveEventHandler(tip->tip.shell, KeyPressMask, False,
    TipShellEventHandler, tip);
}

static void XawTipRealize (Widget w, Mask *mask, XSetWindowAttributes *attr) {
  TipWidget tip = (TipWidget)w;

  if (tip->tip.backing_store == Always ||
    tip->tip.backing_store == NotUseful ||
    tip->tip.backing_store == WhenMapped) {
    *mask |= CWBackingStore;
    attr->backing_store = tip->tip.backing_store;
  }
  else
    *mask &= ~CWBackingStore;
  *mask |= CWOverrideRedirect;
  attr->override_redirect = True;

  XtWindow(w) = XCreateWindow(DisplayOfScreen(XtScreen(w)),
    RootWindowOfScreen(XtScreen(w)),
    XtX(w), XtY(w),
    XtWidth(w) ? XtWidth(w) : 1,
    XtHeight(w) ? XtHeight(w) : 1,
    0U, // unsigned int border_width; see Notes 2026-10-26 DWF at top
    VisualDepth(tip), InputOutput, VisualOf(tip),
    *mask, attr);
}

static void XawTipExpose (Widget w, XEvent *event, Region region) {
  TipWidget tip = (TipWidget)w;
  if (tip->tip.label)
    Xaw3dXftDrawAnyString(XtDisplay(tip), VisualOf(tip), tip->core.colormap,
      XtWindow(tip), tip->tip.font, tip->tip.fontset, tip->tip.xftfont,
      tip->tip.international, tip->tip.normal_GC, &tip->tip.xftfg,
      tip->tip.internal_width, tip->tip.internal_height, NULL,
      tip->tip.encoding, tip->tip.label);
}

static Boolean XawTipSetValues (Widget current, Widget request, Widget cnew,
ArgList args, Cardinal *num_args) {
  TipWidget curtip = (TipWidget)current;
  TipWidget newtip = (TipWidget)cnew;
  Boolean was_resized = False, redisplay = False;
  newtip->core.border_width = 0; // See Notes 2026-10-26 DWF at top

  // Notice if the label text changed.
  if (curtip->tip.label != newtip->tip.label) {
    // As per Initialize, we always dup label.
    if (curtip->tip.label)
      free(curtip->tip.label);
    if (newtip->tip.label == NULL && newtip->core.name != NULL)
      newtip->tip.label = Xaw3dXftAnyStrdup(newtip->tip.encoding,
	newtip->core.name);
    else if (newtip->tip.label != NULL)
      newtip->tip.label = Xaw3dXftAnyStrdup(newtip->tip.encoding,
	newtip->tip.label);
    was_resized = True;
  }
  // If *only* the encoding changed, we gain nothing by repeating the
  // strdup, but it might display differently.
  if (curtip->tip.encoding != newtip->tip.encoding)
    was_resized = True;

  // Notice if the font changed.
  if (curtip->tip.xftfontname != newtip->tip.xftfontname) {
    if (newtip->tip.xftfontname)
      newtip->tip.xftfont = Xaw3dXftGetFont(cnew, newtip->tip.xftfontname);
    else
      newtip->tip.xftfont = NULL;
    was_resized = True;
  }
  if (curtip->tip.font->fid != newtip->tip.font->fid)
    was_resized = True;
  if (curtip->tip.international != newtip->tip.international ||
      newtip->tip.international &&
      curtip->tip.fontset != newtip->tip.fontset)
    was_resized = True;

  // Notice other reasons to recalculate label position.
  if (curtip->tip.internal_width != newtip->tip.internal_width ||
      curtip->tip.internal_height != newtip->tip.internal_height)
    was_resized = True;

  // Notice if the colors or plain old font changed.  We need
  // get_or_change_GCs when international changes because the GCs need to be
  // writable if true and they need to have their font set if false.  Xaw
  // does not allow international to change.
  if (curtip->core.background_pixel != newtip->core.background_pixel)
    redisplay = True;
  if (curtip->tip.foreground != newtip->tip.foreground ||
      curtip->tip.font->fid != newtip->tip.font->fid ||
      curtip->tip.international != newtip->tip.international) {
    get_or_change_GCs(newtip); // Doesn't use bg color
    redisplay = True;
  }

  // Ignore insensitive.
  // if (XtIsSensitive(current) != XtIsSensitive(new))

  // Enable or disable the tip.
  if (curtip->tip.enable != newtip->tip.enable) {
    if (newtip->tip.enable)
      TipEnable(newtip);
    else
      TipDisable(newtip);
  }

  // Finally, act on was_resized and redisplay.  We don't have a resize
  // procedure, but TipGetSize and TipPosition are called every time the tip
  // is popped up.  So the path of least resistance is just to pop down the
  // tip and let nature take its course.
  if (was_resized) {
    ResetTip(newtip, False);
    redisplay = False;
  }

  return redisplay;
}

static void TipGetSize (TipWidget tip) {
  Dimension width=0, height=0;
  if (tip->tip.label)
    Xaw3dXftSizeAnyString(XtDisplay(tip), tip->tip.font, tip->tip.fontset,
      tip->tip.xftfont, tip->tip.international, tip->tip.encoding,
      tip->tip.label, &width, &height);
  XtWidth(tip) = width + tip->tip.internal_width * 2;
  XtHeight(tip) = height + tip->tip.internal_height * 2;
}

// There's no resource to change this
#define	DEFAULT_TIP_OFFSET 12

static void TipPosition (TipWidget tip) {
  Window r, c;
  int rx, ry, wx, wy;
  unsigned mask;
  Position x, y;
  int bw2 = XtBorderWidth(tip) * 2;
  int scr_width = WidthOfScreen(XtScreen(tip));
  int scr_height = HeightOfScreen(XtScreen(tip));
  int win_width = XtWidth(tip) + bw2;
  int win_height = XtHeight(tip) + bw2;

  XQueryPointer(XtDisplay((Widget)tip), XtWindow((Widget)tip),
		&r, &c, &rx, &ry, &wx, &wy, &mask);
  x = rx + DEFAULT_TIP_OFFSET;
  y = ry + DEFAULT_TIP_OFFSET;

  if (x + win_width > scr_width)
      x = scr_width - win_width;
  if (x < 0)
      x = 0;

  if (y + win_height > scr_height)
      y -= win_height + (DEFAULT_TIP_OFFSET << 1);
  if (y < 0)
      y = 0;

  XMoveResizeWindow(XtDisplay(tip), XtWindow(tip),
		    (int)(XtX(tip) = x), (int)(XtY(tip) = y),
		    (unsigned)XtWidth(tip),
		    (unsigned)XtHeight(tip));
}

static void ResetTip (TipWidget tip, Bool add_timeout) {
  if (tip->tip.timer) {
    XtRemoveTimeOut(tip->tip.timer);
    tip->tip.timer = 0;
  }
  if (tip->tip.mapped) {
    if (_Xaw3dXft->tip_do_grab)
      XtRemoveGrab(XtParent((Widget)tip));
    XUnmapWindow(XtDisplay((Widget)tip), XtWindow((Widget)tip));
    tip->tip.mapped = False;
  }
  if (add_timeout)
    tip->tip.timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)tip),
	tip->tip.timeout, TipTimeoutCallback, (XtPointer)tip);
}

static void TipTimeoutCallback (XtPointer client_data, XtIntervalId *id) {
  TipWidget tip = (TipWidget)client_data;
  assert(tip);
  tip->tip.timer = 0;
  TipGetSize(tip);
  XtRealizeWidget((Widget)tip);
  TipPosition(tip);
  if (_Xaw3dXft->show_tips) {
    XMapRaised(XtDisplay((Widget)tip), XtWindow((Widget)tip));
    if (_Xaw3dXft->tip_do_grab)
      XtAddGrab(XtParent((Widget)tip), True, True);
    tip->tip.mapped = True;
  }
}

static void TipShellEventHandler (Widget w, XtPointer client_data,
XEvent *event, Boolean *continue_to_dispatch) {
  TipWidget tip = (TipWidget)client_data;
  assert(tip);
  ResetTip(tip, False);
}

static void TipEventHandler (Widget w, XtPointer client_data, XEvent *event,
Boolean *continue_to_dispatch) {
  TipWidget tip = (TipWidget)client_data;
  assert(tip);
  Boolean add_timeout;
  switch (event->type) {
  case EnterNotify:
    add_timeout = True;
    break;
  case MotionNotify:
    /* If any button is pressed, timer is 0 */
    if (tip->tip.mapped)
      return;
    add_timeout = tip->tip.timer != 0;
    break;
  default:
    add_timeout = False;
  }
  ResetTip(tip, add_timeout);
}
