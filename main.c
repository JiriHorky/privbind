#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

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
	    options.uid=atoi(optarg);
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

    printf("uid=%d, gid=%d, numbinds=%d, daemon=%s\n", options.uid, options.gid, options.numbinds,
	options.daemon_mode?"true":"false");
    return 0;
}
