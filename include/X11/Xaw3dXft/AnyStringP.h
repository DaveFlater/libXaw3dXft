/*
 * AnyStringP.h
 *
 * Widget-agnostic functions for dealing with text.  These functions are in
 * the global namespace, but they are not part of the public API.
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#pragma once

#include <X11/Intrinsic.h>
#include <X11/Xft/Xft.h>
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


/*
             *** RULES ***
  1.  All strings MUST be NUL-terminated unless otherwise specified.
  2.  The value of num_bytes MUST NOT exceed the number of bytes preceding
      the NUL.
  3.  The first num_bytes bytes of the string MUST contain a whole number of
      characters, i.e., MUST NOT end in the middle of a multibyte character.
  4.  A string MUST be less than than 1 GB.  This applies to the input, the
      output, and any temporary strings created in between.
  Violating these rules invokes undefined behavior.
*/

// The Xlib and libXft DrawString functions take at most INT_MAX characters.
// Far fewer can actually be drawn in window at one time.  This limit should
// be lowered to what Text can reliably edit.  FIXME
#define Xaw3dXftAnyStringLengthLimit 999999999U


// ---- String functions ----

/*
  The Xft tutorial warns:  "Note that drawing the same string multiple times
  in the same place will generate the wrong result with AA text."

  When libXft draws anti-aliased text, the fringe of that text is blended
  with the background.  If you then redraw that text in the same place, that
  background at the fringe is not what it was:  it already has half of a
  letter blended into it.  So each time you redraw, the fringe gets more
  corrupted.
*/

// Genericized DrawString with specified length (num_bytes)
extern void Xaw3dXftDrawAnyStringN (
  Display *display, Visual *visual, Colormap cmap, Window window,

  // One of the following will apply.
  XFontStruct *font, XFontSet fontSet, XftFont *xftFont,

  // Pass international resource of widget.
  Boolean international,

  // If xftFont is not null:
  //   Text is drawn with fg
  // else if international:
  //   fontSet must be valid
  //   text_gc must have modifiable font
  //   Foreground must be set in text_gc
  //   Text is drawn with text_gc
  // else plain old X font:
  //   Foreground and font must be set in text_gc
  //   Text is drawn with text_gc
  // Caller is responsible for preparing the background as necessary in all
  // cases.
  GC text_gc, XftColor *fg,

  // Upper left corner
  Position x, Position y,

  // Optional clipping area (pass NULL if unwanted).  For font or fontSet,
  // text_gc will be altered with XSetClipRectangles and then put back with
  // XSetClipMask to None.  In code as incoming (Xaw List.c), this was done
  // to ostensibly non-modifiable GCs without ill effects, and we continue
  // that practice.
  XRectangle *clip,

  // A string in the specified encoding
  XawTextEncoding encoding,
  const void *text,

  // Number of bytes (not characters) to draw from text
  Cardinal num_bytes
);

// Ibid. but using the null terminator to determine num_bytes
extern void Xaw3dXftDrawAnyString (Display *display, Visual *visual,
  Colormap cmap, Window window, XFontStruct *font, XFontSet fontSet,
  XftFont *xftFont, Boolean international, GC text_gc, XftColor *fg,
  Position x, Position y, XRectangle *clip, XawTextEncoding encoding,
  const void *text);


// Genericized TextWidth/TextHeight with specified length (num_bytes)
extern void Xaw3dXftSizeAnyStringN (
  Display *display,

  // One of the following will apply.
  XFontStruct *font, XFontSet fontSet, XftFont *xftFont,

  // Pass international resource of widget.
  Boolean international,

  // A string in the specified encoding
  XawTextEncoding encoding,
  const void *text,

  // Number of bytes (not characters) to draw from text
  Cardinal num_bytes,

  // Results out
  Dimension *width, Dimension *height
);

// Ibid. but using the null terminator to determine num_bytes
extern void Xaw3dXftSizeAnyString (Display *display, XFontStruct *font,
  XFontSet fontSet, XftFont *xftFont, Boolean international,
  XawTextEncoding encoding, const void *text, Dimension *width,
  Dimension *height);


// Genericized strndup with specified length (num_bytes).  Rule 1 is waived
// for the input; the result will be NUL-terminated.  This is the recommended
// treatment for an unterminated string.
extern void *Xaw3dXftAnyStrdupN (XawTextEncoding encoding, const void *text,
  Cardinal num_bytes);

// Ibid. but using a null terminator to determine num_bytes.  Obviously, Rule
// 1 is NOT waived in this case.
extern void *Xaw3dXftAnyStrdup (XawTextEncoding encoding, const void *text);


// Return number of bytes in any string, not counting null terminator
extern Cardinal Xaw3dXftAnyStrlen (XawTextEncoding encoding, const void *text);


// ---- Font metrics ----

/*
  Logical height:  nominal advance from one line to the next, in pixels.

  Logical ascent:  nominal distance between the top of the box (the y
  coordinate passed to Xaw3dXftDrawAnyStringN) and the baseline of the font,
  in pixels.

  Figure width:  nominal character width used for setting tabs, in pixels.
  It is actually the width of the '$' character.

  Pass NULL for unwanted returns.
*/
extern void Xaw3dXftAnyFontMetrics (Display *display, XFontStruct *font,
  XFontSet fontSet, XftFont *xftFont, Boolean international, Dimension *height,
  Dimension *ascent, Dimension *width);


// ---- Special interest functions ----

// For SmeBSB
// Find the coordinates to underline one character in supplied text.  Returns
// True if coordinates are valid, False if cannot comply.
extern Boolean Xaw3dXftLocateUnderline (
  // Similar to Xaw3dXftSizeAnyString
  Display *display,
  XFontStruct *font, XFontSet fontSet, XftFont *xftFont,
  Boolean international,
  XawTextEncoding encoding,
  const void *text,
  int character_index, // characters, not bytes
  // Results out
  Position *x1, Position *x2, Position *y);

// For MultiSrc
// Convert any string to wc encoding.  num_bytes is updated as applicable.
// Caller is responsible for freeing the returned string.
extern wchar_t *Xaw3dXftAnyToWcN (XawTextEncoding encoding, const void *text,
				 Cardinal *num_bytes);

// For MultiSrc
// Convert wc to any encoding.  num_bytes is updated as applicable.  Caller
// is responsible for freeing the returned string.
extern void *Xaw3dXftWcToAnyN (const wchar_t *text, Cardinal *num_bytes,
  XawTextEncoding encoding);
