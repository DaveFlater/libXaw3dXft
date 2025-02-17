/* Xaw3dXft.h,v 1.0 10/11/07 08:07:27 demailly Exp $ */

/***********************************************************

Copyright (c) 2010  J.-P. Demailly

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xaw3dxft/AsciiTextP.h>
#include <X11/Xaw3dxft/ScrollbarP.h>
#include <X11/Xaw3dxft/Xaw3dXftP.h>

#define XAW3DXFT_DEFAULTFONT "Liberation-9"

#ifndef Pixel
typedef unsigned long Pixel;
#endif

Xaw3dXftData * _Xaw3dXft;

void Xaw3dXftSetDefaultHilitColor(void)
{
    _Xaw3dXft->hilit_color = strdup("#000000");
}

void Xaw3dXftSetHilitColor(char * value)
{
    if (_Xaw3dXft->hilit_color)
        free(_Xaw3dXft->hilit_color);
    _Xaw3dXft->hilit_color = strdup(value);
}

void Xaw3dXftSetDefaultFontName(char *name)
{
    if (_Xaw3dXft->default_fontname)
        free(_Xaw3dXft->default_fontname);
    _Xaw3dXft->default_fontname = strdup(name);
}

static int hexa(char c) 
{
    if (c>='0' && c<='9') return c-'0'; else
    if (c>='a' && c<='f') return c-'a'+10; else
    if (c>='A' && c<='F') return c-'A'+10; else
    return 0;
}

void Xaw3dXftSetInsensitiveTwist(char *value)
{
    int i;
    if (!value) return;
    if (*value == '|') 
        _Xaw3dXft->insensitive_twist[0] = 1;
    else
    if (*value == '&') 
        _Xaw3dXft->insensitive_twist[0] = 2;
    else
    if (*value == '^') 
        _Xaw3dXft->insensitive_twist[0] = 3;
    else
    if (*value == '~') {
        if (strlen(value)<3) return;
        _Xaw3dXft->insensitive_twist[0] = 4;
        _Xaw3dXft->insensitive_twist[1] = (hexa(value[1])*16+hexa(value[2]))*257;
        return;
    } else
        _Xaw3dXft->insensitive_twist[0] = 0;
    if (strlen(value)<7) return;
    for (i=1; i<=3; i++)
        _Xaw3dXft->insensitive_twist[i] = (hexa(value[2*i-1])*16+hexa(value[2*i]))*257;
}

XftFont * Xaw3dXftGetFont(Display *dpy, char *name)
{
    if (name) {
        if (strncasecmp(name, "core:",5))
            return XftFontOpenName(dpy, DefaultScreen(dpy), name);
        else
            return XftFontOpenXlfd(dpy, DefaultScreen(dpy), name+5);
    }
    if (!_Xaw3dXft->default_font) {
        if (!_Xaw3dXft->default_fontname)
	    _Xaw3dXft->default_fontname = XAW3DXFT_DEFAULTFONT;
        if (strncasecmp(_Xaw3dXft->default_fontname, "core:",5))
            _Xaw3dXft->default_font = 
                XftFontOpenName(dpy, DefaultScreen(dpy), _Xaw3dXft->default_fontname);
        else
            _Xaw3dXft->default_font = 
                XftFontOpenXlfd(dpy, DefaultScreen(dpy), _Xaw3dXft->default_fontname+5);
        if (!_Xaw3dXft->default_font)
            _Xaw3dXft->default_font = 
                XftFontOpenName(dpy, DefaultScreen(dpy), XAW3DXFT_DEFAULTFONT);
    }
    return _Xaw3dXft->default_font;
}
 
int
Xaw3dXftTextWidth(Widget w, XftFont *font, char *str, int len)
{
    Display *dpy = XtDisplayOfObject(w);
    XGlyphInfo extents = {};
    if (_Xaw3dXft->encoding == -1) {
        XftTextExtentsUtf8(dpy, font, (FcChar8*)str, len, (XGlyphInfo*)&extents);
    } else
    if (_Xaw3dXft->encoding == 8)
        XftTextExtents8(dpy, font, (FcChar8*)str, len, (XGlyphInfo*)&extents);
    else
    if (_Xaw3dXft->encoding == 16)
        XftTextExtents16(dpy, font, (FcChar16*)str, len/2, 
                         (XGlyphInfo*)&extents);
#ifdef DEBUG
    str[len] = '\0';
    printf("%s (%d)\n", str, extents.xOff);
#endif
    return extents.xOff;
}

static void
AdjustBackground(Widget ww, int x, int y, int w, int h)
{
    Display *dpy = XtDisplayOfObject(ww);
    Window win = XtWindowOfObject(ww);
    Pixel bg, fg;
    GC gc;
    Colormap cmap = None;
    XColor xcol;
    static Pixel bg0 = -1, hc;
    static Widget ww0 = None;

    if (_Xaw3dXft->text_bg_hilight == -1 && _Xaw3dXft->text_bg_hilight_color != -1) {
        XtVaGetValues(ww, XtNbackground, &bg, XtNforeground, &fg, NULL);
        if (ww != ww0 || bg != bg0) {
	    XtVaGetValues(ww, XtNcolormap, &cmap, NULL);
            if (cmap==None) cmap = DefaultColormap(dpy, DefaultScreen(dpy));
            xcol.pixel = bg;
            xcol.flags = DoRed | DoGreen | DoBlue;
            XQueryColor(dpy, cmap, &xcol);
            xcol.red = xcol.red ^ ((_Xaw3dXft->text_bg_hilight_color&0xff0000)>>8);
	    xcol.green = xcol.green ^ (_Xaw3dXft->text_bg_hilight_color&0xff00);
	    xcol.blue = xcol.blue ^ ((_Xaw3dXft->text_bg_hilight_color&0xff) << 8);
            XAllocColor(dpy, cmap, &xcol);
            hc = xcol.pixel;
	}
	gc = XtGetGC(ww, 0, 0);
        XSetForeground(dpy, gc, hc);
        XFillRectangle(dpy, win, gc, x, y, w, h);
        XSetForeground(dpy, gc, fg);
    } else
    if (!_Xaw3dXft->string_hilight)
        XClearArea(dpy, win, x, y, w, h, False);
}

void
Xaw3dXftDrawString(Widget w, XftFont *font, int x, int y, char *str, int len)
{
    static XftDraw *draw = NULL;
    static Pixmap pixmap = None;
    static GC gc = None;
    static Window win0 = None;
    static int maxw=0, maxh = 0;
    static Pixel bg0 = -1, bg1 = 0;

    Display *dpy;
    Window win;
    XftColor color;
    XRenderColor xre_color;
    XColor xcol, xhilit;
    Pixel bg, fg;
    Colormap cmap = None;
    XGlyphInfo extents = {};
    int i, j, d;

    if (!font) return;
    if (!len) return;
    dpy = XtDisplayOfObject(w);
    win = XtWindowOfObject(w);
    if (!win) return;

    fg = BlackPixel(dpy, DefaultScreen(dpy));
    XtVaGetValues(w, XtNforeground, &fg, XtNcolormap, &cmap, NULL);

    if (_Xaw3dXft->text_fg_alternate_color != -1)
        fg = _Xaw3dXft->text_fg_alternate_color;

    if (cmap==None) cmap = DefaultColormap(dpy, DefaultScreen(dpy));

    if (_Xaw3dXft->string_use_pixmap) goto use_pixmap;

    draw = XftDrawCreate(dpy, win,
                         DefaultVisual(dpy, DefaultScreen(dpy)), cmap);
    
    xcol.pixel = fg;
    xcol.flags = DoRed | DoGreen | DoBlue;
    XQueryColor(dpy, cmap, &xcol);
    xre_color.red = xcol.red;
    xre_color.green = xcol.green;
    xre_color.blue = xcol.blue;
    xre_color.alpha = 0xffff;
    if (!XtIsSensitive(w)) {
        /* modify color for non sensitive items */
        if (_Xaw3dXft->insensitive_twist[0] < 2) {
            if (_Xaw3dXft->insensitive_twist[0] == 0) {
                xre_color.red = _Xaw3dXft->insensitive_twist[1];
                xre_color.green = _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = _Xaw3dXft->insensitive_twist[3];
	    } else {
                xre_color.red = xcol.red | _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green | _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue | _Xaw3dXft->insensitive_twist[3];
	    }
	} else 
	if (_Xaw3dXft->insensitive_twist[0] < 4) {
	    if (_Xaw3dXft->insensitive_twist[0] == 2) {
                xre_color.red = xcol.red & _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green & _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue & _Xaw3dXft->insensitive_twist[3];
	    } else {
                xre_color.red = xcol.red ^ _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green ^ _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue ^ _Xaw3dXft->insensitive_twist[3];
	    }
	} else
	    xre_color.alpha = _Xaw3dXft->insensitive_twist[1]*257;
    }
    XftColorAllocValue(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
		       cmap, &xre_color, &color);

    if (_Xaw3dXft->encoding == -1) {
        XftTextExtentsUtf8(dpy, font, (FcChar8*)str, len, (XGlyphInfo*)&extents);
        AdjustBackground(w, x, y-font->ascent, extents.xOff, font->height);
        XftDrawStringUtf8(draw, &color, font, x, y, (FcChar8*)str, len);
    } else 
    if (_Xaw3dXft->encoding == 8) {
        XftTextExtents8(dpy, font, (FcChar8*)str, len, (XGlyphInfo*)&extents);
        AdjustBackground(w, x, y-font->ascent, extents.xOff, font->height);
        XftDrawString8(draw, &color, font, x, y, (FcChar8*)str, len);
    } else 
    if (_Xaw3dXft->encoding == 16) {
        XftTextExtents16(dpy, font, (FcChar16*)str, len/2, 
                         (XGlyphInfo*)&extents);
        AdjustBackground(w, x, y-font->ascent, extents.xOff, font->height);
        XftDrawString16(draw, &color, font, x, y, (FcChar16*)str, len/2);
    }
    if (DefaultDepth(dpy, DefaultScreen(dpy))>8)
        XftColorFree(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
		     cmap, &color);
    XftDrawDestroy(draw);
    XFlush(dpy);
    return;

 use_pixmap :

    if (win != win0 || font->max_advance_width>maxw ||
        font->ascent + font->descent > maxh) {
        if (draw) XftDrawDestroy(draw);
        if (pixmap) XFreePixmap(dpy, pixmap);
        if (gc) XFreeGC(dpy, gc);
        draw = NULL;
        win0 = win;
        maxh = font->ascent + font->descent;
        maxw = font->max_advance_width;
    }

    bg = WhitePixel(dpy, DefaultScreen(dpy));
    XtVaGetValues(w, XtNbackground, &bg, NULL);

    if (_Xaw3dXft->string_hilight && _Xaw3dXft->hilit_color && cmap) {
        if (bg != bg0) {
            xcol.pixel = bg;
            xcol.flags = DoRed | DoGreen | DoBlue;
            XQueryColor(dpy, cmap, &xcol);
            XAllocNamedColor(dpy, cmap, _Xaw3dXft->hilit_color, &xhilit, &xhilit); 
            xcol.red = xcol.red ^ xhilit.red;
            xcol.green = xcol.green ^ xhilit.green;
            xcol.blue = xcol.blue ^ xhilit.blue;
            XAllocColor(dpy, cmap, &xcol);
	    bg0 = bg;
            bg1 = xcol.pixel;
        }
        bg = bg1;
    }

    if (!draw) {
        pixmap = XCreatePixmap(dpy, win, maxw, maxh,
                               DefaultDepth(dpy, DefaultScreen(dpy)));
        gc = XCreateGC(dpy, pixmap, 0, 0);
        draw = XftDrawCreate(dpy, pixmap,
		   DefaultVisual(dpy, DefaultScreen(dpy)),
                   cmap);
    }

    xcol.pixel = fg;
    xcol.flags = DoRed | DoGreen | DoBlue;
    XQueryColor(dpy, cmap, &xcol);
    xre_color.red = xcol.red;
    xre_color.green = xcol.green;
    xre_color.blue = xcol.blue;
    xre_color.alpha = 0xffff;
    if (!XtIsSensitive(w)) {
        /* modify color for non sensitive items */
        if (_Xaw3dXft->insensitive_twist[0] < 2) {
            if (_Xaw3dXft->insensitive_twist[0] == 0) {
                xre_color.red = _Xaw3dXft->insensitive_twist[1];
                xre_color.green = _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = _Xaw3dXft->insensitive_twist[3];
	    } else {
                xre_color.red = xcol.red | _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green | _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue | _Xaw3dXft->insensitive_twist[3];
	    }
	} else 
	if (_Xaw3dXft->insensitive_twist[0] < 4) {
	    if (_Xaw3dXft->insensitive_twist[0] == 2) {
                xre_color.red = xcol.red & _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green & _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue & _Xaw3dXft->insensitive_twist[3];
	    } else {
                xre_color.red = xcol.red ^ _Xaw3dXft->insensitive_twist[1];
                xre_color.green = xcol.green ^ _Xaw3dXft->insensitive_twist[2];
                xre_color.blue = xcol.blue ^ _Xaw3dXft->insensitive_twist[3];
	    }
	} else
	    xre_color.alpha = _Xaw3dXft->insensitive_twist[1]*257;
    }
    XftColorAllocValue(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
		       cmap, &xre_color, &color);

    XSetForeground(dpy, gc, bg);
    if (_Xaw3dXft->encoding == -1) {
        i = 0;
        while (i < len) {
            j = i + 1;
            /* find length of UTF8 character */
            if ((str[i] & 0x80))
	        while (j < len && (str[j]&0xc0) == 0x80) j++;
            d = j - i;
            XftTextExtentsUtf8(dpy, font, (FcChar8*)(str+i), d,
                               (XGlyphInfo*)&extents);
            XFillRectangle(dpy, pixmap, gc, 0, 0, extents.xOff, maxh);
            XftDrawStringUtf8(draw, &color, font, 0, font->ascent,
                              (FcChar8*)(str+i), d);
            XCopyArea(dpy, pixmap, win, gc, 0, 0, extents.xOff, maxh,
                      x, y-font->ascent);
            x += extents.xOff;
            i = j;
	}
    } else 
    if (_Xaw3dXft->encoding == 8) {
        i = 0;
        while (i < len) {
            /* find length of 8bit character */
            XftTextExtents8(dpy, font, (FcChar8*)(str+i), 1,
                            (XGlyphInfo*)&extents);
            XFillRectangle(dpy, pixmap, gc, 0, 0, extents.xOff, maxh);
            XftDrawString8(draw, &color, font, 0, font->ascent,
                           (FcChar8*)(str+i), 1);
            XCopyArea(dpy, pixmap, win, gc, 0, 0, extents.xOff, maxh,
                      x, y-font->ascent);
            x += extents.xOff;
            ++i;
	}
    } else 
    if (_Xaw3dXft->encoding == 16) {
        i = 0;
        while (i < len) {
            /* find length of 8bit character */
            XftTextExtents16(dpy, font, (FcChar16*)(str+i), 2,
                            (XGlyphInfo*)&extents);
            XFillRectangle(dpy, pixmap, gc, 0, 0, extents.xOff, maxh);
            XftDrawString16(draw, &color, font, 0, font->ascent,
                           (FcChar16*)(str+i), 2);
            XCopyArea(dpy, pixmap, win, gc, 0, 0, extents.xOff, maxh,
                      x, y-font->ascent);
            x += extents.xOff;
            i += 2;
	}
    }
    if (DefaultDepth(dpy, DefaultScreen(dpy))>8)
        XftColorFree(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
		     cmap, &color);
    XFlush(dpy);
}

#ifdef XAW_ARROW_SCROLLBARS
Widget
Xaw3dXftGetScrollbar(Widget w, XtPointer ptr)
{
    Widget sb = None;
    char *str = (char *)ptr;
    int n;

    if (!w) return None;

    /* Vertical scrollbar of AsciiText/Text widget */
    if (str[0] == 'T') 
        sb = (((AsciiWidget)w)->text.vbar);
    else
    /* Some Parent has vertical scrollbar */
    if (str[0] == 'P') {
        n = str[1] - '0';
        while (n>0 && w) {
	    --n;
	    w = XtParent(w);
	}
        if (n || !w) return None;
        sb = XtNameToWidget(w, "vertical");
    }
    return sb;
}

void
Xaw3dXftHandleMouseWheel (Widget w, XtPointer ptr, XEvent *event, Boolean * flg)
{
    ScrollbarWidget sbw = (ScrollbarWidget) Xaw3dXftGetScrollbar(w, ptr);
    char *str;
    int n = 0;
    if (!sbw) return;
    if (!(sbw->scrollbar.scroll_steps)) {
	str = strchr((char *)ptr, ':');
	if (str)
	    n = atoi(str+1);
	if (n >= 32767) n = 32767;
	if (n <= 0) n = 3;
        sbw->scrollbar.scroll_steps = n;
    }
    XawHandleMouseWheel((Widget)sbw, event, NULL, NULL);
}

void
Xaw3dXftSetMouseWheelHandler(Widget w, XtPointer ptr)
{
    XtAddEventHandler(w, ButtonPressMask, False,
	(XtEventHandler) Xaw3dXftHandleMouseWheel, ptr);
}

void
Xaw3dXftSetMouseWheelSteps(Widget w, XtPointer ptr, int n)
{
    ScrollbarWidget sbw = (ScrollbarWidget) Xaw3dXftGetScrollbar(w, ptr); 
    if (!sbw) return;
    if (n>0 && n<32768) 
	sbw->scrollbar.scroll_steps = n;
}

#endif

void
Xaw3dXftInitialize(void)
{
    Xaw3dXftProc *proc;
    char *str;

    XftInitFtLibrary();

    str = (char *) XtMalloc(28);
    _Xaw3dXft = (Xaw3dXftData *) XtMalloc(sizeof(Xaw3dXftData));
    sprintf(str, "mem:%lu",  (unsigned long) _Xaw3dXft);
    _Xaw3dXft->encoding  = 0;
    _Xaw3dXft->string_hilight  = 0;
    _Xaw3dXft->text_bg_hilight  = 0;
    _Xaw3dXft->string_use_pixmap  = 0;
    _Xaw3dXft->menu_spacing  = 1;
    _Xaw3dXft->show_tips  = 1;
    _Xaw3dXft->tip_do_grab  = 1;
    _Xaw3dXft->border_hack  = 1;
    _Xaw3dXft->no_hilit_reverse  = 0;
    _Xaw3dXft->button_inverse  = 1;
    _Xaw3dXft->button_dashed  = 0;
    _Xaw3dXft->multi_column_menu  = 0;
    _Xaw3dXft->edit_delete_alternative = 0;
    _Xaw3dXft->text_sb_right = 0;    
    memset(_Xaw3dXft->insensitive_twist, 0, 4);
    _Xaw3dXft->text_bg_hilight_color  = -1;
    _Xaw3dXft->tip_background_color  = -1;
    _Xaw3dXft->text_fg_alternate_color  = -1;
    _Xaw3dXft->hilit_color  = NULL;
    _Xaw3dXft->default_fontname  = NULL;
    _Xaw3dXft->default_font  = NULL;
    
    proc = (Xaw3dXftProc *) XtMalloc(sizeof(Xaw3dXftProc));
    _Xaw3dXft->proc = proc;
    proc->set_default_hilit_color = Xaw3dXftSetDefaultHilitColor;
    proc->set_hilit_color = Xaw3dXftSetHilitColor;
    proc->set_default_fontname = Xaw3dXftSetDefaultFontName;
    proc->set_insensitive_twist = Xaw3dXftSetInsensitiveTwist;
    proc->get_font = Xaw3dXftGetFont;
    proc->text_width = Xaw3dXftTextWidth;
    proc->draw_string = Xaw3dXftDrawString;
#ifdef XAW_ARROW_SCROLLBARS
    proc->get_scrollbar =  Xaw3dXftGetScrollbar;
    proc->handle_mousewheel = Xaw3dXftHandleMouseWheel;
    proc->set_mousewheel_handler = Xaw3dXftSetMouseWheelHandler;
    proc->set_mousewheel_steps = Xaw3dXftSetMouseWheelSteps;
#endif
    (void) XrmPermStringToQuark(strdup("Xaw3dXftData"));
    (void) XrmPermStringToQuark(str);
}
