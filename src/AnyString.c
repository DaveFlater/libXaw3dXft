/*
 * AnyString.c
 *
 * Widget-agnostic functions for dealing with text.  These functions are in
 * the global namespace, but they are not part of the public API.
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <uchar.h>
#include <wchar.h>
#include <X11/Xaw3dXft/AnyStringP.h>

// For the RULEs, see AnyStringP.h.

// Autoconf test AC_C_BIGENDIAN defines or undefines WORDS_BIGENDIAN in
// config.h
#ifdef WORDS_BIGENDIAN
#define isBigEndian 1
#else
#define isBigEndian 0
#endif

/*
  Commence endless screaming.

  Both Mb and Wc
  --------------

  In general, Mb/Wc codepoints are implementation-defined except that we seem
  to know that 0 means NUL.
  https://en.cppreference.com/c/string/multibyte
  "A null-terminated multibyte string (NTMBS), or "multibyte string", is a
  sequence of nonzero bytes followed by a byte with value zero (the
  terminating null character)."

  Mb
  --

  Mb supports shift states.
  https://en.cppreference.com/c/string/multibyte
  "In some multibyte encodings, any given multibyte character sequence may
  represent different characters depending on the previous byte sequences,
  known as "shift sequences".  Such encodings are known as state-dependent:
  knowledge of the current shift state is required to interpret each
  character."

  Wc
  --

  wchar_t was defined by the C language standard to be "an integral type
  whose range of values can represent distinct codes for all members of the
  largest extended character set specified among the supported locales" (ISO
  9899:1990 §4.1.5).  Microsoft chose to make wchar_t 16 bits, which is no
  longer sufficient to satisfy its original design intent.  Instead of
  correcting that mistake, the standards bent around it:  UTF-16 comes with
  surrogate pairs, the range of valid Unicode code points is limited to what
  UTF-16 can represent, and you can't assume parity between wc and
  characters.

  The reality is we have a schism in the installed base.  Unix Wc is 32 bits
  and is a fixed-length encoding.  Windows Wc is 16 bits and is a variable-
  length encoding.

  Char2b
  ------

  The code points for Char2b are determined by the encoding of the *font* and
  the corresponding .enc file:
  https://xorg.freedesktop.org/archive/X11R7.7/doc/xorg-docs/fonts/fonts.html#The_fontenc_layer

  The XChar2b data type appears to have been introduced firstly to support
  non-Unicode, double-byte character sets like JIS X 0208, KS C 5601, and GB
  2312.  But now, the fonts using those character sets are the exceptions
  that break the general rule that Char2b is UCS-2 big-endian.

  Xlib
  ----

  The Xwc and Xutf8 font set functions are all implemented using a
  translation to Mb (see libx11/modules/om/generic/omDefault.c).  They fail
  if the locale's codeset doesn't support the Unicode character repertoire
  and are useless for circumventing the locale dependency.  The Xutf8
  functions, furthermore, bail out early on some valid UTF-8 strings,
  irrespective of whether they would have translated successfully to Mb.  You
  get more text out if you convert UTF-8 to Wc yourself and call the Xwc
  function instead.

  Xutf8DrawString failing on valid strings scared me away from using any Xlib
  functions for encoding conversion.

  Xaw (as incoming)
  -----------------

  Xaw code assumes that the codepoints of LF, HT, and SPACE are the same as
  they are in ASCII (there is lots of _Xaw_atowc(XawLF), etc., where
  _Xaw_atowc just does mbtowc on the ASCII char).  Similarly, it assumes that
  the blocks of non-printing control characters are in the same places.  Most
  codesets do in fact preserve ASCII, but this isn't guaranteed; e.g.,
  En_US.IBM-1047 is EBCDIC.
  https://www.ibm.com/docs/en/zos/3.2.0?topic=functions-setlocale-set-locale

  Xaw implemented _XawTextWCToMB and _XawTextMBToWC using the esoteric Xlib
  functions XwcTextListToTextProperty, XmbTextListToTextProperty, and
  XwcTextPropertyToTextList.  These conversions can be done more obviously
  using C library functions.  See also the utility functions in Xlib i18n
  https://xorg.freedesktop.org/archive/X11R7.7/doc/libX11/i18n/framework/framework.html#Utility_Functions
  libx11/src/xlibi18n has _Xmbstoutf8 though it is not documented.

  C library
  ---------

  As of C23, there is no standard, direct conversion between Wc and UTF-
  anything.  You have to convert Wc to Mb first, which causes you to lose any
  characters that don't exist in the locale's codeset (which might be ASCII).

  Coming in C29:
  https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3366.htm
  https://en.cppreference.com/c/header/stdmchar
  Not yet supported in GCC as of 2026-08-08:
  https://gcc.gnu.org/projects/c-status.html

  Consequences
  ------------

  The only practical choice is to rely on non-portable and unsafe
  assumptions:

  1.  ASCII codepoints will be preserved by all encodings (as was already
  assumed by the code incoming).

  2.  The implementation-defined wide character encoding will be UTF-32.
  Identifying UTF-32 and Wc as separate encodings is probably wasteful, but
  it keeps a path open to utilize stdmchar when it is implemented.

  3.  The font-dependent Char2b encoding will be UCS-2 big-endian.  This
  enables us to cover the Basic Multilingual Plane using XDrawString16.
  However, it means that the original use of Char2b for non-Unicode, double-
  byte character sets will work only if translations are completely avoided.
*/

static_assert(sizeof(XChar2b) == sizeof(char16_t));
static_assert(sizeof(wchar_t) == sizeof(char32_t));

// ---- Basics ----

static Cardinal checklen (uint64_t len) {
  if (len > Xaw3dXftAnyStringLengthLimit) // RULE 4
    XtError("libXaw3dXft: character string too long");
  return (Cardinal)len;
}

// Newline (LF) or NUL size
static Cardinal nlsize (XawTextEncoding encoding) {
  switch (encoding) {
  case XawTextEncodingChar2b:
  case XawTextEncodingUCS2:
    return 2;
  case XawTextEncodingUTF32:
    return 4;
  case XawTextEncodingWc:
    return sizeof(wchar_t);
  default: // 8bit, UTF8, Mb (could do size = c16rtomb(s, u'\n', &state))
    return 1;
  }
}

// Return number of bytes in any string, not counting null terminator.
// GNU extension size_t malloc_usable_size(void *ptr) could be used to harden
// this against unterminated strings.
Cardinal Xaw3dXftAnyStrlen (XawTextEncoding encoding, const void *text) {
  uint64_t biglen;
  assert(text);
  switch (encoding) {
  case XawTextEncodingChar2b:
  case XawTextEncodingUCS2:
    {
      const uint16_t *s = text;
      while (*s) ++s;
      biglen = (uint64_t)((uint8_t *)s - (uint8_t *)text);
    }
    break;
  case XawTextEncodingUTF32:
    {
      const uint32_t *s = text;
      while (*s) ++s;
      biglen = (uint64_t)((uint8_t *)s - (uint8_t *)text);
    }
    break;
  case XawTextEncodingWc:
    biglen = wcslen(text) * sizeof(wchar_t);
    break;
  default: // 8bit, UTF8, Mb
    biglen = strlen(text);
  }
  return checklen(biglen); // RULE 4
}

// Generalized strchr(s, '\n')
// The returned pointer points to the first or only byte of the newline.
static const void *nextnl (XawTextEncoding encoding, const void *text) {
  assert(text);
  switch (encoding) {
  case XawTextEncoding8bit:
  case XawTextEncodingUTF8:
  case XawTextEncodingMb:
    return strchr(text, '\n');
  case XawTextEncodingWc:
    return wcschr(text, L'\n');
  case XawTextEncodingChar2b:
    {
      const XChar2b *s = text;
      while (s->byte1 != 0 || s->byte2 != 0) {
	if (s->byte1 == 0 && s->byte2 == '\n')
	  return s;
	++s;
      }
    }
    break;
  case XawTextEncodingUCS2:
    {
      const uint16_t *s = text;
      while (*s) {
	if (*s == '\n')
	  return s;
	++s;
      }
    }
    break;
  case XawTextEncodingUTF32:
    {
      const uint32_t *s = text;
      while (*s) {
	if (*s == '\n')
	  return s;
	++s;
      }
    }
  }
  return NULL;
}

// ---- Font metrics ----

/*
  The metrics provided by the font systems are confusing and offer multiple
  choices.  Route everything through here so that the choices can be changed
  easily.

  Pixel coordinates being quantized, it isn't clear how the baseline is
  counted.  Is the baseline a particular row of pixels?  If so, is it
  included in ascent or descent, or do we have to add one when calculating
  the height?  If the baseline is between rows of pixels, which metrics are
  being rounded by half a pixel?
*/

void Xaw3dXftAnyFontMetrics (Display *display, XFontStruct *font,
XFontSet fontSet, XftFont *xftFont, Boolean international, Dimension *height,
Dimension *ascent, Dimension *width) {
  if (xftFont) {
    /*
      It is unspecified whether the ascent, descent, and height values in
      struct XftFont are logical or max bounds.  Height is sometimes greater
      than and sometimes less than the sum of ascent and descent.

      These values trace back to the ascender, descender, and height fields
      of struct FT_FaceRec.
      https://freetype.org/freetype2/docs/reference/ft2-face_creation.html#ft_facerec
      "The typographic ascender of the face
      "The typographic descender of the face"
      "This value is the vertical distance between two consecutive baselines
      ... If you want the global glyph height, use ascender − descender."

      xftfreetype.c sets XftFont height based on FT ascender − descender if
      fi->minspace is true and FT height otherwise.  Minspace is a property
      that can be set in the font name:  minspace=true.
    */
    if (ascent) *ascent = xftFont->ascent;
    // ???  Choose your poison
    if (height) *height = xftFont->height;
    // if (height) *height = xftFont->ascent + xftFont->descent;
  } else if (international) {
    /*
      struct XFontSetExtents has max_ink_extent and max_logical_extent.  Each
      of these is an XRectangle, so you don't get ascent and descent, you get
      height and y.  The ascent is stored in y as a negative number.  The
      extents change when you change locales and are ill-behaved when there
      are multiple fonts in the font set.
    */
    assert(fontSet);
    XFontSetExtents *fontSetExtents = XExtentsOfFontSet(fontSet);
    // Choose your poison, max_logical_extent or max_ink_extent
    const XRectangle rectangle = fontSetExtents->max_ink_extent;
    if (ascent) *ascent = -rectangle.y;
    if (height) *height = rectangle.height;
  } else {
    /*
      struct XFontStruct has ascent and descent described as the logical
      extent above/below the baseline for spacing.  It also has XCharStruct
      min_bounds and max_bounds giving minimum/maximum ascent and descent
      over all existing characters.  max_bounds.ascent and descent are
      greater than the logical ones.  The logical ones allow some
      ascenders/descenders to go outside the box, which frequently results in
      them being chopped off.  The extents change when you change the
      CHARSET_REGISTRY and CHARSET_ENCODING fields of the XLFD.
    */
    assert(font);
    if (ascent) *ascent = font->max_bounds.ascent;
    if (height) *height = font->max_bounds.ascent + font->max_bounds.descent;
  }
  // Xaw uses the FIGURE_WIDTH property of core fonts when it is present and
  // the width of '$' as a fallback.
  if (width)
    Xaw3dXftSizeAnyStringN(display, font, fontSet, xftFont, international,
      XawTextEncoding8bit, "$", 1, width, NULL);
}

// ---- Converters ----

// Convert between the two 16-bit representations.  Result will be
// null-terminated.  Caller is responsible for freeing the returned string.
static void *convert16 (const void *text, Cardinal num_bytes) {
  assert(text);
  assert(num_bytes % 2 == 0); // RULE 3
  const uint16_t *s = text;
  uint16_t *t = malloc(checklen(num_bytes)+2); // RULE 4
  assert(t);
  const Cardinal num_chars = num_bytes/2;
  for (Cardinal i=0; i<num_chars; ++i) {
    assert(s[i]); // RULE 2
    #if isBigEndian
      t[i] = s[i];
    #else
      t[i] = bswap_16(s[i]);
    #endif
  }
  t[num_chars] = 0;
  return t;
}

// Convert IN PLACE between the two 16-bit representations.
static void inplacecvt16 (void *text, Cardinal num_bytes) {
  assert(text);
  assert(num_bytes % 2 == 0); // RULE 3
#if !isBigEndian
  uint16_t *s = text;
  const Cardinal num_chars = checklen(num_bytes)/2; // RULE 4
  for (Cardinal i=0; i<num_chars; ++i) {
    assert(s[i]); // RULE 2
    s[i] = bswap_16(s[i]);
  }
#endif
}

// Reduce UTF-8 to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *UTF8toChar2b (const char *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const uint8_t *textp = (uint8_t *)text;
  const uint16_t bogusChar = '?';
  const Cardinal nb = checklen(*num_bytes); // RULE 4
  uint16_t *new = calloc(nb+1, sizeof(uint16_t));
  assert(new);
  uint8_t *oldp = (uint8_t *)text;
  uint16_t *newp = new;
  while (oldp - textp < nb && *oldp != 0) {
    if ((*oldp & 0xc0) == 0x80)
      oldp++; // desynced; skip forward
    else if (!(*oldp & 0x80))
      *newp++ = *oldp++;
    else if (oldp + 1 - textp < nb &&
	     (*oldp & 0xe0) == 0xc0 &&
	     (*(oldp+1) & 0xc0) == 0x80) {
      *newp = (uint16_t)(*oldp & 0x1f) << 6 | (*(oldp+1) & 0x3f);
      if (*newp < 0x80) *newp = bogusChar; // overlong encoding
      newp++;
      oldp += 2;
    } else if (oldp + 2 - textp < nb &&
	       (*oldp & 0xf0) == 0xe0 &&
	       (*(oldp+1) & 0xc0) == 0x80 &&
	       (*(oldp+2) & 0xc0) == 0x80) {
      *newp = (uint16_t)(*oldp & 0x0f) << 12 |
	      (uint16_t)(*(oldp+1) & 0x3f) << 6 |
	      (*(oldp+2) & 0x3f);
      if (*newp < 0x800 ||                    // overlong encoding
	  *newp >= 0xd800 && *newp <= 0xdfff) // illegal surrogate
	*newp = bogusChar;
      newp++;
      oldp += 3;
    } else {
      // Broken character, character cut in half by num_bytes, or out of
      // 16-bit range
      *newp++ = bogusChar;
      oldp++;
    }
  }
  *newp = 0;
  *num_bytes = checklen((newp - new) * 2); // RULE 4
  inplacecvt16(new, *num_bytes);
  return (XChar2b *)new;
}

// Convert UTF-8 to UTF-32.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static char32_t *UTF8toUTF32 (const char *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const uint8_t *textp = (uint8_t *)text;
  const uint32_t bogusChar = '?';
  const Cardinal nb = checklen(*num_bytes); // RULE 4
  uint32_t *new = calloc(nb+1, sizeof(uint32_t));
  assert(new);
  uint8_t *oldp = (uint8_t *)text;
  uint32_t *newp = new;
  while (oldp - textp < nb && *oldp != 0) {
    if ((*oldp & 0xc0) == 0x80)
      oldp++; // desynced; skip forward
    else if (!(*oldp & 0x80))
      *newp++ = *oldp++;
    else if (oldp + 1 - textp < nb &&
	     (*oldp & 0xe0) == 0xc0 &&
	     (*(oldp+1) & 0xc0) == 0x80) {
      *newp = (uint32_t)(*oldp & 0x1f) << 6 | (*(oldp+1) & 0x3f);
      if (*newp < 0x80) *newp = bogusChar; // overlong encoding
      newp++;
      oldp += 2;
    } else if (oldp + 2 - textp < nb &&
	       (*oldp & 0xf0) == 0xe0 &&
	       (*(oldp+1) & 0xc0) == 0x80 &&
	       (*(oldp+2) & 0xc0) == 0x80) {
      *newp = (uint32_t)(*oldp & 0x0f) << 12 |
	      (uint32_t)(*(oldp+1) & 0x3f) << 6 |
	      (*(oldp+2) & 0x3f);
      if (*newp < 0x800 ||                    // overlong encoding
	  *newp >= 0xd800 && *newp <= 0xdfff) // illegal surrogate
	*newp = bogusChar;
      newp++;
      oldp += 3;
    } else if (oldp + 3 - textp < nb &&
	       (*oldp & 0xf8) == 0xf0 &&
	       (*(oldp+1) & 0xc0) == 0x80 &&
	       (*(oldp+2) & 0xc0) == 0x80 &&
	       (*(oldp+3) & 0xc0) == 0x80) {
      *newp = (uint32_t)(*oldp & 0x07) << 18 |
	      (uint32_t)(*(oldp+1) & 0x3f) << 12 |
	      (uint32_t)(*(oldp+2) & 0x3f) << 6 |
	      (*(oldp+3) & 0x3f);
      if (*newp <  0x10000 ||  // overlong encoding
	  *newp > 0x10ffff)    // past the end of Unicode
	*newp = bogusChar;
      newp++;
      oldp += 4;
    } else {
      // Broken character or character cut in half by num_bytes
      *newp++ = bogusChar;
      oldp++;
    }
  }
  *newp = 0;
  *num_bytes = checklen((newp - new) * 4); // RULE 4
  return (char32_t *)new;
}

// Reduce UTF-32 to UCS-2.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static char16_t *UTF32toUCS2 (const char32_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const uint32_t *textp = (uint32_t *)text;
  const uint16_t bogusChar = '?';
  assert(*num_bytes % 4 == 0); // RULE 3
  const Cardinal l = checklen(*num_bytes) / 4; // RULE 4
  uint16_t *new = calloc(l+1, sizeof(uint16_t));
  assert(new);
  for (Cardinal i=0; i<l; ++i)
    new[i] = (textp[i] < 0x10000 ? textp[i] : bogusChar);
  new[l] = 0;
  *num_bytes = l*2;
  return (char16_t *)new;
}

// Reduce UTF-32 to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *UTF32toChar2b (const char32_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  XChar2b *new = (XChar2b *)UTF32toUCS2(text, num_bytes);
  inplacecvt16(new, *num_bytes);
  return new;
}

// Convert Mb to UTF-32.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// Based on https://en.cppreference.com/c/string/multibyte/mbrtoc32
static char32_t *MbtoUTF32 (const char *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const char32_t bogusChar = '?';
  char32_t *new = calloc(checklen(*num_bytes) + 1, sizeof(char32_t)); // RULE 4
  assert(new);
  char32_t *p_out = new;
  const char *p_in = text,
              *end = text + *num_bytes;
  mbstate_t state = {0};
  Boolean done = False;
  while (!done && p_in < end) {
    size_t rc = mbrtoc32(p_out, p_in, end - p_in, &state);
    switch (rc) {
    case 0:
      // The character converted and stored was the NUL character.
      done = True;
      break;
    case -1:
      // Encoding error, nothing written to p_out, state is undefined.  Don't
      // know whether this is recoverable, but try.
      ++p_in;
      *p_out++ = bogusChar;
      state = (mbstate_t){0};
      break;
    case -2:
      // The next n bytes constitute an incomplete, but so far valid,
      // multibyte character.  Nothing written to p_out.  This means that we
      // have one broken character but we are done.  Rule 3 not enforced.
      *p_out++ = bogusChar;
      done = True;
      break;
    case -3:
      // The next char32_t from a multi-char32_t character has now been
      // written.  No bytes are processed from the input in this case.
      // "e.g. Big5-HKSCS needing to output 2 different UTF-32 code points
      // for some of its input characters (4 specific input sequences of them
      // result in two UTF-32 code point outputs, to be precise)" — N3366
      // GNU libc never returns -3.
      ++p_out;
      break;
    default:
      // It read rc bytes and stored one char32.
      assert(rc <= MB_LEN_MAX);
      p_in += rc;
      ++p_out;
    }
  }
  *p_out = 0;
  *num_bytes = checklen((p_out - new) * 4); // RULE 4
  return new;
}

// Convert UTF-8 to Wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *UTF8toWc (const char *text, Cardinal *num_bytes) {
  return (wchar_t *)UTF8toUTF32(text, num_bytes);
}

/*
  Convert Mb to Wc.  num_bytes is updated as applicable.  Caller is
  responsible for freeing the returned string.

  This function could be implemented with mbrtowc to remove one use of the
  unsafe assumption, but it's pointless.

  In theory, C99's mbrtowc is the safe conversion from Mb to Wc.  But,
  mbrtowc lacks the -3 return of mbrtoc32.

  glibc implements mbrtoc32 by calling mbrtowc.  It never returns -3.

  Collision with libc's mbtowc on the naming of this function is why I
  started capitalizing Mb and Wc.
*/
static wchar_t *MbtoWc (const char *text, Cardinal *num_bytes) {
  return (wchar_t *)MbtoUTF32(text, num_bytes);
}

// Convert 8bit to Wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *_8bittoWc (const char *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const Cardinal l = checklen(*num_bytes); // RULE 4
  wchar_t *new = calloc(l+1, sizeof(wchar_t));
  assert(new);
  uint8_t *src = (uint8_t *)text;
  for (Cardinal i=0; i<l; ++i) {
    assert(src[i]); // RULE 2
    new[i] = src[i];
  }
  new[l] = 0;
  *num_bytes = checklen(l * sizeof(wchar_t)); // RULE 4
  return new;
}

// Convert UCS-2 to Wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *UCS2toWc (const char16_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  assert(*num_bytes % 2 == 0); // RULE 3
  const Cardinal l = checklen(*num_bytes) / 2; // RULE 4
  wchar_t *new = calloc(l+1, sizeof(wchar_t));
  assert(new);
  uint16_t *src = (uint16_t *)text;
  for (Cardinal i=0; i<l; ++i) {
    assert(src[i]); // RULE 2
    new[i] = src[i];
  }
  new[l] = 0;
  *num_bytes = checklen(l * sizeof(wchar_t)); // RULE 4
  return new;
}

// Convert Char2b to Wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *Char2btoWc (const XChar2b *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  void *ucs2 = convert16(text, *num_bytes);
  wchar_t *ret = UCS2toWc(ucs2, num_bytes);
  free(ucs2);
  return ret;
}

// Convert Mb to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// Note:  mbrtoc16 implements UTF-16 with surrogate pairs, which we don't
// want.  UTF32toChar2b will instead put '?' for characters that don't fit.
static XChar2b *MbtoChar2b (const char *text, Cardinal *num_bytes) {
  char32_t *utf32 = MbtoUTF32(text, num_bytes);
  XChar2b *c2b = UTF32toChar2b(utf32, num_bytes);
  free(utf32);
  return c2b;
}

// Convert Wc to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *WctoChar2b (const wchar_t *text, Cardinal *num_bytes) {
  const char32_t *utf32 = (const char32_t *)text;
  XChar2b *c2b = UTF32toChar2b(utf32, num_bytes);
  return c2b;
}

// The rest of these are only for Xaw3dXftWcToAny (i.e. only to get strings
// back out of MultiSrc).

// Convert Wc to 8bit.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// This function is reachable as follows:
// - Create AsciiText with encoding not 8bit so you get a MultiSrc
// - Use SetValues to change the encoding to 8bit
// - Save to file or string:  MultiSrc exports to 8bit
// It'll probably also be used to reduce cut buffers to STRING encoding.
static char *Wcto8bit (const wchar_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const uint32_t *textp = (uint32_t *)text;
  const uint8_t bogusChar = '?';
  assert(*num_bytes % 4 == 0); // RULE 3
  const Cardinal l = checklen(*num_bytes) / 4; // RULE 4
  uint8_t *new = malloc(l+1);
  assert(new);
  for (Cardinal i=0; i<l; ++i)
    new[i] = (textp[i] < 0x100 ? textp[i] : bogusChar);
  new[l] = 0;
  *num_bytes = l;
  return (char *)new;
}

// Convert Wc to UCS-2.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static char16_t *WctoUCS2 (const wchar_t *text, Cardinal *num_bytes) {
  return UTF32toUCS2((const char32_t *)text, num_bytes);
}

// Convert Wc to Mb.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// wcsnrtombs does the whole string at once but makes it difficult to work
// around invalid characters.
static char *WctoMb (const wchar_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  assert(*num_bytes % sizeof(wchar_t) == 0); // RULE 3
  Cardinal num_wc = checklen(*num_bytes) / sizeof(wchar_t); // RULE 4
  // GNU libc is giving me MB_CUR_MAX = 6 for a UTF-8 locale (expected 4) and
  // MB_LEN_MAX = 16 (how?).
  //   MB_CUR_MAX = 6 → max 1.5 GB malloc
  //   MB_CUR_MAX = 16 → max 4 GB malloc
  // Alternatives:
  // 1. Limit malloc to 1 GB and enforce Rule 4 inside the loop.
  // 2. 2-pass (get the length before malloc and conversion).
  // 3. Start with a conservative guess and realloc if needed.
  char *new = malloc(num_wc * MB_CUR_MAX + 1);
  assert(new);
  char *s = new;
  mbstate_t state = {0};
  for (Cardinal i=0; i<num_wc; ++i) {
    size_t ret = wcrtomb(s, text[i], &state);
    if (ret == -1)
      *s++ = '?';
    else
      s += ret;
  }
  *s = 0;
  *num_bytes = checklen(s - new); // RULE 4
  return new;
}

// Convert Wc to UTF-8.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// Nowhere else do we require a conversion to UTF-8.
static char *WctoUTF8 (const wchar_t *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  assert(*num_bytes % sizeof(wchar_t) == 0); // RULE 3
  Cardinal num_wc = checklen(*num_bytes) / sizeof(wchar_t); // RULE 4
  uint8_t *new = malloc(num_wc * 4 + 1);
  assert(new);
  uint8_t *s = new;
  for (Cardinal i=0; i<num_wc; ++i) {
    const uint32_t c = text[i];
    switch (c) {
    case 0: // Rule 2 violation
      goto cmBHtZWn;
    case 1 ... 0x7f: // Single-byte ASCII character
      *s++ = c;
      break;
    case 0x80 ... 0x7ff: // Two-byte character
      *s++ = 0xc0 | (c & 0x7c0) >> 6;
      *s++ = 0x80 | c & 0x3f;
      break;
    case 0x800 ... 0xd7ff:  // Three-byte character range 1
    case 0xe000 ... 0xffff: // Three-byte character range 2
      *s++ = 0xe0 | (c & 0xf000) >> 12;
      *s++ = 0x80 | (c & 0xfc0) >> 6;
      *s++ = 0x80 | c & 0x3f;
      break;
    case 0x10000 ... 0x10ffff: // Four-byte character
      *s++ = 0xf0 | (c & 0x1c000) >> 18;
      *s++ = 0x80 | (c & 0x3f000) >> 12;
      *s++ = 0x80 | (c & 0xfc0) >> 6;
      *s++ = 0x80 | c & 0x3f;
      break;
    default: // Illegal surrogate or past the end of Unicode
      *s++ = '?';
    }
  }
  cmBHtZWn:
  *s = 0;
  *num_bytes = s - new;
  return (char *)new;
}

// ---- Main functions ----

// Rule 1 is specifically waived so that this can be used to terminate
// unterminated strings.
void *Xaw3dXftAnyStrdupN (XawTextEncoding encoding, const void *text,
			  Cardinal num_bytes) {
  assert(text);
  const Cardinal nbytes = num_bytes + nlsize(encoding);
  void *s = malloc(nbytes);
  assert(s);
  (void) memcpy(s, text, num_bytes); // Rules 2 and 3 not enforced
  (void) memset(s+num_bytes, 0, nbytes-num_bytes);
  return s;
}

void *Xaw3dXftAnyStrdup (XawTextEncoding encoding, const void *text) {
  return Xaw3dXftAnyStrdupN(encoding, text, Xaw3dXftAnyStrlen(encoding, text));
}

// For the drawOne*Line functions, yadj is the baseline for the DrawString
// functions, which is the top y plus the ascent of the font.

// Xaw3dXftDrawAnyString component for a single line with Xft font
static void drawOneXftLine (
  XftFont *xftFont,
  XftColor *fg,
  Position x, Position yadj,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes,
  XftDraw *xftDraw
) {
  if (num_bytes) {
    switch (encoding) {
    case XawTextEncoding8bit:
      XftDrawString8(xftDraw, fg, xftFont, x, yadj, text, num_bytes);
      break;
    case XawTextEncodingUTF8:
      XftDrawStringUtf8(xftDraw, fg, xftFont, x, yadj, text, num_bytes);
      break;
    case XawTextEncodingUCS2:
      XftDrawString16(xftDraw, fg, xftFont, x, yadj, text, num_bytes/2);
      break;
    case XawTextEncodingUTF32:
    case XawTextEncodingWc:
      XftDrawString32(xftDraw, fg, xftFont, x, yadj, text, num_bytes/4);
      break;
    case XawTextEncodingChar2b:
      {
	char16_t *cvt16 = convert16(text, num_bytes);
	XftDrawString16(xftDraw, fg, xftFont, x, yadj, cvt16, num_bytes/2);
	free(cvt16);
      }
      break;
    case XawTextEncodingMb:
      {
	char32_t *cvt32 = MbtoUTF32(text, &num_bytes);
	XftDrawString32(xftDraw, fg, xftFont, x, yadj, cvt32, num_bytes/4);
	free(cvt32);
      }
    }
  }
}

// Xaw3dXftDrawAnyString component for a single line with an XFontSet
static void drawOneXmbLine (
  Display *display, Window window,
  XFontSet fontSet,
  GC gc,
  Position x, Position yadj,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes
) {
  // X(mb,wc,utf8)DrawImageString will fill the background and apply a stipple.
  // X(mb,wc,utf8)DrawString will apply a stipple but won't fill the background.
  if (num_bytes) {
    wchar_t *cvtwc = NULL;
    switch (encoding) {
    case XawTextEncodingUTF32:
    case XawTextEncodingWc:
      XwcDrawString(display, window, fontSet, gc, x, yadj, text,
	num_bytes/sizeof(wchar_t));
      return;
    case XawTextEncodingMb:
      XmbDrawString(display, window, fontSet, gc, x, yadj, text, num_bytes);
      return;
    case XawTextEncodingUTF8:
      // Xlib's UTF-8 converter fails on valid strings.
      // Xutf8DrawString(display, window, fontSet, gc, x, yadj, text, num_bytes);
      cvtwc = UTF8toWc(text, &num_bytes);
      break;
    case XawTextEncoding8bit:
      cvtwc = _8bittoWc(text, &num_bytes);
      break;
    case XawTextEncodingChar2b:
      cvtwc = Char2btoWc(text, &num_bytes);
      break;
    case XawTextEncodingUCS2:
      cvtwc = UCS2toWc(text, &num_bytes);
    }
    XwcDrawString(display, window, fontSet, gc, x, yadj, cvtwc,
      num_bytes/sizeof(wchar_t));
    free(cvtwc);
  }
}

// Xaw3dXftDrawAnyString component for a single line with plain old X font
static void drawOneLine (
  Display *display, Window window,
  XFontStruct *font,
  GC gc,
  Position x, Position yadj,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes
) {
  // XDrawImageString[16] will fill the background but won't apply a stipple.
  // XDrawString[16] will apply a stipple but won't fill the background.
  if (num_bytes) {
    XChar2b *cvt16 = NULL;
    switch (encoding) {
    case XawTextEncoding8bit:
      XDrawString(display, window, gc, x, yadj, text, num_bytes);
      return;
    case XawTextEncodingChar2b:
      XDrawString16(display, window, gc, x, yadj, text, num_bytes/2);
      return;
    case XawTextEncodingUTF8:
      cvt16 = UTF8toChar2b(text, &num_bytes);
      break;
    case XawTextEncodingUCS2:
      cvt16 = convert16(text, num_bytes);
      break;
    case XawTextEncodingUTF32:
      cvt16 = UTF32toChar2b(text, &num_bytes);
      break;
    case XawTextEncodingWc:
      cvt16 = WctoChar2b(text, &num_bytes);
      break;
    case XawTextEncodingMb:
      cvt16 = MbtoChar2b(text, &num_bytes);
    }
    XDrawString16(display, window, gc, x, yadj, cvt16, num_bytes/2);
    free(cvt16);
  }
}

void Xaw3dXftDrawAnyStringN (
  Display *display, Visual *visual, Colormap cmap, Window window,
  XFontStruct *font, XFontSet fontSet, XftFont *xftFont,
  Boolean international,
  GC text_gc, XftColor *fg,
  Position x, Position y,
  XRectangle *clip,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes
) {
  if (num_bytes == 0) return;
  assert(xftFont && fg || text_gc);
  (void) checklen(num_bytes); // RULE 4

  XftDraw *xftDraw = NULL;
  Dimension fontHeight, fontAscent;
  Xaw3dXftAnyFontMetrics(display, font, fontSet, xftFont, international,
    &fontHeight, &fontAscent, NULL);

  // The line-breaking logic is hairy, so do that just once and switch the
  // font systems three times instead (pre-loop, in-loop, post-loop).

  // Pre-loop switch
  if (xftFont) {
    xftDraw = XftDrawCreate(display, window, visual, cmap);
    assert(xftDraw);
    if (clip && !XftDrawSetClipRectangles(xftDraw, 0, 0, clip, 1))
      XtWarning("libXaw3dXft: XftDrawSetClipRectangles failed");
  } else if (clip &&
      !XSetClipRectangles(display, text_gc, 0, 0, clip, 1, YXBanded))
    XtWarning("libXaw3dXft: XSetClipRectangles failed");

  // Begin line-breaking loop
  const void *nl = nextnl(encoding, text);
  Position yadj = y + fontAscent;
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont)
      drawOneXftLine(xftFont, fg, x, yadj, encoding, text, line_bytes, xftDraw);
    else if (international)
      drawOneXmbLine(display, window, fontSet, text_gc, x, yadj, encoding, text,
	             line_bytes);
    else
      drawOneLine(display, window, font, text_gc, x, yadj, encoding, text,
		  line_bytes);
    yadj += fontHeight;

    // End line-breaking loop
    text = nl + nlsize(encoding);
    nl = nextnl(encoding, text);
    const Cardinal skip = line_bytes + nlsize(encoding);
    if (num_bytes >= skip)
      num_bytes -= skip;
    else
      num_bytes = 0;
  }

  // Post-loop switch
  if (xftFont) {
    drawOneXftLine(xftFont, fg, x, yadj, encoding, text, num_bytes, xftDraw);
    XftDrawDestroy(xftDraw);
  } else if (international)
    drawOneXmbLine(display, window, fontSet, text_gc, x, yadj, encoding, text,
                   num_bytes);
  else
    drawOneLine(display, window, font, text_gc, x, yadj, encoding, text,
                num_bytes);
  if (clip && !xftFont && !XSetClipMask(display, text_gc, None))
    XtWarning("libXaw3dXft: XSetClipMask failed");
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftDrawAnyString (Display *display, Visual *visual, Colormap cmap,
Window window, XFontStruct *font, XFontSet fontSet, XftFont *xftFont,
Boolean international, GC text_gc, XftColor *fg, Position x, Position y,
XRectangle *clip, XawTextEncoding encoding, const void *text) {
  Xaw3dXftDrawAnyStringN(display, visual, cmap, window, font, fontSet,
    xftFont, international, text_gc, fg, x, y, clip, encoding, text,
    Xaw3dXftAnyStrlen(encoding, text));
}

// Xaw3dXftSizeAnyString component for a single line with Xft font
static Dimension sizeOneXftLine (Display *display, XftFont *xftFont,
  XawTextEncoding encoding, const void *text, Cardinal num_bytes) {
  if (num_bytes == 0) return 0;
  XGlyphInfo extents;
  switch (encoding) {
  case XawTextEncoding8bit:
    XftTextExtents8(display, xftFont, text, num_bytes, &extents);
    break;
  case XawTextEncodingUTF8:
    XftTextExtentsUtf8(display, xftFont, text, num_bytes, &extents);
    break;
  case XawTextEncodingUCS2:
    XftTextExtents16(display, xftFont, text, num_bytes/2, &extents);
    break;
  case XawTextEncodingUTF32:
  case XawTextEncodingWc:
    XftTextExtents32(display, xftFont, text, num_bytes/4, &extents);
    break;
  case XawTextEncodingChar2b:
    {
      char16_t *cvt16 = convert16(text, num_bytes);
      XftTextExtents16(display, xftFont, cvt16, num_bytes/2, &extents);
      free(cvt16);
    }
    break;
  case XawTextEncodingMb:
    {
      char32_t *cvt32 = MbtoUTF32(text, &num_bytes);
      XftTextExtents32(display, xftFont, cvt32, num_bytes/4, &extents);
      free(cvt32);
    }
  }
  return extents.xOff;
}

// Xaw3dXftSizeAnyString component for a single line with an XFontSet
static Dimension sizeOneXmbLine (XFontSet fontSet, XawTextEncoding encoding,
const void *text, Cardinal num_bytes) {
  if (num_bytes == 0) return 0;
  wchar_t *cvtwc = NULL;
  switch (encoding) {
  case XawTextEncodingUTF32:
  case XawTextEncodingWc:
    return XwcTextEscapement(fontSet, text, num_bytes/sizeof(wchar_t));
  case XawTextEncodingMb:
    return XmbTextEscapement(fontSet, text, num_bytes);
  case XawTextEncodingUTF8:
    // Xlib's UTF-8 converter fails on valid strings.
    // return Xutf8TextEscapement(fontSet, text, num_bytes);
    cvtwc = UTF8toWc(text, &num_bytes);
    break;
  case XawTextEncoding8bit:
    cvtwc = _8bittoWc(text, &num_bytes);
    break;
  case XawTextEncodingChar2b:
    cvtwc = Char2btoWc(text, &num_bytes);
    break;
  case XawTextEncodingUCS2:
    cvtwc = UCS2toWc(text, &num_bytes);
  }
  const Dimension width = XwcTextEscapement(fontSet, cvtwc,
    num_bytes/sizeof(wchar_t));
  free(cvtwc);
  return width;
}

// Xaw3dXftSizeAnyString component for a single line with plain old X font
static Dimension sizeOneLine (XFontStruct *font, XawTextEncoding encoding,
const void *text, Cardinal num_bytes) {
  if (num_bytes == 0) return 0;
  XChar2b *cvt16 = NULL;
  switch (encoding) {
  case XawTextEncoding8bit:
    return XTextWidth(font, text, num_bytes);
  case XawTextEncodingChar2b:
    return XTextWidth16(font, text, num_bytes/2);
  case XawTextEncodingUTF8:
    cvt16 = UTF8toChar2b(text, &num_bytes);
    break;
  case XawTextEncodingUCS2:
    cvt16 = convert16(text, num_bytes);
    break;
  case XawTextEncodingUTF32:
    cvt16 = UTF32toChar2b(text, &num_bytes);
    break;
  case XawTextEncodingWc:
    cvt16 = WctoChar2b(text, &num_bytes);
    break;
  case XawTextEncodingMb:
    cvt16 = MbtoChar2b(text, &num_bytes);
  }
  const Dimension width = XTextWidth16(font, cvt16, num_bytes/2);
  free(cvt16);
  return width;
}

// Genericized TextWidth/TextHeight
void Xaw3dXftSizeAnyStringN (Display *display, XFontStruct *font,
XFontSet fontSet, XftFont *xftFont, Boolean international,
XawTextEncoding encoding, const void *text, Cardinal num_bytes,
Dimension *width, Dimension *height) {
  if (width == NULL && height == NULL) return;
  if (num_bytes == 0) {
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }
  (void) checklen(num_bytes); // RULE 4

  Dimension w=0, wline=0, h=0, fontHeight;
  Xaw3dXftAnyFontMetrics(display, font, fontSet, xftFont, international,
    &fontHeight, NULL, NULL);

  // Begin line-breaking loop
  const void *nl = nextnl(encoding, text);
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont)
      wline = sizeOneXftLine(display, xftFont, encoding, text, line_bytes);
    else if (international)
      wline = sizeOneXmbLine(fontSet, encoding, text, line_bytes);
    else
      wline = sizeOneLine(font, encoding, text, line_bytes);
    if (wline > w) w = wline;
    h += fontHeight;

    // End line-breaking loop
    text = nl + nlsize(encoding);
    nl = nextnl(encoding, text);
    const Cardinal skip = line_bytes + nlsize(encoding);
    if (num_bytes >= skip)
      num_bytes -= skip;
    else
      num_bytes = 0;
  }

  // Post-loop switch
  if (num_bytes) {
    if (xftFont)
      wline = sizeOneXftLine(display, xftFont, encoding, text, num_bytes);
    else if (international)
      wline = sizeOneXmbLine(fontSet, encoding, text, num_bytes);
    else
      wline = sizeOneLine(font, encoding, text, num_bytes);
    if (wline > w) w = wline;
    h += fontHeight;
  }

  if (width) *width = w;
  if (height) *height = h;
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftSizeAnyString (Display *display, XFontStruct *font,
XFontSet fontSet, XftFont *xftFont, Boolean international,
XawTextEncoding encoding, const void *text, Dimension *width,
Dimension *height) {
  Xaw3dXftSizeAnyStringN(display, font, fontSet, xftFont, international,
    encoding, text, Xaw3dXftAnyStrlen(encoding, text), width, height);
}

// ---- Special interest functions ----

// Find the bytes corresponding to the start of a character and the start of
// the next character.  Returns True if results are valid, False if cannot
// comply.
static Boolean locateChar (XawTextEncoding encoding, const void *text,
			   int character_index,
			   Cardinal *b1, Cardinal *b2) {
  if (character_index < 0) return False;
  assert(b1 && b2 && text);
  const Cardinal l = Xaw3dXftAnyStrlen(encoding, text);
  if (l == 0) return False;
  switch (encoding) {
  case XawTextEncoding8bit:
    if (character_index < l) {
      *b1 = character_index;
      *b2 = character_index+1;
      return True;
    }
    break;
  case XawTextEncodingChar2b:
  case XawTextEncodingUCS2:
    {
      Cardinal byte_index = character_index*2;
      if (byte_index < l) {
	*b1 = byte_index;
	*b2 = byte_index+2;
	return True;
      }
    }
    break;
  case XawTextEncodingUTF32:
  case XawTextEncodingWc:
    {
      Cardinal byte_index = character_index*4;
      if (byte_index < l) {
	*b1 = byte_index;
	*b2 = byte_index+4;
	return True;
      }
    }
    break;
  case XawTextEncodingUTF8:
    {
      Cardinal charsFound = 0, i = 0;
      const uint8_t *c = text;
      while (i < l) {
	if ((c[i] & 0xc0) != 0x80 && charsFound++ == character_index) break;
	++i;
      }
      if (i < l) {
	*b1 = i++;
	while ((c[i] & 0xc0) == 0x80 && i < l) ++i;
	*b2 = i;
	return True;
      }
    }
    break;
  case XawTextEncodingMb:
    {
      Cardinal charsFound = 0, i = 0;
      const char *c = text;
      mbstate_t mb = {0};
      size_t ret = 0;
      while (i < l) {
	ret = mbrlen(c+i, l-i, &mb);
	switch (ret) {
	case 0:  // found NUL
	case -1: // encoding error
	case -2: // truncated multibyte character
	  return False;
	}
	if (charsFound++ == character_index) break;
	i += ret;
      }
      if (i < l) {
	*b1 = i;
	*b2 = i + ret;
	return True;
      }
    }
  }
  return False;
}

// X(mb,wc,utf8)TextPerCharExtents almost does it for font sets but it
// doesn't handle line breaking.
Boolean Xaw3dXftLocateCharacter (
  Display *display,
  XFontStruct *font, XFontSet fontSet, XftFont *xftFont,
  Boolean international,
  XawTextEncoding encoding,
  const void *text,
  int character_index, // characters, not bytes
  Position *x1, Position *x2, Position *y) {
  if (character_index < 0) return False;
  assert(x1 && x2 && y);

  Cardinal b1, b2;
  if (locateChar(encoding, text, character_index, &b1, &b2)) {
    assert(b2 > b1);

    Dimension fontHeight;
    Xaw3dXftAnyFontMetrics(display, font, fontSet, xftFont, international,
      &fontHeight, NULL, NULL);

    // Get down to the right line.
    Cardinal linesSkipped = 0;
    const void *line = text, *nl = nextnl(encoding, line);
    while (nl && text + b2 > nl) {
      if (text + b1 <= nl) return False;
      ++linesSkipped;
      line = nl + nlsize(encoding);
      nl = nextnl(encoding, line);
    }
    *y = linesSkipped * fontHeight;
    const Cardinal bytesSkipped = line - text;
    assert(bytesSkipped <= b1);
    b1 -= bytesSkipped;
    b2 -= bytesSkipped;

    // Get the x locations on that line.
    Dimension w2, h2;
    Xaw3dXftSizeAnyStringN(display, font, fontSet, xftFont, international,
                             encoding, line, b2, &w2, &h2);
    *x2 = w2 - 1;
    if (b1) {
      Dimension w1;
      Xaw3dXftSizeAnyStringN(display, font, fontSet, xftFont, international,
                               encoding, line, b1, &w1, NULL);
      *x1 = w1;
    } else *x1 = 0;
    return True;
  }
  return False;
}

wchar_t *Xaw3dXftAnyToWcN (XawTextEncoding encoding, const void *text,
			  Cardinal *num_bytes) {
  assert(text && num_bytes);
  switch (encoding) {
  case XawTextEncoding8bit:
    return _8bittoWc(text, num_bytes);
  case XawTextEncodingUTF8:
    return UTF8toWc(text, num_bytes);
  case XawTextEncodingUCS2:
    return UCS2toWc(text, num_bytes);
  case XawTextEncodingChar2b:
    return Char2btoWc(text, num_bytes);
  case XawTextEncodingMb:
    return MbtoWc(text, num_bytes);
  default: // UTF32, wc
    return (wchar_t *)Xaw3dXftAnyStrdupN(encoding, text, *num_bytes);
  }
}

void *Xaw3dXftWcToAnyN (const wchar_t *text, Cardinal *num_bytes,
XawTextEncoding encoding) {
  assert(text && num_bytes);
  switch (encoding) {
  case XawTextEncoding8bit:
    return Wcto8bit(text, num_bytes);
  case XawTextEncodingUTF8:
    return WctoUTF8(text, num_bytes);
  case XawTextEncodingUCS2:
    return WctoUCS2(text, num_bytes);
  case XawTextEncodingChar2b:
    return WctoChar2b(text, num_bytes);
  case XawTextEncodingMb:
    return WctoMb(text, num_bytes);
  default: // UTF32, wc
    return Xaw3dXftAnyStrdupN(encoding, text, *num_bytes);
  }
}
