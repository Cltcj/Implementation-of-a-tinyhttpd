#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ISspace(x) isspace((int)(x))
//函数说明：检查参数c是否为空格字符，
//也就是判断是否为空格(' ')、定位字符(' \t ')、CR(' \r ')、换行(' \n ')、垂直定位字符(' \v ')或翻页(' \f ')的情况。
//返回值：若参数c 为空白字符，则返回非 0，否则返回 0。

#define SERVER_STRING "Server: ChiLvting's http/0.1.0\r\n"//定义个人server名称

void *accept_request(void* client);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);


/*****************************主函数，也就是函数入口*****************************************/

int main(void)
{
	int server_sock = -1;//服务器套接字
	u_short port = 8888;//端口号
	int client_sock = -1;//客户端套接字
	struct sockaddr_in client_name;//服务器地址
	/*int client_name_len = sizeof(client_name);*/
	socklen_t client_name_len = sizeof(client_name);
	pthread_t newthread;//线程

	server_sock = startup(&port);//动态创建套接字
	printf("http server_sock is %d\n", server_sock);
	printf("http running on port %d\n", port);

	while (1)
	{
		client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);

		printf("New connection....  ip: %s , port: %d\n", inet_ntoa(client_name.sin_addr), ntohs(client_name.sin_port));
		if (client_sock == -1)
			error_die("accept");

		/*if (ptheead_create(&newpthread, NULL, accept_request, client_sock) != 0)*/
		if (pthread_create(&newthread, NULL, accept_request, (void *)&client_sock) != 0)
			perror("ptheead_create");
	}	
	close(server_sock);//关闭套接字
	return 0;
}

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
		//那么调用connect连接成功后，使用getsockname可以正确获得当前正在通信的socket的IP和端口地址。?
		*port = ntohs(name.sin_port);//转换端口号
	}
	if (listen(httpd, 5) < 0)
		error_die("listen");
	return (httpd);

}

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
	/*
	int stat(
	　　const char *filename    //文件或者文件夹的路径
	  　　, struct stat *buf      //获取的信息保存在内存中
		); 也就是通过提供文件描述符获取文件属性
		stat结构体的元素
		struct stat {
		mode_t     st_mode;       //文件对应的模式，文件，目录等
		ino_t      st_ino;        //inode节点号
		dev_t      st_dev;        //设备号码
		dev_t      st_rdev;       //特殊设备号码
		nlink_t    st_nlink;      //文件的连接数
		uid_t      st_uid;        //文件所有者
		gid_t      st_gid;        //文件所有者对应的组
		off_t      st_size;       //普通文件，对应的文件字节数
		time_t     st_atime;      //文件最后被访问的时间
		time_t     st_mtime;      //文件内容最后被修改的时间
		time_t     st_ctime;      //文件状态改变时间
		blksize_t  st_blksize;    //文件内容对应的块大小
		blkcnt_t   st_blocks;     //伟建内容对应的块数量
		};
		*/
	int cgi = 0;
	char* query_string = NULL;
	numchars = get_line(client, buf, sizeof(buf));
	//读取一行http请求到buf中
	i = 0;
	j = 0;
	//对于HTTP报文来说，第一行的内容即为报文的起始行，格式为<method> <request-URL> <version>
	//复制buf到method中，但是不能超过method大小255个
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';//给method增加一个结尾
	//strcasecmp函数忽略大小写的比较字符串函数。
	//若参数s1和s2字符串相等则返回0。s1大于s2则返回大于0 的值，s1 小于s2 则返回小于0的值
	//此函数只在Linux中提供，相当于windows平台的 stricmp。
	//如果既不是get也不是post，服务器无法处理
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

	//如果是GET可能要把url分成两段
	//get请求可能带有？表示查询参数
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
		/*掩码
		#define _S_IFMT 0xF000
		#define _S_IFDIR 0x4000
		#define _S_IFCHR 0x2000
		#define _S_IFIFO 0x1000
		#define _S_IFREG 0x8000
		#define _S_IREAD 0x0100
		#define _S_IWRITE 0x0080
		#define _S_IEXEC 0x0040
		*/
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


void serve_file(int client, const char *filename)//调用 cat 把服务器文件内容返回给浏览器客户端。
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];
	buf[0] = 'A'; buf[1] = '\0';
	while ((numchars > 0) && strcmp("\n", buf))//读取HTTP请求头并丢弃
		numchars = get_line(client, buf, sizeof(buf));
	resource = fopen(filename, "r");//只读打开
	if (resource == NULL)
		//如果文件不存在，则返回not_found
		not_found(client);
	else
	{
		//添加HTTP头
		headers(client, filename);
		//并发送文件内容
		cat(client, resource);
	}
	fclose(resource);//关闭文件句柄
}


void execute_cgi(int client, const char *path,const char *method,const char *query_string)
{
	char buf[1024];
	int cgi_output[2];
	int cgi_input[2];
	pid_t pid;
	int status;
	int i;
	char c;
	int numchars = 1;
	int content_length = -1;
	
	buf[0] = 'A';
	buf[1] = '\0';
	//保证进入
	if (strcasecmp(method, "GET") == 0)//get，直接丢弃?
	{
		while (numchars > 0 && strcmp("\n", buf)) {
			numchars = get_line(client, buf, sizeof(buf));
		}
	}
	else {
		numchars = get_line(client, buf, sizeof(buf));
		while (numchars > 0 && strcmp("\n", buf)) {
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length") == 0) {
				content_length = atoi(&(buf[16]));//找到content-length长度
			}
			numchars = get_line(client, buf, sizeof(buf));
		}
		if (content_length == -1) {
			bad_request(client);
			return;
		}
	}
	//写
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	//#include<unistd.h>
	//int pipe(int filedes[2]);
	//返回值：成功，返回0，否则返回-1。参数数组包含pipe使用的两个文件的描述符。fd[0]:读管道，fd[1]:写管道。
	//必须在fork()中调用pipe()，否则子进程不会继承文件描述符。
	//两个进程不共享祖先进程，就不能使用pipe。但是可以使用命名管道。

	if (pipe(cgi_output) < 0) {
		cannot_execute(client);//500
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}
	//成功建立管道
	if ((pid = fork()) < 0) {
		//子进程创建失败
		cannot_execute(client);
		return;
	}
	//进入子进程
	if (pid == 0) {
		char meth_env[255];//设置request_method 的环境变量
		char query_env[255];//GET 的话设置 query_string 的环境变量
		char length_env[255];//POST 的话设置 content_length 的环境变量
		//标准输入文件的文件描述符：0
		//标准输出文件的文件描述符：1
		//标准错误输出的文件描述符：2

		//dup2创建一个newfd，newfd指向oldfd的位置
		dup2(cgi_output[1], 1);//这就是将标准输出重定向到output管道的写入端，也就是输出内容将会输出到output写入
		dup2(cgi_input[0], 0);//将标准输入重定向到input读取端，也就是将从input[0]读内容到input缓冲
		close(cgi_output[0]);//关闭output管道的的读取端
		close(cgi_input[1]);//关闭input管道的写入端
		sprintf(meth_env, "REQUEST_METHOD=%s", method);//把method保存到环境变量中
		//函数说明：int putenv(const char * string)用来改变或增加环境变量的内容.
		//如果该环境变量原先存在, 则变量内容会依参数string 改变, 否则此参数内容会成为新的环境变量.
		//返回值：执行成功则返回0, 有错误发生则返回-1.
		putenv(meth_env);
		if (strcasecmp(method, "GET") == 0){
			sprintf(query_env, "QUERY_STRING=%s", query_string);//存储query_string到query_env
			putenv(query_env);
		}
		else {
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);//存储content_length到length_env
			putenv(length_env);
		}
		// 表头文件#include<unistd.h>
		// int execl(const char * path,const char * arg,....);
		// 函数说明:  execl()用来执行参数path字符串所代表的文件路径，接下来的参数代表执行该文件时传递过去的argv(0)、argv[1]……，最后一个参数必须用空指针(NULL)作结束。
		// 返回值:    如果执行成功则函数不会返回，执行失败则直接返回-1
		execl(path, path, NULL);
	}
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
		//定义函数：pid_t waitpid(pid_t pid, int * status, int options);
		//函数说明：wait()会暂时停止目前进程的执行, 直到有信号来到或子进程结束. 
		//waitpid不会阻塞
		//如果不在意结束状态值, 则参数status 可以设成NULL. 参数pid 为欲等待的子进程识别码, 其他数值意义如下：
		//1、pid<-1 等待一个指定进程组中的任何子进程，这个进程组的ID等于pid的绝对值
		//2、pid=-1 等待任何子进程, 相当于wait().
		//3、pid=0 等待进程组识别码与目前进程相同的任何子进程.
		//4、pid>0 等待任何子进程识别码为pid 的子进程.
	}

}
//生成返回消息头
void headers(int client, const char *filename)
{
	char buf[1024];
	(void)filename;//使用文件名确定文件类型
	//发送HTTP头
	strcpy(buf, "HTTP/1.0 200 OK\r\n");//200码
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


//复制给客户端文件
void cat(int client, FILE *resource)
{
	//发送文件的内容
	char buf[1024];
	//读取文件到buf中
	fgets(buf, sizeof(buf), resource);
	while (!feof(resource))//判断文件是否读取到末尾
	{
		//读取并发送文件内容
		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}

//解析一行http报文
int get_line(int sock, char *buf, int size)
{
	int i = 0;//初始化起点
	char c = '\0';//
	int n;//用于记录成功读取的字符数目

	while ((i < size - 1) && (c != '\n'))
	{
		/*
		recv 的 flag 设为0
		此时的recv函数读取tcp buffer中的数据到buf中

		与recv函数对应的send()作用是
		把应用层buffer的数据拷贝进socket的内核中的接收缓冲区，从而把数据缓存入内核.

		(发送是TCP的事情).

		等到recv()读取时,就把内核缓存区中的数据拷贝进应用层
		用户的buffer里面,并返回.

		若应用进程一直没有调用recv()进行读取时
		数据会一直缓存在相应的socket的接收缓冲区内
		缓冲区满后，收端会通知发端，接收窗口关闭
		并从tcp buffer中移除已读取的数据

		这里是一个字符一个字符的向外读
		*/
		n = recv(sock, &c, 1, 0); //这里是一个字符一个字符的向外读
		if (n>0)
		{
			if (c == '\r')
			{
				/*
					第一个参数指定接收端套接字描述符
					第二个参数指明一个缓冲区
					第三个参数指明buf的长度
				*/
				/*
				考虑server向client发送数据"_META_DATA_\r\n_USER_DATA_"
				把flags设置为MSG_PEEK时,仅把tcp buffer中的数据
				读取到buf中，并不把已读取的数据从tcp buffer中移除，
				再次调用recv仍可以读到刚才读到的数据
				*/
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					/*
					如果回车符(\r)的后面不是换行符(\n)
					或者读取失败
					就把当前读取的字符置为换行，从而终止循环
					*/
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

//输出具体报错信息
void error_die(const char *sc)
{
	perror(sc);//perror(s) 用来将上一个函数发生错误的原因输出到标准设备(stderr)
	//例如    
	/*fp=fopen("/root/noexitfile","r+");
	if(NULL==fp)
	{
	perror("/root/noexitfile");
	}
	将打印 /root/noexitfile: No such file or directory
	*/
	exit(1);
}

//400
void bad_request(int client)//错误请求代码400
{
	char buf[1024];

	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "<P>Your browser sent a bad request, ");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "such as a POST without a Content-Length.\r\n");
	send(client, buf, sizeof(buf), 0);
}

//500
void cannot_execute(int client)
{//错误代码500，服务器问题
	char buf[1024];

	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
	send(client, buf, strlen(buf), 0);
}

//404
void not_found(int client)
{//客户端错误404
	char buf[1024];

	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "is unavailable or nonexistent.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

//501
void unimplemented(int client)
{//回答不能执行的方法
	char buf[1024];
	//sprintf格式化输出字符串，printf的兄弟，区别在于spintf是输出到了目标缓冲区
	//状态行
	//http/1.1版本号 501错误代码 及原因 
	/*
	状态代码有三位数字组成，第一个数字定义了响应的类别，共分五种类别:
	1xx：指示信息--表示请求已接收，继续处理
	2xx：成功--表示请求已被成功接收、理解、接受
	3xx：重定向--要完成请求必须进行更进一步的操作
	4xx：客户端错误--请求有语法错误或请求无法实现
	5xx：服务器端错误--服务器未能实现合法的请求
	*/
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");//501对应 not implement
	send(client, buf, strlen(buf), 0);//发送

	//消息头:
	//服务器端名
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	//数据类型
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	//因为是错误，所以没有长度
	//空行
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);

	//消息体
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	//html格式，使用 /同名 表示结束
	//网页body里插入<p>标签，这个标签叫做段落标签，它是html中的一个标签
	//只要插入这个标签，文字就会自动换行并分段
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}
