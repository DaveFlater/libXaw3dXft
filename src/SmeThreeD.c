/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
Copyright 1992, 1993 by Kaleb Keithley
© 2026 David Flater

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital, MIT, or Kaleb
Keithley not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

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
#include <assert.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <X11/Xaw3dXft/ThreeDP.h>
#include <X11/Xaw3dXft/SimpleMenP.h>
#include <X11/Xaw3dXft/SmeThreeDP.h>
#include <X11/Xosdefs.h>

/* Initialization of defaults */

#define XtNtopShadowPixmap "topShadowPixmap"
#define XtCTopShadowPixmap "TopShadowPixmap"
#define XtNbottomShadowPixmap "bottomShadowPixmap"
#define XtCBottomShadowPixmap "BottomShadowPixmap"
#define XtNshadowed "shadowed"
#define XtCShadowed "Shadowed"

#define offset(field) XtOffsetOf(SmeThreeDRec, field)

static XtResource resources[] = {
    {XtNshadowWidth, XtCShadowWidth, XtRDimension, sizeof(Dimension),
	offset(sme_threeD.shadow_width), XtRImmediate, (XtPointer) 2},
    {XtNtopShadowPixel, XtCTopShadowPixel, XtRPixel, sizeof(Pixel),
	offset(sme_threeD.top_shadow_pixel), XtRString, XtDefaultForeground},
    {XtNbottomShadowPixel, XtCBottomShadowPixel, XtRPixel, sizeof(Pixel),
	offset(sme_threeD.bot_shadow_pixel), XtRString, XtDefaultForeground},
    {XtNtopShadowPixmap, XtCTopShadowPixmap, XtRPixmap, sizeof(Pixmap),
	offset(sme_threeD.top_shadow_pxmap), XtRImmediate, (XtPointer) NULL},
    {XtNbottomShadowPixmap, XtCBottomShadowPixmap, XtRPixmap, sizeof(Pixmap),
	offset(sme_threeD.bot_shadow_pxmap), XtRImmediate, (XtPointer) NULL},
    {XtNtopShadowContrast, XtCTopShadowContrast, XtRInt, sizeof(int),
	offset(sme_threeD.top_shadow_contrast), XtRImmediate, (XtPointer) 20},
    {XtNbottomShadowContrast, XtCBottomShadowContrast, XtRInt, sizeof(int),
	offset(sme_threeD.bot_shadow_contrast), XtRImmediate, (XtPointer) 40},
    {XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	offset(sme_threeD.user_data), XtRPointer, (XtPointer) NULL},
    {XtNbeNiceToColormap, XtCBeNiceToColormap, XtRBoolean, sizeof(Boolean),
	offset(sme_threeD.be_nice_to_cmap), XtRImmediate, (XtPointer) True},
    {XtNshadowed, XtCShadowed, XtRBoolean, sizeof(Boolean),
	offset(sme_threeD.shadowed), XtRImmediate, (XtPointer) False},
    {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	XtOffsetOf(RectObjRec,rectangle.border_width), XtRImmediate,
	(XtPointer)0}
};

#undef offset

static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Destroy(Widget);
static void ClassPartInitialize(WidgetClass);
static void _XawSme3dDrawShadows(Widget);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

SmeThreeDClassRec smeThreeDClassRec = {
    { /* core fields */
    /* superclass		*/	(WidgetClass) &smeClassRec,
    /* class_name		*/	"SmeThreeD",
    /* widget_size		*/	sizeof(SmeThreeDRec),
    /* class_initialize		*/	NULL,
    /* class_part_initialize	*/	ClassPartInitialize,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	NULL,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* resource_count		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry           */	XtInheritQueryGeometry,
    /* display_accelerator      */	NULL,
    /* extension                */	NULL
    },
    { /* Menu Entry fields */
    /* highlight                */      XtInheritHighlight,
    /* unhighlight              */      XtInheritUnhighlight,
    /* notify                   */      XtInheritNotify,
    /* extension                */      NULL
    },
    { /* threeD fields */
    /* shadow draw              */      _XawSme3dDrawShadows
    }
};

WidgetClass smeThreeDObjectClass = (WidgetClass) &smeThreeDClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#define VisualClass(tdo) (tdo->sme_threeD.visual_class)
#define VisualDepth(tdo) (tdo->sme_threeD.depth)
#define BeNice(tdo)      (tdo->sme_threeD.be_nice_to_cmap)

// The values of visual class are at the bottom of X.h, which comments:
// Note that the statically allocated ones are even numbered and the
// dynamically changeable ones are odd numbered.
#define ShouldStipple(tdo) ( \
  (VisualDepth(tdo) <= 8 || VisualClass(tdo) & 1) && \
  (VisualDepth(tdo) == 1 || BeNice(tdo)))

#define shadowpm_width 8
#define shadowpm_height 8
static char shadowpm_bits[] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

static char mtshadowpm_bits[] = {
    0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24};

static char mbshadowpm_bits[] = {
    0x6d, 0xdb, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb};

static void
AllocTopShadowGC (Widget w)
{
    SmeThreeDObject	tdo = (SmeThreeDObject) w;
    XtGCMask		valuemask;
    XGCValues		myXGCV;

    if (ShouldStipple(tdo)) {
	valuemask = GCTile | GCFillStyle;
	myXGCV.tile = tdo->sme_threeD.top_shadow_pxmap;
	myXGCV.fill_style = FillTiled;
    } else {
	valuemask = GCForeground;
	myXGCV.foreground = tdo->sme_threeD.top_shadow_pixel;
    }
    tdo->sme_threeD.top_shadow_GC = XtGetGC(w, valuemask, &myXGCV);
}

static void
AllocBotShadowGC (Widget w)
{
    SmeThreeDObject	tdo = (SmeThreeDObject) w;
    XtGCMask		valuemask;
    XGCValues		myXGCV;

    if (ShouldStipple(tdo)) {
	valuemask = GCTile | GCFillStyle;
	myXGCV.tile = tdo->sme_threeD.bot_shadow_pxmap;
	myXGCV.fill_style = FillTiled;
    } else {
	valuemask = GCForeground;
	myXGCV.foreground = tdo->sme_threeD.bot_shadow_pixel;
    }
    tdo->sme_threeD.bot_shadow_GC = XtGetGC(w, valuemask, &myXGCV);
}

static void
AllocEraseGC (Widget w)
{
    Widget		parent = XtParent (w);
    SmeThreeDObject	tdo = (SmeThreeDObject) w;
    XtGCMask		valuemask;
    XGCValues		myXGCV;

    valuemask = GCForeground;
    myXGCV.foreground = parent->core.background_pixel;
    tdo->sme_threeD.erase_GC = XtGetGC(w, valuemask, &myXGCV);
}

static void
AllocTopShadowPixmap (Widget new)
{
    SmeThreeDObject	tdo = (SmeThreeDObject) new;
    Widget		parent = XtParent (new);
    Display		*dpy = XtDisplayOfObject (new);
    Screen		*scn = XtScreenOfObject (new);
    unsigned long	top_fg_pixel = 0, top_bg_pixel = 0;
    char		*pm_data;
    Boolean		create_pixmap = FALSE;

    /*
     * I know, we're going to create two pixmaps for each and every
     * shadow'd widget.  Yeuck.  I'm semi-relying on server side
     * pixmap caching.
     */

    if (VisualDepth(tdo) == 1) {
	top_fg_pixel = BlackPixelOfScreen (scn);
	top_bg_pixel = WhitePixelOfScreen (scn);
	pm_data = mtshadowpm_bits;
	create_pixmap = TRUE;
    } else if (ShouldStipple(tdo)) {
	if (parent->core.background_pixel == WhitePixelOfScreen (scn)) {
	    top_fg_pixel = WhitePixelOfScreen (scn);
	    top_bg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	} else if (parent->core.background_pixel == BlackPixelOfScreen (scn)) {
	    top_fg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	    top_bg_pixel = WhitePixelOfScreen (scn);
	} else {
	    top_fg_pixel = parent->core.background_pixel;
	    top_bg_pixel = WhitePixelOfScreen (scn);
	}
	static_assert(Got_XAW_defines);
#ifndef XAW_GRAY_BLKWHT_STIPPLES
	if (parent->core.background_pixel == WhitePixelOfScreen (scn) ||
	    parent->core.background_pixel == BlackPixelOfScreen (scn)) {
	    pm_data = mtshadowpm_bits;
       } else
#endif
	    pm_data = shadowpm_bits;

	create_pixmap = TRUE;
    }

    if (create_pixmap)
	tdo->sme_threeD.top_shadow_pxmap = XCreatePixmapFromBitmapData (dpy,
			RootWindowOfScreen (scn),
			pm_data,
			shadowpm_width,
			shadowpm_height,
			top_fg_pixel,
			top_bg_pixel,
			VisualDepth(tdo));
}

static void
AllocBotShadowPixmap (Widget new)
{
    SmeThreeDObject	tdo = (SmeThreeDObject) new;
    Widget		parent = XtParent (new);
    Display		*dpy = XtDisplayOfObject (new);
    Screen		*scn = XtScreenOfObject (new);
    unsigned long	bot_fg_pixel = 0, bot_bg_pixel = 0;
    char		*pm_data;
    Boolean		create_pixmap = FALSE;

    if (VisualDepth(tdo) == 1) {
	bot_fg_pixel = BlackPixelOfScreen (scn);
	bot_bg_pixel = WhitePixelOfScreen (scn);
	pm_data = mbshadowpm_bits;
	create_pixmap = TRUE;
    } else if (ShouldStipple(tdo)) {
	if (parent->core.background_pixel == WhitePixelOfScreen (scn)) {
	    bot_fg_pixel = grayPixel( WhitePixelOfScreen (scn), dpy, scn);
	    bot_bg_pixel = BlackPixelOfScreen (scn);
	} else if (parent->core.background_pixel == BlackPixelOfScreen (scn)) {
	    bot_fg_pixel = BlackPixelOfScreen (scn);
	    bot_bg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	} else {
	    bot_fg_pixel = parent->core.background_pixel;
	    bot_bg_pixel = BlackPixelOfScreen (scn);
	}
#ifndef XAW_GRAY_BLKWHT_STIPPLES
	if (parent->core.background_pixel == WhitePixelOfScreen (scn) ||
	    parent->core.background_pixel == BlackPixelOfScreen (scn)) {
	    pm_data = mbshadowpm_bits;
	} else
#endif
	    pm_data = shadowpm_bits;

	create_pixmap = TRUE;
    }

    if (create_pixmap)
	tdo->sme_threeD.bot_shadow_pxmap = XCreatePixmapFromBitmapData (dpy,
			RootWindowOfScreen (scn),
			pm_data,
			shadowpm_width,
			shadowpm_height,
			bot_fg_pixel,
			bot_bg_pixel,
			VisualDepth(tdo));
}

void
XawSme3dComputeTopShadowRGB (Widget new, XColor *xcol_out)
{
    if (XtIsSubclass (new, smeThreeDObjectClass)) {
	SmeThreeDObject tdo = (SmeThreeDObject) new;
	Widget w = XtParent (new);
	XColor get_c;
	double contrast;
	Display *dpy = XtDisplayOfObject (new);
	Screen *scn = XtScreenOfObject (new);
	Colormap cmap = w->core.colormap;

	get_c.pixel = w->core.background_pixel;
	if (get_c.pixel == WhitePixelOfScreen (scn) ||
	    get_c.pixel == BlackPixelOfScreen (scn)) {
	    contrast = (100 - tdo->sme_threeD.top_shadow_contrast) / 100.0;
	    xcol_out->red   = contrast * 65535.0;
	    xcol_out->green = contrast * 65535.0;
	    xcol_out->blue  = contrast * 65535.0;
	} else {
	    contrast = 1.0 + tdo->sme_threeD.top_shadow_contrast / 100.0;
	    XQueryColor (dpy, cmap, &get_c);
#define MIN(x,y) (unsigned short) (x < y) ? x : y
	    xcol_out->red   = MIN (65535, (int) (contrast * (double) get_c.red));
	    xcol_out->green = MIN (65535, (int) (contrast * (double) get_c.green));
	    xcol_out->blue  = MIN (65535, (int) (contrast * (double) get_c.blue));
#undef MIN
	}
    } else
	xcol_out->red = xcol_out->green = xcol_out->blue = 0;
}

static void
AllocTopShadowPixel (Widget new)
{
    XColor set_c;
    SmeThreeDObject tdo = (SmeThreeDObject) new;
    Widget w = XtParent (new);
    Display *dpy = XtDisplayOfObject (new);
    Colormap cmap = w->core.colormap;

    XawSme3dComputeTopShadowRGB (new, &set_c);
    (void) XAllocColor (dpy, cmap, &set_c);
    tdo->sme_threeD.top_shadow_pixel = set_c.pixel;
}


void
XawSme3dComputeBottomShadowRGB (Widget new, XColor *xcol_out)
{
    if (XtIsSubclass (new, smeThreeDObjectClass)) {
	SmeThreeDObject tdo = (SmeThreeDObject) new;
	Widget w = XtParent (new);
	XColor get_c;
	double contrast;
	Display *dpy = XtDisplayOfObject (new);
	Screen *scn = XtScreenOfObject (new);
	Colormap cmap = w->core.colormap;

	get_c.pixel = w->core.background_pixel;
	if (get_c.pixel == WhitePixelOfScreen (scn) ||
	    get_c.pixel == BlackPixelOfScreen (scn)) {
	    contrast = tdo->sme_threeD.bot_shadow_contrast / 100.0;
	    xcol_out->red   = contrast * 65535.0;
	    xcol_out->green = contrast * 65535.0;
	    xcol_out->blue  = contrast * 65535.0;
	} else {
	    XQueryColor (dpy, cmap, &get_c);
	    contrast = (100 - tdo->sme_threeD.bot_shadow_contrast) / 100.0;
	    xcol_out->red   = contrast * get_c.red;
	    xcol_out->green = contrast * get_c.green;
	    xcol_out->blue  = contrast * get_c.blue;
	}
    } else
	xcol_out->red = xcol_out->green = xcol_out->blue = 0;
}

static void
AllocBotShadowPixel (Widget new)
{
    XColor set_c;
    SmeThreeDObject tdo = (SmeThreeDObject) new;
    Widget w = XtParent (new);
    Display *dpy = XtDisplayOfObject (new);
    Colormap cmap = w->core.colormap;

    XawSme3dComputeBottomShadowRGB (new, &set_c);
    (void) XAllocColor (dpy, cmap, &set_c);
    tdo->sme_threeD.bot_shadow_pixel = set_c.pixel;
}


static void
ClassPartInitialize (WidgetClass wc)
{
    SmeThreeDClassRec *tdwc = (SmeThreeDClassRec *) wc;
    SmeThreeDClassRec *super =
	(SmeThreeDClassRec *) tdwc->rect_class.superclass;

    if (tdwc->sme_threeD_class.shadowdraw == XtInheritXawSme3dShadowDraw)
	tdwc->sme_threeD_class.shadowdraw = super->sme_threeD_class.shadowdraw;
}


static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    SmeThreeDObject w = (SmeThreeDObject) new;

    Xaw3dXftGetVisualInfo(new, NULL, &VisualClass(w), &VisualDepth(w));

    if (ShouldStipple(w)) {
	AllocTopShadowPixmap (new);
	AllocBotShadowPixmap (new);
    } else {
	if (w->sme_threeD.top_shadow_pixel == w->sme_threeD.bot_shadow_pixel) {
	    AllocTopShadowPixel (new);
	    AllocBotShadowPixel (new);
	}
	w->sme_threeD.top_shadow_pxmap = w->sme_threeD.bot_shadow_pxmap = 0;
    }
    AllocTopShadowGC (new);
    AllocBotShadowGC (new);
    AllocEraseGC (new);
}

static void
Destroy (Widget gw)
{
    SmeThreeDObject w = (SmeThreeDObject) gw;
    XtReleaseGC (gw, w->sme_threeD.top_shadow_GC);
    XtReleaseGC (gw, w->sme_threeD.bot_shadow_GC);
    XtReleaseGC (gw, w->sme_threeD.erase_GC);
    if (w->sme_threeD.top_shadow_pxmap)
	XFreePixmap (XtDisplayOfObject (gw), w->sme_threeD.top_shadow_pxmap);
    if (w->sme_threeD.bot_shadow_pxmap)
	XFreePixmap (XtDisplayOfObject (gw), w->sme_threeD.bot_shadow_pxmap);
}

static Boolean
SetValues (Widget gcurrent, Widget grequest, Widget gnew, ArgList args, Cardinal *num_args)
{
    SmeThreeDObject current = (SmeThreeDObject) gcurrent;
    SmeThreeDObject new = (SmeThreeDObject) gnew;
    Boolean redisplay = FALSE;
    Boolean alloc_top_pixel = FALSE;
    Boolean alloc_bot_pixel = FALSE;
    Boolean alloc_top_pixmap = FALSE;
    Boolean alloc_bot_pixmap = FALSE;

#if 0
    (*threeDWidgetClass->core_class.superclass->core_class.set_values)
	(gcurrent, grequest, gnew, NULL, 0);
#endif
    assert(VisualClass(new) == VisualClass(current));
    assert(VisualDepth(new) == VisualDepth(current));
    const Boolean ShouldStippleNew = ShouldStipple(new),
              ShouldStippleCurrent = ShouldStipple(current);

    if (new->sme_threeD.shadow_width != current->sme_threeD.shadow_width)
	redisplay = TRUE;
    if (ShouldStippleNew != ShouldStippleCurrent) {
        if (ShouldStippleNew) {
	    alloc_top_pixmap = TRUE;
	    alloc_bot_pixmap = TRUE;
	} else {
	    alloc_top_pixel = TRUE;
	    alloc_bot_pixel = TRUE;
	}
	redisplay = TRUE;
    }
    if (!ShouldStippleNew &&
	new->sme_threeD.top_shadow_contrast != current->sme_threeD.top_shadow_contrast)
	alloc_top_pixel = TRUE;
    if (!ShouldStippleNew &&
	new->sme_threeD.bot_shadow_contrast != current->sme_threeD.bot_shadow_contrast)
	alloc_bot_pixel = TRUE;
    if (alloc_top_pixel)
	AllocTopShadowPixel (gnew);
    if (alloc_bot_pixel)
	AllocBotShadowPixel (gnew);
    if (alloc_top_pixmap)
	AllocTopShadowPixmap (gnew);
    if (alloc_bot_pixmap)
	AllocBotShadowPixmap (gnew);
    if (!ShouldStippleNew &&
	new->sme_threeD.top_shadow_pixel != current->sme_threeD.top_shadow_pixel)
	alloc_top_pixel = TRUE;
    if (!ShouldStippleNew &&
	new->sme_threeD.bot_shadow_pixel != current->sme_threeD.bot_shadow_pixel)
	alloc_bot_pixel = TRUE;
    if (ShouldStippleNew) {
	if (alloc_top_pixmap) {
	    XtReleaseGC (gcurrent, current->sme_threeD.top_shadow_GC);
	    AllocTopShadowGC (gnew);
	    redisplay = True;
	}
	if (alloc_bot_pixmap) {
	    XtReleaseGC (gcurrent, current->sme_threeD.bot_shadow_GC);
	    AllocBotShadowGC (gnew);
	    redisplay = True;
	}
    } else {
	if (alloc_top_pixel) {
	    if (new->sme_threeD.top_shadow_pxmap) {
		XFreePixmap (XtDisplayOfObject (gnew), new->sme_threeD.top_shadow_pxmap);
		new->sme_threeD.top_shadow_pxmap = (Pixmap) NULL;
	    }
	    XtReleaseGC (gcurrent, current->sme_threeD.top_shadow_GC);
	    AllocTopShadowGC (gnew);
	    redisplay = True;
	}
	if (alloc_bot_pixel) {
	    if (new->sme_threeD.bot_shadow_pxmap) {
		XFreePixmap (XtDisplayOfObject (gnew), new->sme_threeD.bot_shadow_pxmap);
		new->sme_threeD.bot_shadow_pxmap = (Pixmap) NULL;
	    }
	    XtReleaseGC (gcurrent, current->sme_threeD.bot_shadow_GC);
	    AllocBotShadowGC (gnew);
	    redisplay = True;
	}
    }
    return (redisplay);
}

static void
_XawSme3dDrawShadows(Widget gw)
{
    // 2026-06-29
    // This is the *third* shadow-drawing function after _Xaw3dDrawShadows
    // and _ShadowSurroundedBox in ThreeD.c.  This one was broken.  Complete
    // replacement based on _ShadowSurroundedBox.

    SmeThreeDObject tdo = (SmeThreeDObject) gw;
    Dimension s = tdo->sme_threeD.shadow_width;

    if (s > 0 && XtIsRealized(gw))
    {
	Dimension  w = tdo->rectangle.width,
	           h = tdo->rectangle.height,
	         wms = w - s,
	         hms = h - s,
	          sm = (s > 1 ? s / 2 : 1),
	        wmsm = w - sm,
	        hmsm = h - sm;
	Position  x0 = tdo->rectangle.x,
	          y0 = tdo->rectangle.y,
	          x1 = x0 + w,
	          y1 = y0 + h;
	Display	*dpy = XtDisplayOfObject(gw);
	Window	 win = XtWindowOfObject(gw);
	GC	 top, bot;
	XPoint   pt[6];

	if (tdo->sme_threeD.shadowed)
	{
	    top = tdo->sme_threeD.top_shadow_GC;
	    bot = tdo->sme_threeD.bot_shadow_GC;
	}
	else
	    top = bot = tdo->sme_threeD.erase_GC;

	// The rest is identical to _ShadowSurroundedBox.

	/* top-left shadow */
	pt[0].x = x0;		pt[0].y = y0 + h;
	pt[1].x = x0;		pt[1].y = y0;
	pt[2].x = x0 + w;	pt[2].y = y0;
	pt[3].x = x0 + wmsm;	pt[3].y = y0 + sm - 1;
	pt[4].x = x0 + sm;	pt[4].y = y0 + sm;
	pt[5].x = x0 + sm - 1;	pt[5].y = y0 + hmsm;
	XFillPolygon(dpy, win, top, pt, 6, Complex, CoordModeOrigin);
	if (s > 1)
	{
	    pt[0].x = x0 + s - 1;	pt[0].y = y0 + hms;
	    pt[1].x = x0 + s;		pt[1].y = y0 + s;
	    pt[2].x = x0 + wms;		pt[2].y = y0 + s - 1;
	    XFillPolygon(dpy, win, top, pt, 6, Complex, CoordModeOrigin);
	}

	/* bottom-right shadow */
	pt[0].x = x0;		pt[0].y = y0 + h;
	pt[1].x = x0 + w;	pt[1].y = y0 + h;
	pt[2].x = x0 + w;	pt[2].y = y0;
	pt[3].x = x0 + wmsm;	pt[3].y = y0 + sm - 1;
	pt[4].x = x0 + wmsm;	pt[4].y = y0 + hmsm;
	pt[5].x = x0 + sm - 1;	pt[5].y = y0 + hmsm;
	XFillPolygon(dpy, win, bot, pt, 6, Complex, CoordModeOrigin);
	if (s > 1)
	{
	    pt[0].x = x0 + s - 1;	pt[0].y = y0 + hms;
	    pt[1].x = x0 + wms;		pt[1].y = y0 + hms;
	    pt[2].x = x0 + wms;		pt[2].y = y0 + s - 1;
	    XFillPolygon(dpy, win, bot, pt, 6, Complex, CoordModeOrigin);
	}
    }
}
