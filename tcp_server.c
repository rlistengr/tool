#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <linux/in.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct userdata {
    int fd;
    void (*proc)(struct userdata *user, int event);
}user_data_t;

int epfd;

void server_recv(struct userdata *user, int event) {
    char buf[2000];
    ssize_t ret;
    int fd = user->fd;
    int count = 0;

    if (event & EPOLLIN) {
        while (count < 10) {
            ++count;
            ret = recv(fd, buf, 2000, MSG_DONTWAIT);
            if (ret < 0 && (errno == EAGAIN || errno == EINTR)) {
                return;
            } else if (ret <= 0) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                free(user);
                break;
            } else {
                //printf("fd %d recv msg len %lu \n", fd, ret);
            }
        }
    } else if (event & (EPOLLHUP | EPOLLERR)) {
        printf("err occur %d\n", fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        free(user);
    }
    return;
}

void server_accept(struct userdata *user, int events) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int fd = user->fd;

    int child = accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (child == -1) {
        printf("accept failed %s\n", strerror(errno));
        return;
    } else {
        user_data_t *user_client = malloc(sizeof(user_data_t));
        if (NULL == user_client) {
            printf("malloc failed %s\n", strerror(errno));
            return;
        }
        user_client->fd = child;
        user_client->proc = server_recv;

        struct epoll_event ev;
        ev.data.ptr = user_client;
        ev.events = EPOLLIN|EPOLLERR|EPOLLHUP;
        int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, child, &ev);
        if (ret == -1) {
            printf("epoll_ctl failed %s\n", strerror(errno));
            close(child);
            free(user_client);
        } else {
            printf("accept a new client %d\n", child);
        }
    }

    return;
}

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
    if (bind(fd, (struct sockaddr *)&stAddr, sizeof(stAddr)) == -1) {
        printf("bind failed %d\n", errno);
        return -1;
    }

    if (listen(fd, 10) == -1) {
        printf("listen failed %d\n", errno);
        return -1;
    }

    epfd = epoll_create(1);
    if (epfd == -1) {
        printf("epoll_create failed %d\n", errno);
        return -1;
    }

    int nfds;
    struct epoll_event ev,events;
    int ret = -1;

    user_data_t *user = malloc(sizeof(user_data_t));
    if (NULL == user) {
        printf("malloc failed %s\n", strerror(errno));
        return -1;
    }
    user->fd = fd;
    user->proc = server_accept;

    ev.data.ptr = user;
    ev.events = EPOLLIN|EPOLLERR|EPOLLHUP;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

    while (1) {
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

