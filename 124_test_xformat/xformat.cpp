#include "xformat.h"	
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


/// <summary>
/// 复制参数，线程安全
/// </summary>
/// <param name="stream_index">对应c_streams 下标</param>
/// <param name="dst">输出参数</param>
/// <returns>是否成功</returns>
bool XFormat::CopyPara(int stream_index, AVCodecParameters* dst)
{
	std::unique_lock<std::mutex> lock(mux_);
	if (!c_) // 上下文没有设定
		return false;
	// 索引不正确 
	if (stream_index < 0 || stream_index > c_->nb_streams)
		return false;
	auto re = avcodec_parameters_copy(dst, c_->streams[stream_index]->codecpar);
	if (re < 0)
		return false;
}
// 设置上下文
void XFormat::set_c(AVFormatContext* c) 
{
	std::unique_lock<std::mutex> lock(mux_);
	if (c_) // 清理原值
	{
		if (c_->iformat) // 如果是输入上下文
		{
			avformat_close_input(&c_);
		}
		else if (c_->oformat) // 如果是输出上下文 
		{
			if (c_->pb)
				avio_closep(&c_->pb); // 清理IO上下文
			avformat_free_context(c_);
		}
		else 
		{
			avformat_close_input(&c_);
		}
	}
	c_ = c; // 直接赋值

	// 如果 是资源释放
	if (!c_) return;

	for (int i = 0; i < c->nb_streams; ++i) 
	{
		// 视频
		if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			video_index_ = i;
			video_time_base_ = {c->streams[i]->time_base.num, c->streams[i]->time_base.den}; // 顺便把真实时间基记录下来
		}
		// 音频
		else if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) 
		{
			audio_index_ = i;
			audio_time_base_ = {c->streams[i]->time_base.num, c->streams[i]->time_base.den};
		}
	}
}
