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
#include <stdlib.h>
#include <ncurses.h>

#include "../include/pint.h"
#include "../include/cmdline.h"
#include "../include/curses.h"
#include "../include/formatters.h"
#include "../include/network.h"

command_line_params cmdline_params;

/*
 * Show 'usage' screen.
 */
void show_usage()
{
    printf("PINT - Pint Is Not Telnet, Copyright (C) 2002 Matti Dahlbom\n\n");
    printf("Usage: pint [options] remote_host remote_port\n");
    printf("    or pint [options] -l listen_port [local_ip]\n\n");

    printf("options:\n");
    printf("\t-h, --help\tdisplay this help screen\n");
    printf("\t-l\t\tlisten mode; PINT binds to listen_port and waits\n");
    printf("\t\t\tfor incoming connections\n");
    printf("\t-sp\t\ttelnet -like plain text interpretation for stdin input\n");
    printf("\t\t\t(default)\n");
    printf("\t-se\t\tescaped interpretation for stdin input\n");
    printf("\t-ec\t\tenter sends CR-LF -mode (default)\n");
    printf("\t-en\t\tenter sends nothing -mode\n");
    printf("\t-sow\t\tBytes sent -window uses wide (text+hex) formatting\n");
    printf("\t\t\t(default)\n");
    printf("\t-sop\t\tBytes sent -window uses plain text (telnet -like)\n");
    printf("\t\t\tformatting\n");
    printf("\t-soh\t\tBytes sent -window uses 2-char hex formatting\n");
    printf("\t-siw\t\tBytes received -window uses wide (text+hex) formatting\n");
    printf("\t\t\t(default)\n");
    printf("\t-sip\t\tBytes received -window uses plain text (telnet -like)\n");
    printf("\t\t\tformatting\n");
    printf("\t-sih\t\tBytes received -window uses 2-char hex formatting\n");
    printf("\t-udp\t\tuse UDP (TCP is default)\n");

    printf("\nRuntime keybindings:\n");
    printf("\t- The formatting mode of the Bytes received -window may be toggled\n");
    printf("\tby pressing F1 key.\n");
    printf("\t- The formatting mode of the Bytes sent -window may be toggled\n");
    printf("\tby pressing F2 key.\n");
    printf("\t- Enter key behaviour mode may be toggled by pressing F3 key.\n");
    printf("\t- Stdin input interpretation mode may toggled by pressing F4 key.\n");

    printf("\nUsing the escaped stdin input interpretation mode\n");
    printf("\nEscaped stdin input interpretation mode is a powerful tool especially");
    printf("\nwhen dealing with binary protocol implementations. The following");
    printf("\nescape sequences may be applied:\n\n");
    printf("\t\\xHH\t\tsends arbitrary byte with value of HH, in hex.\n");
    printf("\t\t\tFor example, \\xff would send the byte 0xff (-1)\n\n");
}

/*
 * Handles a switch from command line
 */
void handle_switch(char *s)
{
    if (strcmp(s, "l") == 0)
    {
        cmdline_params.switches |= SWITCH_LISTEN_MASK;
        return;
    }

    if (strcmp(s, "sp") == 0)
    {
        cmdline_params.stdin_interp_mode = STDIN_INTERP_PLAIN_TEXT;
        return;
    }

    if (strcmp(s, "se") == 0)
    {
        cmdline_params.stdin_interp_mode = STDIN_INTERP_ESCAPED;
        return;
    }

    if (strcmp(s, "en") == 0)
    {
        cmdline_params.enter_behaviour_mode = ENTER_SENDS_NOTHING;
        return;
    }

    if (strcmp(s, "ec") == 0)
    {
        cmdline_params.enter_behaviour_mode = ENTER_SENDS_CRLF;
        return;
    }

    if (strcmp(s, "sow") == 0)
    {
        cmdline_params.sock_out_format = FORMATTER_WIDE;
        return;
    }

    if (strcmp(s, "sop") == 0)
    {
        cmdline_params.sock_out_format = FORMATTER_TEXT;
        return;
    }

    if (strcmp(s, "soh") == 0)
    {
        cmdline_params.sock_out_format = FORMATTER_HEX;
        return;
    }

    if (strcmp(s, "siw") == 0)
    {
        cmdline_params.sock_in_format = FORMATTER_WIDE;
        return;
    }

    if (strcmp(s, "sip") == 0)
    {
        cmdline_params.sock_in_format = FORMATTER_TEXT;
        return;
    }

    if (strcmp(s, "sih") == 0)
    {
        cmdline_params.sock_in_format = FORMATTER_HEX;
        return;
    }

    if (strcmp(s, "udp") == 0)
    {
        cmdline_params.socket_type = SOCKTYPE_UDP;
        return;
    }

    /* no such switch found: show usage */
    show_usage();
    finish(0);
}

/*
 * Parses the command line arguments.
 *
 * It is assumed that init_curses() has not yet been called.
 */
void parse_commandline_args(int argc, char **argv)
{
    int i;
    char *endptr;
    char *cur_arg;

    memset(&cmdline_params, 0, sizeof(cmdline_params));

    for (i = 1; i < argc; i++)
    {
        cur_arg = argv[i];

        if (cur_arg[0] == '-')
        {
            /* argument is a switch */
            if (strlen(cur_arg) < 2)
            {
                printf("Bad switch: %s\n", cur_arg);
                finish(0);
            }
            handle_switch(&cur_arg[1]);
            continue;
        }

        if (cmdline_params.switches & SWITCH_LISTEN_MASK)
        {
            /* parse listen_port and optionally local_ip */
            if (cmdline_params.listen_port == 0)
            {
                cmdline_params.listen_port = strtol(cur_arg, &endptr, 10);
                if (*endptr != '\0')
                {
                    printf("Bad value for listen_port: %s\n", cur_arg);
                    finish(0);
                }
            }
            else
            {
                printf("local_ip not supported yet!\n");
                finish(0);
            }

            break;
        }

        /* parse remote_host and remote_port */
        if (cmdline_params.remote_host[0] == 0)
        {
            if (strlen(cur_arg) > HOST_MAXLEN)
            {
                printf("remote_host argument too long: %s\n", cur_arg);
                finish(0);
            }
            strcpy(cmdline_params.remote_host, cur_arg);
            continue;
        }
        else
        {
            cmdline_params.remote_port = strtol(cur_arg, &endptr, 10);
            if (*endptr != '\0')
            {
                printf("Bad value for remote port: %s\n", cur_arg);
                finish(0);
            }
        }
    }

    /* validate arguments */
    if ((cmdline_params.remote_host[0] == 0) &&
        (cmdline_params.listen_port == 0))
    {
        show_usage();
        finish(0);
    }
}
