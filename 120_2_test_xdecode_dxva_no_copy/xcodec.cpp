#include "xcodec.h"

#include <iostream>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")


void PrintErr(int err) 
{
	char buf[1024] = {0};
	av_strerror(err, buf, sizeof(buf) - 1);
	std::cerr << buf << std::endl;
}

XCodec::~XCodec()
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (c_) 
	{
		avcodec_free_context(&c_); //  类生命周期结束时，自动释放
		c_ = nullptr;
	}

}

///////////////////////////////////////////////
/// 创建编码上下文 
/// @para codec_id 编码器 ID号，对应 FFmpeg
/// @return 编码上下文，失败返回 nullptr
///////////////////////////////////////////////
AVCodecContext* XCodec::Create(int codec_id, bool is_encode) {
	// 1. 找到编码器
	AVCodec* codec = nullptr;

	if (is_encode)
		codec = avcodec_find_encoder((AVCodecID)codec_id);
	else
		codec = avcodec_find_decoder((AVCodecID)codec_id);
	if (!codec) {
		std::cerr << "avcodec_find_encoder failed!" << codec_id << std::endl;
		return nullptr;
	}
	
	// 2. 创建上下文
	AVCodecContext* c = avcodec_alloc_context3(codec);
	if (!c) {
		std::cerr << "avcodec_alloc_context3 failed!" << codec_id << std::endl;
		return nullptr;
	}

	// 3. 设置参数默认值
	c->time_base = {1, 25};
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->thread_count = 16;
	return c;
}


///////////////////////////////////////////////
/// 设置对象的编码器上下文 上下文传递到对象中，空间由 XCodec维护 线程安全
/// 加锁，线程安全 （在调用 set_c 时可能有的程序在使用 c_）
///	@para c 编码器上下文 如果 c_ 不为 nullptr，则先清理资源
///////////////////////////////////////////////
void XCodec::set_c(AVCodecContext* c) 
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (c_)
	{
		// 资源清理
		avcodec_free_context(&c_);
	}
	this->c_ = c;	// 对资源做清理并且线程安全
}

///////////////////////////////////////////////
/// 设置编码参数，线程安全
/// C++ 支持函数重载
///////////////////////////////////////////////
bool XCodec::SetOpt(const char* key, const char* value)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	auto re = av_opt_set(c_->priv_data, key, value, 0);
	if (re != 0) 
	{
		std::cerr << "av_opt_set failed!";
		PrintErr(re);
		return false;

	}

	return true;
}

bool XCodec::SetOpt(const char* key, int value)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	auto re = av_opt_set_int(c_->priv_data, key, value, 0);
	if (re != 0) 
	{
		std::cerr << "av_opt_set failed!";
		PrintErr(re);
		return false;
	}

	return true;
}
///////////////////////////////////////////////
/// 打开编码器 线程安全
///////////////////////////////////////////////
bool XCodec::Open()
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	auto re = avcodec_open2(c_, NULL, NULL); // AVCodec 在创建 Context时指定过了
	if (re != 0)
	{
		PrintErr(re);
		return false;
	}
	return true;
}

///////////////////////////////////////////////
/// 创建一个 AVFrame，根据 AVCodecContext，需要调用
/// 者释放
///////////////////////////////////////////////
AVFrame* XCodec::CreateFrame()
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return nullptr;
	auto frame = av_frame_alloc();
	if (!frame)
	{
		std::cerr << "av_frame_alloc failed!" << std::endl; 
		return nullptr;
	}
	frame->width = c_->width;
	frame->height = c_->height;
	frame->format = c_->pix_fmt;
	auto re = av_frame_get_buffer(frame, 0);
	if (re != 0) 
	{
		std::cerr << "av_frame_get_buffer failed!";
		av_frame_free(&frame);
		PrintErr(re);
		return nullptr;
	}
	return frame;
}
