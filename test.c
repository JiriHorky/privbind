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
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include <pthread.h>

#define TEST_PORT 987

struct cmdoptions {
    int port; /* Port number to bind */
    int numbinds; /* Number of binds to perform */
    int wait; /* Number of seconds to wait after last bind */
    int processes; /* Number of processes to perform the bind from */
    int threads; /* Number of threads to perform the bind from */
} options={ TEST_PORT, 1, 0, 1, 1 };

void help( const char *progname )
{
    printf("testbin - Try to bind a privileged port\n");
    printf("Usage: %s [-n NUM] [-p NUM] [-w NUM]\n", progname);
    printf("\n");
    printf("-p NUM - Port number to bind. Default is %d\n", options.port);
    printf("-n NUM - Number of times to attempt bind. Default is %d\n", options.numbinds);
    printf("-w NUM - Number of seconds to wait after last bind.\n");
    printf("-P NUM - Number of processes to bind from.\n");
    printf("-T NUM - Number of threads to bind from.\n");
}

int parse_cmdline( int argc, char *argv[] )
{
    /* Fill in default values */
    int opt;

    while( (opt=getopt(argc, argv, "+n:w:p:hP:T:" ))!=-1 ) {
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
        case 'P':
            options.processes=atoi(optarg);
            break;
        case 'T':
            options.threads=atoi(optarg);
            break;
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

    printf("Process %d thread %lx: About to bind\n", getpid(), pthread_self() );
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    int res=bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    gettimeofday(&tv2, NULL);
    tv2.tv_sec-=tv1.tv_sec;
    if(tv1.tv_usec>tv2.tv_usec) {
       tv2.tv_sec--;
       tv2.tv_usec+=1000000;
    }
    tv2.tv_usec-=tv1.tv_usec;

    printf("Process %d thread %lx: Bind returned %d. Took %ld.%06ld secs\n", getpid(),
      pthread_self(), res, tv2.tv_sec, tv2.tv_usec );
    close(fd);

    if(res==-1) {
        perror("bind");
        return 0;
    }

    return 1;
}

void *perform_binds( void *p )
{
   int i=options.numbinds;

   printf("Process %d thread %lx performs %d binds\n", getpid(), pthread_self(), i);

   while( i-->0 ) {
      perform_bind(NULL);
   }

   return NULL;
}

int main(int argc, char *argv[])
{
    parse_cmdline( argc, argv );

    printf("running as user: %d (%s).\n", getuid(), getpwuid(getuid())->pw_name);
    printf("running as group: %d (%s).\n", getgid(), getgrgid(getgid())->gr_name);
    printf("running as process %d\n", getpid() );

    fflush(stdout);

    /* Make sure we have the correct concurrency level */
    int i;
    int parent=1;
    for( i=1; parent && i<options.processes; ++i ) {
       if( fork()==0 ) {
          printf("Created new process %d\n", getpid() );
          parent=0; /* We are not the ones who need to wait() at the end */
       }
    }

    pthread_t *threads=calloc(sizeof(pthread_t), options.threads-1);

    for( i=1; i<options.threads; ++i ) {
       pthread_create( threads+i-1, NULL, perform_binds, NULL );
    }
    /* And once more for the main thread */
    perform_binds(NULL);

    /* Sleep to check if the parent sticks around after the bind (if -n 1
       is used, it shouldn't */
    if( options.wait>0 ) {
        printf("Sleeping for %d seconds...\n", options.wait);
        sleep(options.wait);
    }

    /* Collect all started threads */
    for( i=1; i<options.threads; ++i ) {
       pthread_join(threads[i-1], NULL );

       printf("Process %d joined thread %lx\n", getpid(), threads[i-1]);
    }

    printf("Process %d done\n", getpid() );

    fflush(stdout);

    if( parent ) {
       for( i=1; i<options.processes; ++i ) {
          int status;

          int pid=wait(&status);

          printf("Reaped process %d\n", pid);
       }
    }

    return 0;
}
