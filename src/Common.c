/*
 * Common.c
 *
 * Code pulled out of several widgets to reduce repetition.  These functions
 * are in the global namespace, but they are not part of the public API.  See
 * also, AnyString.
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#include <assert.h>
#include <limits.h>
#include <X11/Xaw3dXft/Xaw3d.h>
#include <X11/Xaw3dXft/CommonP.h>

// FIXME lingering dependency on global struct for default font
#include <X11/Xaw3dXft/Xaw3dXft.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <sysexits.h>
#define XAW3DXFT_DEFAULTFONT "Liberation-9"

static_assert(Got_XAW_defines);

/*
  Calling XftFontOpen* repeatedly is *amazingly* slow.  This was hidden by
  Xaw3dXft 1.x because 99.99% of calls just returned the previously opened
  _Xaw3dXft->default_font.  To achieve comparable performance with every
  Label looking up the font by name, cache all Xft fonts by their
  non-canonicalized names and never call XftFontClose.
*/
typedef struct _xftcache {
  const char *name;
  XftFont *xftFont;
  struct _xftcache *next;
} xftcache;

static xftcache *xftFontCache = NULL;

static void cacheNewFont (const char *name, XftFont *xftFont) {
  xftcache *new = malloc(sizeof(xftcache));
  assert(new);
  new->name = strdup(name);
  assert(new->name);
  new->xftFont = xftFont;
  new->next = xftFontCache;
  xftFontCache = new;
}

static XftFont *findOrGetFont (Widget object, const char *name) {
  for (xftcache *l = xftFontCache; l; l = l->next)
    if (!strcmp(name, l->name))
      return l->xftFont;
  Display *dpy = XtDisplayOfObject(object);
  int scrnum = XScreenNumberOfScreen(XtScreenOfObject(object));
  XftFont *xftFont;
  if (strncasecmp(name, "core:",5))
    xftFont = XftFontOpenName(dpy, scrnum, name);
  else
    xftFont = XftFontOpenXlfd(dpy, scrnum, name+5);
  if (!xftFont) {
    // XftFontOpen never returns null.  It just silently substitutes a
    // different font.  But if the impossible happens, this message will be
    // more helpful than a crash.
    fprintf(stderr, "Xft font not found: %s\n", name);
    exit(EX_UNAVAILABLE);
  }
  cacheNewFont(name, xftFont);
  return xftFont;
}

XftFont *Xaw3dXftGetFont (Widget object, char *name) {
  if (name)
    return findOrGetFont(object, name);
  // FIXME
  if (!_Xaw3dXft->default_font) {
    if (!_Xaw3dXft->default_fontname)
      _Xaw3dXft->default_fontname = XAW3DXFT_DEFAULTFONT;
    _Xaw3dXft->default_font = findOrGetFont(object, _Xaw3dXft->default_fontname);
  }
  return _Xaw3dXft->default_font;
}

void Xaw3dXftGetXftColor (Display *display, Visual *visual, Colormap cmap,
Pixel pixel, XftColor *result) {
  XRenderColor xre_color = {0U, 0U, 0U, USHRT_MAX}; // default black
  XColor xcol;
  xcol.pixel = pixel;
  xcol.flags = DoRed | DoGreen | DoBlue;
  XQueryColor(display, cmap, &xcol);
  xre_color.red = xcol.red;
  xre_color.green = xcol.green;
  xre_color.blue = xcol.blue;
  XftColorAllocValue(display, visual, cmap, &xre_color, result);
}

/* FIXME
  We currently have three different stippling methods:

  1. When the widgets draw with a stipple, they do this:
    v.font       = font->fid;
    v.fill_style = FillTiled;
    v.tile       = XmuCreateStippledPixmap(screen, fg, bg, depth);
    v.graphics_exposures = False;

  2. stipplePixmap modifies a pixmap by adding an entry to the colorTable and
  then changing half of the pixels to point to that.

  3. drawOneXftLine fills a rectangle with fill_style = FillStippled.  While
  method 1 uses a pixmap with specified fg and bg, this method uses a bitmap
  as a stencil for the bg color being sprayed by the GC.
*/

GC Xaw3dXftGetStippleGC (Widget w, Pixel bg) {
  static Boolean first = True;
  static Pixmap p;
  if (first) {
    // libx11/src/CrBFData.c XCreateBitmapFromData:
    // "D is any drawable on the same screen that the pixmap will be used in."
    first = false;
    Display *display = XtDisplayOfObject(w);
    // Avoid requiring XtIsRealized(w)
    Window window = RootWindowOfScreen(XtScreenOfObject(w));
    uint8_t data[] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    p = XCreateBitmapFromData(display, window, (char*)data, 8, 8);
  }
  XGCValues v;
  v.foreground = bg;
  v.stipple = p;
  v.fill_style = FillStippled;
  v.graphics_exposures = False;
  return XtGetGC(w, GCForeground|GCStipple|GCFillStyle|GCGraphicsExposures, &v);
}

GC Xaw3dXftGetTextGC (Widget w, Pixel fg, XFontStruct *font,
		      Boolean international) {
  XGCValues v;
  v.foreground = fg;
  v.graphics_exposures = False;
#ifdef XAW_INTERNATIONALIZATION
  if (international)
    /* Since Xmb/wcDrawString eats the font, I must use XtAllocateGC. */
    // That means:  Xmb/wcDrawString does XSetFont on the GC.
    return XtAllocateGC(w, 0, GCForeground|GCGraphicsExposures, &v, GCFont, 0);
#endif
  assert(font);
  v.font = font->fid;
  return XtGetGC(w, GCForeground|GCFont|GCGraphicsExposures, &v);
}

Status Xaw3dXftGetDrawableDimensions (Display *display, Drawable d,
Dimension *width, Dimension *height, Cardinal *depth) {
  Status status = 0;
  if (d) {
    // XGetGeometry does not check for null pointers, and the int sizes are
    // all wrong.
    Window rootWindow;
    int x, y;
    unsigned int rwidth, rheight, borderWidth, rdepth;
    status = XGetGeometry(display, d, &rootWindow, &x, &y, &rwidth, &rheight,
			  &borderWidth, &rdepth);
    if (status) {
      if (width)  *width = rwidth;
      if (height) *height = rheight;
      if (depth)  *depth = rdepth;
    }
  }
  if (!status) {
    if (width)  *width = 0;
    if (height) *height = 0;
  }
  return status;
}

void Xaw3dXftCopy (Display *display, Drawable src, Drawable dest,
GC gc, Dimension width, Dimension height, Cardinal depth, Position dest_x,
Position dest_y) {
  if (depth == 1)
    XCopyPlane(display, src, dest, gc, 0, 0, width, height, dest_x, dest_y, 1);
  else
    XCopyArea(display, src, dest, gc, 0, 0, width, height, dest_x, dest_y);
}
