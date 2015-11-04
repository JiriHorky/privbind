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
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

#include "stub.h"

#include "ipc.h"

static int master_quit=0; /* Whether helper process quit - assume false at first */
static int reuse_port=-1; /* if we should setup SO_REUSEPORT socket option */

FUNCREDIR1( close, int, int );
FUNCREDIR3( bind, int, int, const struct sockaddr *, socklen_t );

static void master_cleanup()
{
    /* The helper process has quit */
    master_quit=1;
    unsetenv("LD_PRELOAD");
    close(COMM_SOCKET);
}

/* Acquire or release the lock on the communication socket */
static int acquire_lock( int acquire )
{
   static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
   struct flock lock;

   lock.l_whence=SEEK_SET;
   lock.l_start=0;
   lock.l_len=0;

   if( acquire ) {
      if( pthread_mutex_lock(&mutex)!=0 )
      {
         /* The mutex is invalid. Assume we terminated */
         master_cleanup();

         return 0;
      }

      /* Acquire the fcntl lock on the socket. Need to retry in case of EINTR */
      lock.l_type=F_WRLCK;

      int res;
      while( (res=fcntl(COMM_SOCKET, F_SETLKW, &lock))!=0 && errno==EINTR )
         ;

      if( res!=0 )
         pthread_mutex_unlock(&mutex);

      return res==0;
   } else {
      /* Need to unlock, opposite order than locking */

      /* Release the fcntl lock */
      lock.l_type=F_UNLCK;
      int res;

      while( (res=fcntl(COMM_SOCKET, F_SETLKW, &lock))!=0 && errno==EINTR )
         ;

      pthread_mutex_unlock(&mutex);

      return res==0;
   }
}

int bind( int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)
{
#ifdef SO_REUSEPORT
   /**/
   if (reuse_port < 0){
     char *env_reuse = getenv("PRIVBIND_REUSE_PORT");
     reuse_port = (env_reuse != NULL && strlen(env_reuse) == 1 && env_reuse[0] == '1');
   }
   if (reuse_port){
     int optval = 1;
     int retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
     if (retval != 0)
       return retval;
   }
#endif
  
   /* First of all, attempt the bind. We only need the socket if it fails with access denied */

   int oret=_bind( sockfd, my_addr, addrlen );
   if( oret==0 || errno!=EACCES )
      return oret;

   /* In most cases, we can let the bind go through as is */
   if( master_quit || (my_addr->sa_family!=AF_INET && my_addr->sa_family!=AF_INET6) 
       || (my_addr->sa_family==AF_INET && addrlen<sizeof(struct sockaddr_in)) 
       || (my_addr->sa_family==AF_INET6 && addrlen<sizeof(struct sockaddr_in6))  
     ) { 
      errno=EACCES;
      return oret;
   }

   /* Prepare the ancillary data for passing the actual FD */
   struct msghdr msghdr={.msg_name=NULL};
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

    /* Check family of the request, only INET and INET6 should make it here */
    if (my_addr->sa_family==AF_INET)
      request.data.bind4.addr=*(struct sockaddr_in *) my_addr;
    else if (my_addr->sa_family==AF_INET6)
      request.data.bind6.addr=*(struct sockaddr_in6 *) my_addr;

    int retval=oret;

    if( acquire_lock(1) ) {
       if( sendmsg( COMM_SOCKET, &msghdr, MSG_NOSIGNAL )>0 ) {
          /* Request was sent - wait for reply */
          struct ipc_msg_reply reply;

          if( recv( COMM_SOCKET, &reply, sizeof(reply), 0 )>0 ) {
             retval=reply.data.stat.retval;
             if( retval<0 )
                errno=reply.data.stat.error;
          } else {
             /* It would appear that the other process has closed, just return the original retval */
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

       acquire_lock(0);
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
