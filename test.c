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

int perform_bind( )
{
    int fd;
    struct sockaddr_in addr;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("test: socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int res=bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    close(fd);

    if(res==-1) {
        perror("test: bind");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    printf("running as user: %d (%s).\n", getuid(), getpwuid(getuid())->pw_name);
    printf("running as group: %d (%s).\n", getgid(), getgrgid(getgid())->gr_name);

    if( perform_bind() )
        printf("bind of port %d succeeded.\n", TEST_PORT);
    printf("Retrying the bind\n");
    if( perform_bind() )
        printf("bind of port %d succeeded again.\n", TEST_PORT);

    /* Sleep to check if the parent sticks around after the bind (if -n 1
       is used, it shouldn't */
    printf("Sleeping for 10 seconds...\n");
    sleep(10);

    return 0;
}
