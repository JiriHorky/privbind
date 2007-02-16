#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in addr;

    printf("test started\n");
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("test: socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(987);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("test: bind");
        exit(1);
    }

    printf("test: OK\n");

    exit(0);
}
