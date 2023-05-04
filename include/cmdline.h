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

#ifndef __PINT_CMDLINE_H
#define __PINT_CMDLINE_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define HOST_MAXLEN 256

#define SWITCH_LISTEN_MASK 0x0001

typedef struct command_line_params_type
{
    union
    {
        char remote_host[HOST_MAXLEN + 1];
        char local_ip[HOST_MAXLEN + 1];
    };
    union
    {
        int remote_port;
        int listen_port;
    };
    int switches;
    int stdin_interp_mode;
    int enter_behaviour_mode;
    int sock_out_format;
    int sock_in_format;
    int socket_type;
} command_line_params;

/* data externs */
extern command_line_params cmdline_params;

#endif
