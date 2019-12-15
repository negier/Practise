#pragma warning(disable:4996)

#include <stdio.h>

#include "glew.h"
#include "glut.h"

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

// 设置1来选择播放什么类型的视频文件
#define LOAD_RGB24   0
#define LOAD_BGR24   0
#define LOAD_BGRA    0
#define LOAD_YUV420P 1

// 窗口宽高
int screen_w = 500, screen_h = 500;
// 视频文件的宽高
const int pixel_w = 320, pixel_h = 180;

//Bit per Pixel，每像素多少比特
#if LOAD_BGRA
const int bpp = 32;
#elif LOAD_RGB24|LOAD_BGR24
const int bpp = 24;
#elif LOAD_YUV420P
const int bpp = 12;
#endif

FILE* fp = NULL;

//YUV文件的大小
unsigned char buffer[pixel_w * pixel_h * bpp / 8];
//RGB文件的大小。每个像素由R,G,B组成，所以需要宽乘以高乘以3
unsigned char buffer_convert[pixel_w * pixel_h * 3];

inline unsigned char CONVERT_ADJUST(double tmp)
{
	return (unsigned char)((tmp >= 0 && tmp <= 255) ? tmp : (tmp < 0 ? 0 : 255));
}

//YUV420P to RGB24
void CONVERT_YUV420PtoRGB24(unsigned char* yuv_src, unsigned char* rgb_dst, int nWidth, int nHeight)
{

	unsigned char Y, U, V, R, G, B;
	unsigned char* y_planar, * u_planar, * v_planar;
	int rgb_width, u_width;
	rgb_width = nWidth * 3;

	// u分量的宽是nWidth除以2。>>1其实就是除以2的意思
	u_width = (nWidth >> 1);

	// y分量的大小
	int ypSize = nWidth * nHeight;
	// u分量的大小
	int upSize = (ypSize >> 2);

	int offSet = 0;

	// 设置指针
	// 指向yuv文件y分量的开头
	y_planar = yuv_src;
	// 指向yuv文件u分量的开头
	u_planar = yuv_src + ypSize;
	// 指向yuv文件v分量的开头
	v_planar = u_planar + upSize;

	for (int i = 0; i < nHeight; i++)
	{
		for (int j = 0; j < nWidth; j++)
		{

			// 从Y平面中获取Y值。nWidth*i就是宽*行的意思，通过加以自增i可以一个一个的访问我们需要的Y。
			Y = *(y_planar + nWidth * i + j);
			
			// (i >> 1)*(u_width)：i >> 1 就是行除以2的意思，u_width是nWidth除以2，这是因为Y分量和UV分量之比是4:1。
			// j >> 1：因为YUV420中是Y和UV分量是2:1的关系，所以要除以2（j>>1），每两个Y分量共享同一个UV分量，不然UV分量很快就读完了。
			offSet = (i >> 1) * (u_width)+(j >> 1);

			/*  
			 *  TODO 
			 *  这里没有太明白，总觉得它写反了，但我改不过运行后和SDL yuv player做了对比后不对，而它是对的
			 *  u_plannar是u分量的开头，怎么可能赋值给V，v_planar更过分，怎么可能赋值给U
			 */
			// Get the U value from the v planar
			// 从V分量中获取U值
			U = *(v_planar + offSet);
			// Get the V value from the u planar
			// 从U平面中获取V值
			V = *(u_planar + offSet);

			// ==========================YUV转换成RGB============================
			// 方法一
			R = CONVERT_ADJUST((Y + (1.4075 * (V - 128))));
			G = CONVERT_ADJUST((Y - (0.3455 * (U - 128) - 0.7169 * (V - 128))));
			B = CONVERT_ADJUST((Y + (1.7790 * (U - 128))));

			/*
			// 以下公式来自微软的MSDN(试了下，感觉有点灰蒙蒙的)
			// 方法二
			int C,D,E;
			C = Y - 16;
			D = U - 128;
			E = V - 128;
			R = CONVERT_ADJUST(( 298 * C + 409 * E + 128) >> 8);
			G = CONVERT_ADJUST(( 298 * C - 100 * D - 208 * E + 128) >> 8);
			B = CONVERT_ADJUST(( 298 * C + 516 * D + 128) >> 8);
			R = ((R - 128) * .6 + 128 )>255?255:(R - 128) * .6 + 128;
			G = ((G - 128) * .6 + 128 )>255?255:(G - 128) * .6 + 128;
			B = ((B - 128) * .6 + 128 )>255?255:(B - 128) * .6 + 128;
			*/

			// rgb_width * i就是rgb_width * 行。j*3是因为每次会存储3个，分别是R、G、B。
			offSet = rgb_width * i + j * 3;

			// 这是小端（Little Endian）的分量存储方式
			rgb_dst[offSet] = B;
			rgb_dst[offSet + 1] = G;
			rgb_dst[offSet + 2] = R;
		}
	}
}

// 显示画面
void display(void) {
	if (fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp) != pixel_w * pixel_h * bpp / 8) {

		// 无限循环。读完了又从头开始读
		fseek(fp, 0, SEEK_SET);
		
		fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp);
	}

	// ======================确保视频图像填满窗口==========================
	/*  
	 *  显示的时候发现图像位于窗口的左下角，所以我们需要通过设置（-1,1）让它到左上角去。
	 *  OpenGL坐标系和数学中的是一模一样，（0，0）是中心原点，左上角是（-1,1），左下角是（-1，-1）。
	 */
	glRasterPos3f(-1.0f, 1.0f, 0);
	
	/*  
	 *  使用OpenGL直接显示像素数据，会发现图像是倒着的。因此需要在Y轴方向对图像进行翻转。
	 * 
	 *  对视频图像进行缩放和翻转操作。
	 *  两个参数用于指定在x轴、y轴上放大的倍数（如果数值小于1则是缩小）。如果指定负值，则可以实现翻转。
	 */
	glPixelZoom((float)screen_w / (float)pixel_w, -(float)screen_h / pixel_h);

// glDrawPixels可以绘制内存中的像素数据。如果是YUV42OP那么需要先转换成RGB24，然后才可以绘制。
#if LOAD_BGRA
	glDrawPixels(pixel_w, pixel_h, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
#elif LOAD_RGB24
	glDrawPixels(pixel_w, pixel_h, GL_RGB, GL_UNSIGNED_BYTE, buffer);
#elif LOAD_BGR24
	glDrawPixels(pixel_w, pixel_h, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer);
#elif LOAD_YUV420P
	CONVERT_YUV420PtoRGB24(buffer, buffer_convert, pixel_w, pixel_h);
	glDrawPixels(pixel_w, pixel_h, GL_RGB, GL_UNSIGNED_BYTE, buffer_convert);
#endif

	// ============================显示================================
	// GLUT_DOUBLE
	glutSwapBuffers();

	// GLUT_SINGLE
	//glFlush();
}

// 每隔40ms调用自身，执行display函数绘制显示画面。延时了的递归。
void timeFunc(int value) {
	display();
	glutTimerFunc(40, timeFunc, 0);
}

int main(int argc, char* argv[])
{
	
/* 
 *  根据条件选择编译哪条语句来初始化文件指针fp。
 *
 *  你可能好奇为什么明明有if-else等条件判断语句，为什么还要用这些#if-#elif呢？
 *	第一个区别，你只能将if-else写在函数里面，不能写在外面。
 *  第二个区别，两者完全不一样，#if是条件编译，也就是符合条件，下面的这些才执行编译，所以也更高效。
 *  所以，像这种定义的全局常量来说，最适合用#if-#elif了。
 */
#if LOAD_BGRA
	fp = fopen("test_bgra_320x180.rgb", "rb+");
#elif LOAD_RGB24
	fp = fopen("test_rgb24_320x180.rgb", "rb+");
#elif LOAD_BGR24
	fp = fopen("test_bgr24_320x180.rgb", "rb+");
#elif LOAD_YUV420P
	fp = fopen("test_yuv420p_320x180.yuv", "rb+");
#endif

	if (fp == NULL) {
		printf("Cannot open this file.\n");
		return -1;
	}

	/*  
	 *  初始化GLUT库。
	 *  
	 *  GLUT是实用工具库，基本上是用于做窗口界面的，并且是跨平台的。
	 */
	glutInit(&argc, argv);

	/* 
	 * 设置显示模式。
	 * 
	 * GLUT_DOUBLE，表示使用双缓冲区窗口，相应的绘图函数要使用glutSwapBuffers()。如果是GLUT_SINGLE，表示用的单缓冲窗口，
	 * 则需要使用glFlush()绘图。
	 * GLUT_RGB，指定RGB颜色模式的窗口。
	 */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	//glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );

	// 设置窗口位置坐标
	glutInitWindowPosition(100, 100);

	// 设置窗口的宽高
	glutInitWindowSize(screen_w, screen_h);

	// 创建窗口
	glutCreateWindow("Simplest Video Play OpenGL");

	// 打印OpenGL的版本，比如我的是 OpenGL Version: 4.5.0 - Build 22.20.16.4749
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

	// 设置绘图函数。操作系统在必要的时候会调用这个函数对窗口进行重新绘制操作，比如窗口创建时、改变窗口大小时。
	glutDisplayFunc(&display);

	/* 
	 *  设置定时器。
	 *
	 *  表示40ms后调用一下timeFunc一下。因为窗口创建时，操作系统会调用一次display绘制一下窗口，那么我们在其40秒后
	 *  在通过timeFunc函数调用display是为了1秒25帧。
	 */
	glutTimerFunc(40, timeFunc, 0);

	// 开始GLUT事件处理循环，播放视频。一旦调用，这个程序将永远不会返回。
	glutMainLoop();

	return 0;
}

