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

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/errno.h>
#include <ncurses.h>

#include "../include/pint.h"
#include "../include/curses.h"
#include "../include/network.h"

char *socket_type_names[] = {"TCP", "UDP", "RAW"};

/*
 * Sets a descriptor into non-blocking mode
 *
 * Returns 0 on success, -1 on error
 */
int set_nonblocking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
    {
        deinit_curses();
        printf("fcntl() failed for descriptor %d (%s)\n", fd, strerror(errno));

        return -1;
    }
}

/*
 * Connects to a remote host.
 *
 * Returns a socket descriptor if succesful, and -1 if error.
 */
int connect_to_remote_host(char *remote_host, int remote_port)
{
    int sockfd;
    int flags;
    struct sockaddr_in remote_addr;
    struct hostent *remote_hostent;
    char msg[512];

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    remote_hostent = (struct hostent *)gethostbyname(remote_host);

    memcpy(&remote_addr.sin_addr, remote_hostent->h_addr, remote_hostent->h_length);

    switch (socket_type)
    {
    case SOCKTYPE_TCP:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        break;
    case SOCKTYPE_UDP:
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        break;
    default:
        deinit_curses();
        printf("illegal socket_type!\n");
        return -1;
    }

    if (sockfd == -1)
    {
        deinit_curses();
        printf("socket() failed (%s)\n", strerror(errno));

        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1)
    {
        deinit_curses();
        printf("connect() failed (%s)\n", strerror(errno));

        return -1;
    }

    sprintf(msg, "Connected to %s:%d (%s)\n", remote_host, remote_port,
            socket_type_names[socket_type]);
    write_info_wnd(msg);

    return sockfd;
}

/*
 * Creates a new socket and binds it to local port and then calls listen().
 * The actual accept() loop must be dealt with elsewhere.
 *
 * ##TODO## bind to local host also
 *
 * Returns a socket descriptor or -1 on error
 */
int create_server_socket(int local_port)
{
    char msg[512];
    int sockfd;
    struct sockaddr_in myaddr;
    struct hostent *myhostent;

    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(local_port);
    //    myhostent = (struct hostent *)gethostbyname(local_host);

    //    memcpy(&myaddr.sin_addr, myhostent->h_addr, myhostent->h_length);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    switch (socket_type)
    {
    case SOCKTYPE_TCP:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        break;
    case SOCKTYPE_UDP:
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        break;
    default:
        deinit_curses();
        printf("unsupported socket_type %d\n", socket_type);
        return;
    }

    if (sockfd == -1)
    {
        deinit_curses();
        printf("socket() failed (%s)\n", strerror(errno));
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)))
    {
        deinit_curses();
        printf("bind() failed (%s)\n", strerror(errno));
        return -1;
    }

    if (socket_type == SOCKTYPE_TCP)
    {
        if (listen(sockfd, 5))
        {
            deinit_curses();
            printf("listen() failed (%s)\n", strerror(errno));
            return -1;
        }
    }

    sprintf(msg, "Bound to local %s port %d\n",
            socket_type_names[socket_type], local_port);
    write_info_wnd(msg);

    return sockfd;
}

/*
 * Accepts a remote TCP connection. Blocks until there is a new
 * incoming connection.
 *
 * Returns socket descriptor for the new connection or -1 if error
 */
int accept_incoming_connection(int server_sockfd)
{
    socklen_t socklen;
    struct sockaddr_in remote_addr;
    int sockfd;
    char msg[512];

    socklen = sizeof(remote_addr);

    sockfd = accept(server_sockfd, (struct sockaddr *)&remote_addr, &socklen);
    if (sockfd == -1)
    {
        deinit_curses();
        printf("accept() failed (%s)\n", strerror(errno));
        return -1;
    }

    sprintf(msg, "Got connection from %s\n", inet_ntoa(remote_addr.sin_addr));
    write_info_wnd(msg);

    return sockfd;
}
