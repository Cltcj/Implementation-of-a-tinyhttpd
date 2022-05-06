# Implementation-of-a-tinyhttpd

**tinyhttpd全流程图：**

![image](https://user-images.githubusercontent.com/81791654/166947375-dc108274-083e-4ee1-b131-400f95a177ba.png)

这个项目很简单，它是用来学习如何制作一个[tinyhttpd](https://sourceforge.net/projects/tinyhttpd/)，以及对tinyhttpd的改进

实现一个web服务器：

第一步：你得会写一个简单的HTML页面，以及关于 http 协议的一些知识、socket网络编程相关知识、进程通信

已经会写一些简单的HTML页面，且对 http 协议有了一些了解，那我们该如何搭建我们的服务器呢？

注意 http 只是应用层协议,我们仍然需要选择一个传输层的协议来完成我们的传输数据工作,这里我们的开发协议选择是 TCP+HTTP, 也就是说服务器搭建浏览依照 TCP,对数据进行解析和响应工作遵循 HTTP 的原则.

这样我们的思路很清晰了,编写一个 TCP 并发服务器,只不过收发消息的格式采用的是 HTTP 协议,如下图:

![image](https://user-images.githubusercontent.com/81791654/166148072-c0a21e3d-528d-4e4e-ba86-2a8ff0ec3aa1.png)


**具体流程：**

* 1.服务器启动，在指定端口或随机选取端口绑定 httpd 服务。
     
>即:`server_sock = startup(&port);`//如果此时port为0的就需要动态创建套接字

>① 调用socket()打开一个网络通讯端口（文件描述符）。

>② 调用setsockopt()设置端口复用。

>③ 服务器调用bind()绑定一个固定的网络地址和端口号。

>④ 如果端口号为0，调用getsockname()获取一个本地端口。

>⑤ 调用listen()进行监听。
     
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

* 2.服务器调用accept()接受连接，accept()返回时传出客户端的地址和端口号。

```c
client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
``` 
     
* 3.循环创建新线程用accept_request()函数处理请求

```c
pthread_create(&newthread, NULL, accept_request, (void *)&client_sock)`
//newthread传出参数，保存系统为我们分配好的线程ID,client_sock为向线程函数accept_request传递的参数。
```
     
* 4.调用get_line()解析一行http报文
     
`numchars = get_line(client, buf, sizeof(buf));`
     
这里需要了解http协议的格式：

http协议分成两个大的部分，一个是请求，一个是响应。无论是请求还是相应都包含两个部分，一个是header，另外一个是body。（body可选）

每个Header一行一个，换行符是\r\n。当连续遇到两个\r\n时，Header部分结束，后面的数据全部是Body。

Body的数据类型由Content-Type头来确定，如果是网页，Body就是文本，如果是图片，Body就是图片的二进制数据。

如下图（[来源于](https://www.jianshu.com/p/f5a5db039737)）：

![image](https://user-images.githubusercontent.com/81791654/167098578-c9c13d6e-fbb4-4eac-9f24-5d07fbba9ce3.png)

所以get_line()的功能是：得到一行数据,只要读到为 `\n`时 ,就认为是一行结束，如果读到 `\r`时 ,再用 `MSG_PEEK` 的方式读入一个字符，如果是 `\n`，从 `socket` 读出

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
				// 把flags设置为MSG_PEEK时,仅把tcp buffer中的数据读取到buf中，并不把已读取的数据从tcp buffer中移除。
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
//如果没有在缓存中找到换行符，则该字符串以null结尾，如果读取了终止符中的任何一个，字符串的最后一个字符将是换行符(\n)，并且字符串以null字符结束，所以在函数的最后`buf[i] = ‘\0’`
```


* 5.取出 HTTP 请求中的 method (GET 或 POST) 和 url,。对于 GET 方法，如果有携带参数，则 query_string 指针指向 url 中 ？ 后面的 GET 参数。   
         
```c

//接收客户端的http请求报文
//接收字符处理：提取空格字符前的字符，至多254个
while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
{
	method[i] = buf[j];//根据http请求报文格式，这里得到的是请求方法
	i++;
	j++;
}
method[i] = '\0';//给method增加一个结尾

//忽略大小写比较字符串，判断使用的是那种请求方法
if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
{
	unimplemented(client); //既不是POST也不是GET，告知客户端所请求的方法未能实现
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
	url[i] = buf[j];//得到URL
	i++;
	j++;
}
url[i] = '\0';

//如果方法是get
if (strcasecmp(method, "GET") == 0)
{
	query_string = url;// query_string 指针指向 url 中 ？ 后面的 GET 参数
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
```

* 6.格式化 url 到 path 数组，表示浏览器请求的服务器文件路径，在 tinyhttpd 中服务器文件是在 htdocs 文件夹下。当 url 以 / 结尾，或 url 是个目录，则默认在 path 中加上 test.html，表示访问主页。

```c
//url中的路径格式化到path
sprintf(path, "httpdocs%s", url);//把url添加到htdocs后赋值给path
if (path[strlen(path) - 1] == '/')//如果是以/结尾，需要把test.html添加到后面
	//以/结尾，说明path只是一个目录，此时需要设置为首页test.html
	strcat(path, "test.html");//strcat连接两个字符串
```


* 7.如果文件路径合法，对于无参数的 GET 请求，直接输出服务器文件到浏览器，即用 HTTP 格式写到套接字上，然后跳到（11）。其他情况（带参数 GET，POST 方式，url 为可执行文件），则调用 excute_cgi 函数执行 cgi 脚本。

```c
if (!cgi)//如果不是cgi,直接返回
	serve_file(client, path);//跳到11，结束
else
	execute_cgi(client, path, method, query_string);//执行cgi
```


* 8.执行CGI脚本-->读取整个 HTTP 请求并丢弃，如果是 POST 则找出 Content-Length. 把 HTTP 200  状态码写到套接字。

```c
if (strcasecmp(method, "GET") == 0)//get，一般用于获取/查询资源信息
{
	while (numchars > 0 && strcmp("\n", buf)) {//读取并丢弃头部信息
		numchars = get_line(client, buf, sizeof(buf));//从客户端读取
	}
}
else {//POST方法，一般用于更新资源信息
	numchars = get_line(client, buf, sizeof(buf));
	//获取HTTP消息实体的传输长度
	while (numchars > 0 && strcmp("\n", buf)) {//不为空，且不为换行符
		buf[15] = '\0';
		if (strcasecmp(buf, "Content-Length") == 0) {//如果是Content_Length字段
			content_length = atoi(&(buf[16]));//找到content-length长度用于描述HTTP消息实体
		}
		numchars = get_line(client, buf, sizeof(buf));
	}
	if (content_length == -1) {
		bad_request(client);//请求的页面数据为空，没有数据
		return;
	}
}
//写
sprintf(buf, "HTTP/1.0 200 OK\r\n");
send(client, buf, strlen(buf), 0);
```

* 9.建立两个管道，cgi_input 和 cgi_output,（fd[0]:读入端，fd[1]:写入端） 并 fork 一个进程。

```c
if (pipe(cgi_output) < 0) {
	cannot_execute(client);//500
	return;
}
if (pipe(cgi_input) < 0) {
	cannot_execute(client);
	return;
}
//成功建立管道

//fork子进程，这样创建了父子进程间的IPC(进程间通信)通道
if ((pid = fork()) < 0) {
	//子进程创建失败
	cannot_execute(client);
	return;
}
```


* 10.在子进程中，把 STDOUT 重定向到 cgi_output 的写入端，把 STDIN 重定向到 cgi_input 的读取端，关闭 cgi_input 的写入端 和 cgi_output 的读取端，设置 request_method 的环境变量，GET 的话设置 query_string 的环境变量，POST 的话设置 content_length 的环境变量，这些环境变量都是为了给 cgi 脚本调用，接着用 execl 运行 cgi 程序。

```c
//进入子进程
if (pid == 0) {
	char meth_env[255];//设置request_method 的环境变量
	char query_env[255];//GET 的话设置 query_string 的环境变量
	char length_env[255];//POST 的话设置 content_length 的环境变量
	dup2(cgi_output[1], 1);//这就是将标准输出重定向到output管道的写入端，也就是输出内容将会输出到output写入
	dup2(cgi_input[0], 0);//将标准输入重定向到input读取端，也就是将从input[0]读内容到input缓冲
	close(cgi_output[0]);//关闭output管道的的读取端
	close(cgi_input[1]);//关闭input管道的写入端
	sprintf(meth_env, "REQUEST_METHOD=%s", method);//把method保存到环境变量中
	putenv(meth_env);//putenv函数的作用是增加环境变量
	if (strcasecmp(method, "GET") == 0){//get
		//设置query_string的环境变量
		sprintf(query_env, "QUERY_STRING=%s", query_string);//存储query_string到query_env
		putenv(query_env);
	}
	else {//post
		//设置content_Length的环境变量
		sprintf(length_env, "CONTENT_LENGTH=%d", content_length);//存储content_length到length_env
		putenv(length_env);
	}
	execl(path, path, NULL);//exec函数簇，执行cgi脚本，获取cgi的标准输出作为相应内容发送给客户端
}
```

* 11.在父进程中，关闭 cgi_input 的读取端 和 cgi_output 的写入端，如果 POST 的话，把 POST 数据写入 cgi_input，已被重定向到 STDIN，读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT。接着关闭所有管道，等待子进程结束。具体如：浏览器和tinyhttpd交互过程图


![image](https://user-images.githubusercontent.com/81791654/167124868-3da08518-a0a7-40d7-9e14-2935e9e25d05.png)

```c
else {
	close(cgi_output[1]);	
	close(cgi_input[0]);
	if (strcasecmp(method, "POST") == 0) {
		for (i = 0; i < content_length; i++) {
			recv(client, &c, 1, 0);
			write(cgi_input[1], &c, 1);
		}
	}
	//依次发送给客户端
	while (read(cgi_output[0], &c, 1) > 0) {
		send(client, &c, 1, 0);
	}
	close(cgi_output[0]);//ouput的读
	close(cgi_input[1]);//关闭input的写
	waitpid(pid, &status, 0);//等待子进程中止
}
```

* 12.关闭与浏览器的连接。
	





## 基础知识：

[1、Html介绍](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/Html)

[2、http 超文本传输协议](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/Html)

[3、socket 网络编程](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/socket%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B)

[4、项目代码](https://github.com/Cltcj/Implementation-of-a-tinyhttpd/tree/main/tinyhttpd1.0)

