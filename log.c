#include <stdarg.h>    
#include <time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>

#define SERVER_IP   "127.0.0.1"
#define BUF_SIZE    2000
#define DEFAULT_PORT    12345

void sendlog(unsigned short port, const char *fmt, ...)
{
    char buf[BUF_SIZE];
    static int fd = -1;

    if (fd == -1) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (port == 0) {
        port = DEFAULT_PORT;
    }

    va_list args;
    char time_data[16];
    struct timespec stTime;
    struct tm *lt;
    int offset;

    clock_gettime(CLOCK_REALTIME, &stTime);
    lt = localtime(&stTime.tv_sec);
    time_data[strftime(time_data, 16, "%H:%M:%S", lt)] = '\0';
    
    offset = snprintf(buf, BUF_SIZE, "%s.%09lu:", time_data, stTime.tv_nsec);

    va_start(args, fmt);
    offset += vsnprintf(buf+offset, BUF_SIZE-offset, fmt, args);
    va_end(args);
    
    if ((offset+1) > (BUF_SIZE-1)) {
        offset = BUF_SIZE-2;
    }
    buf[offset] = '\n';
    buf[offset+1] = 0;

    if (fd != -1) {
        struct sockaddr_in stAddr;

        memset(&stAddr, 0, sizeof(stAddr));
        stAddr.sin_family = AF_INET;
        stAddr.sin_port = htons(port);
        stAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        sendto(fd, buf, offset+2, MSG_DONTWAIT, (struct sockaddr *)&stAddr, sizeof(stAddr));
    }

    return;
}

#define FILE_PATH  "/mnt/sdcard/log"
static int write_log_mode = 1; //0- printf

void writelog(const char *fmt, ...)
{
    va_list args;
    char time_data[16];
    struct timespec stTime;
    struct tm *lt;
    int offset;
    static FILE *fp = NULL;
    
    if (NULL == fp) {
        if (write_log_mode == 0) {
            fp = stderr;
        } else {
            char path[100];
            char time[16]; 
            struct timespec stTime;
            clock_gettime(CLOCK_REALTIME, &stTime);
            struct tm *lt = localtime(&stTime.tv_sec);
            time[strftime(time, sizeof(time), "%H-%M-%S", lt)] = '\0';
            snprintf(path, 100, "%s-%s.txt", FILE_PATH, time);
            fp = fopen(path, "a+");
        }
        //printf("send thread fopen ret = %p, reason:%s\n", fp, strerror(errno));

        //fprintf(fp, "%s\n", "time");          
        //fflush(fp);
    }   

    if (NULL != fp) { 
        clock_gettime(CLOCK_REALTIME, &stTime);
        lt = localtime(&stTime.tv_sec);
        time_data[strftime(time_data, 16, "%H:%M:%S", lt)] = '\0';
    
        fprintf(fp, "%s.%09lu:", time_data, stTime.tv_nsec);

        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
        fprintf(fp, "\n");
    } 

    return;
}

#define LOG_BUF_SIZE   200
#define LOG_COUNT_MAX  2000
#define LOG_PRINT_SIGNAL_NUM SIGUSR1

typedef struct log_node {
    struct log_node *next;
    char buf[LOG_BUF_SIZE];
}log_node_t;

log_node_t *log_head = NULL;
log_node_t *log_end = NULL;
int log_count = 0;
int log_init = 0;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void printCacheLog(int sig) {
    log_node_t *curr;
    
    printf("* * * * * * * * * * * *\n");
    pthread_mutex_lock(&log_lock);
    curr = log_head;
    while (curr) {
        printf("%s", curr->buf);
        curr = curr->next;
    }
    pthread_mutex_unlock(&log_lock);
    
    return;
}

void cachelog(const char *fmt, ...) {
    va_list args;
    char time_data[16];
    struct timespec stTime;
    struct tm *lt;
    int offset;
    log_node_t *node = NULL;
    
    pthread_mutex_lock(&log_lock);
    if (log_init == 0) {
        signal(LOG_PRINT_SIGNAL_NUM, printCacheLog);
        log_init = 1;
    }
    if (log_count == LOG_COUNT_MAX) {
        node = log_head;
        log_head = log_head->next;
        --log_count;
    }
    pthread_mutex_unlock(&log_lock);
    if (node == NULL) {
        node = (log_node_t *)malloc(sizeof(log_node_t));
        if (NULL == node) {
            return;
        }
    }
    
    clock_gettime(CLOCK_REALTIME, &stTime);
    lt = localtime(&stTime.tv_sec);
    time_data[strftime(time_data, 16, "%H:%M:%S", lt)] = '\0';
    
    offset = snprintf(node->buf, LOG_BUF_SIZE, "%s.%09lu:", time_data, stTime.tv_nsec);

    va_start(args, fmt);
    offset += vsnprintf(node->buf+offset, LOG_BUF_SIZE-offset, fmt, args);
    va_end(args);
    
    if ((offset+1) > (LOG_BUF_SIZE-1)) {
        offset = LOG_BUF_SIZE-2;
    }
    node->buf[offset] = '\n';
    node->buf[offset+1] = 0;
    
    pthread_mutex_lock(&log_lock);
    if (log_count == 0) {
        log_head = node;
        log_end = node;
    } else {
        log_end->next = node;
        log_end = node;
    }
    node->next = NULL;
    ++log_count;
    pthread_mutex_unlock(&log_lock);

    return;
}

#define BT_BUF_SIZE 100
//-rdynamic 使用gcc编译时需要带上这个选项
void logBackTrace(void) {
    int j, nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;
    
    nptrs = backtrace(buffer, BT_BUF_SIZE);
    //printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        return;
    }

    for (j = 0; j < nptrs; j++) {
        //cachelog(strings[j]);
        //sendlog(0, strings[j]);
        writelog(strings[j]);
    }

    free(strings);
}

int main() {
    int i = 1;
    char buf[4000];

    while (i < 3000) {
        memset(buf, (i%10)+48, i);
        buf[i+1] = 0;

        sendlog(0, "%d--%s", i, buf);
        //logBackTrace();
        i++;
    }

    while(1);
    
    return 0;
}


