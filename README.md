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
     
     即:`server_sock = startup(&port);`//如果此时port为0的就需要动态创建套接字
     ① 调用socket()打开一个网络通讯端口（文件描述符）。
     ② 调用setsockopt()设置端口复用。
     ③ 服务器调用bind()绑定一个固定的网络地址和端口号。
     ④ 如果端口号为0，调用getsockname()获取一个本地端口。
     ⑤ 调用listen()进行监听。
     
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

     （2）服务器调用accept()接受连接，accept()返回时传出客户端的地址和端口号。
```c
client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
``` 
     
     （3）循环创建新线程用accept_request()函数处理请求
```c
pthread_create(&newthread, NULL, accept_request, (void *)&client_sock)`
//newthread传出参数，保存系统为我们分配好的线程ID,client_sock为向线程函数accept_request传递的参数。
```
     
     （4）调用get_line()解析一行http报文
     
     `numchars = get_line(client, buf, sizeof(buf));`
     
     这里需要了解http协议的格式：
     
     http协议分成两个大的部分，一个是请求，一个是相应。无论是请求还是相应都包含两个部分，一个是header，另外一个是body。（body可选）
     每个Header一行一个，换行符是\r\n。当连续遇到两个\r\n时，Header部分结束，后面的数据全部是Body。
     Body的数据类型由Content-Type头来确定，如果是网页，Body就是文本，如果是图片，Body就是图片的二进制数据。
     
     如下图（[来源于](https://www.jianshu.com/p/f5a5db039737)）：
     
     ![image](https://user-images.githubusercontent.com/81791654/167098578-c9c13d6e-fbb4-4eac-9f24-5d07fbba9ce3.png)

     具体为：得到一行数据,只要读到为 `\n`时 ,就认为是一行结束，如果读到 `\r`时 ,再用 `MSG_PEEK` 的方式读入一个字符，如果是 `\n`，从 `socket` 用读出

```c
//解析一行http报文
int get_line(int sock, char *buf, int size)
{
	int i = 0;//初始化起点
	char c = '\0';//
	int n;//用于记录成功读取的字符数目

	while ((i < size - 1) && (c != '\n'))
	{
		//recv 的 flag 设为0，此时的recv函数读取tcp buffer中的数据到buf中
		n = recv(sock, &c, 1, 0); //这里是一个字符一个字符的向外读
		if (n>0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;//依次向后读
			i++;
		}
		else
			/*没有成功接收到字符，以换行符结尾，结束循环*/
			c = '\n';
	}
	/*以空字符结尾，作为字符串*/
	buf[i] = '\0';
	return (i);
}
```
     
         
```c
//处理从套接字上监听到的一个 HTTP 请求
void *accept_request(void *tclient)
{
	int client = *(int *)tclient;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0;
	char* query_string = NULL;
	numchars = get_line(client, buf, sizeof(buf));
	//读取一行http请求到buf中
	i = 0;
	j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';//给method增加一个结尾
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(client);
		return NULL;
	}

	//如果是post激活cgi
	if (strcasecmp(method, "POST") == 0)
		cgi = 1;

	//不管get还是post，都要进行读取url地址
	i = 0;//处理空字符
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
	//继续读取request-URL
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;
		if (*query_string == '?')//带?需要激活cgi
		{
			cgi = 1;//需要执行cgi，解析参数，设置标志位为1
			//将解析参数截取下来
			*query_string = '\0';//这里变成url结尾
			query_string++;//query_string作为剩下的起点
		}
	}
	//以上已经将起始行解析完毕

	//url中的路径格式化到path
	sprintf(path, "httpdocs%s", url);//把url添加到htdocs后赋值给path
	if (path[strlen(path) - 1] == '/')//如果是以/结尾，需要把index.html添加到后面
		//以/结尾，说明path只是一个目录，此时需要设置为首页index.html
		strcat(path, "test.html");//strcat连接两个字符串
	if (stat(path, &st) == -1)//执行成功则返回0，失败返回-1
	{//如果不存在，那么读取剩下的请求行内容丢弃
		while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
			numchars = get_line(client, buf, sizeof(buf));
		not_found(client);//调用错误代码404
	}
	else
	{
		if ((st.st_mode & S_IFMT) == S_IFDIR)//mode_t     st_mode;表示文件对应的模式，文件，目录等，做与运算能得到结果
			strcat(path, "/test.html");//如果是目录，显示index.html这里和前面是否重复？
		if ((st.st_mode & S_IXUSR) ||
			(st.st_mode & S_IXGRP) ||
			(st.st_mode & S_IXOTH))//IXUSR X可执行，R读，W写，USR用户,GRP用户组，OTH其他用户
			cgi = 1;

		if (!cgi)//如果不是cgi,直接返回
			serve_file(client, path);
		else
			execute_cgi(client, path, method, query_string);//是的话，执行cgi
	}

	close(client);
	return NULL;
}
```
     

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
