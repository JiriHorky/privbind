#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#define TEST_PORT 987

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in addr;

    printf("running as user: %d (%s).\n", getuid(), getpwuid(getuid())->pw_name);
    printf("running as group: %d (%s).\n", getgid(), getgrgid(getgid())->gr_name);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("test: socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("test: bind");
        exit(1);
    }

    printf("bind of port %d succeeded.\n", TEST_PORT);

    /* Sleep to check if the parent sticks around after the bind (if -n 1
       is used, it shouldn't */
    printf("Sleeping for 10 seconds...\n");
    sleep(10);

    exit(0);
}
