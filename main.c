#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

#define FALSE (0!=0)
#define TRUE (0==0)

struct cmdoptions {
    uid_t uid; /* UID to turn into */
    gid_t gid; /* GID to turn into */
    int numbinds; /* Number of binds to catch before program can make do on its own */
    int daemon_mode; /* Assume process is a daemon */
} options;

int parse_cmdline( int argc, char *argv[] )
{
    /* Fill in default values */
    options.numbinds=0;
    options.daemon_mode=FALSE;
    options.uid=0;
    options.gid=0;
    
    int opt;

    while( (opt=getopt(argc, argv, "+n:du:" ))!=-1 ) {
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
    return 0;
}
