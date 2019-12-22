## FFmpeg 
Windows Visual Studio项目

简单的使用FFmpeg将视频解码为yuv文件输出

**ffmpeg version: 4.2.1**


[点击查看核心代码](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/TestFFmpeg.cpp)

**YUV**
![YUV的组成](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/YUV.png)

**FFmpeg解码流程**
![FFmpeg解码流程图](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/ffmpeg%E8%A7%A3%E7%A0%81%E6%B5%81%E7%A8%8B.jpg)

## Visual Studio配置
将相关的include、lib、dll文件放在项目的目录里。inlude和lib需要配置，dll不用。
![配置include](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/Image%201.png)
![配置lib](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/Image%202.png)
![配置lib,附加依赖项](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/Image%204.png)
![项目的结构](https://github.com/negier/Practise/blob/master/FFmpeg/Decode/Image%207.png)
