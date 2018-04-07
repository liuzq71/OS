# OS（没有名字）
一个基于ARM9的操作系统内核

### 特点：
1. 具体实现参考linux  
2. 运行环境韦东山JZ2440开发板，暂不支持mini2440  
3. 编译环境为Windows(Linux也是可以的,可能要稍作修改) 
4. 支持内存管理  
5. 支持内核线程
6. 支持yaffs文件系统
7. 已实现nand flash,LCD,serial,usb slave,RTC,sound相关驱动
8. 暂时没有名字  
#### 就这么多吧，有兴趣的加我QQ：891085309，备注内核开发


### 准备：
1. 一台Windows电脑  
2. JZ2440开发板


### 环境搭建：
仅适用于windows环境，其它环境参考着解决。
1. XShell(串口通讯用)  
2. DNW(控制台版本，windows下运行)  
安装教程：http://blog.chinaunix.net/uid-29641438-id-4462545.html  
3. git shell(win cmd使用make总有小问题)  
MinGW(TDM-GCC 也可以，一定要有make， MinGW的make在bin目录加了前缀，去掉即可)  
4. ubuntu子系统/linux(可能用不到)  
用于制作yaffs镜像，有linux虚拟机也可以。

### 编译：

```
./compile.sh
```

### 下载：

```
./download.sh
```
