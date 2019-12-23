#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(){
    int fd = socket(AF_INET, SOCK_STREAM,0);

    if (fd == -1) {
        printf("create socket failed %d\n", errno);
        return -1;
    }
    
    struct sockaddr_in stAddr;
    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = 9090;
    stAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (struct sockaddr *)&stAddr, sizeof(stAddr)) == -1) {
        printf("connect failed %d\n", errno);
        return -1;
    }

    char sendbuf[2000];
    unsigned long count = 0;
    struct timeval start;
    struct timeval end;
    int ret;
    double delta;

    while (1) {
        if (count == 0) {
            gettimeofday(&start, 0);
        }
    
        ret = send(fd, sendbuf, 2000,0);
        if (ret <= 0) {
            printf("send failed %s\n", strerror(errno));
            break;
        }
    
        count += ret;

        gettimeofday(&end, 0);
        delta = ((double)(end.tv_sec-start.tv_sec)*1000000 + ((double)(end.tv_usec - start.tv_usec)));
        if (delta > 200000) {
            struct tm *lt = localtime(&end.tv_sec);
            char buf[200];
            buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
            printf("time %s.%06lu rate=%fMbps\n", buf, end.tv_usec, (double)count/delta*1000000/1024/128);
            count = 0;
        }
    } 
    
    close(fd);

    return 0;

}

