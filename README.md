# hust-embedded
####Lab1：
- 利用读卡器将开发主机上的香橙派映像使用烧录软件balenaEtcher将ubuntu PC的图形界面烧入SD卡然后插入开发板。
- 利用网线连接开发板后远程ssh连接开发板，禁用桌面并且配置本地编译环境。最后编译运行lab1.

####Lab2：
  - 了解Linux下的LCD显示驱动接口：framebuffer的使用原理。
  - 阅读代码，并且据此实现点、线、矩形等图形的绘制。
  - 了解双缓冲机制和双缓冲的绘图过程。

####Lab3：
- 了解怎么从文件获得图片，以及图片读取后在内存存储的结构，了解图片像素颜色的三种类型：RGB_8880（不透明jpg图片）、RGBA_8888（透明png图片）、ALPHA_8（矢量字体的字模图片）。
- 由此实现这三种类型图片的绘制。

####Lab4：
- 学习和了解linux下的input event触摸屏驱动接口和多点触摸协议，
- 利用该接口和协议实现多指在屏幕上绘制移动轨迹，要求每个手指轨迹的颜色不相同，并且做到轨迹连续，较粗，再额外实现一个清楚屏幕的按钮，点击后清除屏幕内容。

####Lab5：
- 正确配置并且启动蓝牙服务，熟练使用蓝牙的常用命令工具.
- 通过RFCOMM串口实现蓝牙的无线通信，可以是两个开发板之间相互连接也可以是开发板和手机等其它蓝牙设备的连接，测试通过lab5.

####Lab6：
- 实现了一个与平板使用蓝牙连接的图片浏览器。
- 实现了通过蓝牙将平板上的图片传输到开发板上，并且显示出来。
- 实现了使用触摸屏手指操作的双指图片放缩，单指图片拖动，以及在图片上使用手指涂鸦，最后将涂鸦过的图片保存到本地的功能。