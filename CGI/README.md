# CGI

## 什么是CGI？

&emsp;&emsp;公共网关接口（Common Gateway Interface，CGI）是Web 服务器运行时外部程序的规范，按CGI 编写的程序可以扩展服务器功能。CGI 应用程序能与浏览器进行交互，还可通过数据API与数据库服务器等外部数据源进行通信，从数据库服务器中获取数据。格式化为HTML文档后，发送给浏览器，也可以将从浏览器获得的数据放到数据库中。几乎所有服务器都支持CGI，可用任何语言编写CGI，包括流行的C、C ++、Java、VB 和Delphi 等。CGI分为标准CGI和间接CGI两种。标准CGI使用命令行参数或环境变量表示服务器的详细请求，服务器与浏览器通信采用标准输入输出方式。间接CGI又称缓冲CGI，在CGI程序和CGI接口之间插入一个缓冲程序，缓冲程序与CGI接口间用标准输入输出进行通信。（来自百度百科）

## CGI如何工作？

如我们在网页上点击一个链接或 URL 的流程：

1. 使用你的浏览器访问 URL 并连接到 HTTP web 服务器。
1. Web 服务器接收到请求信息后会解析 URL，并查找访问的文件在服务器上是否存在，如果存在返回文件的内容，否则返回错误信息。
1. 浏览器从服务器上接收信息，并显示接收的文件或者错误信息。

CGI 程序可以是 Python 脚本，PERL 脚本，SHELL 脚本，C 或者 C++ 程序等。

## CGI程序中HTTP头部经常使用的信息

![image](https://user-images.githubusercontent.com/81791654/167091672-cfd47145-8d90-4012-a7b5-a990105b49fc.png)


## CGI环境变量

所有的CGI程序都接收以下的环境变量，这些变量在CGI程序中发挥了重要的作用：

![image](https://user-images.githubusercontent.com/81791654/167091729-9f22fade-6b0c-43b2-b1cc-9e2bc3620095.png)



## 使用python创建一个cgi程序

```py
#!/usr/bin/python
#coding:utf-8
import sys,os
length = os.getenv('CONTENT_LENGTH')
# os.getenv()--Python中的method方法返回环境变量键的值(如果存在)，否则返回默认值。
 
if length:
    postdata = sys.stdin.read(int(length))
    print "Content-type:text/html\n" # 输出内容"Content-type:text/html"发送到浏览器并告知浏览器显示的内容类型为"text/html"。
    print '<html>'
    print '<head>'
    print '<title>POST</title>'
    print '</head>'
    print '<body>'
    print '<h2> POST data </h2>'
    print '<ul>'
    for data in postdata.split('&'):
        print  '<li>'+data+'</li>'
    print '</ul>'
    print '</body>'
    print '</html>'
     
else:
    print "Content-type:text/html\n"
    print 'no found'
```
" Content-type:text/html"即为HTTP头部的一部分，它会发送给浏览器告诉浏览器文件的内容类型。

## 参考

[Python CGI编程](https://www.runoob.com/python/python-cgi.html) 
