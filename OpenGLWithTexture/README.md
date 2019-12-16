# OpenGL With Texture
使用OpenGL的Texture播放视频画面，即用GPU来进行YUV到RGB的转变

## 流程
1. 初始化
2. 创建窗口
3. 设置绘图函数
4. 设置定时器
5. 初始化Shader
    1. 创建一个Shader对象
        1. 编写Vertex Shader和Fragment Shader源码
        2. 创建两个Shader实例
        3. 给Shader实例指定源码
        4. 在线编译Shader源码
    2. 创建一个Program对象
        1. 创建program
        2. 绑定shader到program
        3. 链接program
        4. 使用program
    3. 初始化Texture
        1. 定义顶点数组
        2. 设置顶点数组
        3. 初始化纹理
6. 进入消息循环
    1. 设置纹理
    2. 绘制
    3. 显示
![流程图](https://github.com/negier/Practise/blob/master/OpenGLWithTexture/simplest_video_play_opengl_texture.jpg)

## 坐标
**opengl 物理坐标**
![](https://github.com/negier/Practise/blob/master/OpenGLWithTexture/Image%202.png)
**opengl 纹理坐标**
![](https://github.com/negier/Practise/blob/master/OpenGLWithTexture/Texture.png)

## 顶点着色器、片元着色器
一句话总结：”Vertex Shader负责搞定像素位置，填写gl_Position;Fragment Shader负责搞定像素外观，填写gl_FragColor。“
