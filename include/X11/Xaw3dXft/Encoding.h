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

typedef enum {
  XawTextEncoding8bit   = 0, // Default ISO-8859-1
  XawTextEncodingChar2b = 1, // XChar2b (big-endian UCS-2)
  XawTextEncodingUTF8   = 2, // UTF-8
  XawTextEncoding16bit  = 3  // FcChar16 (UCS-2 in machine byte order)
} XawTextEncoding;

#define XtNencoding "encoding"
#define XtCEncoding "Encoding"

#define some16(e) (e == XawTextEncodingChar2b || e == XawTextEncoding16bit)
#define nlsize(e) (some16(e) ? 2 : 1)
