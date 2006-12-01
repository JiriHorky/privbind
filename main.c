#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

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

    // Create a couple of sockets for communication with our children
    int sv[2];
    if( socketpair(AF_UNIX, SOCK_DGRAM, 0, sv)<0 ) {
	perror("Error creating communication sockets");

	return 2;
    }

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
#define PRELOADLIBNAME "libprivbind-0.so"
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
	
	execvp(new_argv[0], new_argv);
	break;
    default:
	/* We are the parent */

	/* Close the child socket */
	close(sv[0]);

	int status;
	waitpid(child_pid, &status, 0);
	break;
    }

    return 0;
}
