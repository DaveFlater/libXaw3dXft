/*
 * AnyString.h
 * Widget-agnostic functions for dealing with text and related issues
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#pragma once

#include <X11/Intrinsic.h>
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dXft/Xaw3d.h>
#include <X11/Xaw3dXft/Encoding.h>

/*
  Raison d'etre

  Original state:  widgets contained long, complex, repeated trees of
  conditionals to deal with three different font systems, multiple encodings,
  etc.  Over time these diverged and acquired different features, quirks, and
  bugs that were hard to explain.

  Goal state:  widgets make single calls to generic functions.  Behavior is
  consistent.

  Tactic:  pass all of the parameters that might apply and do the decision
  trees inside the functions.

  Result:  some functions with very long parameter lists, which are ugly but
  more maintainable.

  In an ideal world, you would just pass a widget and the scavenger hunt
  would be done inside the functions.  But the widgets are inconsistent about
  where various things are found.  Some of our callers don't even inherit
  from Core!  So make the callers do the scavenger hunt themselves.
*/

#ifdef __cplusplus
extern "C" {
#endif

// Certain widget resources disappear entirely if XAW_INTERNATIONALIZATION is
// disabled.  Callers should use these macros in the parameter lists to avoid
// adding ifdefs.  Note that the typedef for XFontSet goes away if
// XAW_INTERNATIONALIZATION is disabled.  That's why fontSet is void*.
static_assert(Got_XAW_defines);
#ifdef XAW_INTERNATIONALIZATION
#define international(w) w->simple.international
#define labelFontSet(w)  w->label.fontset
#else
#define international(w) False
#define labelFontSet(w)  NULL
#endif

// Genericized DrawString
extern void Xaw3dXftDrawAnyStringLen (
  Display *display, Visual *visual, Colormap cmap, Window window,

  // One of the following will apply.
  XFontStruct *font, void *fontSet, XftFont *xftFont,

  // Pass XtIsSensitive(widget) and international(widget)
  Boolean sensitive,
  Boolean international,

  // If xftFont is not null:
  //   Background is cleared to bg
  //   Text is drawn with fg
  //   If !sensitive, stipple is applied using stipple_gc
  //   Setting fg.color.alpha has an effect, but it's not what you'd want.
  //   (Transparency is not relative to the specified bg.)
  // else if international:
  //   fontSet must be valid
  //   text_gc must have modifiable font
  //   Foreground must be set in text_gc
  //   Background is not cleared
  //   Text is drawn with text_gc
  // else plain old X font:
  //   Foreground and font must be set in text_gc
  //   Background is not cleared
  //   Text is drawn with text_gc
  GC text_gc, GC stipple_gc, XftColor *fg, XftColor *bg,

  Position x, Position y,

  // A string in the specified encoding
  XawTextEncoding encoding,
  void *text,

  // Number of bytes (not characters) to draw from text
  Cardinal num_bytes
);

// Ibid. but using the null teminator to determine num_bytes
extern void Xaw3dXftDrawAnyString (Display *display, Visual *visual,
  Colormap cmap, Window window, XFontStruct *font, void *fontSet,
  XftFont *xftFont, Boolean sensitive, Boolean international, GC text_gc,
  GC stipple_gc, XftColor *fg, XftColor *bg, Position x, Position y,
  XawTextEncoding encoding, void *text);

// Genericized TextWidth/TextHeight
extern void Xaw3dXftSizeAnyStringLen (
  Display *display,

  // One of the following will apply.
  XFontStruct *font, void *fontSet, XftFont *xftFont,

  // Pass international(widget)
  Boolean international,

  // A string in the specified encoding
  XawTextEncoding encoding,
  void *text,

  // Number of bytes (not characters) to draw from text
  Cardinal num_bytes,

  // Results out
  Dimension *width, Dimension *height
);

// Ibid. but using the null teminator to determine num_bytes
extern void Xaw3dXftSizeAnyString (Display *display, XFontStruct *font,
  void *fontSet, XftFont *xftFont, Boolean international,
  XawTextEncoding encoding, void *text, Dimension *width, Dimension *height);

// Strdup for any encoding
extern void *Xaw3dXftAnyStrdup (XawTextEncoding encoding, void *text);

// Give a Pixel, get a XftColor
extern void Xaw3dXftGetXftColor (Display *display, Visual *visual,
  Colormap cmap, Pixel pixel, XftColor *result);

// Give a Pixel, get a stipple_gc.  We need a widget or non-widget Object
// here to take advantage of the GC caching that is done by Xt.
extern GC Xaw3dXftGetStippleGC (Widget w, Pixel bg);

#ifdef __cplusplus
}
#endif
