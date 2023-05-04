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

#include <string.h>
#include <ncurses.h>

#include "../include/formatters.h"
#include "../include/cmdline.h"

/* display format stuff */
display_format *sock_out_format, *sock_in_format;
display_format display_formats[NUM_FORMATTERS];
int sock_out_format_type, sock_in_format_type;

/*
 * Returns TRUE if the byte represents a printable character and
 * not a control code.
 *
 * ##TODO## replace by some decent implementation
 */
int isprintable(int c)
{
  switch (c)
  {
  case 10:  /* CR */
  case 13:  /* LF */
  case 127: /* backspace */
  case 9:   /* TAB */
    return FALSE;
  default:
    return TRUE;
  }
}

/*
 * Wide formatter
 */
void wide_formatter(char *pattern, int c, char *token_buf)
{
  if (isprintable(c))
  {
    sprintf(token_buf, pattern, c, 2, c);
  }
  else
  {
    sprintf(token_buf, pattern, ' ', 2, c);
  }
}

/*
 * Text formatter (telnet -like output)
 */
void text_formatter(char *pattern, int c, char *token_buf)
{
  sprintf(token_buf, pattern, c);
}

/*
 * Hex formatter
 */
void hex_formatter(char *pattern, int c, char *token_buf)
{
  sprintf(token_buf, pattern, 2, c);
}

/*
 * Toggles the socket output display format type
 *
 * Returns new format type
 */
int toggle_sock_out_format()
{
  switch (sock_out_format_type)
  {
  case FORMATTER_WIDE:
    sock_out_format_type = FORMATTER_TEXT;
    sock_out_format = &display_formats[sock_out_format_type];
    break;
  case FORMATTER_TEXT:
    sock_out_format_type = FORMATTER_HEX;
    sock_out_format = &display_formats[sock_out_format_type];
    break;
  case FORMATTER_HEX:
    sock_out_format_type = FORMATTER_WIDE;
    sock_out_format = &display_formats[sock_out_format_type];
    break;
  default:
    deinit_curses();
    printf("bad value for sock_out_format_type: %d\n", sock_out_format_type);
    finish(-1);
    break;
  }

  return sock_out_format_type;
}

/*
 * Toggles the socket input display format type
 *
 * Returns new format type
 */
int toggle_sock_in_format()
{
  switch (sock_in_format_type)
  {
  case FORMATTER_WIDE:
    sock_in_format_type = FORMATTER_TEXT;
    sock_in_format = &display_formats[sock_in_format_type];
    break;
  case FORMATTER_TEXT:
    sock_in_format_type = FORMATTER_HEX;
    sock_in_format = &display_formats[sock_in_format_type];
    break;
  case FORMATTER_HEX:
    sock_in_format_type = FORMATTER_WIDE;
    sock_in_format = &display_formats[sock_in_format_type];
    break;
  default:
    deinit_curses();
    printf("bad value for sock_in_format_type: %d\n", sock_in_format_type);
    finish(-1);
    break;
  }

  return sock_in_format_type;
}

/*
 * Initializes all formatters and sets cur_display_format to
 * a default value.
 */
void init_formatters()
{
  /* create wide formatter */
  memset(display_formats, 0, sizeof(display_formats));
  strcpy(display_formats[FORMATTER_WIDE].pattern, "%c(%0*x) ");
  display_formats[FORMATTER_WIDE].formatter = wide_formatter;

  /* create text formatter */
  strcpy(display_formats[FORMATTER_TEXT].pattern, "%c");
  display_formats[FORMATTER_TEXT].formatter = text_formatter;

  /* create hex formatter */
  strcpy(display_formats[FORMATTER_HEX].pattern, "%0*x ");
  display_formats[FORMATTER_HEX].formatter = hex_formatter;

  /* set default formatters from command line data */
  sock_out_format_type = cmdline_params.sock_out_format;
  sock_out_format = &display_formats[sock_out_format_type];
  sock_in_format_type = cmdline_params.sock_in_format;
  sock_in_format = &display_formats[sock_in_format_type];
}
