#include <iostream>
extern "C" {
	#include "SDL.h"
}
/* 
 *  解定义main。
 *  因为在SDL_main.h这个文件发现有一条语句#define main SDL_main，由于我的主函数就是main，结果直接被当做SDL_main识别了。 
 */
#undef main

#pragma warning(disable:4996)

// 视频文件的宽高。单位像素
const int pixel_w = 960, pixel_h = 540;

// 视频播放窗口的宽高。单位像素
int screen_w = 960, screen_h = 540;

const int bpp = 12;

/*  
 * bpp/8 = （1 + 1/4 + 1/4）=1.5，因为是YUV嘛，所以要乘以1.5。
 * YUV, UV分量是Y分量长宽的一半，即1/4。
 */
const int bufferSize = pixel_w * pixel_h * bpp / 8; 

unsigned char buffer[bufferSize];
int n;

#define REFRESH_EVENT (SDL_USEREVENT + 1) // 刷新视频帧事件
#define BREAK_EVENT (SDL_USEREVENT + 2) // 退出软件事件

int thread_exit = 0; // 线程退出标记
int pause_video = 0; // 暂停视频播放
int gray_video = 0; // 灰度视频标记
int refresh_video(void *opaque) {
	thread_exit = 0;
	while (thread_exit == 0)
	{
		if (pause_video == 0)
		{
			SDL_Event event;
			event.type = REFRESH_EVENT;

			// 发送一个事件
			SDL_PushEvent(&event);
			
			/*
			 * 延迟40ms。也就是每秒25帧的意思，1000/40 = 25.这是一个工具函数。
			 */
			SDL_Delay(40);
		}
	}
	thread_exit = 0;
	// 发送停止事件
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);
	return 0;
}

int main()
{
	SDL_Window* screen;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture; // 将YUV给Texture纹理，一个yuv对应一个Texture
	SDL_Rect sdlRect; // SDL_Rect规定了Texture的显示位置
	Uint32 pixformat;
	FILE* fp;
	
	// 初始化
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	// 创建窗口SDL_Window
	screen = SDL_CreateWindow("YUV Player.(Power SDL2)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen)
	{
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	// 创建渲染器SDL_Renderer
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	// Y + V + U  (3 planes)
	pixformat = SDL_PIXELFORMAT_IYUV;

	// 创建纹理SDL_Texture
	sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_TARGET, pixel_w, pixel_h);

	// mode:rb。这里b必须加，尤其是区分二进制文件和文本文件的系统上，表示这是一个二进制文件。
	fp = fopen("output.yuv", "rb");
	if (fp == NULL)
	{
		printf("cannot open this file\n");
		return -1;
	}

	// 线程的句柄
	SDL_Thread* refresh_thread;
	// 创建一个线程
	refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	
	SDL_Event event;
	while (1)
	{

		// 等待一个事件
		SDL_WaitEvent(&event);

		// 事件判断
		if (event.type == REFRESH_EVENT)
		{
			if (fread(buffer, 1, bufferSize, fp) != bufferSize)
			{
				// 指针回到文件头。无限循环播放YUV文件
				rewind(fp);
				fread(buffer, 1, bufferSize, fp);
			}

			if (!gray_video == 0)
			{
				/*
				 *  色度分量在偏置处理前的取值范围是-128至127，这时候的无色对应的是0值。经过偏置处理后，色度分量取值
				 *  变成了0至255，因而此时的无色对应的就是128了。
				 */
				memset(buffer + pixel_w * pixel_h, 128, pixel_w * pixel_h / 2);
			}

			// 设置纹理的数据
			SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);

			// 为视频加10像素的黑框
			sdlRect.x = 10;
			sdlRect.y = 10;
			sdlRect.w = screen_w - 20;
			sdlRect.h = screen_h - 20;
			SDL_RenderClear(sdlRenderer);

			// 将纹理的数据拷贝给渲染器
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);

			// 显示
			SDL_RenderPresent(sdlRenderer);
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			// 调整后的屏幕宽高自动赋值给传入的参数
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			switch (event.key.keysym.sym)
			{
				case SDLK_SPACE: // 点击空格键退出视频
					pause_video = !pause_video;
					break;
				case SDLK_y: // 点击y键播放灰度视频（仅y分量）
					gray_video = !gray_video;
					break;
				default:
					break;
			}
		}
		else if (event.type == SDL_QUIT) // 退出视频播放
		{
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT)
		{
			break;
		}
	}

	// 退出SDL系统
	SDL_Quit();
	
	return 0;
}

