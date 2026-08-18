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
  2026-08

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

  When applied to a Char2b-encoded string having any single-byte encodable
  character at its beginning (meaning, typically, the ASCII characters), that
  results in the empty C string consisting of a single 0 byte.  Any code that
  subsequently tries to read it as Char2b will immediately do an
  out-of-bounds read.

  Conclusion:  The Char2b encoding in Xaw was tested only with double-byte
  characters in a non-Unicode, double-byte character set.  Otherwise, it
  never worked.
*/

typedef enum {
  XawTextEncoding8bit   = 0, // char, ISO-8859-1, Xlib STRING
  XawTextEncodingChar2b = 1, // XChar2b, UCS-2 big-endian [αβ]
  XawTextEncodingUTF8   = 2, // char, char8_t, FcChar8, UTF-8
  XawTextEncodingUCS2   = 3, // char16_t, FcChar16, UCS-2 [α]
  XawTextEncodingUTF32  = 4, // char32_t, FcChar32, UTF-32, UCS-4
  XawTextEncodingMb     = 5, // char, narrow multibyte, locale's codeset [γ]
  XawTextEncodingWc     = 6  // wchar_t, wide string [δ]
} XawTextEncoding;

/*

Notes:

[α] UCS-2 is the Unicode Basic Multilingual Plane as 16-bit values.
Microsoft encodes higher Unicode code points using surrogate pairs, which are
defined in UTF-16 but not in UCS-2.  Surrogate pairs are not supported by
Xaw3dXft.

[β] In the core X11 fonts system, the code points for XChar2b are determined
by the encoding of the *font* and the corresponding .enc file.  The
assumption that XChar2b values are UCS-2 fails if the font uses a
non-Unicode, double-byte character set like JIS X 0208, KS C 5601, or GB
2312.  If such a font is used, Xaw3dXft may translate strings incorrectly.
https://xorg.freedesktop.org/archive/X11R7.7/doc/xorg-docs/fonts/fonts.html#The_fontenc_layer

[γ] The interpretation of narrow multibyte strings is determined by the
codeset from the currently active C locale; e.g., UTF-8 from en_US.UTF-8, ISO
8859-7 from el_GR.ISO8859-7, or ASCII from the default "C" locale.
https://github.com/DaveFlater/libXaw3dXft#locales

[δ] As of C23, everything about wide strings remains implementation-defined,
and there is no reliable translation between wchar_t and UTF-anything.  A
remedy is on track for C29 but is not yet implemented.  As a stopgap,
Xaw3dXft relies on the Unix-centric assumption that wchar_t is UTF-32.
https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3366.htm
https://en.cppreference.com/c/header/stdmchar

*/

#define XtNencoding "encoding"
#define XtCEncoding "Encoding"
