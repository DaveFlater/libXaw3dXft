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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <X11/Xaw3dXft/AnyStringP.h>

static_assert(Got_XAW_defines);
static_assert(sizeof(XChar2b) == 2);
static_assert(sizeof(FcChar16) == 2);

// configure.ac autoconf test AC_C_BIGENDIAN defines or undefines
// WORDS_BIGENDIAN in config.h
#ifdef WORDS_BIGENDIAN
#define isBigEndian 1
#else
#define isBigEndian 0
#endif

// Convert between the two 16-bit representations.  Result will be
// null-terminated.  Caller is responsible for freeing the returned string.
static void *convert16 (void *text, Cardinal num_bytes) {
  XChar2b *s = text, *t = malloc(num_bytes+2);
  assert(t);
  const Cardinal num_chars = num_bytes/2;
  for (Cardinal i=0; i<num_chars; ++i) {
    #if isBigEndian
      t[i] = s[i];
    #else
      t[i] = (XChar2b){s[i].byte2, s[i].byte1};
    #endif
    if (t[i].byte1 == 0 && t[i].byte2 == 0)
      break;
  }
  t[num_chars] = (XChar2b){0,0};
  return t;
}

// Reduce UTF-8 to Char2b for the old X11 core fonts system without involving
// locales, XmbDrawString, or iconv.  num_bytes is updated as applicable.
// Caller is responsible for freeing the returned string.
static XChar2b *fromutf8 (char *text, Cardinal *num_bytes) {
  assert(text);
  assert(num_bytes);
  const uint8_t *textp = (uint8_t *)text; // signed char is objectively evil
  const uint16_t bogusChar = '?';         // ditto
  Cardinal nb = *num_bytes;
  size_t l = strlen(text);
  if (nb < l) l = nb;
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
  XChar2b *s = (XChar2b *)new;
  #if !isBigEndian
    const Cardinal num_chars = newp - new;
    for (Cardinal i=0; i<num_chars; ++i)
      s[i] = (XChar2b){s[i].byte2, s[i].byte1};
  #endif
  return s;
}

// Return number of bytes in any string, not counting null terminator
static Cardinal AnyStrlen (XawTextEncoding encoding, void *text) {
  assert(text);
  if (some16(encoding)) {
    XChar2b *s = text;
    Cardinal l = 0;
    while (s->byte1 != 0 || s->byte2 != 0) {
      ++l;
      ++s;
    }
    return 2*l;
  }
  return strlen(text);
}

void *Xaw3dXftAnyStrdup (XawTextEncoding encoding, void *text) {
  assert(text);
  if (some16(encoding)) {
    Cardinal nbytes = AnyStrlen(encoding, text) + 2;
    XChar2b *s = malloc(nbytes);
    assert(s);
    return memcpy(s, text, nbytes);
  }
  return strdup(text);
}

// Generalized strchr(s, '\n')
// For 16-bit encodings, the returned pointer points to the first byte of
// the newline.
static void *nextnl (XawTextEncoding encoding, void *text) {
  assert(text);
  if (some16(encoding)) {
    XChar2b *s = text;
    while (s->byte1 != 0 || s->byte2 != 0) {
      if (encoding == XawTextEncodingChar2b &&
	    s->byte1 == 0 && s->byte2 == '\n' ||
	  encoding == XawTextEncoding16bit &&
	    s->byte2 == 0 && s->byte1 == '\n')
	return s;
      ++s;
    }
    return NULL;
  }
  return strchr(text, '\n');
}

// Xaw3dXftSizeAnyString component for a single line with Xft font
static Dimension sizeOneXftLine (Display *display, XftFont *xftFont,
  XawTextEncoding encoding, void *text, Cardinal num_bytes) {
  if (num_bytes == 0) return 0;
  XGlyphInfo extents;
  FcChar16 *cvt16 = NULL;
  switch (encoding) {
  case XawTextEncoding8bit:
    XftTextExtents8(display, xftFont, text, num_bytes, &extents);
    break;
  case XawTextEncodingChar2b:
    cvt16 = convert16(text, num_bytes);
    XftTextExtents16(display, xftFont, cvt16, num_bytes/2, &extents);
    break;
  case XawTextEncoding16bit:
    XftTextExtents16(display, xftFont, text, num_bytes/2, &extents);
    break;
  case XawTextEncodingUTF8:
    XftTextExtentsUtf8(display, xftFont, text, num_bytes, &extents);
  }
  if (cvt16) free(cvt16);
  return extents.xOff;
}

// Xaw3dXftDrawAnyString component for a single line with Xft font
static void drawOneXftLine (
  Display *display, Window window,
  XftFont *xftFont,
  XftColor *fg, XftColor *bg,
  Position x, Position y,
  XawTextEncoding encoding,
  void *text,
  Cardinal num_bytes,
  XftDraw *xftDraw,
  Boolean sensitive,
  GC stipple_gc
) {
  if (num_bytes) {
    // Avoid doing convert16 twice by passing the already-converted string to
    // sizeOneXftLine.
    FcChar16 *cvt16 = NULL;
    Dimension width = 0;
    if (encoding == XawTextEncodingChar2b) {
      cvt16 = convert16(text, num_bytes);
      width = sizeOneXftLine(display, xftFont, XawTextEncoding16bit, cvt16,
	num_bytes);
    } else
      width = sizeOneXftLine(display, xftFont, encoding, text, num_bytes);

    // It's mandatory to reset the area over which XftDrawString will draw.
    // Redrawing over previous lettering causes degradation because it
    // anti-aliases over the previous fringe instead of the original
    // background.

    // FIXME
    // This obliterates a background pixmap.
    XftDrawRect(xftDraw, bg, x, y, width, xftFont->height);

    // This restores a background pixmap to its original state, but that's
    // not always what we need.
    // (void)XClearArea(display, window, x, y, width, xftFont->height, False);

    // Draw the string.
    Position yadj = y + xftFont->ascent;
    switch (encoding) {
    case XawTextEncoding8bit:
      XftDrawString8(xftDraw, fg, xftFont, x, yadj, text, num_bytes);
      break;
    case XawTextEncodingChar2b:
      XftDrawString16(xftDraw, fg, xftFont, x, yadj, cvt16, num_bytes/2);
      free(cvt16);
      break;
    case XawTextEncoding16bit:
      XftDrawString16(xftDraw, fg, xftFont, x, yadj, text, num_bytes/2);
      break;
    case XawTextEncodingUTF8:
      XftDrawStringUtf8(xftDraw, fg, xftFont, x, yadj, text, num_bytes);
    }

    // Apply the stipple.
    if (!sensitive)
      XFillRectangle(display, window, stipple_gc, x, y, width, xftFont->height);
  }
}

#ifdef XAW_INTERNATIONALIZATION
// Xaw3dXftDrawAnyString component for a single line with an XFontSet
static void drawOneXmbLine (
  Display *display, Window window,
  XFontSet fontSet,
  GC gc,
  Position x, Position y,
  void *text,
  Cardinal num_bytes,
  XFontSetExtents *extents
) {
  /*
     X(mb,utf8)DrawImageString fill the background and apply the insensitive
     stipple.  X(mb,utf8)DrawString apply the insensitive stipple but do not
     fill the background.  Their parameters are the same.
  */
  if (num_bytes) {
    /*
       There doesn't appear to be an answer that makes this agree with
       XDrawString for a given font.  max_ink_extent.y is way too big.
       max_logical_extent.y is smaller than font->max_bounds.ascent.
    */
    Position yadj = y - extents->max_logical_extent.y; // y is negative
    /*
       Xutf8DrawString malfunctions when a non-UTF8 codeset is set in the
       locale.  It's a function pointer.  As far as I can guess, the
       implementation is _Xutf8DefaultDrawString in
       libx11/modules/om/generic/omDefault.c.  That does utf8_to_mbs and
       calls _XmbDefaultDrawString on the result.  Unconditionally calling
       XmbDrawString regardless of the encoding resource is no more broken
       and is easier to explain than calling Xutf8DrawString sometimes.
    */
    XmbDrawString(display, window, fontSet, gc, x, yadj, text, num_bytes);
  }
}
#endif

// Xaw3dXftDrawAnyString component for a single line with plain old X font
static void drawOneLine (
  Display *display, Window window,
  XFontStruct *font,
  GC gc,
  Position x, Position y,
  XawTextEncoding encoding,
  void *text,
  Cardinal num_bytes
) {
  // XDrawImageString[16] fill the background but do not apply the
  // insensitive stipple.  XDrawString[16] apply the insensitive stipple but
  // do not fill the background.  Their parameters are the same.
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
    case XawTextEncoding16bit:
      cvt16 = convert16(text, num_bytes);
      break;
    case XawTextEncodingUTF8:
      cvt16 = fromutf8(text, &num_bytes);
    }
    XDrawString16(display, window, gc, x, yadj, cvt16, num_bytes/2);
    free(cvt16);
  }
}

void Xaw3dXftDrawAnyStringLen (
  Display *display, Visual *visual, Colormap cmap, Window window,
  XFontStruct *font, void *fontSet, XftFont *xftFont,
  Boolean sensitive,
  Boolean international,
  GC text_gc, GC stipple_gc, XftColor *fg, XftColor *bg,
  Position x, Position y,
  XawTextEncoding encoding,
  void *text,
  Cardinal num_bytes
) {
  if (num_bytes == 0) return;
  assert(!xftFont || sensitive || stipple_gc);
  assert(xftFont || text_gc);

  // The Boolean international resource is from Xaw.  The docs say:  when
  // true, use fontSet; when false, use font.  When true, encoding is from
  // locale.

  // The line-breaking logic is hairy, so do that just once and switch the
  // font systems three times (pre-loop, in-loop, post-loop).  The logic is
  // still duplicated in Xaw3dXftSizeAnyStringLen.
  XftDraw *xftDraw = NULL;
#ifdef XAW_INTERNATIONALIZATION
  XFontSetExtents *extents = NULL;
#endif

  // Pre-loop switch
  if (xftFont)
    xftDraw = XftDrawCreate(display, window, visual, cmap);
  else
#ifdef XAW_INTERNATIONALIZATION
  if (international) {
    assert(fontSet);
    extents = XExtentsOfFontSet(fontSet);
  } else
#endif
    assert(font);

  // Begin line-breaking loop
  void *nl = nextnl(encoding, text);
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont) {
      drawOneXftLine(display, window, xftFont, fg, bg, x, y, encoding,
	text, line_bytes, xftDraw, sensitive, stipple_gc);
      y += xftFont->height;
    } else
#ifdef XAW_INTERNATIONALIZATION
    if (international) {
      drawOneXmbLine(display, window, fontSet, text_gc, x, y, text,
	line_bytes, extents);
      y += extents->max_logical_extent.height;
    } else
#endif
    {
      drawOneLine(display, window, font, text_gc, x, y, encoding, text, line_bytes);
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
    drawOneXftLine(display, window, xftFont, fg, bg, x, y, encoding,
      text, num_bytes, xftDraw, sensitive, stipple_gc);
    XftDrawDestroy(xftDraw);
  } else
#ifdef XAW_INTERNATIONALIZATION
  if (international)
    drawOneXmbLine(display, window, fontSet, text_gc, x, y, text,
      num_bytes, extents);
  else
#endif
    drawOneLine(display, window, font, text_gc, x, y, encoding, text, num_bytes);
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftDrawAnyString (Display *display, Visual *visual,
  Colormap cmap, Window window, XFontStruct *font, void *fontSet,
  XftFont *xftFont, Boolean sensitive, Boolean international, GC text_gc,
  GC stipple_gc, XftColor *fg, XftColor *bg, Position x, Position y,
  XawTextEncoding encoding, void *text) {
  Xaw3dXftDrawAnyStringLen(display, visual, cmap, window, font, fontSet,
    xftFont, sensitive, international, text_gc, stipple_gc, fg, bg, x, y,
    encoding, text, AnyStrlen(encoding, text));
}

#ifdef XAW_INTERNATIONALIZATION
// Xaw3dXftSizeAnyString component for a single line with an XFontSet
static Dimension sizeOneXmbLine (void *fontSet, void *text, Cardinal num_bytes)
{
  if (num_bytes == 0) return 0;
  // See comments in drawOneXmbLine.  We're not using Xutf8TextEscapement.
  return XmbTextEscapement(fontSet, text, num_bytes);
}
#endif

// Xaw3dXftSizeAnyString component for a single line with plain old X font
static Dimension sizeOneLine (XFontStruct *font, XawTextEncoding encoding,
  void *text, Cardinal num_bytes) {
  if (num_bytes == 0) return 0;
  XChar2b *cvt16 = NULL;
  switch (encoding) {
  case XawTextEncoding8bit:
    return XTextWidth(font, text, num_bytes);
  case XawTextEncodingChar2b:
    return XTextWidth16(font, text, num_bytes/2);
  case XawTextEncoding16bit:
    cvt16 = convert16(text, num_bytes);
    break;
  case XawTextEncodingUTF8:
    cvt16 = fromutf8(text, &num_bytes);
  }
  Dimension temp = XTextWidth16(font, cvt16, num_bytes/2);
  free(cvt16);
  return temp;
}

// Genericized TextWidth/TextHeight
void Xaw3dXftSizeAnyStringLen (Display *display, XFontStruct *font,
  void *fontSet, XftFont *xftFont, Boolean international,
  XawTextEncoding encoding, void *text, Cardinal num_bytes, Dimension *width,
  Dimension *height) {
  if (width == NULL && height == NULL) return;
  if (num_bytes == 0) {
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }

  // Line-breaking duplicated from Xaw3dXftDrawAnyStringLen
#ifdef XAW_INTERNATIONALIZATION
  XFontSetExtents *extents = NULL;
#endif
  Dimension w=0, wline=0, h=0;

  // Pre-loop switch
  if (xftFont)
    ;
  else
#ifdef XAW_INTERNATIONALIZATION
  if (international) {
    assert(fontSet);
    extents = XExtentsOfFontSet(fontSet);
  } else
#endif
    assert(font);

  // Begin line-breaking loop
  void *nl = nextnl(encoding, text);
  while (nl != NULL && num_bytes > 0) {
    Cardinal line_bytes = nl - text;
    if (line_bytes > num_bytes)
      line_bytes = num_bytes;

    // In-loop switch
    if (xftFont) {
      wline = sizeOneXftLine(display, xftFont, encoding, text, line_bytes);
      h += xftFont->height;
    } else
#ifdef XAW_INTERNATIONALIZATION
    if (international) {
      wline = sizeOneXmbLine(fontSet, text, line_bytes);
      h += extents->max_logical_extent.height;
    } else
#endif
    {
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
    } else
#ifdef XAW_INTERNATIONALIZATION
    if (international) {
      wline = sizeOneXmbLine(fontSet, text, num_bytes);
      h += extents->max_logical_extent.height;
    } else
#endif
    {
      wline = sizeOneLine(font, encoding, text, num_bytes);
      h += font->max_bounds.ascent + font->max_bounds.descent;
    }
    if (wline > w) w = wline;
  }

  if (width) *width = w;
  if (height) *height = h;
}

// Ibid. but using the null teminator to determine num_bytes
void Xaw3dXftSizeAnyString (Display *display, XFontStruct *font, void *fontSet,
  XftFont *xftFont, Boolean international, XawTextEncoding encoding,
  void *text, Dimension *width, Dimension *height) {
  Xaw3dXftSizeAnyStringLen(display, font, fontSet, xftFont, international,
    encoding, text, AnyStrlen(encoding, text), width, height);
}

// Find the bytes corresponding to the start of a character and the start of
// the next character.  Not applicable to mb strings (international).
// Returns True if results are valid, False if cannot comply.
static Boolean locateChar (XawTextEncoding encoding, void *text,
			   int character_index,
			   Cardinal *b1, Cardinal *b2) {
  if (character_index < 0) return False;
  assert(b1 && b2);
  const Cardinal l = AnyStrlen(encoding, text);
  switch (encoding) {
  case XawTextEncoding8bit:
    if (character_index < l) {
      *b1 = character_index;
      *b2 = character_index+1;
      return True;
    }
    break;
  case XawTextEncodingChar2b:
  case XawTextEncoding16bit:
    Cardinal byte_index = character_index*2;
    if (byte_index < l) {
      *b1 = byte_index;
      *b2 = byte_index+2;
      return True;
    }
    break;
  case XawTextEncodingUTF8:
    Cardinal charsFound = 0, i = 0;
    uint8_t *c = text;
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
  return False;
}

Boolean Xaw3dXftLocateUnderline (
  Display *display,
  XFontStruct *font, void *fontSet, XftFont *xftFont,
  Boolean international,
  XawTextEncoding encoding,
  void *text,
  int character_index, // characters, not bytes
  Position *x1, Position *x2, Position *y) {
  if (character_index < 0) return False;
  assert(x1 && x2 && y);

#ifdef XAW_INTERNATIONALIZATION
  if (international) {
    Cardinal l = AnyStrlen(encoding, text);
    XRectangle ink[l], logical[l];
    int nchars;
    // Behavior is undefined for a multiline string.
    if (XmbTextPerCharExtents(fontSet, text, l, ink, logical, l, &nchars,
                              NULL, NULL) && character_index < nchars) {
      const XRectangle ti=ink[character_index], tl=logical[character_index];
      *x1 = ti.x;
      *x2 = ti.x + ti.width - 1;
      *y  = tl.height + 2; // +1 or +2?  Results are inconsistent.
      return True;
    }
    return False;
  }
#endif

  Cardinal b1, b2;
  if (locateChar(encoding, text, character_index, &b1, &b2)) {
    assert(b2 > b1);

    // Get us on the right line
    Cardinal linesSkipped = 0;
    void *line = text, *nl = nextnl(encoding, line);
    while (nl && text + b2 > nl) {
      if (text + b1 <= nl) return False;
      ++linesSkipped;
      line = nl + nlsize(encoding);
      nl = nextnl(encoding, line);
    }
    *y = linesSkipped * (xftFont ? xftFont->height :
                         font->max_bounds.ascent + font->max_bounds.descent);
    const Cardinal bytesSkipped = line - text;
    assert(bytesSkipped <= b1);
    b1 -= bytesSkipped;
    b2 -= bytesSkipped;

    // Guess where the baseline is relative to the bottom of the drawing
    // (Font sets not handled here)
    Position baseline = (xftFont ?
                         xftFont->descent - 1 : font->max_bounds.descent);

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
