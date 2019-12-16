#pragma warning(disable:4996)

#include <stdio.h>

#include "glew.h"
#include "glut.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

//Select one of the Texture mode (Set '1'):
#define TEXTURE_DEFAULT   1
//Rotate the texture
#define TEXTURE_ROTATE    0
//Show half of the Texture
#define TEXTURE_HALF      0

const int screen_w = 500, screen_h = 500;
const int pixel_w = 320, pixel_h = 180;
//YUV file
FILE* infile = NULL;
unsigned char buf[pixel_w * pixel_h * 3 / 2];
unsigned char* plane[3];


GLuint p;
GLuint id_y, id_u, id_v; // Texture id
GLuint textureUniformY, textureUniformU, textureUniformV;


#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

void display(void) {
	if (fread(buf, 1, pixel_w * pixel_h * 3 / 2, infile) != pixel_w * pixel_h * 3 / 2) {
		// 无限循环。读完了又从头开始读
		fseek(infile, 0, SEEK_SET);
		fread(buf, 1, pixel_w * pixel_h * 3 / 2, infile);
	}

	// GLSL源码写错，在屏幕上就会看到绿色
	glClearColor(0.0, 255, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	//Y
	// 激活纹理单位
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, id_y);
	// 根据像素数据，生成一个2D纹理
	// TODO 为啥都是GL_RED？
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w, pixel_h, 0, GL_RED, GL_UNSIGNED_BYTE, plane[0]);

	glUniform1i(textureUniformY, 0);
	//U
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, id_u);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w / 2, pixel_h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, plane[1]);
	glUniform1i(textureUniformU, 1);
	//V
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, id_v);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w / 2, pixel_h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, plane[2]);
	glUniform1i(textureUniformV, 2);

	// Draw
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// Show
	//Double
	glutSwapBuffers();
	//Single
	//glFlush();
}

void timeFunc(int value) {
	display();
	// Timer: 40ms
	glutTimerFunc(40, timeFunc, 0);
}

char* textFileRead(char* filename)
{
	char* s = (char*)malloc(8000);
	memset(s, 0, 8000);
	FILE* infile = fopen(filename, "r");
	int len = fread(s, 1, 8000, infile);
	fclose(infile);
	// 字符串结束符'\0'
	s[len] = 0;
	return s;
}

void InitShaders()
{
	GLint vertCompiled, fragCompiled, linked;
	GLint v, f;
	const char* vs, * fs;

	// 创建Vertex Shader和Fragment Shader这两个实例
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	// 从文件中读取顶点着色器和片元着色器的GLSL源码
	char vsh[] = "Shader.vsh";
	char fsh[] = "Shader.fsh";
	vs = textFileRead(vsh);
	fs = textFileRead(fsh);

	// 给Vertex Shader实例指定源码
	glShaderSource(v, 1, &vs, NULL);
	// 给Fragment Shader实例指定源码
	glShaderSource(f, 1, &fs, NULL);

	// 在线编译Vertex Shader源码
	glCompileShader(v);
	// Debug
	glGetShaderiv(v, GL_COMPILE_STATUS, &vertCompiled);
	printf("Vertex Shader编译情况：%s\n", vertCompiled==0?"false":"true");
	// 在线编译Fragment Shader源码
	glCompileShader(f);
	// Debug
	glGetShaderiv(f, GL_COMPILE_STATUS, &fragCompiled);
	printf("Fragment Shader编译情况：%s\n", fragCompiled == 0 ? "false" : "true");

	// 创建Program
	p = glCreateProgram();
	
	// 将Vertex Shader和Fragment Shader绑定到Program
	glAttachShader(p, v);
	glAttachShader(p, f);

	// 获取*顶点*属性名（vertexIn,textureIn）的位置，绑定给ATTRIB_VERTEX，ATTRIB_TEXTURE
	glBindAttribLocation(p, ATTRIB_VERTEX, "vertexIn");
	glBindAttribLocation(p, ATTRIB_TEXTURE, "textureIn");

	// 链接Program
	glLinkProgram(p);
	// Debug
	glGetProgramiv(p, GL_LINK_STATUS, &linked);
	printf("Program链接阶段编译情况：%s\n", linked == 0 ? "false" : "true");

	// 使用Program
	glUseProgram(p);

	// 从GLSL源码中获取Uniform变量位置
	textureUniformY = glGetUniformLocation(p, "tex_y");
	textureUniformU = glGetUniformLocation(p, "tex_u");
	textureUniformV = glGetUniformLocation(p, "tex_v");

	// 物体坐标（左下，右下，左上，右上）
#if TEXTURE_ROTATE
	static const GLfloat vertexVertices[] = {
		-1.0f, -0.5f,
		 0.5f, -1.0f,
		-0.5f,  1.0f,
		 1.0f,  0.5f,
	};
#else
	static const GLfloat vertexVertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f,  1.0f,
	};
#endif

	// 纹理坐标（左下，右下，左上，右上），写的是计算机的坐标，不然显示是反着的。计算机坐标原点在左上角，纹理坐标原点在左下角。
#if TEXTURE_HALF
	static const GLfloat textureVertices[] = {
		0.0f,  1.0f,
		0.5f,  1.0f,
		0.0f,  0.0f,
		0.5f,  0.0f,
	};
#else
	static const GLfloat textureVertices[] = {
		0.0f,  1.0f,
		1.0f,  1.0f,
		0.0f,  0.0f,
		1.0f,  0.0f,
	};
#endif
	// 设置物理坐标
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	// 设置纹理坐标
	glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	glEnableVertexAttribArray(ATTRIB_TEXTURE);

	// ==============================初始化纹理============================
	// 生成你要操作对的纹理对象的索引
	glGenTextures(1, &id_y);
	// 产生纹理索引后，才能对该纹理进行操作。glBindTexture()告诉OpenGL下面对纹理的任何操作都是针对它所绑定的纹理对象的。
	glBindTexture(GL_TEXTURE_2D, id_y);
	// 设置纹理的一些属性。glTexParameteri用来确定如何把图像从纹理图像空间映射到帧缓冲图像空间。即把纹理像素映射成像素。
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &id_u);
	glBindTexture(GL_TEXTURE_2D, id_u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &id_v);
	glBindTexture(GL_TEXTURE_2D, id_v);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

}



int main(int argc, char* argv[])
{
	// 打开YUV文件
	if ((infile = fopen("test_yuv420p_320x180.yuv", "rb")) == NULL) {
		printf("cannot open this file\n");
		return -1;
	}

	// Y数据
	plane[0] = buf;
	// U数据
	plane[1] = plane[0] + pixel_w * pixel_h;
	// V数据
	plane[2] = plane[1] + pixel_w * pixel_h / 4;

	// 初始化Glut库
	glutInit(&argc, argv);

	/* 
	 *  指定窗口模式
	 *  GLUT_DOUBLE:指定双缓存窗口
	 *  GLUT_RGBA:指定RGBA颜色模式的窗口
	 */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA /*| GLUT_STENCIL | GLUT_DEPTH*/);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(screen_w, screen_h);
	glutCreateWindow("Simplest Video Play OpenGL");

	printf("Version: %s\n", glGetString(GL_VERSION));
	
	// 初始化glew库，初始化后你才可以使用它里面的Shader、Program、Texture等相关函数
	glewInit();

	// 设置绘图函数。操作系统在必要的时候会调用这个函数对窗口进行重新绘制操作，比如窗口创建时、改变窗口大小时。
	glutDisplayFunc(&display);

	/*
	 *  设置定时器。
	 *
	 *  表示40ms后调用一下timeFunc函数一下。因为窗口创建时，操作系统会调用一次display绘制一下窗口，那么我们在其40秒后
	 *  在通过timeFunc函数调用display是为了1秒25帧。
	 */
	glutTimerFunc(40, timeFunc, 0);

	// 初始化Shader
	InitShaders();

	// 开始GLUT事件处理循环，播放视频。一旦调用，这个程序将永远不会返回。
	glutMainLoop();

	return 0;
}

