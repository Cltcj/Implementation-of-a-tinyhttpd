# Implementation-of-a-tinyhttpd

&emsp;&emsp;这个项目很简单，它是对tinyhttpd的复现，以及改进

实现一个web服务器：

第一步：你得会写一个简单的HTML页面，以及关于 http 协议的一些知识、socket网络编程相关知识、进程通信

已经会写一些简单的HTML页面，且对 http 协议有了一些了解，那我们该如何搭建我们的服务器呢？

注意 http 只是应用层协议,我们仍然需要选择一个传输层的协议来完成我们的传输数据工作,这里我们的开发协议选择是 TCP+HTTP, 也就是说服务器搭建浏览依照 TCP,对数据进行解析和响应工作遵循 HTTP 的原则.

这样我们的思路很清晰了,编写一个 TCP 并发服务器,只不过收发消息的格式采用的是 HTTP 协议,如下图:

![image](https://user-images.githubusercontent.com/81791654/166148072-c0a21e3d-528d-4e4e-ba86-2a8ff0ec3aa1.png)










工作流程：

     （1） 服务器启动，在指定端口或随机选取端口绑定 httpd 服务。
     
     即:`server_sock = startup(&port);`//如果此时port是指定了且不为0的就调用startup()函数动态创建套接字
     
```c
//开启http服务，包括绑定端口，监听，开启线程处理链接
int startup(u_short *port)
{
	int httpd = 0, option;
	struct sockaddr_in name;//套接字地址
	//设置http socket
	httpd = socket(PF_INET, SOCK_STREAM, 0);//创建TCP套接字
	if (httpd == -1)//创建失败
		error_die("socket");

	socklen_t optlen;
	optlen = sizeof(option);
	option = 1;
	setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

	memset(&name, 0, sizeof(name));//初始化套接字地址
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		error_die("bind");	
	}
	if (*port == 0)//随机端口，动态分配
	{
		/*int namelen = sizeof(name);*/
		socklen_t namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)//获取一个套接字的本地接口
		{
			error_die("getsockname");
		}
		//那么调用connect连接成功后，使用getsockname可以正确获得当前正在通信的socket的IP和端口地址
		*port = ntohs(name.sin_port);//转换端口号
	}
	if (listen(httpd, 5) < 0)
		error_die("listen");
	return (httpd);
}
```

     （2）收到一个 HTTP 请求时（其实就是 listen 的端口 accpet 的时候），派生一个线程运行 accept_request 函数。

     （3）取出 HTTP 请求中的 method (GET 或 POST) 和 url,。对于 GET 方法，如果有携带参数，则 query_string 指针指向 url 中 ？ 后面的 GET 参数。

     （4） 格式化 url 到 path 数组，表示浏览器请求的服务器文件路径，在 tinyhttpd 中服务器文件是在 htdocs 文件夹下。当 url 以 / 结尾，或 url 是个目录，则默认在 path 中加上 index.html，表示访问主页。

     （5）如果文件路径合法，对于无参数的 GET 请求，直接输出服务器文件到浏览器，即用 HTTP 格式写到套接字上，跳到（10）。其他情况（带参数 GET，POST 方式，url 为可执行文件），则调用 excute_cgi 函数执行 cgi 脚本。

    （6）读取整个 HTTP 请求并丢弃，如果是 POST 则找出 Content-Length. 把 HTTP 200  状态码写到套接字。

    （7） 建立两个管道，cgi_input 和 cgi_output, 并 fork 一个进程。

    （8） 在子进程中，把 STDOUT 重定向到 cgi_outputt 的写入端，把 STDIN 重定向到 cgi_input 的读取端，关闭 cgi_input 的写入端 和 cgi_output 的读取端，设置 request_method 的环境变量，GET 的话设置 query_string 的环境变量，POST 的话设置 content_length 的环境变量，这些环境变量都是为了给 cgi 脚本调用，接着用 execl 运行 cgi 程序。

    （9） 在父进程中，关闭 cgi_input 的读取端 和 cgi_output 的写入端，如果 POST 的话，把 POST 数据写入 cgi_input，已被重定向到 STDIN，读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT。接着关闭所有管道，等待子进程结束。这一部分比较乱，见下图说明：

![image](https://user-images.githubusercontent.com/81791654/166947375-dc108274-083e-4ee1-b131-400f95a177ba.png)





## 基础知识：

[1、Html介绍](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/Html)

[2、http 超文本传输协议](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/Html)

[3、socket 网络编程](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/socket%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B)
