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
  point and ever since, Xaw's Label has done the following in Initialize,
  regardless of encoding:

    if (lw->label.label == NULL) 
        lw->label.label = XtNewString(lw->core.name);
    else {
        lw->label.label = XtNewString(lw->label.label);
    }

  If the character mapping is Unicode or any other that preserves the ASCII
  code points, an ASCII character c will be represented in Char2b by a pair
  of bytes {0, c}.  XtNewString will truncate the string at the 0, and any
  code that subsequently tries to read the string as Char2b will do an out-
  of-bounds read.

  Conclusion:  The Char2b encoding in Xaw was tested only with CJK characters
  in a non-Unicode, double-byte character set.  This situation has been
  resolved in Xaw3dXft for the label resource.  The core name, however, is
  handled by Xt as a regular C string, so it cannot be made safe for Char2b.
*/

typedef enum {
  XawTextEncoding8bit   = 0, // char [α]
  XawTextEncodingChar2b = 1, // XChar2b [α]
  XawTextEncodingUTF8   = 2, // char, char8_t, FcChar8, UTF-8
  XawTextEncodingUCS2   = 3, // char16_t, FcChar16, UCS-2 [β]
  XawTextEncodingUTF32  = 4, // char32_t, FcChar32, UTF-32, UCS-4
  XawTextEncodingMb     = 5, // char, narrow multibyte, locale's codeset [γ]
  XawTextEncodingWc     = 6  // wchar_t, wide string [δ]
} XawTextEncoding;

/*

Notes:

[α] 8bit and Char2b are the encodings supported by XDrawString and
XDrawString16 respectively.  Ultimately, the interpretation of 8bit and
Char2b strings is determined by the font that is supplied in the graphics
context (GC) that is used to render them.  That font can be changed at the
discretion of the application; thus, 8bit and Char2b strings by themselves
have no interpretation.  If Xaw3dXft needs to translate an 8bit or Char2b
string to or from a different encoding, it assumes a Unicode mapping.  This
results in an interpretation of ISO 8859-1 for 8bit and big-endian UCS-2 [β]
for Char2b.  The fonts for which this assumption is valid are those whose X
Logical Font Description (XLFD) font name ends with -iso10646-1 (for the
entire Unicode Basic Multilingual Plane) or -iso8859-1 (for Latin-1
characters only).

[β] UCS-2 is the Unicode Basic Multilingual Plane represented with 16-bit
values.  Microsoft encodes higher Unicode code points using surrogate pairs,
which are defined in UTF-16 but not in UCS-2.  Surrogate pairs are not
supported by Xaw3dXft.

[γ] The interpretation of narrow multibyte strings is determined by the
codeset from the currently active C locale; e.g., UTF-8 from en_US.UTF-8,
ISO 8859-7 from el_GR.ISO8859-7, or ASCII from the default "C" locale.

[δ] As of the C23 standard, everything about wide strings remains
implementation-defined and there is no reliable translation between wchar_t
and UTF-anything.  Xaw3dXft assumes that wchar_t is UTF-32, which is true
under normal circumstances for all mainstream Unix/Linux distributions but
not for strong proprietary flavors.

*/

#define XtNencoding "encoding"
#define XtCEncoding "Encoding"
