/*
 * AnyString.c
 * Widget-agnostic functions for dealing with text and related issues
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <X11/Xaw3dXft/AnyString.h>

static_assert(Got_XAW_defines);
static_assert(sizeof(XChar2b) == 2);
static_assert(sizeof(FcChar16) == 2);

// If this isn't good enough, it can be determined easily at run time.
// See also:
// autoconf AC_C_BIGENDIAN
// Boost.Predef BOOST_ENDIAN_*
// Boost.Endian
#if defined(__BIG_ENDIAN__) || defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
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

// Give a Pixel, get a XftColor
void Xaw3dXftGetXftColor (Display *display, Visual *visual, Colormap cmap,
Pixel pixel, XftColor *result) {
  XRenderColor xre_color = {0U, 0U, 0U, USHRT_MAX}; // default black
  XColor xcol;
  xcol.pixel = pixel;
  xcol.flags = DoRed | DoGreen | DoBlue;
  XQueryColor(display, cmap, &xcol);
  xre_color.red = xcol.red;
  xre_color.green = xcol.green;
  xre_color.blue = xcol.blue;
  XftColorAllocValue(display, visual, cmap, &xre_color, result);
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

/*
  We currently have three different stippling methods:

  1. When the widgets draw with a stipple, they do this:
    v.foreground = fg;
    v.background = bg;
    v.font       = font->fid;
    v.fill_style = FillTiled;
    v.tile       = XmuCreateStippledPixmap(screen, fg, bg, depth);
    v.graphics_exposures = False;

  XmuCreateStippledPixmap:  This function creates a two pixel by one pixel
  stippled pixmap of specified depth on the specified screen.  The pixmap is
  cached so that multiple requests share the same pixmap.  The pixmap should
  be freed with XmuReleaseStippledPixmap to maintain correct reference
  counts.

  2. stipplePixmap modifies a pixmap by adding an entry to the colorTable and
  then changing half of the pixels to point to that.

  3. drawOneXftLine fills a rectangle with fill_style = FillStippled.  While
  method 1 uses a pixmap with specified fg and bg, this method uses a bitmap
  as a stencil for the bg color being sprayed by the GC.
*/

// Give a Pixel, get a stipple_gc for DrawAnyString.  We need a widget or
// non-widget Object here to take advantage of the GC caching that is done by
// Xt.
GC Xaw3dXftGetStippleGC (Widget w, Pixel bg) {
  static Boolean first = True;
  static Pixmap p;
  if (first) {
    // libx11/src/CrBFData.c XCreateBitmapFromData:
    // "D is any drawable on the same screen that the pixmap will be used in."
    first = false;
    Display *display = XtDisplayOfObject(w);
    // Avoid requiring XtIsRealized(w)
    Window window = RootWindowOfScreen(XtScreenOfObject(w));
    uint8_t data[] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    p = XCreateBitmapFromData(display, window, (char*)data, 8, 8);
  }
  XGCValues v;
  v.foreground = bg;
  v.stipple = p;
  v.fill_style = FillStippled;
  v.graphics_exposures = False;
  return XtGetGC(w, GCForeground|GCStipple|GCFillStyle|GCGraphicsExposures, &v);
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

    // Fill the background.
    XftDrawRect(xftDraw, bg, x, y, width, xftFont->height);

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
) {
  if (num_bytes == 0) return;
  assert(text_gc);
  assert(!xftFont || sensitive || stipple_gc);

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
void Xaw3dXftSizeAnyStringLen (
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
) {
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
