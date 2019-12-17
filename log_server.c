#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

#define SERVER_PORT 12345
#define BUF_SIZE    2000

int main() {
    int fd = -1;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == fd) {
        printf("create socket failed errno = %d\n", errno);
        return -1;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    if (-1 == bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("bind failed errno = %d\n", errno);
        return -1;
    }

    char buf[BUF_SIZE];
    ssize_t ret;
    while (1) {
        ret = recvfrom(fd, buf, BUF_SIZE, 0, NULL, NULL);
        if (ret > 0) {
            printf("%s", buf);
        }
    }

    return 0;
}



