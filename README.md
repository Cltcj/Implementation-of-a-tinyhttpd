# Implementation-of-a-tinyhttpd

&emsp;&emsp;这个项目很简单，它是对tinyhttpd的复现，以及改进

实现一个web服务器，第一步：你得会写一个简单的HTML页面，以及关于 http 协议的一些知识

## Html语言基础

### 什么是Html

&emsp;&emsp;Html（Hyper Texture Markup Language）是超文本标记语言，在计算机中以 .html 或者.htm 作为扩展名,可以被 浏览器识别,就是经常见到的网页. Html 的语法非常简洁,比较松散,以相应的英语单词关键字进行组合,html 标签不区分大小写,标签大多数成对出现,有开始,有结束,例如 <html></html>,但是并没有要求必须成对出现.同时也有固定的短标签,例如<br/>,<hr/>. 目前互联网上的绝大部分网页都是使用HTML编写的。

### Html标签

* 标题
HTML 标题（Heading）是通过<h1> - <h6> 标签来定义的。如：

```html
<!DOCTYPE html>
<html>
<head>
</head>
<body>

<h1>这是标题 1</h1>
<h2>这是标题 2</h2>
<h3>这是标题 3</h3>
<h4>这是标题 4</h4>
<h5>这是标题 5</h5>
<h6>这是标题 6</h6>

</body>
</html>
```
![image](https://user-images.githubusercontent.com/81791654/166139399-77eec3a7-2d9f-4342-8287-e5307aa5058d.png)

  
