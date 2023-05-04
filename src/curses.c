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

#include <ncurses.h>
#include <sys/ioctl.h>

#include "../include/curses.h"
#include "../include/pint.h"

/* misc variables */
bool curses_initialized = FALSE;

/* windows */
WINDOW *curses_wnd = NULL;
WINDOW *sock_in_wnd_frame = NULL;
WINDOW *sock_out_wnd_frame = NULL;
WINDOW *info_wnd_frame = NULL;
WINDOW *sock_in_wnd = NULL;
WINDOW *sock_out_wnd = NULL;
WINDOW *info_wnd = NULL;

/* window dimensions */
int sock_in_wnd_cols, sock_in_wnd_rows;
int sock_out_wnd_cols, sock_out_wnd_rows;
int info_wnd_cols, info_wnd_rows;

/* linewrap data for sock_in/sock_out windows */
int sock_out_linelen;
int sock_in_linelen;

/*
 * Retrieves the size of the current terminal window.
 */
void get_term_size(int *rows, int *cols)
{
    struct winsize ws;

    memset(&ws, 0, sizeof(ws));

    if (ioctl(0, TIOCGWINSZ, &ws) < 0)
    {
        deinit_curses();
        printf("Couldn't get term size (%s)", strerror(errno));
        finish(-1);
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

/*
 * Writes a string to info window.
 */
void write_info_wnd(char *s)
{
    wprintw(info_wnd, s);

    wrefresh(info_wnd_frame);
    wrefresh(info_wnd);
}

/*
 * Clears and resets the sock_out window.
 */
void clear_sock_out_wnd()
{
    wclear(sock_out_wnd);
    wrefresh(sock_out_wnd);
    sock_out_linelen = 0;
}

/*
 * Clears and resets the sock_in window.
 */
void clear_sock_in_wnd()
{
    wclear(sock_in_wnd);
    wrefresh(sock_in_wnd);
    sock_in_linelen = 0;
}

/*
 * Writes a string to socket input window with linewrapping.
 */
void write_sock_in_wnd(char *s)
{
    int token_len;

    token_len = strlen(s);
    if ((sock_in_wnd_cols - sock_in_linelen) < token_len)
    {
        wprintw(sock_in_wnd, "\n");
        sock_in_linelen = 0;
    }

    sock_in_linelen += token_len;

    wprintw(sock_in_wnd, s);

    wrefresh(sock_in_wnd_frame);
    wrefresh(sock_in_wnd);
}

/*
 * Writes a string to socket output window with linewrapping.
 */
void write_sock_out_wnd(char *s)
{
    int token_len;

    token_len = strlen(s);
    if ((sock_out_wnd_cols - sock_out_linelen) < token_len)
    {
        wprintw(sock_out_wnd, "\n");
        sock_out_linelen = 0;
    }

    sock_out_linelen += token_len;

    wprintw(sock_out_wnd, s);

    wrefresh(sock_out_wnd_frame);
    wrefresh(sock_out_wnd);
}

/*
 * Handles terminal resizing. Resizes and refreshes all windows.
 */
void resize_curses()
{
    int rows, cols, h, info_h;

    get_term_size(&rows, &cols);

    clear();
    refresh();

    /* calculate new dimensions */
    sock_in_wnd_cols = sock_out_wnd_cols = info_wnd_cols = cols - 2;
    h = 0.4 * rows;
    sock_in_wnd_rows = sock_out_wnd_rows = h - 2;
    info_h = rows - 2 * h;
    info_wnd_rows = info_h - 2;

    /* move & resize windows */
    wresize(sock_in_wnd_frame, h, cols);
    mvwin(sock_in_wnd_frame, 0, 0);
    if (wresize(sock_in_wnd, sock_in_wnd_rows, sock_in_wnd_cols) == ERR)
    {
        deinit_curses();
        printf("wresize() failed for sock_in_wnd\n");
        finish(-1);
    }

    if (mvwin(sock_in_wnd, 1, 1) == ERR)
    {
        deinit_curses();
        printf("mvwin() failed for sock_in_wnd\n");
        finish(-1);
    }

    wsetscrreg(sock_in_wnd, 0, sock_in_wnd_rows);

    if (wresize(sock_out_wnd_frame, h, cols) == ERR)
    {
        deinit_curses();
        printf("wresize() failed for sock_out_wnd_frame\n");
        finish(-1);
    }

    if (mvwin(sock_out_wnd_frame, h, 0) == ERR)
    {
        deinit_curses();
        printf("mvwin() failed for sock_out_wnd_frame\n");
        printf("h: %d, rows: %d, cols: %d\n", h, rows, cols);
        finish(-1);
    }

    if (wresize(sock_out_wnd, sock_out_wnd_rows, sock_out_wnd_cols) == ERR)
    {
        deinit_curses();
        printf("wresize() failed for sock_out_wnd\n");
        finish(-1);
    }

    if (mvwin(sock_out_wnd, h + 1, 1) == ERR)
    {
        deinit_curses();
        printf("mvwin() failed for sock_out_wnd\n");
        finish(-1);
    }

    wsetscrreg(sock_out_wnd, 0, sock_out_wnd_rows);

    if (wresize(info_wnd_frame, info_h, cols) == ERR)
    {
        deinit_curses();
        printf("wresize() failed for info_wnd_frame\n");
        finish(-1);
    }

    if (mvwin(info_wnd_frame, 2 * h, 0) == ERR)
    {
        deinit_curses();
        printf("mvwin() failed for info_wnd_frame\n");
        finish(-1);
    }

    if (wresize(info_wnd, info_wnd_rows, info_wnd_cols) == ERR)
    {
        deinit_curses();
        printf("wresize() failed for info_wnd\n");
        finish(-1);
    }

    if (mvwin(info_wnd, 2 * h + 1, 1) == ERR)
    {
        deinit_curses();
        printf("mvwin() failed for info_wnd\n");
        finish(-1);
    }

    wsetscrreg(info_wnd, 0, info_wnd_rows);

    /* decorate window frames */
    box(sock_in_wnd_frame, 0, 0);
    mvwaddstr(sock_in_wnd_frame, 0, 2, "Bytes received");

    box(sock_out_wnd_frame, 0, 0);
    mvwaddstr(sock_out_wnd_frame, 0, 2, "Bytes sent");

    box(info_wnd_frame, 0, 0);
    mvwaddstr(info_wnd_frame, 0, 2, "Info");

    wrefresh(sock_in_wnd_frame);
    wrefresh(sock_in_wnd);

    wrefresh(sock_out_wnd_frame);
    wrefresh(sock_out_wnd);

    wrefresh(info_wnd_frame);
    wrefresh(info_wnd);
}

/*
 * Inits ncurses environment.
 */
void init_curses()
{
    if (!curses_initialized)
    {
        curses_wnd = initscr();
        nonl();
        cbreak();
        noecho();

        curses_initialized = TRUE;

        /* create all windows */
        sock_in_wnd_frame = newwin(4, 4, 0, 0);
        sock_in_wnd = newwin(3, 3, 1, 1);
        scrollok(sock_in_wnd, TRUE);

        sock_out_wnd_frame = newwin(4, 4, 4, 0);
        sock_out_wnd = newwin(3, 3, 5, 1);
        scrollok(sock_out_wnd, TRUE);

        info_wnd_frame = newwin(4, 4, 8, 0);
        info_wnd = newwin(3, 3, 9, 1);
        scrollok(info_wnd, TRUE);

        sock_out_linelen = 0;
        sock_in_linelen = 0;

        resize_curses();
    }
}

/*
 * Deinits the ncurses environment
 */
void deinit_curses()
{
    if (curses_initialized)
    {
        if (info_wnd != NULL)
        {
            delwin(info_wnd);
            info_wnd = NULL;
        }

        if (sock_out_wnd != NULL)
        {
            delwin(sock_out_wnd);
            sock_out_wnd = NULL;
        }

        if (sock_in_wnd != NULL)
        {
            delwin(sock_in_wnd);
            sock_in_wnd = NULL;
        }

        if (info_wnd_frame != NULL)
        {
            delwin(info_wnd_frame);
            info_wnd_frame = NULL;
        }

        if (sock_out_wnd_frame != NULL)
        {
            delwin(sock_out_wnd_frame);
            sock_out_wnd_frame = NULL;
        }

        if (sock_in_wnd_frame != NULL)
        {
            delwin(sock_in_wnd_frame);
            sock_in_wnd_frame = NULL;
        }

        endwin();

        curses_initialized = FALSE;
    }
}
