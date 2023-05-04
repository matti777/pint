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

#ifndef __PINT_H
#define __PINT_H

#define STDIN_INPUT_BUFFER_SIZE 4096
#define READ_BUFFER_SIZE 4096
#define ESCAPE_CHARS_BUFFER_SIZE 16

#define SOCK_IN_BUFFER_SIZE 10000
#define SOCK_OUT_BUFFER_SIZE 10000

enum INPUT_ESCAPE_MODES
{
	ESCAPE_MODE_INACTIVE = 0,
	ESCAPE_MODE_STARTED,
	ESCAPE_MODE_HEX_STARTED,
	ESCAPE_MODE_HEX_1READ
};

enum ENTER_BEHAVIOUR_MODES
{
	ENTER_SENDS_CRLF = 0,
	ENTER_SENDS_NOTHING
};

enum STDIN_INPUT_INTERPRETATION_MODES
{
	STDIN_INTERP_PLAIN_TEXT = 0,
	STDIN_INTERP_ESCAPED
};

extern int errno;
extern int h_errno;

/* data externs */
extern int socket_type;

/* function prototypes */
extern void finish(int sig);
extern void resize(int sig);

#endif
