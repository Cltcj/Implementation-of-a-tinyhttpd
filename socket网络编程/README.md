# socket 编程

## 网络字节序

&emsp;&emsp;内存中的多字节数据相对于内存地址有大端和小端之分，磁盘文件中的多字节数据相对于文件 中的偏移地址也有大端小端之分。网络数据流同样有大端小端之分，那么如何定义网络数据流的地址呢？发送主机 通常将发送缓冲区中的数据按内存地址从低到高的顺序发出，接收主机把从网络上接到的字节依次保存在接收缓冲 区中，也是按内存地址从低到高的顺序保存，因此，网络数据流的地址应这样规定：先发出的数据是低地址，后发 出的数据是高地址。

&emsp;&emsp;TCP/IP 协议规定，网络数据流应采用大端字节序，即低地址高字节。如果发送主机是小端字节序，或者接收主机如果是小端字节序的都需要进行字节序的转换。

&emsp;&emsp;为使网络程序具有可移植性，使同样的 C 代码在大端和小端计算机上编译后都能正常运行，可以调用以下库函 数做网络字节序和主机字节序的转换。

```c
#include <arpa/inet.h> 
uint32_t htonl(uint32_t hostlong); 
uint16_t htons(uint16_t hostshort); 
uint32_t ntohl(uint32_t netlong); 
uint16_t ntohs(uint16_t netshort);
```
其中：h 表示 host，n 表示 network，l 表示 32 位长整数，s 表示 16 位短整数。

## IP 地址转换函数

```c
#include <arpa/inet.h> 
int inet_pton(int af, const char *src, void *dst); 
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size); 
```
支持 IPv4 和 IPv6 
可重入函数 其中 inet_pton 和 inet_ntop 不仅可以转换 IPv4 的 in_addr，还可以转换 IPv6 的 in6_addr。 
因此函数接口是 `void *addrptr`。

## sockaddr 数据结构

&emsp;&emsp;strcut sockaddr 很多网络编程函数诞生早于 IPv4 协议，那时候都使用的是 sockaddr 结构体,为了向前兼容，现 在 sockaddr 退化成了（`void *`）的作用，传递一个地址给函数，至于这个函数是 sockaddr_in 还是 sockaddr_in6，由 地址族确定，然后函数内部再强制类型转化为所需的地址类型。

![image](https://user-images.githubusercontent.com/81791654/166858961-bc25a435-471f-4ccc-86be-1c4282567115.png)


```c
struct sockaddr 
{    sa_family_t sa_family;    /* address family, AF_xxx */ 
     char sa_data[14];        /* 14 bytes of protocol address */ 
};
```

&emsp;&emsp;使用 sudo grep -r "struct sockaddr_in {" /usr 命令可查看到 struct sockaddr_in 结构体的定义。一般其默认的存 储位置：/usr/include/linux/in.h 文件中

```c
struct sockaddr_in { 
     __kernel_sa_family_t sin_family; /* Address family */ 地址结构类型 
     __be16 sin_port; /* Port number */ 端口号 
     struct in_addr sin_addr; /* Internet address */ IP 地址 
     /* Pad to size of `struct sockaddr'. */ 
     unsigned char __pad[__SOCK_SIZE__ - sizeof(short int) - 
     sizeof(unsigned short int) - sizeof(struct in_addr)]; 
};
struct in_addr { /* Internet address. */ 
     __be32 s_addr; 
};

struct sockaddr_in6 { 
     unsigned short int sin6_family; /* AF_INET6 */ 
     __be16 sin6_port; /* Transport layer port # */ 
     __be32 sin6_flowinfo; /* IPv6 flow information */ 
     struct in6_addr sin6_addr; /* IPv6 address */ 
     __u32 sin6_scope_id; /* scope id (new in RFC2553) */ 
};

struct in6_addr { 
     union { 
          __u8 u6_addr8[16]; 
          __be16 u6_addr16[8]; 
          __be32 u6_addr32[4]; 
     } in6_u; 
     #define s6_addr     in6_u.u6_addr8 
     #define s6_addr16   in6_u.u6_addr16 
     #define s6_addr32   in6_u.u6_addr32 
};

#define UNIX_PATH_MAX 108 
struct sockaddr_un { 
     __kernel_sa_family_t sun_family; /* AF_UNIX */ 
     char sun_path[UNIX_PATH_MAX]; /* pathname */ 
};

```

&emsp;&emsp;IPv4 和 IPv6 的地址格式定义在 netinet/in.h 中，IPv4 地址用 sockaddr_in 结构体表示，包括 16 位端口号和 32 位 IP 地址，IPv6 地址用 sockaddr_in6 结构体表示，包括 16 位端口号、128 位 IP 地址和一些控制字段。UNIX Domain Socket 的地址格式定义在 sys/un.h 中，用 sock-addr_un 结构体表示。各种 socket 地址结构体的开头都是相同的，前 16 位 表示整个结构体的长度（并不是所有 UNIX 的实现都有长度字段，如 Linux 就没有），后 16 位表示地址类型。IPv4、 IPv6 和 Unix Domain Socket 的地址类型分别定义为常数 AF_INET、AF_INET6、AF_UNIX。这样，只要取得某种 sockaddr 结构体的首地址，不需要知道具体是哪种类型的 sockaddr 结构体，就可以根据地址类型字段确定结构体中的内容。 因此，socket API 可以接受各种类型的 sockaddr 结构体指针做参数，例如 bind、accept、connect 等函数，这些函数 的参数应该设计成 `void *`类型以便接受各种类型的指针，但是 **sock API 的实现早于 ANSI C 标准化，那时还没有 `void *` 类型**，**因此这些函数的参数都用 `struct sockaddr *`类型表示，在传递参数之前要强制类型转换一下**，例如：

```c
struct sockaddr_in servaddr; 
bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)); /* initialize servaddr */
```

## 网络套接字函数

### socket模型创建流程图

![image](https://user-images.githubusercontent.com/81791654/166859059-a70a3901-c6f5-4d1d-81ba-2d30ad5f861f.png)

### socket函数 

```c
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
domain:
	AF_INET 这是大多数用来产生socket的协议，使用TCP或UDP来传输，用IPv4的地址
	AF_INET6 与上面类似，不过是来用IPv6的地址
	AF_UNIX 本地协议，使用在Unix和Linux系统上，一般都是当客户端和服务器在同一台及其上的时候使用
type:
	SOCK_STREAM 这个协议是按照顺序的、可靠的、数据完整的基于字节流的连接。这是一个使用最多的socket类型，这个socket是使用TCP来进行传输。
	SOCK_DGRAM 这个协议是无连接的、固定长度的传输调用。该协议是不可靠的，使用UDP来进行它的连接。
	SOCK_SEQPACKET该协议是双线路的、可靠的连接，发送固定长度的数据包进行传输。必须把这个包完整的接受才能进行读取。
	SOCK_RAW socket类型提供单一的网络访问，这个socket类型使用ICMP公共协议。（ping、traceroute使用该协议）
	SOCK_RDM 这个类型是很少使用的，在大部分的操作系统上没有实现，它是提供给数据链路层使用，不保证数据包的顺序
protocol:
	传0 表示使用默认协议。
返回值：
	成功：返回指向新创建的socket的文件描述符，失败：返回-1，设置errno
```

&emsp;&emsp;socket()打开一个网络通讯端口，如果成功的话，就像open()一样返回一个文件描述符，应用程序可以像读写文件一样用read/write在网络上收发数据，如果socket()调用出错则返回-1。对于IPv4，domain参数指定为AF_INET。对于TCP协议，type参数指定为SOCK_STREAM，表示面向流的传输协议。如果是UDP协议，则type参数指定为SOCK_DGRAM，表示面向数据报的传输协议。protocol参数的介绍从略，指定为0即可。

### bind函数

```c
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
sockfd：
	socket文件描述符
addr:
	构造出IP地址加端口号
addrlen:
	sizeof(addr)长度
返回值：
	成功返回0，失败返回-1, 设置errno
```

&emsp;&emsp;服务器程序所监听的网络地址和端口号通常是固定不变的，客户端程序得知服务器程序的地址和端口号后就可以向服务器发起连接，因此服务器需要调用bind绑定一个固定的网络地址和端口号。

&emsp;&emsp;bind()的作用是将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件描述符监听addr所描述的地址和端口号。前面讲过，`struct sockaddr *`是一个通用指针类型，addr参数实际上可以接受多种协议的sockaddr结构体，而它们的长度各不相同，所以需要第三个参数addrlen指定结构体的长度。如：

```c
struct sockaddr_in servaddr;
bzero(&servaddr, sizeof(servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
servaddr.sin_port = htons(6666);
```

```c
void bzero(void *s, int n);
#bzero()将参数s 所指的内存区域前n 个字节全部设为零。
```

&emsp;&emsp;首先将整个结构体清零，然后设置地址类型为AF_INET，网络地址为INADDR_ANY，这个宏表示本地的任意IP地址，因为服务器可能有多个网卡，每个网卡也可能绑定多个IP地址，这样设置可以在所有的IP地址上监听，直到与某个客户端建立了连接时才确定下来到底用哪个IP地址，端口号为6666。

### listen函数

```c
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
int listen(int sockfd, int backlog);
sockfd:
	socket文件描述符
backlog:
	排队建立3次握手队列和刚刚建立3次握手队列的链接数和
```

查看系统默认backlog：`cat /proc/sys/net/ipv4/tcp_max_syn_backlog`

&emsp;&emsp;典型的服务器程序可以同时服务于多个客户端，当有客户端发起连接时，服务器调用的accept()返回并接受这个连接，如果有大量的客户端发起连接而服务器来不及处理，尚未accept的客户端就处于连接等待状态，listen()声明sockfd处于监听状态，并且最多允许有backlog个客户端处于连接待状态，如果接收到更多的连接请求就忽略。listen()成功返回0，失败返回-1。

**相关知识补充：**

1、线程创建和结束
背景知识：
&emsp;&emsp;在一个文件内的多个函数通常通常都是按照main函数中出现的顺序来执行，但是在分时系统下，我们可以让每个函数都作为一个逻辑流并发执行，最简单的方式就是采用多线程策略。在main函数中调用多线程接口创建接口创建线程，每个线程对应特定的函数（操作），这样就可以不按照main函数中各个函数出现的顺序来执行，避免了忙等的情况。线程基本操作的接口如下：

**相关接口：**

**创建线程 - pthread_create**
```c
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
```
* 函数参数：
`pthread_t`：传出参数，保存系统为我们分配好的线程`ID`，当前`Linux`中可理解为：`typedef unsigned long int pthread_t`。
`attr`：通常传`NULL`，表示使用线程默认属性。若想使用具体属性也可以修改该参数。
`start_routine`：函数指针，指向线程主函数(线程体)，该函数运行结束，则线程结束。
`arg`：线程主函数执行期间所使用的参数。

**获取线程ID - pthread_self**
```c
#include <pthread.h>
pthread_t pthread_self(void);//调用时，会打印线程ID
```

**等待线程结束 - pthread_join**
阻塞等待线程退出，获取线程退出状态。其作用，对应进程中的waitpid() 函数。
```c
#include <pthread.h>
int pthread_join(pthread_t thread, void **retval);
```

**结束线程 - pthread_exit**

```c
void pthread_exit(void *retval);
```
&emsp;&emsp;在线程中禁止调用 **exit** 函数，否则会导致整个进程退出，取而代之的是调用 **pthread_exit** 函数，这个函数是使一个线程退出，如果主线程调用 **pthread_exit** 函数也不会使整个进程退出，不影响其他线程的执行。

**NOTE：pthread_exit或者return返回的指针所指向的内存单元必须是全局的或者是用malloc分配的，不能在线程函数的栈上分配，因为当其它线程得到这个返回指针时线程函数已经退出了，栈空间就会被回收。**

