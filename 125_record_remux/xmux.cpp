#include "xmux.h"
#include <iostream>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")

/// <summary>
/// 打开封装
/// 创建输出上下文，为上下文添加视频音频流
/// </summary>
/// <param name="url">地址</param>
/// <returns></returns>
AVFormatContext* XMux::Open(const char* url) 
{
	AVFormatContext* c = nullptr;
	// 创建上下文
	// 直接通过 url推断格式
	auto re = avformat_alloc_output_context2(&c, NULL, NULL, url);
	BERR(re);

	// 添加 视频音频流
	auto vs = avformat_new_stream(c, NULL);	// 视频流
	vs->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	auto as = avformat_new_stream(c,NULL);	// 音频流
	as->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
	
	// 打开IO
	// url 写入文件
	re = avio_open(&c->pb, url, AVIO_FLAG_WRITE);
	BERR(re);

	return c;
}

bool XMux::WriteHeader()
{	
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_) // 上下文没有设定
		return false;
	auto re = avformat_write_header(c_, nullptr);
	BERR(re);
	// 打印输出上下文
	av_dump_format(c_, 0, c_->url, 1);
	return true;
}

bool XMux::Write(AVPacket* pkt)
{
	
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_)
		return false;
	// 写入一帧数据，内部缓冲排序 dts, 通过 pkt = null 可以写入缓冲
	auto re = av_interleaved_write_frame(c_,pkt);
	BERR(re);
	return true;
}

bool XMux::WriteEnd()
{
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_)
		return false;
	// 写入缓冲
	av_interleaved_write_frame(c_, NULL);
	// 写入 尾部trailer
	// MP4 的 moov 索引头是在写入文件尾时才正式生成并刷入磁盘
	auto re = av_write_trailer(c_);
	BERR(re);
	return true;
}
