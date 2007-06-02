/*
 * privbind - allow unpriviledged apps to bind to priviledged ports
 * Copyright (C) 2007 Nadav Har'el
 * Copyright (C) 2007 Shachar Shemesh
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#define TEST_PORT 987

struct cmdoptions {
    int port; /* Port number to bind */
    int numbinds; /* Number of binds to perform */
    int wait; /* Number of seconds to wait after last bind */
} options={ TEST_PORT, 1, 0 };

void help( const char *progname )
{
    printf("testbin - Try to bind a privileged port\n");
    printf("Usage: %s [-n NUM] [-p NUM] [-w NUM]\n", progname);
    printf("\n");
    printf("-p NUM - Port number to bind. Default is %d\n", options.port);
    printf("-n NUM - Number of times to attempt bind. Default is %d\n", options.numbinds);
    printf("-w NUM - Number of seconds to wait after last bind.\n");
}

int parse_cmdline( int argc, char *argv[] )
{
    /* Fill in default values */
    int opt;

    while( (opt=getopt(argc, argv, "+n:w:p:h" ))!=-1 ) {
        switch(opt) {
        case 'n':
            options.numbinds=atoi(optarg);
            break;
        case 'w':
            options.wait=atoi(optarg);
            break;
        case 'p':
            options.port=atoi(optarg);
            break;
        case 'h':
            help(argv[0]);
            exit(0);
        case '?':
            help(argv[0]);
            exit(1);
        }
    }

    return optind;
}
int perform_bind( )
{
    int fd;
    struct sockaddr_in addr;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 0;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(options.port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int res=bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    close(fd);

    if(res==-1) {
        perror("bind");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    parse_cmdline( argc, argv );

    printf("running as user: %d (%s).\n", getuid(), getpwuid(getuid())->pw_name);
    printf("running as group: %d (%s).\n", getgid(), getgrgid(getgid())->gr_name);

    while( options.numbinds-->0 ) {
        printf("Attempting to bind port %d\n", options.port);

        if( perform_bind() )
            printf("Success\n");
    }

    /* Sleep to check if the parent sticks around after the bind (if -n 1
       is used, it shouldn't */
    if( options.wait>0 ) {
        printf("Sleeping for %d seconds...\n", options.wait);
        sleep(options.wait);
    }

    return 0;
}
