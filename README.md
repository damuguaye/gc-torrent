# gc-torrent
用c++和go写的一个bt下载器/bt downloader
本项目主要基于[go-torrent](https://github.com/archeryue/go-torrent)
c++部分代码参考[cpp-torrent](https://github.com/ACking-you/cpp-torrent)

# 项目介绍
用c++和go写了一个bt下载器，只实现了下载功能。
c++负责对torrent文件进行解析，得到tracker网址以及用于校验的hash值(也用c++实现了sha1校验)。
将c++的代码打包为dll动态库，由go语言调用获取到torrent文件的信息，并由go进行通信下载。

# 测试使用步骤
1.(已完成，可略过)用cmd或者终端进入include目录下，输入**g++ -std=c++17 -shared torrent_file.cpp -o tf.dll**
2.(已完成，可略过)用cmd或者终端进入main目录下，输入**go build**
3.运行main.exe或main，直接输入**main**并运行，如果想自己指定torrent文件，在main后面添加 **-p yourFilePath**，把yourFilePath换成文件路径
