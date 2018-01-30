# OS
一个基于ARM的操作系统内核

特点：
1.具体实现参考linux
2.运行环境韦东山JZ2440开发板，暂时不支持mini2440
3.编译环境为Windows
4.暂时没有名字
4.就这么多吧，有兴趣的加我QQ：891085309，记住备注内核开发，备注内核开发，备注内核开发


准备：
1.一台Windows电脑
2.JZ2440开发板


环境搭建：
1.XShell(串口通讯用，其他串口工具也可以)
2.DNW(好了，控制台版本，windows下运行的哦)
  安装教程：http://blog.chinaunix.net/uid-29641438-id-4462545.html
3.git shell(win10 cmd使用make总有小问题，用它就好了)
4.MinGW(TDM-GCC 也可以，一定要有make， MinGW的make在bin目录加了前缀，去掉即可)
5.ubuntu子系统(不强求，只是git shell+MinGW有时也不够用)
  制作yaffs镜像时，需要在linux环境，不想安装虚拟机也可以。

编译：双击compile.sh
下载：双击download.sh