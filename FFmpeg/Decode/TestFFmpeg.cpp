/*
 *   
 *                 |-AVInputFormat
 *                 |                               |- AVCodecContext
 * AVFormatContext-|-AVStream[0]-AVCodecParameters-|
 *                 |                               |- AVCodec
 *	               |      
 *                 |                               |- AVCodecContext
 *                 |-AVStream[1]-AVCodecParameters-|
 *                                                 |- AVCodec
 *
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS
#pragma warning(disable:4996)

extern "C" {
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
	#include "libavutil/imgutils.h"
}

//Output YUV420P 
#define OUTPUT_YUV420P 0

int main()
{
	AVFormatContext* pFormatCtx;
	int i, videoindex,currentIndex;
	AVCodecParameters* pCodecPrt;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame, * pFrameYUV;
	AVPacket* packet;
	struct SwsContext* img_convert_ctx;

	FILE* fp_yuv;
	int ret, got_picture;
	char filepath[] = "wuhouyangguang.mp4";

	avformat_network_init();
	
	// 分配一个AVFormatContext.
	pFormatCtx = avformat_alloc_context();

	// 打开输入视频文件 
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	// 获取视频文件信息
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
	
	//查找解码器
	pCodec = avcodec_find_decoder(pCodecPrt->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}

	//分配一个AVCodecContext
	pCodecCtx = avcodec_alloc_context3(NULL);

	if (pCodecCtx == NULL) {
		printf("Create codecContext failed.\n");
	}
	avcodec_parameters_to_context(pCodecCtx, pCodecPrt);

	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}

	// 打印数据看看
	printf("封装视频的播放时长%d\n", pFormatCtx->duration);
	printf("封装视频的码率%d\n", pFormatCtx->bit_rate);

	printf("视频编码的名称%s\n", pFormatCtx->iformat->name);
	printf("视频编码的长名称%s\n", pFormatCtx->iformat->long_name);
	printf("视频编码的扩展名%s\n", pFormatCtx->iformat->extensions);
	
	printf("视频编码的像素格式%d\n", pCodecCtx->pix_fmt); //0就是YUV420P
	printf("视频编码的宽和高%d*%d\n", pCodecCtx->width,pCodecCtx->height); //0就是YUV420P

	printf("解码器的名称%s\n", pCodec->name);
	printf("解码器的长名称%s\n", pCodec->long_name);
	printf("解码器的类型%s\n", pCodec->type);

	// 内存分配函数。就是简单的封装了系统函数malloc()，并做了一些错误检查工作
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));

	//Output Information-----------------------------
	printf("------------- File Information ------------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	#ifdef OUTPUT_YUV420P 
		fp_yuv = fopen("output.yuv", "wb+");
	#endif  

	//分配一个SwsContext
	img_convert_ctx = sws_getContext(pCodecPrt->width, pCodecPrt->height, pCodecCtx->pix_fmt, pCodecPrt->width, pCodecPrt->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	
	// 分配一个AVFrame
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	//根据传入的参数返回一个所需的数据大小（字节）
	int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	uint8_t* outBuffer = (uint8_t*)av_malloc(bufferSize);
	
	//向pFrameYUV填充数据
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, outBuffer,AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	
	currentIndex = 0;
	//------------------------------
	while (av_read_frame(pFormatCtx, packet) >= 0) { //从输入文件读取一帧
		if (packet->stream_index == videoindex) {

			// 解码一帧压缩数据
			ret = avcodec_send_packet(pCodecCtx, packet);
			
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			
			if (avcodec_receive_frame(pCodecCtx, pFrame)==0) {
				
				printf("关键帧：%d\n", pFrame->key_frame);
				printf("帧类型：%d\n", pFrame->pict_type); //I P B
				printf("帧大小：%d\n", pFrame->linesize[0]* pFrame->height);

				/* 解码后YUV格式的视频像素数据保存在AVFrame的data[0],data[1],data[2]中。但这些像素值不是连续存储的，每行
				有效像素之后存储了一些无效像素。以亮度Y数据为例，data[0]中一共包含了linesize[0]*height个数据。但是出于优
				化等方面的考虑，linesize[0]实际上并不等于宽度width,而是一个比宽度大一些的值。所以需要用sws_scale()转换。转
				换后去除了无效数据，width和linesize[0]取值相等	*/
				sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data, pFrame->linesize, 0,
					pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
#ifdef OUTPUT_YUV420P
				int y_size = pCodecCtx->width * pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("写入第%d帧.\n",++currentIndex);
				printf("\n");
#endif
			}
		}
		av_packet_unref(packet);
	}

#ifdef OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	// 释放申请的内存。就是简单的封装了系统函数free()
	av_free(outBuffer);
	av_free(pFrameYUV);

	//关闭解码器
	avcodec_free_context(&pCodecCtx);
	avcodec_close(pCodecCtx);

	//关闭输入视频文件
	avformat_close_input(&pFormatCtx);

	//释放AVFormatContext和它所有的流
	avformat_free_context(pFormatCtx);

	return 0;
}