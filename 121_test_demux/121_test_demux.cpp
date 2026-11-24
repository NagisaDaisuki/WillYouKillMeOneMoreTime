#include <iostream>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

void PrintErr(int err) 
{
	char buf[1024] = {0};
	av_strerror(err, buf, sizeof(buf) - 1);
	std::cerr << buf << std::endl;
}
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
	AVPacket pkt;
	for (;;) 
	{
		ret = av_read_frame(fmt_ctx,&pkt);
		CHKERR(ret);
		if (pkt.stream_index == vs->index) 
		{
			std::cout << "视频流信息: ";
		}
		else if (pkt.stream_index == as->index)
		{
			std::cout << "音频流信息: ";
		}
		

		// 正常释放 pkt 空间
		std::cout << pkt.pts << " : " << pkt.dts <<
			" : " << pkt.size << std::endl;
		
		av_packet_unref(&pkt);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// 清理全局上下文
	avformat_close_input(&fmt_ctx);

	return EXIT_SUCCESS;
}