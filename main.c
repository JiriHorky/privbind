/*
 * privbind - allow unpriviledged apps to bind to priviledged ports
 * Copyright (C) 2006-2007 Shachar Shemesh
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "paths.h"
#include "ipc.h"

#define FALSE (0!=0)
#define TRUE (0==0)

struct cmdoptions {
    uid_t uid; /* UID to turn into */
    gid_t gid; /* GID to turn into */
    int numbinds; /* Number of binds to catch before program can make do on its own */
    const char *libname; /* Path to library to use as preload */
} options;

void usage( const char *progname )
{
    fprintf(stderr, "Usage: %s -u UID [-g GID] [-n NUM] command [arguments ...]\n", progname);
    fprintf(stderr, "Run '%s -h' for more information.\n", progname);
}
void help( const char *progname )
{
    printf("%s - run a program as an unpriviledged user, while still being\n",
	   PACKAGE_STRING);
    printf("     able to bind to low ports.\n");
    printf("Usage: %s -u UID [-g GID] [-n NUM] command [arguments ...]\n", progname);
    printf("\n"
	"-u - Name or id of user to run as (mandatory)\n"
	"-g - Name or id of group to run as (default: the user's default group)\n"
	"-n - number of binds to catch. After this many binds have happened,\n"
        "     the helper proccess exits.\n"
        "-l - Explicitly specify the path to the libary to use for preload\n"
        "     This option is for debug use only.\n"
	"-h - This help screen\n");
}

int parse_cmdline( int argc, char *argv[] )
{
    /* Fill in default values */
    options.numbinds=0;
    options.uid=0;
    options.gid=-1;
    options.libname=PKGLIBDIR "/" PRELOADLIBNAME;
    
    int opt;

    while( (opt=getopt(argc, argv, "+n:u:g:l:h" ))!=-1 ) {
        switch(opt) {
        case 'n':
            options.numbinds=atoi(optarg);
	    if (options.numbinds < 0) {
	        fprintf(stderr, "Illegal number of binds passed: '%s'\n",
		  optarg);
		exit(1);
	    }
            break;
        case 'u':
            {
                struct passwd *pw=getpwnam(optarg);
                if( pw!=NULL ) {
                    options.uid=pw->pw_uid;
                    /* set the user's default group */
                    if( options.gid==-1 ) {
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
            {
                if ( *optarg == '\0' ) {
                    fprintf(stderr, "Empty group parameters\n");
                    exit(1);
                }
                struct group *gr=getgrnam(optarg);
                if( gr!=NULL ) {
                    options.gid=gr->gr_gid;
                } else {
                    char *endptr;
                    options.gid=strtol(optarg, &endptr, 10);
                    if( *endptr != '\0') {
                        fprintf(stderr, "Group name '%s' not found\n", optarg);
                        exit(1);
                    }
                    if( options.gid < 0 ) {
                        fprintf(stderr, "Illegal group id %d\n", options.gid);
                        exit(1);
                    }
                }
            }
            break;
        case 'l':
            options.libname=optarg;
            break;
        case 'h':
            help(argv[0]);
            exit(0);
        case '?':
            usage(argv[0]);
            exit(1);
        }
    }

    if(options.uid==0){
        fprintf(stderr, "Missing UID (-u) option.\n");
        usage(argv[0]);
        exit(1);
    }
    if(options.gid==-1){
        fprintf(stderr, "Missing GID (-g) option.\n");
        usage(argv[0]);
        exit(1);
    }

    if( (argc-optind)<=0 ) {
        fprintf(stderr, "ERROR: missing a command to run.\n");
        usage(argv[0]);
        exit(1);
    }
    return optind;
}

/* Technically speaking, the "child" is the parent process. Internally, we call it by its semantics
 * rather than by its function.
 */
int process_child( int sv[2], int argc, char *argv[] )
{
    /* Drop privileges */
    if( setgroups(0, NULL )<0 ) {
        perror("privbind: setgroups");
        return 2;
    }
    if( setgid(options.gid)<0 ) {
        perror("privbind: setgid");
        close(sv[0]);
        return 2;
    }
    if( setuid(options.uid)<0 ) {
        perror("privbind: setuid");
        close(sv[0]);
        return 2;
    }

    /* Close the parent socket */
    close(sv[1]);

    /* Rename the child socket to the pre-determined fd */
    if( dup2(sv[0], COMM_SOCKET)<0 ) {
        perror("privbind: dup2");
        return 2;
    }
    close(sv[0]);

    /* Set the LD_PRELOAD environment variable */
    char *ldpreload=getenv("LD_PRELOAD");
    if( ldpreload==NULL ) {
        setenv("LD_PRELOAD", options.libname, FALSE );
    } else {
        char *newpreload=malloc(strlen(ldpreload)+strlen(options.libname)+1);
        if( newpreload==NULL ) {
            fprintf(stderr, "privbind: Error creating preload environment - out of memory\n");
            return 2;
        }

        sprintf( newpreload, "%s:%s", options.libname, ldpreload );

        setenv("LD_PRELOAD", newpreload, TRUE );

        free(newpreload);
    }

    /* Set up the variables for exec */
    char **new_argv=calloc(argc+1, sizeof(char*) );
    if( new_argv==NULL ) {
        fprintf(stderr, "privbind: Error creating new command line: out of memory\n");
        return 2;
    }

    int i;
    for( i=0; i<argc; ++i ) {
        new_argv[i]=argv[i];
    }

    execvp(new_argv[0], new_argv);
    perror("privbind: exec");
    return 2;
}

/* See comment for "process_child" regarding reverse roles */
int process_parent( int sv[2] )
{
    /* Some of the run options mean that we terminate before our "child". We don't want to confuse
     * the child with SIGCHLD of which it is not aware.
     */
    int grandchild_pid=fork();

    if( grandchild_pid==-1 ) {
        perror("privbind: Error creating grandchild process");

        return 1;
    }

    if( grandchild_pid!=0 ) {
        /* We are the grandchild's parent. Terminate cleanly to indicate to our parent that it's ok
         * to start the actual program.
         */
        return 0;
    }

    /* Close the child socket */
    close(sv[0]);

    /* wait for request from the child */
    do {
        struct msghdr msghdr={0};
        struct cmsghdr *cmsg;
        char buf[CMSG_SPACE(sizeof(int))];
        struct ipc_msg_req request;
        struct iovec iov;
        struct ipc_msg_reply reply = {0};
        int recvbytes;

        msghdr.msg_control=buf;
        msghdr.msg_controllen=sizeof(buf);

        iov.iov_base = &request;
        iov.iov_len = sizeof request;

        msghdr.msg_iov = &iov;
        msghdr.msg_iovlen = 1;

        if ( (recvbytes = recvmsg( sv[1], &msghdr, 0)) > 0) {
            if ((cmsg = (struct cmsghdr *)CMSG_FIRSTHDR(&msghdr)) != NULL) {
                switch (request.type) {
                case MSG_REQ_NONE:
                    reply.type = MSG_REP_NONE;
                    if (send(sv[1], &reply, sizeof reply, 0) != sizeof reply)
                        perror("privbind: send");
                    break;
                case MSG_REQ_BIND:
                    reply.type = MSG_REP_STAT;
                    int sock;
                    if (cmsg->cmsg_len == CMSG_LEN(sizeof(int))
                            && cmsg->cmsg_level == SOL_SOCKET
                            && cmsg->cmsg_type == SCM_RIGHTS)
                        sock = *((int*)CMSG_DATA(cmsg));
                    else {
                        sock = -1;
                    }
                    reply.data.stat.retval =
                        bind(sock, (struct sockaddr *)&request.data.bind.addr,
                                sizeof request.data.bind.addr);
                    if (reply.data.stat.retval < 0)
                        reply.data.stat.error = errno;
	            reply.data.stat.calls_left = (options.numbinds == 0) ? -1 :
		       options.numbinds - 1;
                    if (send(sv[1], &reply, sizeof reply, 0) != sizeof reply)
                        perror("privbind: send");
                    if (sock > -1 && close(sock))
                        perror("privbind: close");
                    break;
                default:
                    fprintf(stderr, "privbind: bad request type: %d\n",
                            request.type);
                    break;
                }
            } else {
                fprintf(stderr, "privbind: empty request\n");
            }
        } else if (recvbytes == 0) {
            /* If the child closed its end of the socket, it means the
               child has exited. We have nothing more to do. */

            return 0;
        } else {
            perror("privbind: recvmsg");
        }
    } while (options.numbinds == 0 || --options.numbinds > 0);


    /* If we got here, the child has done the number of binds
       specified by the -n option, and we have nothing more to do
       and should exit, leaving behind no helper process */

    return 0;
}

int main( int argc, char *argv[] )
{
    int skipcount=parse_cmdline( argc, argv );
    int ret=0;

    /* Warn if we're run as SUID */
    if( getuid()!=geteuid() ) {
        fprintf(stderr, "!!!!Running privbind SUID is a security risk!!!!\n");
    }

    /* Create a couple of sockets for communication with our children */
    int sv[2];
    if( socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv)<0 ) {
        perror("privbind: socketpair");
        return 2;
    }

    pid_t child_pid=fork();
    /* http://sourceforge.net/mailarchive/message.php?msg_name=20070601194744.GA29875%40fermat.math.technion.ac.il
     * Reverse the usual role of "parent" and "child".
     * Parent process perform "child" actions - running the command.
     * Child process is the one that listens on the socket and handles the binds
     */
    switch(child_pid) {
    case -1:
        perror("privbind: fork");
        exit(1);

    case 0:
        /* We are the child */
        ret=process_parent( sv );
        break;
    default:
        /* We are the parent */

        {
            /* Wait for the child to exit. */
            int status=0;

            do {
                waitpid( child_pid, &status, 0 );
            } while( !WIFEXITED(status) && !WIFSIGNALED(status) );

            if( WIFEXITED(status) ) {
                ret=WEXITSTATUS(status);

                if( ret==0 ) {
                    /* Child has indicated that it is ready */
                    ret=process_child( sv, argc-skipcount, argv+skipcount );
                }
            } else {
                fprintf(stderr, "privbind: helper process terminated with signal %d\n", WTERMSIG(status) );
                ret=2;
            }
        }
        break;
    }

    return ret;
}
