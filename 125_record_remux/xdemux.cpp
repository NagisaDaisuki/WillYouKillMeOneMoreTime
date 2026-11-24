#include "xdemux.h"
#include <iostream>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")

AVFormatContext* XDemux::Open(const char* url)
{
	AVFormatContext* c = nullptr;
	// 打开封装上下文
	auto re = avformat_open_input(&c, url, NULL,
		NULL); // 选择项后面 rtsp设置 
	BERR(re);

	// 获取媒体信息
	re = avformat_find_stream_info(c, NULL);
	BERR(re);

	// 打印输入封装信息
	av_dump_format(c, 0, url, 0); // 设定输出上下文 0
	return c;
}

bool XDemux::Read(AVPacket* pkt)
{
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_)
		return false;
	auto re = av_read_frame(c_, pkt);
	BERR(re);
	return true;
}

bool XDemux::Seek(long long pts, int stream_index)
{
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_)
		return false;
	auto re = av_seek_frame(c_, stream_index, pts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
	BERR(re);
	return true;
}


