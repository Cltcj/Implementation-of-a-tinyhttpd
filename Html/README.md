# Html语言基础

## 什么是Html

&emsp;&emsp;Html（Hyper Texture Markup Language）是超文本标记语言，在计算机中以 .html 或者.htm 作为扩展名,可以被 浏览器识别,就是经常见到的网页. Html 的语法非常简洁,比较松散,以相应的英语单词关键字进行组合,html 标签不区分大小写,标签大多数成对出现,有开始,有结束,例如 `<html></html>`,但是并没有要求必须成对出现.同时也有固定的短标签,例如`<br/>,<hr/>`.目前互联网上的绝大部分网页都是使用HTML编写的。

## Html标签

**标题**

HTML 标题（Heading）是通过`<h1> - <h6>` 标签来定义的。如：

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
![image](https://user-images.githubusercontent.com/81791654/166139399-77eec3a7-2d9f-4342-8287-e5307aa5058d.png#pic_center)

**HTML 段落** 

HTML 段落是通过标签 `<p>` 来定义的。如：

```html
<html>
<head>
</head>
<body>

<p>这是一个段落。</p>
<p>这是一个段落。</p>

</body>
</html>
```
![image](https://user-images.githubusercontent.com/81791654/166139688-807696d4-8d76-4d2f-bcb9-405fda9caed3.png)

**HTML 链接**

HTML 链接是通过标签 `<a>` 来定义的。

```html
<a href="https://github.com/Cltcj/Implementation-of-a-tinyhttpd">本仓库的地址</a>
```

**HTML 图像**

HTML 图像是通过标签 `<img>` 来定义的。如：

```html
<img src="./img1.png" alt="这是一张无效图" title="无效图展示" width="258" height="39" />
```
![image](https://user-images.githubusercontent.com/81791654/166146148-8dd12c31-c496-4543-afa1-3aa43ed112af.png)

图像属性: 
  1. src=”./img1.png” 图片来源,必写 
  2. alt=”这是一张无效图” 图片不显示时,显示的内容 
  3. title=”无效图展示” 鼠标移动到图片上时显示的文字 
  4. width=”258” 图片显示的宽度 
  5. height=”39” 图片显示的高度

**HTML 文本标签**

`<font>`标签,可以设置颜色和字体大小属性 颜色表示方法(可以参考网站: http://tool.oschina.net/commons?type=3): 
  1. 英文单词 red green blue … 
  2. 使用 16 进制的形式表示颜色:#ffffff 
  3. 使用 rgb(255,255,0) 字体大小可以使用 size 属性,大小范围为 1-7,其中 7 最大,1 最小. 
  4. 有时候需要使用换行标签 ,这是一个短标签 `<br/>` 与之对应另外还有一个水平线也是短标签, `<hr/>`,水平线也可以设置颜色和大小

**HTML 列表标签**

列表标签分无序列表和有序列表,分别对应`<ul>`和`<ol>`.

无序列表的格式如下: 
```html
<ul> 
  <li>列表内容 1</li> 
  <li>列表内容 2</li> 
  … 
</ul> 
```
无序列表可以设置 type 属性: 
  1. 实心圆圈:type=disc 
  2. 空心圆圈:type=circle 
  3. 小方块: type=square 
 
有序列表的格式如下: 
```html
<ol> 
  <li>列表内容 1</li> 
  <li>列表内容 2</li> 
  … 
</ol> 
```
有序列表同样可以设置 type 属性 
  1. 数字:type=1,也是默认方式 
  2. 英文字母:type=a 或 type=A 
  3. 罗马数字:type=i 或 type=I

## 写一个简单的Hmtl页面
```html
<!DOCTYPE html>
<html>
    <head>
        <title>注册信息</title>
        <meta charset="utf-8">
    </head>
    <body style="background-color:#C0C0C0;">
    <body>
        <h1 style="margin-bottom:0;">博客登录</h1></div>
        <br>
        <form action="register.cgi" method="POST">
            账号: <input type="TEXT" name="zhanghao" value="" size="10" maxlength="5">
            <br>
            <br>
            密码: <input type="password" value="" name="mima" size="10">
            <br>
            <br>
            <input type="hidden" value="隐藏的内容" name="mihiddenma" size="10">

            爱好：<input type="checkbox" name="xiebug" checked="checked">写bug<input type="checkbox" name="changge">看书
            <br>
            <br>
            自我介绍：<br>
            <textarea cols="35" rows="10" name="ziwojieshao">
这里是自我介绍...
            </textarea>
            <br>
            <br>
            地址：
            <select name="dizhi">
                <option value="sichuan">湖南</option>
                <option value="beijing">湖北</option>
                <option value="shanghai">深圳</option>
                <option value="sichuan">四川</option>
                <option value="beijing">北京</option>
                <option value="shanghai">上海</option>
            </select>
            <br>
            <br>
            <input type="submit" value="提交">
            <input type="reset" value="重置">
        </form>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
            <br>
<div id="footer" style="background-color:#FFA500;clear:both;text-align:center;">
版权 © Cltcj.com</div>

    </body>
</html>
```
写出来的模样如下图：
![image](https://user-images.githubusercontent.com/81791654/166147035-d91ac9c7-ba67-435b-97c5-440692d2b329.png)


