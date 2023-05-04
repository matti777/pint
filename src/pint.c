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
#include <signal.h>
#include <unistd.h>
#include <ncurses.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <asm/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../include/pint.h"
#include "../include/curses.h"
#include "../include/formatters.h"
#include "../include/cmdline.h"
#include "../include/network.h"

/* stdin reading stuff */
unsigned char stdin_input_buffer[STDIN_INPUT_BUFFER_SIZE];
int stdin_bytes_read;
char escape_chars[ESCAPE_CHARS_BUFFER_SIZE];
int escape_chars_read;
long last_escape_char_sec;
long last_escape_char_usec;

/* bytes written to sock in/out wnds for later reformatting */
unsigned char sock_in_buffer[SOCK_IN_BUFFER_SIZE];
unsigned char sock_out_buffer[SOCK_OUT_BUFFER_SIZE];
int bytes_in_sock_in_buffer;
int bytes_in_sock_out_buffer;

/* enter key behaviour mode */
int enter_behaviour_mode;

/* stdin input interpretation mode */
int stdin_input_interpretation_mode;

/* socket type: TCP UDP or RAW */
int socket_type;

/* UDP remote address; used for listen mode with UDP */
struct sockaddr_in udp_remote_addr;
socklen_t udp_remote_addr_len;
int udp_remote_addr_given;

/* key sequences for special keys */
int seq_f1_len;
char *seq_f1;
int seq_f2_len;
char *seq_f2;
int seq_f3_len;
char *seq_f3;
int seq_f4_len;
char *seq_f4;

/*
 * Reads escape sequences for function keys etc. from
 * termcap.
 */
void read_escape_sequences()
{
	char *termname, *tmp;

	termname = (char *)getenv("TERM");
	if (termname == NULL)
	{
		printf("environment variable TERM not set!\n");
		finish(-1);
	}

	if (tgetent(NULL, termname) == 0)
	{
		printf("termcap entry for %s not found\n", termname);
		finish(-1);
	}
	else
	{
		tmp = (char *)tgetstr("k1", NULL);
		seq_f1_len = strlen(tmp);
		seq_f1 = (char *)malloc(seq_f1_len * sizeof(char) + 1);
		strcpy(seq_f1, tmp);

		tmp = (char *)tgetstr("k2", NULL);
		seq_f2_len = strlen(tmp);
		seq_f2 = (char *)malloc(seq_f2_len * sizeof(char) + 1);
		strcpy(seq_f2, tmp);

		tmp = (char *)tgetstr("k3", NULL);
		seq_f3_len = strlen(tmp);
		seq_f3 = (char *)malloc(seq_f3_len * sizeof(char) + 1);
		strcpy(seq_f3, tmp);

		tmp = (char *)tgetstr("k4", NULL);
		seq_f4_len = strlen(tmp);
		seq_f4 = (char *)malloc(seq_f4_len * sizeof(char) + 1);
		strcpy(seq_f4, tmp);
	}
}

/*
 * Initialization
 */
void init()
{
	int rows, cols;
	int win_rows;

	signal(SIGINT, finish);
	signal(SIGKILL, finish);
	signal(SIGWINCH, resize);

	memset(stdin_input_buffer, 0, STDIN_INPUT_BUFFER_SIZE * sizeof(char));
	stdin_bytes_read = 0;

	memset(escape_chars, 0, ESCAPE_CHARS_BUFFER_SIZE * sizeof(char));
	escape_chars_read = 0;
	last_escape_char_sec = 0;
	last_escape_char_usec = 0;

	enter_behaviour_mode = ENTER_SENDS_CRLF;
	stdin_input_interpretation_mode = STDIN_INTERP_PLAIN_TEXT;

	bytes_in_sock_in_buffer = 0;
	bytes_in_sock_out_buffer = 0;

	udp_remote_addr_given = FALSE;
	memset(&udp_remote_addr, 0, sizeof(udp_remote_addr));

	read_escape_sequences();
}

/*
 * Deinitialization.
 */
void deinit()
{
	if (seq_f1 != NULL)
		free(seq_f1);
	if (seq_f2 != NULL)
		free(seq_f2);
	if (seq_f3 != NULL)
		free(seq_f3);
	if (seq_f4 != NULL)
		free(seq_f4);
}

/*
 * Matches two char arrays of len1 and len2. The array lengths must
 * must equal as well as the corresponding bytes in the arrays.
 *
 * Returns TRUE if the arrays length and contents match.
 * ##TODO## How about strncmp()?
 */
int array_match(int len1, char *array1, int len2, char *array2)
{
	int i, match;

	if (len1 != len2)
	{
		return FALSE;
	}

	match = TRUE;
	for (i = 0; i < len1; i++, array1++, array2++)
	{
		if (*array1 != *array2)
		{
			match = FALSE;
			break;
		}
	}

	return match;
	;
}

/*
 * Matches the bytes in escape_chars[] against the special (function) key
 * sequences.
 *
 * Returns TRUE if match found, FALSE if not.
 */
int match_sequences()
{
	int newmode, i;
	char token[16];

	if (array_match(escape_chars_read, escape_chars, seq_f1_len, seq_f1))
	{
		/* toggle socket input display formatting mode */
		newmode = toggle_sock_in_format();
		switch (newmode)
		{
		case FORMATTER_WIDE:
			write_info_wnd("Using wide formatting for Bytes received window\n");
			break;
		case FORMATTER_TEXT:
			write_info_wnd("Using plain text formatting for Bytes received window\n");
			break;
		case FORMATTER_HEX:
			write_info_wnd("Using hex formatting for Bytes received window\n");
			break;
		}

		/* apply reformatting to sock_in window from buffer */
		if (bytes_in_sock_in_buffer < SOCK_IN_BUFFER_SIZE)
		{
			clear_sock_in_wnd();

			for (i = 0; i < bytes_in_sock_in_buffer; i++)
			{
				sock_in_format->formatter(sock_in_format->pattern,
										  sock_in_buffer[i], token);
				write_sock_in_wnd(token);
			}
		}
		else
		{
			write_info_wnd("reformatting disabled.\n");
		}

		return TRUE;
	}

	if (array_match(escape_chars_read, escape_chars, seq_f2_len, seq_f2))
	{
		/* toggle socket output display formatting mode */
		newmode = toggle_sock_out_format();
		switch (newmode)
		{
		case FORMATTER_WIDE:
			write_info_wnd("Using wide formatting for Bytes sent window\n");
			break;
		case FORMATTER_TEXT:
			write_info_wnd("Using plain text formatting for Bytes sent window\n");
			break;
		case FORMATTER_HEX:
			write_info_wnd("Using hex formatting for Bytes sent window\n");
			break;
		}

		/* apply reformatting to sock_out window from buffer */
		if (bytes_in_sock_out_buffer < SOCK_OUT_BUFFER_SIZE)
		{
			clear_sock_out_wnd();

			for (i = 0; i < bytes_in_sock_out_buffer; i++)
			{
				sock_out_format->formatter(sock_out_format->pattern,
										   sock_out_buffer[i], token);
				write_sock_out_wnd(token);
			}
		}
		else
		{
			write_info_wnd("reformatting disabled.\n");
		}

		return TRUE;
	}

	if (array_match(escape_chars_read, escape_chars, seq_f3_len, seq_f3))
	{
		/* toggles enter key behaviour */
		switch (enter_behaviour_mode)
		{
		case ENTER_SENDS_NOTHING:
			enter_behaviour_mode = ENTER_SENDS_CRLF;
			write_info_wnd("Enter now sends CR LF\n");
			break;
		case ENTER_SENDS_CRLF:
			enter_behaviour_mode = ENTER_SENDS_NOTHING;
			write_info_wnd("Enter now sends nothing\n");
			break;
		}

		return TRUE;
	}

	if (array_match(escape_chars_read, escape_chars, seq_f4_len, seq_f4))
	{
		/* toggles stdin input interpretation mode */
		switch (stdin_input_interpretation_mode)
		{
		case STDIN_INTERP_PLAIN_TEXT:
			stdin_input_interpretation_mode = STDIN_INTERP_ESCAPED;
			write_info_wnd("Using escaped interpretation for input\n");
			break;
		case STDIN_INTERP_ESCAPED:
			stdin_input_interpretation_mode = STDIN_INTERP_PLAIN_TEXT;
			write_info_wnd("Using plain text interpretation for input\n");
			break;
		}

		return TRUE;
	}
}

/*
 * Translates escaped sequences into raw bytes for sending.
 *
 * return: number of bytes to send
 */
int translate_stdin_buffer(unsigned char *dest)
{
	int i, count;
	unsigned char c;
	int escape_mode, escape_seq_len;
	char msg[512];
	char hex[3], *end_ptr;
	long hex_value;

	/* plain text mode: just copy the buffer */
	if (stdin_input_interpretation_mode == STDIN_INTERP_PLAIN_TEXT)
	{
		memcpy(dest, stdin_input_buffer, stdin_bytes_read * sizeof(char));
		return stdin_bytes_read;
	}

	/* escaped mode: parse the escape sequences */
	if (stdin_input_interpretation_mode == STDIN_INTERP_ESCAPED)
	{
		escape_mode = ESCAPE_MODE_INACTIVE;

		for (i = 0, count = 0; i < stdin_bytes_read; i++)
		{
			c = stdin_input_buffer[i];

			if (c == '\\')
			{
				escape_mode = ESCAPE_MODE_STARTED;
			}
			else
			{
				switch (escape_mode)
				{
				case ESCAPE_MODE_STARTED:
					switch (c)
					{
					case 'x':
						escape_mode = ESCAPE_MODE_HEX_STARTED;
						break;
					default:
						sprintf(msg, "Bad escape sequence \\%c\n", c);
						write_info_wnd(msg);
						return 0;
					}
					break;
				case ESCAPE_MODE_HEX_STARTED:
					escape_mode = ESCAPE_MODE_HEX_1READ;
					hex[0] = (char)c;
					break;
				case ESCAPE_MODE_HEX_1READ:
					escape_mode = ESCAPE_MODE_INACTIVE;
					hex[1] = (char)c;
					hex[2] = '\0';

					hex_value = strtol(hex, &end_ptr, 16);
					if (*end_ptr != '\0')
					{
						sprintf(msg, "Bad hex number %s\n", hex);
						write_info_wnd(msg);
						return 0;
					}

					*dest++ = (char)hex_value;
					count++;
					break;
				default:
					*dest++ = c;
					count++;
					break;
				}
			}
		}

		return count;
	}
}

/*
 * Handles stdin input. Upon pressing ENTER, the stdin input buffer
 * is translated and sent to the remote host.
 *
 * Bytes originating from pressing special keys are not sent
 * but parsed out and processed separately.
 */
void handle_stdin_input(int input, int sockfd)
{
	static unsigned char send_buf[STDIN_INPUT_BUFFER_SIZE];
	ssize_t num_sent;
	int num_translated;
	char msg[512];
	int i;
	char token[16];
	struct timeval tv;
	struct timezone tz;
	long timediff;
	int x, y;

	/* handle tab */
	if (input == 9)
	{
		// ##TODO show help screen

		return;
	}

	/* handle backspace */
	if (input == 127)
	{
		if (stdin_bytes_read > 0)
		{
			getyx(info_wnd, y, x);
			mvwdelch(info_wnd, y, x - 1);
			wrefresh(info_wnd);
			stdin_bytes_read--;
		}
		return;
	}

	/* handle escape character */
	if (input == 27)
	{
		escape_chars[0] = 27;
		escape_chars_read = 1;
		gettimeofday(&tv, &tz);
		last_escape_char_sec = tv.tv_sec;
		last_escape_char_usec = tv.tv_usec;
		return;
	}

	if (escape_chars_read > 0)
	{
		/* active escape sequence exists. see if the current input
		   byte is part of the sequence. This is done by a timeout of
		   250ms. Avoid overflow in timediff by a 2s limit. */
		gettimeofday(&tv, &tz);

		if (((tv.tv_sec - last_escape_char_sec) < 2) &&
			(tv.tv_sec >= last_escape_char_sec))
		{
			timediff = (tv.tv_sec - last_escape_char_sec) * 1000000 +
					   (tv.tv_usec - last_escape_char_usec);
			if (timediff < 250000)
			{
				/* the byte is part of the escape sequence */
				// sprintf(msg, "tv_sec: %u tv_usec: %u chars: %d\n",
				// tv.tv_sec, tv.tv_usec, escape_chars_read + 1);
				// write_info_wnd(msg);
				escape_chars[escape_chars_read++] = (char)input;

				/* compare against the defined key sequences */
				if (!match_sequences())
				{
					/* no match: keep recording the sequence */
					last_escape_char_sec = tv.tv_sec;
					last_escape_char_usec = tv.tv_usec;
				}

				return;
			}
		}
	}

	/* echo the character */
	wechochar(info_wnd, input);

	/* look for buffer overflow, being prepared for possible cr lf */
	if (stdin_bytes_read > (STDIN_INPUT_BUFFER_SIZE - 2))
	{
		deinit_curses();
		printf("avoiding stdin input buffer overflow\n");
		finish(-1);
	}

	//    sprintf(msg, "read: %d\n", input);
	//    write_info_wnd(msg);

	if (input == 13)
	{
		/* check if we should send cr/lf or both on enter */
		if (enter_behaviour_mode == ENTER_SENDS_CRLF)
		{
			stdin_input_buffer[stdin_bytes_read++] = 10;
			stdin_input_buffer[stdin_bytes_read++] = 13;
		}

		/* translate stdin input buffer to bytes for sending */
		num_translated = translate_stdin_buffer(send_buf);

		if (num_translated == 0)
		{
			stdin_bytes_read = 0;
			return;
		}

		/* send the buffer */
		num_sent = write(sockfd, send_buf, num_translated);

		if (num_sent < 0)
		{
			sprintf(msg, "Error writing to the connection (%s)\n", strerror(errno));
			write_info_wnd(msg);

			stdin_bytes_read = 0;
			return;
		}

		/* display bytes written into the socket */
		for (i = 0; i < num_sent; i++)
		{
			sock_out_format->formatter(sock_out_format->pattern,
									   send_buf[i], token);
			write_sock_out_wnd(token);

			/* store byte for later reformatting */
			if (bytes_in_sock_out_buffer < SOCK_OUT_BUFFER_SIZE)
			{
				sock_out_buffer[bytes_in_sock_out_buffer++] = send_buf[i];
				if (bytes_in_sock_out_buffer == SOCK_OUT_BUFFER_SIZE)
				{
					write_info_wnd("sock_out_buffer full, disabling reformatting\n");
				}
			}
		}

		sprintf(msg, "wrote %d bytes into the socket\n", num_sent);
		write_info_wnd(msg);
		stdin_bytes_read = 0;
	}
	else
	{
		stdin_input_buffer[stdin_bytes_read++] = (unsigned char)input;
	}
}

/*
 * Manages socket input.
 */
void handle_socket_input(int num_read, unsigned char *buf)
{
	int i;
	char token[16];

	for (i = 0; i < num_read; i++)
	{
		sock_in_format->formatter(sock_in_format->pattern,
								  buf[i], token);
		write_sock_in_wnd(token);

		/* store byte for later reformatting */
		if (bytes_in_sock_in_buffer < SOCK_IN_BUFFER_SIZE)
		{
			sock_in_buffer[bytes_in_sock_in_buffer++] = buf[i];
			if (bytes_in_sock_in_buffer == SOCK_IN_BUFFER_SIZE)
			{
				write_info_wnd("sock_in_buffer full, disabling reformatting\n");
			}
		}
	}
}

/*
 * Reads given socket and prints output on stdout. Also read stdin and write
 * the input to the socket.
 */
void handle_connection(int sockfd)
{
	static unsigned char read_buf[READ_BUFFER_SIZE];
	int keep_reading = 1;
	int maxfd = 0;
	fd_set rset;
	ssize_t n;
	char msg[512];
	int i;

	maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

	while (keep_reading)
	{
		FD_ZERO(&rset);
		FD_SET(STDIN_FILENO, &rset);
		FD_SET(sockfd, &rset);

		select(maxfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &rset))
		{
			n = read(STDIN_FILENO, read_buf, READ_BUFFER_SIZE);
			if ((n < 0) && (errno != EWOULDBLOCK))
			{
				sprintf(msg, "Error reading stdin (%s)\n", strerror(errno));
				write_info_wnd(msg);

				shutdown(sockfd, SHUT_WR);
				finish(-1);
			}

			if (n == 0)
			{
				/* read EOF from stdin */
				finish(0);
			}

			for (i = 0; i < n; i++)
			{
				handle_stdin_input(read_buf[i], sockfd);
			}
		}

		if (FD_ISSET(sockfd, &rset))
		{
			/*
			   if first time to read with listen mode/UDP, need to use recvfrom() and
			   then connect()
			*/
			if ((socket_type == SOCKTYPE_UDP) &&
				(cmdline_params.switches & SWITCH_LISTEN_MASK) &&
				(!udp_remote_addr_given))
			{
				udp_remote_addr_len = sizeof(udp_remote_addr);
				udp_remote_addr_given = TRUE;
				n = recvfrom(sockfd, read_buf, READ_BUFFER_SIZE, 0,
							 (struct sockaddr *)&udp_remote_addr, &udp_remote_addr_len);

				if (n < 0)
				{
					sprintf(msg, "recvfrom() failed (%s)\n", strerror(errno));
					write_info_wnd(msg);
				}
				else
				{
					if (connect(sockfd, (struct sockaddr *)&udp_remote_addr, udp_remote_addr_len))
					{
						sprintf(msg, "UDP: Error connecting to %s (%s)\n",
								inet_ntoa(udp_remote_addr.sin_addr), strerror(errno));
						write_info_wnd(msg);
					}
					else
					{
						sprintf(msg, "Using %s:%d for udp remote host:port\n",
								inet_ntoa(udp_remote_addr.sin_addr),
								ntohs(udp_remote_addr.sin_port), udp_remote_addr_len);
						write_info_wnd(msg);
					}
				}
			}
			else
			{
				/* otherwise, use normal read() */
				n = read(sockfd, read_buf, READ_BUFFER_SIZE);
			}

			if ((n < 0) && (errno != EWOULDBLOCK))
			{
				sprintf(msg, "Error reading socket (%s)\n", strerror(errno));
				write_info_wnd(msg);

				shutdown(sockfd, SHUT_WR);
				finish(-1);
			}

			handle_socket_input(n, read_buf);
		}
	}
}

/*
 * Invokes initialization methods, acquires a socket and
 * invokes the connection handler.
 */
int main(int argc, char *argv[])
{
	int sockfd, server_sockfd;
	char msg[512];

	init();
	parse_commandline_args(argc, argv);
	init_formatters();
	init_curses();

	enter_behaviour_mode = cmdline_params.enter_behaviour_mode;
	stdin_input_interpretation_mode = cmdline_params.stdin_interp_mode;
	socket_type = cmdline_params.socket_type;

	if (cmdline_params.switches & SWITCH_LISTEN_MASK)
	{
		/* acquire socket descriptor by listening incoming connections */
		if ((server_sockfd = create_server_socket(cmdline_params.listen_port)) == -1)
		{
			finish(-1);
		}

		if (socket_type == SOCKTYPE_TCP)
		{
			if ((sockfd = accept_incoming_connection(server_sockfd)) == -1)
			{
				finish(-1);
			}
		}
		else
		{
			/* UDP */
			sockfd = server_sockfd;
		}
	}
	else
	{
		/* acquire socket descriptor by connecting to remote host */
		if ((sockfd = connect_to_remote_host(cmdline_params.remote_host,
											 cmdline_params.remote_port)) == -1)
		{
			finish(-1);
		}
	}

	if (set_nonblocking(STDIN_FILENO) == -1)
	{
		finish(-1);
	}

	if (set_nonblocking(sockfd))
	{
		finish(-1);
	}

	write_info_wnd("For help, run pint with no arguments.\n");
	handle_connection(sockfd);

	finish(0);
}

/*
 * Translates a signal code to textual name.
 */
const char *get_signal_name(int sig)
{
	switch (sig)
	{
	case SIGHUP:
		return "SIGHUP";
		break;
	case SIGINT:
		return "SIGINT";
		break;
	case SIGKILL:
		return "SIGKILL";
		break;
	case SIGWINCH:
		return "SIGWINCH";
		break;
	default:
		return "unknown";
		break;
	}
}

/*
 * SIGWINCH handler.
 */
void resize(int sig)
{
	resize_curses();
}

/*
 * SIGHUP/SIGKILL handler.
 */
void finish(int sig)
{
	deinit();
	deinit_curses();

	if (sig == 0)
	{
		exit(0);
	}
	else if (sig < 0)
	{
		printf("Error %d, exiting..\n", sig);
		exit(sig);
	}
	else
	{
		printf("got signal %d (%s), exiting..\n", sig, get_signal_name(sig));
		exit((sig < 0) ? sig : 0);
	}
}
