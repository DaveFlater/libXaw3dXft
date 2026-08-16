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
  Violating these rules invokes undefined behavior.
*/


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
Cardinal Xaw3dXftAnyStrlen (XawTextEncoding encoding, const void *text);


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
extern wchar_t *Xaw3dXftAnyToWc (XawTextEncoding encoding, const void *text,
				 Cardinal *num_bytes);
