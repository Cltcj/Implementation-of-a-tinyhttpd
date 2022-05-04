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

```c
struct sockaddr 
{    sa_family_t sa_family;    /* address family, AF_xxx */ 
     char sa_data[14];        /* 14 bytes of protocol address */ 
};
```














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
