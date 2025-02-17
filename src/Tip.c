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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <X11/Xaw3dxft/Xaw3dP.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xaw3dxft/TipP.h>
#include <X11/Xaw3dxft/XawInit.h>
#include <X11/Xaw3dxft/Xaw3dXftP.h>

#include <stdlib.h>

#define	TIP_EVENT_MASK (ButtonPressMask	  |	\
			ButtonReleaseMask |	\
			ButtonMotionMask  |	\
/* 			PointerMotionMask |*/	\
			KeyPressMask	  |	\
			KeyReleaseMask	  |	\
			EnterWindowMask	  |	\
			LeaveWindowMask)

/*
 * Types
 */
typedef struct _WidgetInfo {
    Widget widget;
    String label;
    struct _WidgetInfo *next;
} WidgetInfo;

typedef struct _XawTipInfo {
    Screen *screen;
    TipWidget tip;
    Bool mapped;
    WidgetInfo *widgets;
    struct _XawTipInfo *next;
} XawTipInfo;

typedef struct {
    XawTipInfo *info;
    WidgetInfo *winfo;
} TimeoutInfo;

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
static WidgetInfo *CreateWidgetInfo(Widget);
static WidgetInfo *FindWidgetInfo(XawTipInfo *, Widget);
static XawTipInfo *CreateTipInfo(Widget);
static XawTipInfo *FindTipInfo(Widget);
static void ResetTip(XawTipInfo *, WidgetInfo *, Bool);
static void TipTimeoutCallback(XtPointer, XtIntervalId *);
static void TipLayout(XawTipInfo *);
static void TipPosition(XawTipInfo *);

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
#ifdef XAW_INTERNATIONALIZATION
  {XtNfontSet, XtCFontSet, XtRFontSet, sizeof(XFontSet),
    offset(fontset), XtRString, XtDefaultFontSet},
#endif
  {XtNlabel, XtCLabel, XtRString, sizeof(String),
    offset(label), XtRString, NULL},
  {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
    offset(encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
  {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    offset(internal_height), XtRImmediate, (XtPointer)2},
  {XtNinternalWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    offset(internal_width), XtRImmediate, (XtPointer)2},
  {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof(int),
    offset(backing_store), XtRImmediate,
    (XtPointer)(Always + WhenMapped + NotUseful)},
  {XtNtimeout, XtCTimeout, XtRInt, sizeof(int),
    offset(timeout), XtRImmediate, (XtPointer)1200},
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

static XawTipInfo *TipInfoList = NULL;
static TimeoutInfo TimeoutData;

/*
 * Implementation
 */

/*
 * XmuCvtBackingStoreToString() from XFree86's distribution, because
 * X.Org's distribution doesn't have it.
 */

/*ARGSUSED*/
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

static void
XawTipClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtAddConverter(XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
		   NULL, 0);
    XtSetTypeConverter(XtRBackingStore, XtRString, XawCvtBackingStoreToString,
		       NULL, 0, XtCacheNone, NULL);
}

/*ARGSUSED*/
static void
XawTipInitialize(Widget req, Widget w, ArgList args, Cardinal *num_args)
{
    TipWidget tip = (TipWidget)w;
    XGCValues values;

    tip->tip.timer = 0;

    values.foreground = tip->tip.foreground;
    values.background = tip->core.background_pixel;
    values.font = tip->tip.font->fid;
    values.graphics_exposures = False;

    tip->tip.gc = XtAllocateGC(w, 0, GCForeground | GCBackground | GCFont |
			       GCGraphicsExposures, &values, GCFont, 0);

    if (_Xaw3dXft->encoding)
	tip->tip.xftfont = Xaw3dXftGetFont(XtDisplayOfObject(w), tip->tip.xftfontname);
    else
	tip->tip.xftfont = NULL;
}

static void
XawTipDestroy(Widget w)
{
    XawTipInfo *info = FindTipInfo(w);
    WidgetInfo *winfo;
    TipWidget tip = (TipWidget)w;

    if (tip->tip.timer)
	XtRemoveTimeOut(tip->tip.timer);

    XtReleaseGC(w, tip->tip.gc);

    XtRemoveEventHandler(XtParent(w), KeyPressMask, False,
			 TipShellEventHandler, (XtPointer)NULL);

    while (info->widgets) {
	winfo = info->widgets->next;
	XtFree((char *)info->widgets->label);
	XtFree((char *)info->widgets);
	info->widgets = winfo;
    }

    if (info == TipInfoList)
	TipInfoList = TipInfoList->next;
    else {
	XawTipInfo *p = TipInfoList;

	while (p && p->next != info)
	    p = p->next;
	if (p)
	    p->next = info->next;
    }

    XtFree((char *)info);
}

static void
XawTipRealize(Widget w, Mask *mask, XSetWindowAttributes *attr)
{
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
				XtBorderWidth(w),
				DefaultDepthOfScreen(XtScreen(w)),
				InputOutput, CopyFromParent, *mask, attr);

    if (_Xaw3dXft->tip_background_color != -1)
	XtVaSetValues(w, XtNbackground, _Xaw3dXft->tip_background_color, NULL);
}

static void
XawTipExpose(Widget w, XEvent *event, Region region)
{
    TipWidget tip = (TipWidget)w;
    GC gc = tip->tip.gc;
    char *nl, *label = tip->tip.label;
    Position y;
    int len;
    static int bw = -1;

    if (_Xaw3dXft->border_hack) {
	/* work around composition/Xft related bug on some X servers... */
        if (bw == -1)
            bw = XtBorderWidth(w);
        if (bw)
            XSetWindowBorderWidth(XtDisplayOfObject(w), XtWindowOfObject(w), 0);
        for (y=0; y<bw; y++) {
	    len = 2*y + 1;
            XDrawRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
                           XtGetGC(w, 0, 0), y, y, 
                           XtWidth(w)-len, XtHeight(w)-len);
	}
    }

    y = tip->tip.internal_height + ((_Xaw3dXft->encoding)?
	tip->tip.xftfont->ascent + _Xaw3dXft->menu_spacing : 
	tip->tip.font->max_bounds.ascent );

#ifdef XAW_INTERNATIONALIZATION
    if (tip->tip.international == True && !_Xaw3dXft->encoding) {
	Position ksy = tip->tip.internal_height;
	XFontSetExtents *ext = XExtentsOfFontSet(tip->tip.fontset);

	ksy += abs(ext->max_ink_extent.y);

	while ((nl = index(label, '\n')) != NULL) {
	    XmbDrawString(XtDisplay(w), XtWindow(w), tip->tip.fontset,
			  gc, tip->tip.internal_width, ksy, label,
			  (int)(nl - label));
	    ksy += ext->max_ink_extent.height;
	    label = nl + 1;
	}
	len = strlen(label);
	if (len)
	    XmbDrawString(XtDisplay(w), XtWindow(w), tip->tip.fontset, gc,
			  tip->tip.internal_width, ksy, label, len);
    }
    else
#endif
    if (_Xaw3dXft->encoding) {
	bw = XtBorderWidth(w);
	while ((nl = index(label, '\n')) != NULL) {
	    Xaw3dXftDrawString(w, tip->tip.xftfont, 
		tip->tip.internal_width+3, y+_Xaw3dXft->border_hack,
		label, (int)(nl - label));
	    y += tip->tip.xftfont->height + 3*_Xaw3dXft->menu_spacing;
	    label = nl + 1;
	}
	len = strlen(label);
	if (len)
	    Xaw3dXftDrawString(w, tip->tip.xftfont, 
		tip->tip.internal_width+3, y+_Xaw3dXft->border_hack,
		label, len);
    } else {
	while ((nl = index(label, '\n')) != NULL) {
	    if (tip->tip.encoding)
		XDrawString16(XtDisplay(w), XtWindow(w), gc,
			      tip->tip.internal_width, y,
			      (XChar2b*)label, (int)(nl - label) >> 1);
	    else
		XDrawString(XtDisplay(w), XtWindow(w), gc,
			    tip->tip.internal_width, y,
			    label, (int)(nl - label));
	    y += tip->tip.font->max_bounds.ascent +
		 tip->tip.font->max_bounds.descent;
	    label = nl + 1;
	}
	len = strlen(label);
	if (len) {
	    if (tip->tip.encoding)
		XDrawString16(XtDisplay(w), XtWindow(w), gc,
			      tip->tip.internal_width, y,
			      (XChar2b*)label, len >> 1);
	    else
		XDrawString(XtDisplay(w), XtWindow(w), gc,
			    tip->tip.internal_width, y, label, len);
	}
    }
}

/*ARGSUSED*/
static Boolean
XawTipSetValues(Widget current, Widget request, Widget cnew, ArgList args, Cardinal *num_args)
{
    TipWidget curtip = (TipWidget)current;
    TipWidget newtip = (TipWidget)cnew;
    Boolean redisplay = False;

    if (curtip->tip.font->fid != newtip->tip.font->fid ||
		curtip->tip.foreground != newtip->tip.foreground) {
	XGCValues values;

	values.foreground = newtip->tip.foreground;
	values.background = newtip->core.background_pixel;
	values.font = newtip->tip.font->fid;
	values.graphics_exposures = False;
	XtReleaseGC(cnew, curtip->tip.gc);
	newtip->tip.gc = XtAllocateGC(cnew, 0, GCForeground | GCBackground |
				      GCFont | GCGraphicsExposures, &values,
				      GCFont, 0);
	redisplay = True;
    }

    return (redisplay);
}

static void
TipLayout(XawTipInfo *info)
{
    XFontStruct	*font = info->tip->tip.font;
    XftFont *xftfont = info->tip->tip.xftfont;
    int width = 0, height;
    char *nl, *label = info->tip->tip.label;

    if (_Xaw3dXft->encoding) {
	height = xftfont->height + 4;
	if ((nl = index(label, '\n')) != NULL) {
	    /*CONSTCOND*/
	    while (True) {
	        int w;
		w = Xaw3dXftTextWidth((Widget)info->tip, 
                                  xftfont, label, (int)(nl - label));
		if (w > width)
		    width = w;
		if (*nl == '\0')
		    break;
		label = nl + 1;
		if (*label)
		    height += xftfont->height + 3*_Xaw3dXft->menu_spacing;
		if ((nl = index(label, '\n')) == NULL)
		    nl = index(label, '\0');
	    }
	}
	else
	    width = Xaw3dXftTextWidth((Widget)info->tip, xftfont, label, strlen(label));
    } else
#ifdef XAW_INTERNATIONALIZATION
    if (info->tip->tip.international == True && !_Xaw3dXft->encoding) {
	XFontSet fset = info->tip->tip.fontset;
	XFontSetExtents *ext = XExtentsOfFontSet(fset);

	height = ext->max_ink_extent.height;
	if ((nl = index(label, '\n')) != NULL) {
	    /*CONSTCOND*/
	    while (True) {
		int w = XmbTextEscapement(fset, label, (int)(nl - label));

		if (w > width)
		    width = w;
		if (*nl == '\0')
		    break;
		label = nl + 1;
		if (*label)
		    height += ext->max_ink_extent.height;
		if ((nl = index(label, '\n')) == NULL)
		    nl = index(label, '\0');
	    }
	}
	else
	    width = XmbTextEscapement(fset, label, strlen(label));
    }
    else
#endif
    {
	height = font->max_bounds.ascent + font->max_bounds.descent;
	if ((nl = index(label, '\n')) != NULL) {
	    /*CONSTCOND*/
	    while (True) {
		int w = info->tip->tip.encoding ?
			XTextWidth16(font, (XChar2b*)label,
				     (int)(nl - label) >> 1) :
			XTextWidth(font, label, (int)(nl - label));

		if (w > width)
		    width = w;
		if (*nl == '\0')
		    break;
		label = nl + 1;
		if (*label)
		    height += font->max_bounds.ascent + font->max_bounds.descent;
		if ((nl = index(label, '\n')) == NULL)
		    nl = index(label, '\0');
	    }
	}
	else
	    width = info->tip->tip.encoding ?
		    XTextWidth16(font, (XChar2b*)label, strlen(label) >> 1) :
		    XTextWidth(font, label, strlen(label));
    }
    XtWidth(info->tip) = width + info->tip->tip.internal_width * 2
			 + ((_Xaw3dXft->encoding)? 6:0);
    XtHeight(info->tip) = height + info->tip->tip.internal_height * 2;
}

#define	DEFAULT_TIP_OFFSET	12

static void
TipPosition(XawTipInfo *info)
{
    Window r, c;
    int rx, ry, wx, wy;
    unsigned mask;
    Position x, y;
    int bw2 = XtBorderWidth(info->tip) * 2;
    int scr_width = WidthOfScreen(XtScreen(info->tip));
    int scr_height = HeightOfScreen(XtScreen(info->tip));
    int win_width = XtWidth(info->tip) + bw2;
    int win_height = XtHeight(info->tip) + bw2;

    XQueryPointer(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip),
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

    XMoveResizeWindow(XtDisplay(info->tip), XtWindow(info->tip),
		      (int)(XtX(info->tip) = x), (int)(XtY(info->tip) = y),
		      (unsigned)XtWidth(info->tip),
		      (unsigned)XtHeight(info->tip));
}

static WidgetInfo *
CreateWidgetInfo(Widget w)
{
    WidgetInfo *winfo = XtNew(WidgetInfo);

    winfo->widget = w;
    winfo->label = NULL;
    winfo->next = NULL;

    return (winfo);
}

static WidgetInfo *
FindWidgetInfo(XawTipInfo *info, Widget w)
{
    WidgetInfo *winfo, *wlist = info->widgets;

    if (wlist == NULL)
	return (info->widgets = CreateWidgetInfo(w));

    for (winfo = wlist; wlist; winfo = wlist, wlist = wlist->next)
	if (wlist->widget == w)
	    return (wlist);

    return (winfo->next = CreateWidgetInfo(w));
}

static XawTipInfo *
CreateTipInfo(Widget w)
{
    XawTipInfo *info = XtNew(XawTipInfo);
    Widget shell = w;

    while (XtParent(shell))
	shell = XtParent(shell);

    info->tip = (TipWidget)XtCreateWidget("tip", tipWidgetClass,
					  shell, NULL, 0);
    XtRealizeWidget((Widget)info->tip);
    info->screen = XtScreen(w);
    info->mapped = False;
    info->widgets = NULL;
    info->next = NULL;
    XtAddEventHandler(shell, KeyPressMask, False, TipShellEventHandler,
		      (XtPointer)NULL);

    return (info);
}

static XawTipInfo *
FindTipInfo(Widget w)
{
    XawTipInfo *info, *list = TipInfoList;
    Screen *screen;

    if (list == NULL)
	return (TipInfoList = CreateTipInfo(w));

    screen = XtScreen(w);
    for (info = list; list; info = list, list = list->next)
	if (list->screen == screen)
	    return (list);

    return (info->next = CreateTipInfo(w));
}

static void
ResetTip(XawTipInfo *info, WidgetInfo *winfo, Bool add_timeout)
{
    if (info->tip->tip.timer) {
	XtRemoveTimeOut(info->tip->tip.timer);
	info->tip->tip.timer = 0;
    }
    if (info->mapped) {
	if (_Xaw3dXft->tip_do_grab)
	    XtRemoveGrab(XtParent((Widget)info->tip));
	XUnmapWindow(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip));
	info->mapped = False;
    }
    if (add_timeout) {
	TimeoutData.info = info;
	TimeoutData.winfo = winfo;
	info->tip->tip.timer =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)info->tip),
			    info->tip->tip.timeout, TipTimeoutCallback,
			    (XtPointer)&TimeoutData);
    }
}

static void
TipTimeoutCallback(XtPointer closure, XtIntervalId *id)
{
    TimeoutInfo *cinfo = (TimeoutInfo *)closure;
    XawTipInfo *info = cinfo->info;
    WidgetInfo *winfo = cinfo->winfo;
    Arg args[2];

    info->tip->tip.label = winfo->label;
    info->tip->tip.encoding = 0;
    XtSetArg(args[0], XtNencoding, &info->tip->tip.encoding);
#ifdef XAW_INTERNATIONALIZATION
    info->tip->tip.international = False;
    XtSetArg(args[1], XtNinternational, &info->tip->tip.international);
    XtGetValues(winfo->widget, args, 2);
#else
    XtGetValues(winfo->widget, args, 1);
#endif

    TipLayout(info);
    TipPosition(info);
    if (_Xaw3dXft->show_tips) {
        XMapRaised(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip));
        if (_Xaw3dXft->tip_do_grab)
            XtAddGrab(XtParent((Widget)info->tip), True, True);
        info->mapped = True;
    }
}

/*ARGSUSED*/
static void
TipShellEventHandler(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    XawTipInfo *info = FindTipInfo(w);

    ResetTip(info, FindWidgetInfo(info, w), False);
}

/*ARGSUSED*/
static void
TipEventHandler(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    XawTipInfo *info = FindTipInfo(w);
    Boolean add_timeout;

    switch (event->type) {
	case EnterNotify:
	    add_timeout = True;
	    break;
	case MotionNotify:
	    /* If any button is pressed, timer is 0 */
	    if (info->mapped)
		return;
	    add_timeout = info->tip->tip.timer != 0;
	    break;
	default:
	    add_timeout = False;
	    break;
    }
    ResetTip(info, FindWidgetInfo(info, w), add_timeout);
}

/*
 * Public routines
 */
void
XawTipEnable(Widget w, String label)
{
    if (XtIsWidget(w) && label && *label) {
	XawTipInfo *info = FindTipInfo(w);
	WidgetInfo *winfo = FindWidgetInfo(info, w);

	if (winfo->label)
	    XtFree((char *)winfo->label);
	winfo->label = XtNewString(label);

	XtAddEventHandler(w, TIP_EVENT_MASK, False, TipEventHandler,
			  (XtPointer)NULL);
    }
}

void
XawTipDisable(Widget w)
{
    if (XtIsWidget(w)) {
	XawTipInfo *info = FindTipInfo(w);

	XtRemoveEventHandler(w, TIP_EVENT_MASK, False, TipEventHandler,
			     (XtPointer)NULL);
	ResetTip(info, FindWidgetInfo(info, w), False);
    }
}
