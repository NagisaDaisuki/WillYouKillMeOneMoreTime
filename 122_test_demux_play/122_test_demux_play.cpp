#include <iostream>
#include <thread>
#include "xdecode.h"
#include "xvideoview.h"
#include "xsdl.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

#define CHKERR(err) if (err!=0) {PrintErr(err);getchar();return -1;}

int main(int argc, char* argv[]) 
{
	int ret = 0;
	// 全局上下文
	AVFormatContext* fmt_ctx = NULL;
	std::string filename = "Pensil.mp4";
	// 打开媒体文件，解封装输入上下文
	ret = avformat_open_input(&fmt_ctx, filename.c_str(),
		NULL,	// 解封装封装器格式 NULL 自动探测(根据后缀名或文件头探测)
		NULL);	// 参数设置, rtsp设置
	CHKERR(ret);
	

	// 获取媒体信息 无头部格式
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	CHKERR(ret);

	// 打印封装信息函数
	av_dump_format(fmt_ctx, 0, filename.c_str(),
		0 // 0 表示上下文是输入 1 表示上下文输出
	);
	AVStream* as = nullptr;	// 音频流
	AVStream* vs = nullptr;	// 视频流
	// 拿到音视频流信息
	for (int i = 0; i < fmt_ctx->nb_streams; ++i)
	{
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			as = fmt_ctx->streams[i];
			std::cout << "=========音频=========" << std::endl;
			std::cout << as->codecpar->sample_rate << std::endl;
		}
		else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vs = fmt_ctx->streams[i];
			std::cout << "=========视频=========" << std::endl;
			std::cout << vs->codecpar->width << std::endl;
			std::cout << vs->codecpar->height << std::endl;

		}
	}
	
	// 拿到视频编码器 ID
	auto video_codec_id = vs->codecpar->codec_id;
	// 视频编码信息
	auto video_par = vs->codecpar;

	//////////////////////////////////////////////////
	/// 视频解码器初始化
	XDecode decode;

	auto decode_c = XDecode::Create(video_codec_id,false);	// 创建一个解码器
	// 解封装的视频编码参数，传递给解码上下文
	avcodec_parameters_to_context(decode_c, video_par);
	// decode_c->thread_count = 16;

	// 设置到解码器中，线程安全，设置完后 decode_c 不在其类外部使用
	decode.set_c(decode_c);
	// 打开解码器
	if (!decode.Open())
	{
		std::cout << "decode Open failed!" << std::endl;
		return -1;
	}

	// 创建解码输出的空间
	auto frame = decode.CreateFrame();
	//////////////////////////////////////////////////
	
	//////////////////////////////////////////////////
	/// 渲染初始化
	/// 在打开 h.264 裸流得读取到第一帧之后才能拿到 视频宽高数据
	/// 现在直接打开封装好的 mp4 文件 直接可以提前拿到视频信息	
	auto view = XVideoView::Create();
	view->Init(video_par->width, video_par->height, (XVideoView::PixFormat)video_par->format);

	//////////////////////////////////////////////////

	AVPacket pkt;
	for (;;) 
	{
		ret = av_read_frame(fmt_ctx,&pkt);
		CHKERR(ret);
		if (pkt.stream_index == vs->index) 
		{
			std::cout << "视频流信息: ";
			// 解码视频
			if (decode.Send(&pkt))
			{
				while (decode.Recv(frame))
				{
					std::cout << frame->pts << " " << std::flush;
					view->DrawFrame(frame); // 渲染视频
				}
			}

		}
		else if (pkt.stream_index == as->index)
		{
			std::cout << "音频流信息: ";
		}
		

		// 正常释放 pkt 空间
		// std::cout << pkt.pts << " : " << pkt.dts <<
		// 	" : " << pkt.size << std::endl;
		
		av_packet_unref(&pkt);
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
	// 释放 frame
	av_frame_free(&frame);

	// 清理全局上下文
	avformat_close_input(&fmt_ctx);

	return EXIT_SUCCESS;
}