/*
 * privbind - allow unpriviledged apps to bind to priviledged ports
 * Copyright (C) 2006 Shachar Shemesh
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
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "ipc.h"

#define FALSE (0!=0)
#define TRUE (0==0)

struct cmdoptions {
    uid_t uid; /* UID to turn into */
    gid_t gid; /* GID to turn into */
    int numbinds; /* Number of binds to catch before program can make do on its own */
    int daemon_mode; /* Assume process is a daemon */
} options;

void usage()
{
    printf("%s - allow programs running in unpriviledged mode to bind to low ports\n", PACKAGE_STRING);
    printf("Usage:\n"
	"-u - UID to run as (mandatory)\n"
	"-g - GID to run as (mandatory)\n"
	"-n - number of binds to catch. After this many binds have happened, all root proccesses exit\n"
	"-d - Daemon mode - assume that the process being run is a daemon\n"
	"-h - This help screen\n");
}

int parse_cmdline( int argc, char *argv[] )
{
    /* Fill in default values */
    options.numbinds=0;
    options.daemon_mode=FALSE;
    options.uid=0;
    options.gid=0;
    
    int opt;

    while( (opt=getopt(argc, argv, "+n:du:g:h" ))!=-1 ) {
	switch(opt) {
	case 'n':
	    options.numbinds=atoi(optarg);
	    break;
	case 'd':
	    options.daemon_mode=TRUE;
	    break;
	case 'u':
	    {
		struct passwd *pw=getpwnam(optarg);
		if( pw!=NULL ) {
		    options.uid=pw->pw_uid;
		    if( options.gid==0 ) {
			options.gid=pw->pw_gid;
		    }
		} else {
		    options.uid=atoi(optarg);
		    if( options.uid==0 ) {
			fprintf(stderr, "Username '%s' not found\n", optarg);
			exit(1);
		    }
		}
	    }
	    break;
	case 'g':
	    options.gid=atoi(optarg);
	    break;
	case 'h':
	    usage();
	    exit(0);
	}
    }

    if( options.uid==0 || options.gid==0 ) {
	fprintf(stderr, "Must set both UID and GID for program to run as\n");

	exit(1);
    }

    if( (argc-optind)<=0 ) {
	fprintf(stderr, "Did not supply command name to run\n");
	exit(1);
    }
    return optind;
}

int main( int argc, char *argv[] )
{
    int skipcount=parse_cmdline( argc, argv );

    /* Warn if we're run as SUID */
    if( getuid()!=geteuid() ) {
	fprintf(stderr, "!!!!Running privbind SUID is a security risk!!!!\n");
    }

    // Create a couple of sockets for communication with our children
    int sv[2];
    if( socketpair(AF_UNIX, SOCK_DGRAM, 0, sv)<0 ) {
	perror("Error creating communication sockets");

	return 2;
    }

    printf("main.c: fork\n");
    pid_t child_pid=fork();
    switch(child_pid) {
    case -1:
	/* Fork failed */
	break;
    case 0:
	/* We are the child */

	/* Drop privileges */
	if( setgroups(0, NULL )<0 ) {
	    perror("setgroups failed");
	    exit(2);
	}
	if( setgid(options.gid)<0 ) {
	    perror("setgid failed");
	    exit(2);
	}
	if( setuid(options.uid)<0 ) {
	    perror("setuid failed");
	    exit(2);
	}

	/* Close the parent socket */
	close(sv[1]);

	/* Rename the child socket to the pre-determined fd */
	if( dup2(sv[0], COMM_SOCKET)<0 ) {
	    perror("dup2 failed");
	    exit(2);
	}
	close(sv[0]);

	/* Set the LD_PRELOAD environment variable */
	char *ldpreload=getenv("LD_PRELOAD");
	if( ldpreload==NULL ) {
	    setenv("LD_PRELOAD", PRELOADLIBNAME, FALSE );
	} else {
	    char *newpreload=malloc(strlen(ldpreload)+sizeof(PRELOADLIBNAME)+1);
	    if( newpreload==NULL ) {
		fprintf(stderr, "Error creating preload environment - not enough memory\n");
		exit(2);
	    }

	    sprintf( newpreload, "%s:%s", PRELOADLIBNAME, ldpreload );

	    setenv("LD_PRELOAD", newpreload, TRUE );

	    free(newpreload);
	}

	/* Set up the variables for exec */
	char **new_argv=calloc(argc-skipcount+1, sizeof(char*) );
	if( new_argv==NULL ) {
	    fprintf(stderr, "Error creating new command line: not enougn memory\n");
	    exit(2);
	}

	int i;
	for( i=0; i<argc-skipcount; ++i ) {
	    new_argv[i]=argv[i+skipcount];
	}
	
	printf("child: execvp\n");
	execvp(new_argv[0], new_argv);
	perror("exec failed");
	return 2;
	break;
    default:
	/* We are the parent */

	/* Close the child socket */
	close(sv[0]);

	/* wait for request from the child */
	do {
	  struct msghdr msghdr={0};
	  struct cmsghdr *cmsg;
	  char buf[CMSG_SPACE(sizeof(int))];
	  struct ipc_msg_req request;
	  struct iovec iov;
	  int s;

	  msghdr.msg_control=buf;
	  msghdr.msg_controllen=sizeof(buf);

	  iov.iov_base = &request;
	  iov.iov_len = sizeof request;

	  msghdr.msg_iov = &iov;
	  msghdr.msg_iovlen = 1;

	  printf("parent: recvmsg\n");
	  if ( recvmsg( sv[1], &msghdr, 0) >= 0) {
	    if ((cmsg = (struct cmsghdr *)CMSG_FIRSTHDR(&msghdr)) != NULL) {
	      printf("main: request.type: %d\n", request.type);
	      switch (request.type) {
	      case MSG_REQ_NONE:
		printf ("main: request type: NONE\n");
		break;
	      case MSG_REQ_BIND:
		if (cmsg->cmsg_len == CMSG_LEN(sizeof(int))
		    && cmsg->cmsg_level == SOL_SOCKET
		    && cmsg->cmsg_type == SCM_RIGHTS)
		  s = *((int*)CMSG_DATA(cmsg));
		else {
		  s = -1;       /* descriptor was not passed */
		  printf("main: no descriptor passed with bind request\n");
		}
		printf("main: request type: BIND; fd: %d port %d addr: %s\n",
		       s,
		       ntohs(request.data.bind.addr.sin_port),
		       inet_ntoa(*(struct in_addr *)
				 &request.data.bind.addr.sin_addr));
		if (bind(s, (struct sockadd *)&request.data.bind.addr,
			 sizeof request.data.bind.addr))
		  perror("main: bind failed");
		else
		  printf("main: bind succeeded\n");
		break;
	      default:
		printf("main: type %d isn't recognized (BIND=%d)!\n", request.type, MSG_REQ_BIND);
		break;
	      }
	    } else {
	      printf("main: got an empty request\n");
	    }
	  } else {
	    perror("main: recvmsg");
	  }
	} while (1);

	int status;
	waitpid(child_pid, &status, 0);
	break;
    }

    return 0;
}
