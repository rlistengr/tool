[TOC]

# tool

包含一些问题定位使用的基本工具代码
- 日志
  - 通过向一个udp套接字发送报文的方式记录日志
  - 通过写文件的方式记录日志
  - 通过记录在内存中的方式记录日志
- 调用栈的记录



## 通过udp套接字的方式记录日志

1. 使用函数sendlog，参数是端口号+fmt可变参数
2. 端口为0时，使用默认端口号12345；目的ip默认是127.0.0.1
3. 查看日志可以有两种方式
   - tcpdump抓包，后面可以通过wireshark follow这个udp流来查看日志
   - 起一个监听12345端口的udp服务，代码在log_server.c中，运行这个server即可

## 通过写文件的方式记录日志

1. 使用函数writelog，参数是fmt可变参数
2. 根据mode决定是直接printf或者写入文件，mode=0是printf，非0写文件
3. 文件路径有宏FILE_PATH决定

## 通过内存记录日志

1. 使用函数cachelog，参数是fmt可变参数
2. 最多存储最新的LOG_COUNT_MAX条日志，可以修改宏增大
3. 显示，通过向目标进行发送信号，默认使用信号SIGUSR1,即使用kill -10 目标进程id即可

## 调用栈记录

1. 使用接口logBackTrace，不需要参数
2. 记录的方式可以选择上诉任何一种，需要修改代码

## 使用通用接口
1. 使用函数logRecord，参数是fmt可变参数

2. 通过全局变量log_mode决定使用什么方式打印，分别是上面的四种方式

   \#define LOG_MODE_SENDUDP    1

   \#define LOG_MODE_WRITEFILE  2

   \#define LOG_MODE_PRINTF     3

   \#define LOG_MODE_CACHE      4

