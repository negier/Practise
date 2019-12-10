#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#pragma warning(disable:4996)
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
}
#undef main

#define REFRESH_EVENT (SDL_USEREVENT + 1)
#define BREAK_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;
int pause_video = 0;
int gray_video = 0;

int refresh_video(void* opaque) {
	thread_exit = 0;
	while (thread_exit == 0)
	{
		if (pause_video == 0)
		{
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
			SDL_Delay(40);
		}
	}
	thread_exit = 0;
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);
	return 0;
}

int main()
{
	// FFmpeg
	AVFormatContext* pFormatCtx;
	int i, videoindex, currentIndex;
	AVCodecParameters* pCodecPrt;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame, * pFrameYUV;
	AVPacket* packet;
	struct SwsContext* img_convert_ctx;
	int ret, got_picture;
	char filepath[] = "wuhouyangguang.mp4";

	// SDL
	SDL_Window* screen;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	Uint32 pixformat;
	int screen_w, screen_h;

	// FFmpeg
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecPrt = pFormatCtx->streams[videoindex]->codecpar;
	pCodec = avcodec_find_decoder(pCodecPrt->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL) {
		printf("Create codecContext failed.\n");
	}
	avcodec_parameters_to_context(pCodecCtx, pCodecPrt);
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	img_convert_ctx = sws_getContext(pCodecPrt->width, pCodecPrt->height, pCodecCtx->pix_fmt, pCodecPrt->width, pCodecPrt->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	uint8_t* outBuffer = (uint8_t*)av_malloc(bufferSize);
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, outBuffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	screen_w = pCodecPrt->width;
	screen_h = pCodecPrt->height;
	screen = SDL_CreateWindow("YUV Player.(Power SDL2)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen)
	{
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	pixformat = SDL_PIXELFORMAT_IYUV;
	sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_TARGET, pCodecPrt->width, pCodecPrt->height);
	if (sdlTexture == NULL)
	{
		return -1;
	}
	SDL_Thread* refresh_thread;
	refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	SDL_Event event;

	currentIndex = 0;
	while (1)
	{
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT)
		{
			while (1)
			{
				if (av_read_frame(pFormatCtx, packet) < 0) {
					thread_exit = 1;
				}
				if (packet->stream_index == videoindex)
				{
					break;
				}
			}
			ret = avcodec_send_packet(pCodecCtx, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {

				/*
				 * 转换。
				 *
				 * 去除无效数据。
				 * 这步之前，linesize[0]大于width，之后，linesize[0]等于width。
				 */
				sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data, pFrame->linesize, 0,
					pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

				if (!gray_video == 0)
				{

					// 设置值。128是无色，这里的意思是从U分量起到末尾所有值都设置为128无色，那么有效值就只有data[0]的Y分量了，UV分量合起来占Y分量的一半，所以除以2
					memset(pFrameYUV->data[1], 128, pCodecCtx->width* pCodecCtx->height/2);

				}

				// 更新纹理。你可能好奇，为什么只穿了data[0]，这里面只有Y分量，为什么是彩色的，是这样，它是指针，它可以往后读的，具体纹理的大小，你创建的时候已经给到了。
				SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pCodecCtx->width);

				sdlRect.x = 10;
				sdlRect.y = 10;
				sdlRect.w = screen_w - 20;
				sdlRect.h = screen_h - 20;
				SDL_RenderClear(sdlRenderer);
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
				SDL_RenderPresent(sdlRenderer);
				printf("播放第%d帧.\n", ++currentIndex);
				printf("\n");
			}
			av_packet_unref(packet);
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			switch (event.key.keysym.sym)
			{
			case SDLK_SPACE:
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
	SDL_Quit();
	av_free(outBuffer);
	av_free(pFrameYUV);
	avcodec_free_context(&pCodecCtx);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);
	return 0;
}

