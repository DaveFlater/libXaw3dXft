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

static_assert(sizeof(XChar2b) == 2);
static_assert(sizeof(FcChar16) == 2);

// configure.ac autoconf test AC_C_BIGENDIAN defines or undefines
// WORDS_BIGENDIAN in config.h
#ifdef WORDS_BIGENDIAN
#define isBigEndian 1
#else
#define isBigEndian 0
#endif

/*
  mb and wc are a pain.

  The font set functions Xutf8TextEscapement, Xutf8Draw[Image]String, and the
  corresponding Xwc* functions are all implemented using a translation to mb
  (see libx11/modules/om/generic/omDefault.c).  They fail if the locale's
  codeset doesn't support the Unicode character repertoire and are useless
  for circumventing the locale dependency.

  In general, mb/wc codepoints are implementation-defined except that we seem
  to know that 0 means NUL.
  https://en.cppreference.com/c/string/multibyte
  "A null-terminated multibyte string (NTMBS), or "multibyte string", is a
  sequence of nonzero bytes followed by a byte with value zero (the
  terminating null character)."

  Xaw code assumes that the codepoints of LF, HT, and SPACE are the same
  (there is lots of _Xaw_atowc(XawLF), etc., where _Xaw_atowc just does
  mbtowc on the ASCII char).  Most codesets do in fact preserve ASCII, but
  this isn't guaranteed; e.g., En_US.IBM-1047 is EBCDIC.
  https://www.ibm.com/docs/en/zos/3.2.0?topic=functions-setlocale-set-locale

  mb supports shift states.
  https://en.cppreference.com/c/string/multibyte
  "In some multibyte encodings, any given multibyte character sequence may
  represent different characters depending on the previous byte sequences,
  known as "shift sequences".  Such encodings are known as state-dependent:
  knowledge of the current shift state is required to interpret each
  character."

  As of C23, there is no standard, direct conversion between wc and
  UTF-anything.  You have to convert wc to mb first, which causes you to lose
  any characters that don't exist in the locale's codeset (which might be
  ASCII).  It is, therefore, only practical to rely on the unsafe assumption
  that the implementation-defined wide character encoding will be UTF-16 or
  UTF-32.

  Coming in C29:
  https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3366.htm
  https://en.cppreference.com/c/header/stdmchar
  Not yet supported in GCC as of 2026-08-08:
  https://gcc.gnu.org/projects/c-status.html

  Xaw implemented _XawTextWCToMB and _XawTextMBToWC using the esoteric Xlib
  functions XwcTextListToTextProperty, XmbTextListToTextProperty, and
  XwcTextPropertyToTextList.  These conversions can be done more obviously
  using C library functions.
*/

// Newline (LF) or NUL size
static Cardinal nlsize (XawTextEncoding encoding) {
  switch (encoding) {
  case XawTextEncodingChar2b:
  case XawTextEncodingUCS2:
    return 2;
  case XawTextEncodingUTF32:
    return 4;
  case XawTextEncodingwc:
    return sizeof(wchar_t);
  default: // 8bit, UTF8, mb (could do size = c16rtomb(s, u'\n', &state))
    return 1;
  }
}

// Convert between the two 16-bit representations.  Result will be
// null-terminated.  Caller is responsible for freeing the returned string.
static void *convert16 (const void *text, Cardinal num_bytes) {
  assert(text);
  assert(num_bytes % 2 == 0); // RULE 3
  const uint16_t *s = text;
  uint16_t *t = malloc(num_bytes+2);
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
  const Cardinal num_chars = num_bytes/2;
  for (Cardinal i=0; i<num_chars; ++i) {
    assert(s[i]); // RULE 2
    s[i] = bswap_16(s[i]);
  }
#endif
}

// Reduce UTF-8 to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *UTF8toChar2b (const char *text, Cardinal *num_bytes) {
  assert(text);
  assert(num_bytes);
  const uint8_t *textp = (uint8_t *)text;
  const uint16_t bogusChar = '?';
  Cardinal nb = *num_bytes;
  size_t l = strlen(text);
  assert(nb <= l); // RULE 2
  l = nb;
  uint16_t *new = calloc(l+1, sizeof(uint16_t));
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
      // character cut in half by num_bytes or out of 16-bit range
      *newp++ = bogusChar;
      oldp++;
    }
  }
  *newp = 0;
  *num_bytes = (newp - new) * 2;
  inplacecvt16(new, *num_bytes);
  return (XChar2b *)new;
}

// Return number of bytes in any string, not counting null terminator
Cardinal Xaw3dXftAnyStrlen (XawTextEncoding encoding, const void *text) {
  assert(text);
  switch (encoding) {
  case XawTextEncodingChar2b:
  case XawTextEncodingUCS2:
    {
      const uint16_t *s = text;
      while (*s) ++s;
      return (Cardinal)((uint8_t *)s - (uint8_t *)text);
    }
  case XawTextEncodingUTF32:
    {
      const uint32_t *s = text;
      while (*s) ++s;
      return (Cardinal)((uint8_t *)s - (uint8_t *)text);
    }
  case XawTextEncodingwc:
    {
      const wchar_t *s = text;
      while (*s) ++s;
      return (Cardinal)((uint8_t *)s - (uint8_t *)text);
    }
  default: // 8bit, UTF8, mb
    return strlen(text);
  }
}

// Reduce UTF-32 to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *UTF32toChar2b (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const uint32_t *textp = (uint32_t *)text;
  const uint16_t bogusChar = '?';
  const Cardinal bytelen = Xaw3dXftAnyStrlen(XawTextEncodingUTF32, text),
                      nb = *num_bytes;
  assert(nb <= bytelen); // RULE 2
  assert(nb % 4 == 0);   // RULE 3
  const Cardinal l = nb / 4;
  uint16_t *new = calloc(l+1, sizeof(uint16_t));
  assert(new);
  for (Cardinal i=0; i<l; ++i)
    new[i] = (textp[i] < 0x10000 ? textp[i] : bogusChar);
  new[l] = 0;
  *num_bytes = l*2;
  inplacecvt16(new, *num_bytes);
  return (XChar2b *)new;
}

// Convert mb to UTF32.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// Based on https://en.cppreference.com/c/string/multibyte/mbrtoc32
static char32_t *mbtoUTF32 (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const char32_t bogusChar = '?';
  char32_t *new = calloc(*num_bytes + 1, sizeof(char32_t));
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
      ++p_out;
      break;
    default:
      // It read rc bytes and stored one char32.
      p_in += rc;
      ++p_out;
    }
  }
  *p_out = 0;
  *num_bytes = (p_out - new) * sizeof(char32_t);
  return new;
}

// ----- Begin unsafe conversions -----

// The following conversions are relying on the unsafe assumption that the
// implementation-defined wide character encoding is UTF-32.  This assertion
// fails on Windows.  FIXME when stdmchar gets implemented.
static_assert(sizeof(wchar_t) == sizeof(char32_t));

// Convert wc to UTF32.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static char32_t *wctoUTF32 (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  // Ignoring num_bytes, Rule 2 not enforced
  return (char32_t *)Xaw3dXftAnyStrdup(XawTextEncodingwc, text);
}

// Convert UTF32 to wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *UTF32towc (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  // Ignoring num_bytes, Rule 2 not enforced
  return (wchar_t *)Xaw3dXftAnyStrdup(XawTextEncodingUTF32, text);
}

// Convert 8bit to wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *_8bittowc (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  const Cardinal l = *num_bytes;
  wchar_t *new = calloc(l+1, sizeof(wchar_t));
  assert(new);
  uint8_t *src = (uint8_t *)text;
  for (Cardinal i=0; i<l; ++i) {
    assert(src[i]); // RULE 2
    new[i] = src[i];
  }
  new[l] = 0;
  *num_bytes = l * sizeof(wchar_t);
  return new;
}

// Convert UCS2 to wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *UCS2towc (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  assert(*num_bytes % 2 == 0); // RULE 3
  const Cardinal l = *num_bytes / 2;
  wchar_t *new = calloc(l+1, sizeof(wchar_t));
  assert(new);
  uint16_t *src = (uint16_t *)text;
  for (Cardinal i=0; i<l; ++i) {
    assert(src[i]); // RULE 2
    new[i] = src[i];
  }
  new[l] = 0;
  *num_bytes = l * sizeof(wchar_t);
  return new;
}

// ----- End unsafe conversions -----

// Convert Char2b to wc.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static wchar_t *Char2btowc (const void *text, Cardinal *num_bytes) {
  assert(text && num_bytes);
  void *ucs2 = convert16(text, *num_bytes);
  wchar_t *ret = UCS2towc(ucs2, num_bytes);
  free(ucs2);
  return ret;
}

// Convert mb to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
// Note:  mbrtoc16 implements UTF-16 with surrogate pairs, which we don't
// want.  UTF32toChar2b will instead put '?' for characters that don't fit.
static XChar2b *mbtoChar2b (const char *text, Cardinal *num_bytes) {
  char32_t *utf32 = mbtoUTF32(text, num_bytes);
  XChar2b *c2b = UTF32toChar2b(utf32, num_bytes);
  free(utf32);
  return c2b;
}

// Convert wc to Char2b.  num_bytes is updated as applicable.  Caller is
// responsible for freeing the returned string.
static XChar2b *wctoChar2b (const char *text, Cardinal *num_bytes) {
  char32_t *utf32 = wctoUTF32(text, num_bytes);
  XChar2b *c2b = UTF32toChar2b(utf32, num_bytes);
  free(utf32);
  return c2b;
}

void *Xaw3dXftAnyStrdup (XawTextEncoding encoding, const void *text) {
  assert(text);
  const Cardinal nbytes = Xaw3dXftAnyStrlen(encoding, text) + nlsize(encoding);
  void *s = malloc(nbytes);
  assert(s);
  return memcpy(s, text, nbytes);
}

// Generalized strchr(s, '\n')
// The returned pointer points to the first or only byte of the newline.
static const void *nextnl (XawTextEncoding encoding, const void *text) {
  assert(text);
  switch (encoding) {
  case XawTextEncoding8bit:
  case XawTextEncodingUTF8:
  case XawTextEncodingmb:
    return strchr(text, '\n');
  case XawTextEncodingwc:
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

// Xaw3dXftDrawAnyString component for a single line with Xft font
static void drawOneXftLine (
  XftFont *xftFont,
  XftColor *fg,
  Position x, Position y,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes,
  XftDraw *xftDraw
) {
  if (num_bytes) {
    Position yadj = y + xftFont->ascent;
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
      XftDrawString32(xftDraw, fg, xftFont, x, yadj, text, num_bytes/4);
      break;
    case XawTextEncodingChar2b:
      {
	FcChar16 *cvt16 = convert16(text, num_bytes);
	XftDrawString16(xftDraw, fg, xftFont, x, yadj, cvt16, num_bytes/2);
	free(cvt16);
      }
      break;
    case XawTextEncodingmb:
      {
	char32_t *cvt32 = mbtoUTF32(text, &num_bytes);
	XftDrawString32(xftDraw, fg, xftFont, x, yadj, cvt32, num_bytes/4);
	free(cvt32);
      }
      break;
    case XawTextEncodingwc:
      {
	char32_t *cvt32 = wctoUTF32(text, &num_bytes);
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
  Position x, Position y,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes,
  XFontSetExtents *extents
) {
  // X(mb,wc,utf8)DrawImageString will fill the background and apply a stipple.
  // X(mb,wc,utf8)DrawString will apply a stipple but won't fill the background.
  if (num_bytes) {
    /*
       There doesn't appear to be an answer that makes this agree with
       XDrawString for a given font.  max_ink_extent.y is way too big.
       max_logical_extent.y is smaller than font->max_bounds.ascent.
    */
    Position yadj = y - extents->max_logical_extent.y; // y is negative
    wchar_t *cvtwc = NULL;
    switch (encoding) {
    case XawTextEncodingwc:
      XwcDrawString(display, window, fontSet, gc, x, yadj, text,
	num_bytes/sizeof(wchar_t));
      return;
    case XawTextEncodingmb:
      XmbDrawString(display, window, fontSet, gc, x, yadj, text, num_bytes);
      return;
    case XawTextEncodingUTF8:
      Xutf8DrawString(display, window, fontSet, gc, x, yadj, text, num_bytes);
      return;
    case XawTextEncoding8bit:
      cvtwc = _8bittowc(text, &num_bytes);
      break;
    case XawTextEncodingChar2b:
      cvtwc = Char2btowc(text, &num_bytes);
      break;
    case XawTextEncodingUCS2:
      cvtwc = UCS2towc(text, &num_bytes);
      break;
    case XawTextEncodingUTF32:
      cvtwc = UTF32towc(text, &num_bytes);
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
  Position x, Position y,
  XawTextEncoding encoding,
  const void *text,
  Cardinal num_bytes
) {
  // XDrawImageString[16] will fill the background but won't apply a stipple.
  // XDrawString[16] will apply a stipple but won't fill the background.
  if (num_bytes) {
    XChar2b *cvt16 = NULL;
    Position yadj = y + font->max_bounds.ascent;
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
    case XawTextEncodingwc:
      cvt16 = wctoChar2b(text, &num_bytes);
      break;
    case XawTextEncodingmb:
      cvt16 = mbtoChar2b(text, &num_bytes);
    }
    XDrawString16(display, window, gc, x, yadj, cvt16, num_bytes/2);
    free(cvt16);
  }
}

void Xaw3dXftDrawAnyStringLen (
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

  // The Boolean international resource is from Xaw.  The docs say:  when
  // true, use fontSet; when false, use font.

  // The line-breaking logic is hairy, so do that just once and switch the
  // font systems three times (pre-loop, in-loop, post-loop).  The logic is
  // still duplicated in Xaw3dXftSizeAnyStringLen.
  XftDraw *xftDraw = NULL;
  XFontSetExtents *extents = NULL;

  // Pre-loop switch
  if (xftFont) {
    xftDraw = XftDrawCreate(display, window, visual, cmap);
    assert(xftDraw);
    if (clip && !XftDrawSetClipRectangles(xftDraw, 0, 0, clip, 1))
      XtWarning("libXaw3dXft:  XftDrawSetClipRectangles failed");
  } else if (international) {
    assert(fontSet);
    extents = XExtentsOfFontSet(fontSet);
  } else
    assert(font);
  if (clip && !xftFont &&
      !XSetClipRectangles(display, text_gc, 0, 0, clip, 1, YXBanded))
    XtWarning("libXaw3dXft:  XSetClipRectangles failed");

  // Begin line-breaking loop
  const void *nl = nextnl(encoding, text);
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont) {
      drawOneXftLine(xftFont, fg, x, y, encoding, text, line_bytes, xftDraw);
      y += xftFont->height;
    } else if (international) {
      drawOneXmbLine(display, window, fontSet, text_gc, x, y, encoding, text,
	             line_bytes, extents);
      y += extents->max_logical_extent.height;
    } else {
      drawOneLine(display, window, font, text_gc, x, y, encoding, text,
		  line_bytes);
      y += font->max_bounds.ascent + font->max_bounds.descent;
    }

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
    drawOneXftLine(xftFont, fg, x, y, encoding, text, num_bytes, xftDraw);
    XftDrawDestroy(xftDraw);
  } else if (international)
    drawOneXmbLine(display, window, fontSet, text_gc, x, y, encoding, text,
                   num_bytes, extents);
  else
    drawOneLine(display, window, font, text_gc, x, y, encoding, text, num_bytes);
  if (clip && !xftFont && !XSetClipMask(display, text_gc, None))
    XtWarning("libXaw3dXft:  XSetClipMask failed");
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftDrawAnyString (Display *display, Visual *visual, Colormap cmap,
Window window, XFontStruct *font, XFontSet fontSet, XftFont *xftFont,
Boolean international, GC text_gc, XftColor *fg, Position x, Position y,
XRectangle *clip, XawTextEncoding encoding, const void *text) {
  Xaw3dXftDrawAnyStringLen(display, visual, cmap, window, font, fontSet,
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
    XftTextExtents32(display, xftFont, text, num_bytes/4, &extents);
    break;
  case XawTextEncodingChar2b:
    {
      FcChar16 *cvt16 = convert16(text, num_bytes);
      XftTextExtents16(display, xftFont, cvt16, num_bytes/2, &extents);
      free(cvt16);
    }
    break;
  case XawTextEncodingmb:
    {
      char32_t *cvt32 = mbtoUTF32(text, &num_bytes);
      XftTextExtents32(display, xftFont, cvt32, num_bytes/4, &extents);
      free(cvt32);
    }
    break;
  case XawTextEncodingwc:
    {
      char32_t *cvt32 = wctoUTF32(text, &num_bytes);
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
  case XawTextEncodingwc:
    return XwcTextEscapement(fontSet, text, num_bytes/sizeof(wchar_t));
  case XawTextEncodingmb:
    return XmbTextEscapement(fontSet, text, num_bytes);
  case XawTextEncodingUTF8:
    return Xutf8TextEscapement(fontSet, text, num_bytes);
  case XawTextEncoding8bit:
    cvtwc = _8bittowc(text, &num_bytes);
    break;
  case XawTextEncodingChar2b:
    cvtwc = Char2btowc(text, &num_bytes);
    break;
  case XawTextEncodingUCS2:
    cvtwc = UCS2towc(text, &num_bytes);
    break;
  case XawTextEncodingUTF32:
    cvtwc = UTF32towc(text, &num_bytes);
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
  case XawTextEncodingwc:
    cvt16 = wctoChar2b(text, &num_bytes);
    break;
  case XawTextEncodingmb:
    cvt16 = mbtoChar2b(text, &num_bytes);
  }
  const Dimension width = XTextWidth16(font, cvt16, num_bytes/2);
  free(cvt16);
  return width;
}

// Genericized TextWidth/TextHeight
void Xaw3dXftSizeAnyStringLen (Display *display, XFontStruct *font,
XFontSet fontSet, XftFont *xftFont, Boolean international,
XawTextEncoding encoding, const void *text, Cardinal num_bytes,
Dimension *width, Dimension *height) {
  if (width == NULL && height == NULL) return;
  if (num_bytes == 0) {
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }

  // Line-breaking duplicated from Xaw3dXftDrawAnyStringLen
  XFontSetExtents *extents = NULL;
  Dimension w=0, wline=0, h=0;

  // Pre-loop switch
  if (xftFont)
    ;
  else if (international) {
    assert(fontSet);
    extents = XExtentsOfFontSet(fontSet);
  } else
    assert(font);

  // Begin line-breaking loop
  const void *nl = nextnl(encoding, text);
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont) {
      wline = sizeOneXftLine(display, xftFont, encoding, text, line_bytes);
      h += xftFont->height;
    } else if (international) {
      wline = sizeOneXmbLine(fontSet, encoding, text, line_bytes);
      h += extents->max_logical_extent.height;
    } else {
      wline = sizeOneLine(font, encoding, text, line_bytes);
      h += font->max_bounds.ascent + font->max_bounds.descent;
    }
    if (wline > w) w = wline;

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
    if (xftFont) {
      wline = sizeOneXftLine(display, xftFont, encoding, text, num_bytes);
      h += xftFont->height;
    } else if (international) {
      wline = sizeOneXmbLine(fontSet, encoding, text, num_bytes);
      h += extents->max_logical_extent.height;
    } else {
      wline = sizeOneLine(font, encoding, text, num_bytes);
      h += font->max_bounds.ascent + font->max_bounds.descent;
    }
    if (wline > w) w = wline;
  }

  if (width) *width = w;
  if (height) *height = h;
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftSizeAnyString (Display *display, XFontStruct *font,
XFontSet fontSet, XftFont *xftFont, Boolean international,
XawTextEncoding encoding, const void *text, Dimension *width,
Dimension *height) {
  Xaw3dXftSizeAnyStringLen(display, font, fontSet, xftFont, international,
    encoding, text, Xaw3dXftAnyStrlen(encoding, text), width, height);
}

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
    {
      Cardinal byte_index = character_index*4;
      if (byte_index < l) {
	*b1 = byte_index;
	*b2 = byte_index+4;
	return True;
      }
    }
    break;
  case XawTextEncodingwc:
    {
      Cardinal byte_index = character_index*sizeof(wchar_t);
      if (byte_index < l) {
	*b1 = byte_index;
	*b2 = byte_index+sizeof(wchar_t);
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
  case XawTextEncodingmb:
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
	case -2: // truncated mb character
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

// Font sets offer X(mb,wc,utf8)TextPerCharExtents.  Nice as that is, it
// doesn't handle line breaks, and the metrics are even more inconsistent.
Boolean Xaw3dXftLocateUnderline (
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

    Cardinal lineHeight;
    Position baseline; // relative to the bottom of the drawing
    if (xftFont) {
      lineHeight = xftFont->height;
      baseline = xftFont->descent - 1;
    } else if (international) {
      assert(fontSet);
      XFontSetExtents *extents = XExtentsOfFontSet(fontSet);
      lineHeight = extents->max_logical_extent.height;
      baseline = lineHeight + extents->max_logical_extent.y; // y is negative
    } else {
      assert(font);
      lineHeight = font->max_bounds.ascent + font->max_bounds.descent;
      baseline = font->max_bounds.descent;
    }

    // Get down to the right line
    Cardinal linesSkipped = 0;
    const void *line = text, *nl = nextnl(encoding, line);
    while (nl && text + b2 > nl) {
      if (text + b1 <= nl) return False;
      ++linesSkipped;
      line = nl + nlsize(encoding);
      nl = nextnl(encoding, line);
    }
    *y = linesSkipped * lineHeight;
    const Cardinal bytesSkipped = line - text;
    assert(bytesSkipped <= b1);
    b1 -= bytesSkipped;
    b2 -= bytesSkipped;

    // Get the location on that line
    Dimension w2, h2;
    Xaw3dXftSizeAnyStringLen(display, font, fontSet, xftFont, international,
                             encoding, line, b2, &w2, &h2);
    *y += (Position)h2 - baseline + 1;
    *x2 = w2 - 1;
    if (b1) {
      Dimension w1;
      Xaw3dXftSizeAnyStringLen(display, font, fontSet, xftFont, international,
                               encoding, line, b1, &w1, NULL);
      *x1 = w1;
    } else *x1 = 0;
    return True;
  }
  return False;
}
