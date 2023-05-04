/*
The MIT License (MIT)

PINT (Pint Is Not Telnet) - advanced debug tool for TCP/IP networks
Copyright (C) 2002 Matti Dahlbom

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __PINT_CURSES_H
#define __PINT_CURSES_H

struct WINDOW;

/* function externs */
extern void init_curses();
extern void deinit_curses();
extern void resize_curses();

extern void write_info_wnd(char *);
extern void write_sock_in_wnd(char *);
extern void write_sock_out_wnd(char *);

extern void clear_sock_out_wnd();
extern void clear_sock_in_wnd();

/* data externs */
extern WINDOW *sock_in_wnd;
extern WINDOW *sock_out_wnd;
extern WINDOW *info_wnd;

extern int sock_inwnd_cols, sock_in_wnd_rows;
extern int sock_out_wnd_cols, sock_out_wnd_rows;
extern int info_wnd_cols, info_wnd_rows;

#endif
