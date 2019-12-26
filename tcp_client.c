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
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdlib.h>

int tcp_connect(unsigned short port, char *serverIp) {
    int fd = socket(AF_INET, SOCK_STREAM,0);
    if (fd == -1) {
        printf("create socket failed %s\n", strerror(errno));
        return -1;
    }
    
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        printf("fcntl get failed %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        printf("fcntl set failed %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    struct sockaddr_in stAddr;
    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(port);
    stAddr.sin_addr.s_addr = inet_addr(serverIp);
    if (connect(fd, (struct sockaddr *)&stAddr, sizeof(stAddr)) == -1 && errno != EINPROGRESS) {
        printf("connect failed %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

typedef struct userdata {
    int fd;
    void (*proc)(struct userdata *user, int event);
}user_data_t;

int epfd;

void tcpConnCB(struct userdata *user, int events) { 
    char sendbuf[2000];
    static unsigned long count = 0;
    static struct timeval start;
    static struct timeval end;
    int ret;
    double delta;
    int fd = user->fd;

    if (events & EPOLLOUT){
        if (count == 0) {
            gettimeofday(&start, 0);
        }
    
        ret = send(fd, sendbuf, 2000,0);
        if (ret <= 0) {
            printf("send failed %s\n", strerror(errno));
        } else {
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
    } else if (events & (EPOLLHUP|EPOLLERR)) {
        printf("err occur %s \n", strerror(errno));
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        free(user);
    } 

    return;
}

int main(){
    epfd = epoll_create(1);
    if (-1 == epfd) {
        printf("epoll create failed %s\n", strerror(errno));
        return -1;
    }
    
    int nfds;
    struct epoll_event ev, events;
    int ret = -1;
    int client;
    int maxClient = 3;
    int currClient = 0;
    int run = 1;
    user_data_t *user;
    
    while (run == 1) {
        if (currClient < maxClient) {
            client = tcp_connect(9090, "127.0.0.1");
            if (client == -1) {
                return -1;
            } else {
                user = malloc(sizeof(user_data_t));
                if (NULL == user) {
                    printf("malloc failed %s\n", strerror(errno));
                    return -1;
                }
                user->fd = client;
                user->proc = tcpConnCB;

                ev.data.ptr = user;
                ev.events = EPOLLOUT|EPOLLERR|EPOLLHUP;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
                if (ret == -1) {
                    printf("epoll_ctl add failed %s\n", strerror(errno));
                    close(client);
                } else {
                    ++currClient;
                }
            }
        }

        nfds = epoll_wait(epfd, &events, 1, -1);
        if (nfds == 1) {
            user = (user_data_t *)events.data.ptr;
            user->proc(user, events.events);
        } else {
            printf("epoll_wait failed %s\n", strerror(errno));
        }
    }

    return 0;
}

