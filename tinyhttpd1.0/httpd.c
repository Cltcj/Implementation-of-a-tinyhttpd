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
//����˵����������c�Ƿ�Ϊ�ո��ַ���
//Ҳ�����ж��Ƿ�Ϊ�ո�(' ')����λ�ַ�(' \t ')��CR(' \r ')������(' \n ')����ֱ��λ�ַ�(' \v ')��ҳ(' \f ')�������
//����ֵ��������c Ϊ�հ��ַ����򷵻ط� 0�����򷵻� 0��

#define SERVER_STRING "Server: ChiLvting's http/0.1.0\r\n"//�������server����

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


/*****************************��������Ҳ���Ǻ������*****************************************/

int main(void)
{
	int server_sock = -1;//�������׽���
	u_short port = 8888;//�˿ں�
	int client_sock = -1;//�ͻ����׽���
	struct sockaddr_in client_name;//��������ַ
	/*int client_name_len = sizeof(client_name);*/
	socklen_t client_name_len = sizeof(client_name);
	pthread_t newthread;//�߳�

	server_sock = startup(&port);//��̬�����׽���
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
	close(server_sock);//�ر��׽���
	return 0;
}

//����http���񣬰����󶨶˿ڣ������������̴߳�������
int startup(u_short *port)
{
	int httpd = 0, option;
	struct sockaddr_in name;//�׽��ֵ�ַ
	//����http socket
	httpd = socket(PF_INET, SOCK_STREAM, 0);//����TCP�׽���
	if (httpd == -1)//����ʧ��
		error_die("socket");

	socklen_t optlen;
	optlen = sizeof(option);
	option = 1;
	setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

	memset(&name, 0, sizeof(name));//��ʼ���׽��ֵ�ַ
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		error_die("bind");	
	}
	if (*port == 0)//����˿ڣ���̬����
	{
		/*int namelen = sizeof(name);*/
		socklen_t namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)//��ȡһ���׽��ֵı��ؽӿ�
		{
			error_die("getsockname");
		}
		//��ô����connect���ӳɹ���ʹ��getsockname������ȷ��õ�ǰ����ͨ�ŵ�socket��IP�Ͷ˿ڵ�ַ��?
		*port = ntohs(name.sin_port);//ת���˿ں�
	}
	if (listen(httpd, 5) < 0)
		error_die("listen");
	return (httpd);

}

//������׽����ϼ�������һ�� HTTP ����
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
	����const char *filename    //�ļ������ļ��е�·��
	  ����, struct stat *buf      //��ȡ����Ϣ�������ڴ���
		); Ҳ����ͨ���ṩ�ļ���������ȡ�ļ�����
		stat�ṹ���Ԫ��
		struct stat {
		mode_t     st_mode;       //�ļ���Ӧ��ģʽ���ļ���Ŀ¼��
		ino_t      st_ino;        //inode�ڵ��
		dev_t      st_dev;        //�豸����
		dev_t      st_rdev;       //�����豸����
		nlink_t    st_nlink;      //�ļ���������
		uid_t      st_uid;        //�ļ�������
		gid_t      st_gid;        //�ļ������߶�Ӧ����
		off_t      st_size;       //��ͨ�ļ�����Ӧ���ļ��ֽ���
		time_t     st_atime;      //�ļ���󱻷��ʵ�ʱ��
		time_t     st_mtime;      //�ļ���������޸ĵ�ʱ��
		time_t     st_ctime;      //�ļ�״̬�ı�ʱ��
		blksize_t  st_blksize;    //�ļ����ݶ�Ӧ�Ŀ��С
		blkcnt_t   st_blocks;     //ΰ�����ݶ�Ӧ�Ŀ�����
		};
		*/
	int cgi = 0;
	char* query_string = NULL;
	numchars = get_line(client, buf, sizeof(buf));
	//��ȡһ��http����buf��
	i = 0;
	j = 0;
	//����HTTP������˵����һ�е����ݼ�Ϊ���ĵ���ʼ�У���ʽΪ<method> <request-URL> <version>
	//����buf��method�У����ǲ��ܳ���method��С255��
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';//��method����һ����β
	//strcasecmp�������Դ�Сд�ıȽ��ַ���������
	//������s1��s2�ַ�������򷵻�0��s1����s2�򷵻ش���0 ��ֵ��s1 С��s2 �򷵻�С��0��ֵ
	//�˺���ֻ��Linux���ṩ���൱��windowsƽ̨�� stricmp��
	//����Ȳ���getҲ����post���������޷�����
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(client);
		return NULL;
	}

	//�����post����cgi
	if (strcasecmp(method, "POST") == 0)
		cgi = 1;

	//����get����post����Ҫ���ж�ȡurl��ַ
	i = 0;//������ַ�
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
	//������ȡrequest-URL
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	//�����GET����Ҫ��url�ֳ�����
	//get������ܴ��У���ʾ��ѯ����
	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;
		if (*query_string == '?')//��?��Ҫ����cgi
		{
			cgi = 1;//��Ҫִ��cgi���������������ñ�־λΪ1
			//������������ȡ����
			*query_string = '\0';//������url��β
			query_string++;//query_string��Ϊʣ�µ����
		}
	}
	//�����Ѿ�����ʼ�н������

	//url�е�·����ʽ����path
	sprintf(path, "httpdocs%s", url);//��url��ӵ�htdocs��ֵ��path
	if (path[strlen(path) - 1] == '/')//�������/��β����Ҫ��index.html��ӵ�����
		//��/��β��˵��pathֻ��һ��Ŀ¼����ʱ��Ҫ����Ϊ��ҳindex.html
		strcat(path, "test.html");//strcat���������ַ���
	if (stat(path, &st) == -1)//ִ�гɹ��򷵻�0��ʧ�ܷ���-1
	{//��������ڣ���ô��ȡʣ�µ����������ݶ���
		while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
			numchars = get_line(client, buf, sizeof(buf));
		not_found(client);//���ô������404
	}
	else
	{
		/*����
		#define _S_IFMT 0xF000
		#define _S_IFDIR 0x4000
		#define _S_IFCHR 0x2000
		#define _S_IFIFO 0x1000
		#define _S_IFREG 0x8000
		#define _S_IREAD 0x0100
		#define _S_IWRITE 0x0080
		#define _S_IEXEC 0x0040
		*/
		if ((st.st_mode & S_IFMT) == S_IFDIR)//mode_t     st_mode;��ʾ�ļ���Ӧ��ģʽ���ļ���Ŀ¼�ȣ����������ܵõ����
			strcat(path, "/test.html");//�����Ŀ¼����ʾindex.html�����ǰ���Ƿ��ظ���
		if ((st.st_mode & S_IXUSR) ||
			(st.st_mode & S_IXGRP) ||
			(st.st_mode & S_IXOTH))//IXUSR X��ִ�У�R����Wд��USR�û�,GRP�û��飬OTH�����û�
			cgi = 1;


		if (!cgi)//�������cgi,ֱ�ӷ���
			serve_file(client, path);
		else
			execute_cgi(client, path, method, query_string);//�ǵĻ���ִ��cgi
	}

	close(client);
	return NULL;
}


void serve_file(int client, const char *filename)//���� cat �ѷ������ļ����ݷ��ظ�������ͻ��ˡ�
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];
	buf[0] = 'A'; buf[1] = '\0';
	while ((numchars > 0) && strcmp("\n", buf))//��ȡHTTP����ͷ������
		numchars = get_line(client, buf, sizeof(buf));
	resource = fopen(filename, "r");//ֻ����
	if (resource == NULL)
		//����ļ������ڣ��򷵻�not_found
		not_found(client);
	else
	{
		//���HTTPͷ
		headers(client, filename);
		//�������ļ�����
		cat(client, resource);
	}
	fclose(resource);//�ر��ļ����
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
	//��֤����
	if (strcasecmp(method, "GET") == 0)//get��ֱ�Ӷ���?
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
				content_length = atoi(&(buf[16]));//�ҵ�content-length����
			}
			numchars = get_line(client, buf, sizeof(buf));
		}
		if (content_length == -1) {
			bad_request(client);
			return;
		}
	}
	//д
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	//#include<unistd.h>
	//int pipe(int filedes[2]);
	//����ֵ���ɹ�������0�����򷵻�-1�������������pipeʹ�õ������ļ�����������fd[0]:���ܵ���fd[1]:д�ܵ���
	//������fork()�е���pipe()�������ӽ��̲���̳��ļ���������
	//�������̲��������Ƚ��̣��Ͳ���ʹ��pipe�����ǿ���ʹ�������ܵ���

	if (pipe(cgi_output) < 0) {
		cannot_execute(client);//500
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}
	//�ɹ������ܵ�
	if ((pid = fork()) < 0) {
		//�ӽ��̴���ʧ��
		cannot_execute(client);
		return;
	}
	//�����ӽ���
	if (pid == 0) {
		char meth_env[255];//����request_method �Ļ�������
		char query_env[255];//GET �Ļ����� query_string �Ļ�������
		char length_env[255];//POST �Ļ����� content_length �Ļ�������
		//��׼�����ļ����ļ���������0
		//��׼����ļ����ļ���������1
		//��׼����������ļ���������2

		//dup2����һ��newfd��newfdָ��oldfd��λ��
		dup2(cgi_output[1], 1);//����ǽ���׼����ض���output�ܵ���д��ˣ�Ҳ����������ݽ��������outputд��
		dup2(cgi_input[0], 0);//����׼�����ض���input��ȡ�ˣ�Ҳ���ǽ���input[0]�����ݵ�input����
		close(cgi_output[0]);//�ر�output�ܵ��ĵĶ�ȡ��
		close(cgi_input[1]);//�ر�input�ܵ���д���
		sprintf(meth_env, "REQUEST_METHOD=%s", method);//��method���浽����������
		//����˵����int putenv(const char * string)�����ı�����ӻ�������������.
		//����û�������ԭ�ȴ���, ��������ݻ�������string �ı�, ����˲������ݻ��Ϊ�µĻ�������.
		//����ֵ��ִ�гɹ��򷵻�0, �д������򷵻�-1.
		putenv(meth_env);
		if (strcasecmp(method, "GET") == 0){
			sprintf(query_env, "QUERY_STRING=%s", query_string);//�洢query_string��query_env
			putenv(query_env);
		}
		else {
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);//�洢content_length��length_env
			putenv(length_env);
		}
		// ��ͷ�ļ�#include<unistd.h>
		// int execl(const char * path,const char * arg,....);
		// ����˵��:  execl()����ִ�в���path�ַ�����������ļ�·�����������Ĳ�������ִ�и��ļ�ʱ���ݹ�ȥ��argv(0)��argv[1]���������һ�����������ÿ�ָ��(NULL)��������
		// ����ֵ:    ���ִ�гɹ��������᷵�أ�ִ��ʧ����ֱ�ӷ���-1
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
		//���η��͸��ͻ���
		while (read(cgi_output[0], &c, 1) > 0) {
			send(client, &c, 1, 0);
		}
		close(cgi_output[0]);//ouput�Ķ�
		close(cgi_input[1]);//�ر�input��д
		waitpid(pid, &status, 0);//�ȴ��ӽ�����ֹ
		//���庯����pid_t waitpid(pid_t pid, int * status, int options);
		//����˵����wait()����ʱֹͣĿǰ���̵�ִ��, ֱ�����ź��������ӽ��̽���. 
		//waitpid��������
		//������������״ֵ̬, �����status �������NULL. ����pid Ϊ���ȴ����ӽ���ʶ����, ������ֵ�������£�
		//1��pid<-1 �ȴ�һ��ָ���������е��κ��ӽ��̣�����������ID����pid�ľ���ֵ
		//2��pid=-1 �ȴ��κ��ӽ���, �൱��wait().
		//3��pid=0 �ȴ�������ʶ������Ŀǰ������ͬ���κ��ӽ���.
		//4��pid>0 �ȴ��κ��ӽ���ʶ����Ϊpid ���ӽ���.
	}

}
//���ɷ�����Ϣͷ
void headers(int client, const char *filename)
{
	char buf[1024];
	(void)filename;//ʹ���ļ���ȷ���ļ�����
	//����HTTPͷ
	strcpy(buf, "HTTP/1.0 200 OK\r\n");//200��
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


//���Ƹ��ͻ����ļ�
void cat(int client, FILE *resource)
{
	//�����ļ�������
	char buf[1024];
	//��ȡ�ļ���buf��
	fgets(buf, sizeof(buf), resource);
	while (!feof(resource))//�ж��ļ��Ƿ��ȡ��ĩβ
	{
		//��ȡ�������ļ�����
		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}

//����һ��http����
int get_line(int sock, char *buf, int size)
{
	int i = 0;//��ʼ�����
	char c = '\0';//
	int n;//���ڼ�¼�ɹ���ȡ���ַ���Ŀ

	while ((i < size - 1) && (c != '\n'))
	{
		/*
		recv �� flag ��Ϊ0
		��ʱ��recv������ȡtcp buffer�е����ݵ�buf��

		��recv������Ӧ��send()������
		��Ӧ�ò�buffer�����ݿ�����socket���ں��еĽ��ջ��������Ӷ������ݻ������ں�.

		(������TCP������).

		�ȵ�recv()��ȡʱ,�Ͱ��ں˻������е����ݿ�����Ӧ�ò�
		�û���buffer����,������.

		��Ӧ�ý���һֱû�е���recv()���ж�ȡʱ
		���ݻ�һֱ��������Ӧ��socket�Ľ��ջ�������
		�����������ն˻�֪ͨ���ˣ����մ��ڹر�
		����tcp buffer���Ƴ��Ѷ�ȡ������

		������һ���ַ�һ���ַ��������
		*/
		n = recv(sock, &c, 1, 0); //������һ���ַ�һ���ַ��������
		if (n>0)
		{
			if (c == '\r')
			{
				/*
					��һ������ָ�����ն��׽���������
					�ڶ�������ָ��һ��������
					����������ָ��buf�ĳ���
				*/
				/*
				����server��client��������"_META_DATA_\r\n_USER_DATA_"
				��flags����ΪMSG_PEEKʱ,����tcp buffer�е�����
				��ȡ��buf�У��������Ѷ�ȡ�����ݴ�tcp buffer���Ƴ���
				�ٴε���recv�Կ��Զ����ղŶ���������
				*/
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					/*
					����س���(\r)�ĺ��治�ǻ��з�(\n)
					���߶�ȡʧ��
					�Ͱѵ�ǰ��ȡ���ַ���Ϊ���У��Ӷ���ֹѭ��
					*/
					c = '\n';
			}
			buf[i] = c;//��������
			i++;
		}
		else
			/*û�гɹ����յ��ַ����Ի��з���β������ѭ��*/
			c = '\n';
	}
	/*�Կ��ַ���β����Ϊ�ַ���*/
	buf[i] = '\0';
	return (i);
}

//������屨����Ϣ
void error_die(const char *sc)
{
	perror(sc);//perror(s) ��������һ���������������ԭ���������׼�豸(stderr)
	//����    
	/*fp=fopen("/root/noexitfile","r+");
	if(NULL==fp)
	{
	perror("/root/noexitfile");
	}
	����ӡ /root/noexitfile: No such file or directory
	*/
	exit(1);
}

//400
void bad_request(int client)//�����������400
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
{//�������500������������
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
{//�ͻ��˴���404
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
{//�ش���ִ�еķ���
	char buf[1024];
	//sprintf��ʽ������ַ�����printf���ֵܣ���������spintf���������Ŀ�껺����
	//״̬��
	//http/1.1�汾�� 501������� ��ԭ�� 
	/*
	״̬��������λ������ɣ���һ�����ֶ�������Ӧ����𣬹����������:
	1xx��ָʾ��Ϣ--��ʾ�����ѽ��գ���������
	2xx���ɹ�--��ʾ�����ѱ��ɹ����ա���⡢����
	3xx���ض���--Ҫ������������и���һ���Ĳ���
	4xx���ͻ��˴���--�������﷨����������޷�ʵ��
	5xx���������˴���--������δ��ʵ�ֺϷ�������
	*/
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");//501��Ӧ not implement
	send(client, buf, strlen(buf), 0);//����

	//��Ϣͷ:
	//����������
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	//��������
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	//��Ϊ�Ǵ�������û�г���
	//����
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);

	//��Ϣ��
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	//html��ʽ��ʹ�� /ͬ�� ��ʾ����
	//��ҳbody�����<p>��ǩ�������ǩ���������ǩ������html�е�һ����ǩ
	//ֻҪ���������ǩ�����־ͻ��Զ����в��ֶ�
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}
