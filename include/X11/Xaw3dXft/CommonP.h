/*
 * CommonP.h
 *
 * Code pulled out of several widgets to reduce repetition.  These functions
 * are in the global namespace, but they are not part of the public API.  See
 * also, AnyString.
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#pragma once

#include <X11/Intrinsic.h>
#include <X11/Xft/Xft.h>

// Get an XftFont.
// This function caches fonts.  Never call XftFontClose on the returned font!
extern XftFont *Xaw3dXftGetFont (Widget object, char *name);

// Give a Pixel, get a XftColor
extern void Xaw3dXftGetXftColor (Display *display, Visual *visual,
  Colormap cmap, Pixel pixel, XftColor *result);

// GC getters need a widget or non-widget Object to take advantage of the GC
// caching that is done by Xt.

// Give a Pixel, get a GC where
//   foreground = bg
//   stipple = a 50% bitmap
//   fill_style = FillStippled
// It sprays bg color with a stencil that leaves half the pixels intact.
extern GC Xaw3dXftGetStippleGC (Widget w, Pixel bg);

// Get a GC where foreground color and font are prepared for DrawString.  If
// international is true, the GC is allocated with modifiable font because
// Xmb/wcDrawString does XSetFont on the GC.  Pass international(widget) or
// menuIntl(widget) for international.
extern GC Xaw3dXftGetTextGC (Widget w, Pixel fg, XFontStruct *font,
			     Boolean international);

// Get width, height, and depth of a drawable (most likely a pixmap).
// Returns Status from XGetGeometry (nonzero = good).  In case of failure,
// with and height are set to zero, depth is unmodified.  Unwanted returns
// can be passed as NULL.
extern Status Xaw3dXftGetDrawableDimensions (Display *display, Drawable d,
  Dimension *width, Dimension *height, Cardinal *depth);

// If depth is 1, do XCopyPlane; otherwise, do XCopyArea.
extern void Xaw3dXftCopy (Display *display, Drawable src, Drawable dest,
  GC gc, Dimension width, Dimension height, Cardinal depth, Position dest_x,
  Position dest_y);
