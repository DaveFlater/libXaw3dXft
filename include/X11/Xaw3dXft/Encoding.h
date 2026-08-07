/*
 * Encoding.h
 * Values for the encoding resource
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#pragma once

/*
  2026-07-07 DWF

  When I arrived on the scene, I found these macros repeated in Label.h and
  Tip.h:

  #define XawTextEncoding8bit     0
  #define XawTextEncodingChar2b   1

  The Label widget acquired its encoding resource in X11R5's Xaw.  At that
  point and ever since, Label has done this in Initialize, regardless of
  encoding:

    if (lw->label.label == NULL) 
        lw->label.label = XtNewString(lw->core.name);
    else {
        lw->label.label = XtNewString(lw->label.label);
    }

  When applied to a Char2b-encoded string having any of the most common
  characters at its beginning, that results in the empty C string consisting
  of a single 0 byte.  Any code that subsequently tries to read it as Char2b
  will immediately do an out-of-bounds read.

  Conclusion:  The Char2b encoding *never worked* in Xaw.
*/

/*
  When international is true, you are passing "multibyte character strings"
  as expected by XmbDrawString.  Their interpretation for display is
  determined by the codeset that is set in the locale (e.g., en_US.UTF-8 or
  el_GR.ISO8859-7), not by the encoding resource.  Xaw3dXft knows nothing
  about that codeset but still needs to find newlines and null characters to
  do line breaking.  It uses the encoding resource to determine how to do
  that.  For any codeset that preserves single-byte ASCII, which includes all
  parts of ISO-8859, the right answer is XawTextEncoding8bit.
*/

/*
  UCS-2 is the Basic Multilingual Plane as 16-bit values.  UTF-16 extends the
  character repertoire beyond the Basic Multilingual Plane using pairs of
  "high surrogates" and "low surrogates" which are disallowed by UCS-2.
  libXft doesn't bother to decode those surrogate pairs and neither does
  libXaw3dXft, so the encoding is just UCS-2.
*/

typedef enum {
  XawTextEncoding8bit   = 0, // char, ISO-8859-1, Xlib STRING
  XawTextEncodingChar2b = 1, // XChar2b, UCS-2BE (UCS-2 big-endian)
  XawTextEncodingUTF8   = 2, // char, char8_t, FcChar8, UTF-8
  XawTextEncodingUCS2   = 3, // char16_t, FcChar16, UCS-2 (not UTF-16)
  XawTextEncodingUTF32  = 4  // char32_t, FcChar32, UTF-32, UCS-4
} XawTextEncoding;

#define XtNencoding "encoding"
#define XtCEncoding "Encoding"
