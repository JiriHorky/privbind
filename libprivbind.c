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
#include "config.h"

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>

#include "stub.h"

#include "ipc.h"

static int master_quit=0; /* Whether root process quit - assume false at first */

FUNCREDIR1( close, int, int );
FUNCREDIR3( bind, int, int, const struct sockaddr *, socklen_t );

static void master_cleanup()
{
    /* The root process has quit */
    master_quit=1;
    unsetenv("LD_PRELOAD");
    close(COMM_SOCKET);
}

int bind( int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)
{
    /* First of all, attempt the bind. We only need the socket if it fails with access denied */

    int oret=_bind( sockfd, my_addr, addrlen );
    if( oret==0 || errno!=EACCES )
        return oret;

    /* Only use this struct after you made sure that it is, indeed, an AF_INET */
    struct sockaddr_in *in_addr=(struct sockaddr_in *)my_addr;

    /* In most cases, we can let the bind go through as is */
    if( master_quit || my_addr->sa_family!=AF_INET || addrlen<sizeof(struct sockaddr_in) ) {
        errno=EACCES;
	return oret;
    }

    /* Prepare the ancillary data for passing the actual FD */
    struct msghdr msghdr={0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];
    int *fdptr;

    msghdr.msg_control=buf;
    msghdr.msg_controllen=sizeof(buf);

    cmsg=CMSG_FIRSTHDR(&msghdr);
    cmsg->cmsg_level=SOL_SOCKET;
    cmsg->cmsg_type=SCM_RIGHTS;
    cmsg->cmsg_len=CMSG_LEN(sizeof(int));

    /* Initialize the payload */
    fdptr=(int *)CMSG_DATA(cmsg);
    fdptr[0]=sockfd;
    msghdr.msg_controllen=cmsg->cmsg_len;

    /* Don't forget the data component */
    struct ipc_msg_req request;
    struct iovec iov;

    msghdr.msg_iov=&iov;
    msghdr.msg_iovlen=1;

    iov.iov_base=&request;
    iov.iov_len=sizeof(request);

    request.type=MSG_REQ_BIND;
    request.data.bind.addr=*in_addr;

    int retval=oret;

    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    if ( pthread_mutex_lock(&mutex) ) {
	errno=EACCES;
	return oret;
    }

    if( sendmsg( COMM_SOCKET, &msghdr, MSG_NOSIGNAL )>0 ) {
	/* Request was sent - wait for reply */
	struct ipc_msg_reply reply;
	
	if( recv( COMM_SOCKET, &reply, sizeof(reply), 0 )>0 ) {
	    pthread_mutex_unlock(&mutex);
	    retval=reply.data.stat.retval;
	    if( retval<0 )
		errno=reply.data.stat.error;
	} else {
	    /* It would appear that the other process has closed, just return the original retval */
            pthread_mutex_unlock(&mutex);
            master_cleanup();
	}
    } else {
	/* An error has occured! */
	if( errno==EPIPE || errno==ENOTCONN || errno==EBADF ) {
            master_cleanup();
	} else {
	    perror("privbind communication socket error");
            master_cleanup();
	}
    }

    /* Make sure we return the original errno, regardless of what caused us to fail */
    if( retval!=0 )
        errno=EACCES;

    return retval;
}

int close(int fd)
{
   /* Is it our fd? */
   if( fd!=COMM_SOCKET )
      /* No - pass it on to "close" */
      return _close(fd);

   /* Yes - override the close and return an error */
   errno=EBADF;
   return -1;
}
