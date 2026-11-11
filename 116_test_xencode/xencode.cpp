#include "xencode.h"
#include <iostream>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

static void PrintErr(int err) 
{
	char buf[1024] = {0};
	av_strerror(err, buf, sizeof(buf) - 1);
	std::cerr << buf << std::endl;
}

///////////////////////////////////////////////
/// 创建编码上下文 
/// @para codec_id 编码器 ID号，对应 FFmpeg
/// @return 编码上下文，失败返回 nullptr
///////////////////////////////////////////////
AVCodecContext* XEncode::Create(int codec_id) {
	// 1. 找到编码器
	auto codec = avcodec_find_encoder((AVCodecID)codec_id);
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
/// 设置对象的编码器上下文 上下文传递到对象中，空间由 XEncode维护
/// 加锁，线程安全 （在调用 set_c 时可能有的程序在使用 c_）
///	@para c 编码器上下文 如果 c_ 不为 nullptr，则先清理资源
///////////////////////////////////////////////
void XEncode::set_c(AVCodecContext* c) 
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
bool XEncode::SetOpt(const char* key, const char* value)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	auto re = av_opt_set(c_->priv_data, key, value, 0);
	if (re != 0) 
	{
		std::cerr << "av_opt_set failed!";
		PrintErr(re);
	}

	return true;
}

bool XEncode::SetOpt(const char* key, int value)
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
bool XEncode::Open()
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
/// 编码数据 线程安全 每次新创建 AVPacket
/// @para frame 空间由用户维护
/// @return 失败返回 nullptr, 返回的 AVPacket 用户需要
/// av_packet_free() 清理
///////////////////////////////////////////////
AVPacket* XEncode::Encode(const AVFrame* frame) 
{
	if (!frame) return nullptr;

	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return nullptr;
	
	// 发送到编码线程
	auto re = avcodec_send_frame(c_, frame);
	if (re != 0) return nullptr; // 直接传入空 frame
	auto pkt = av_packet_alloc();
	if (!pkt) return nullptr;
	// 接收编码线程数据
	re = avcodec_receive_packet(c_, pkt);
	// 接收成功后 返回pkt
	if (re == 0) {
		return pkt;
	}
	if (re == AVERROR(EAGAIN) || re == AVERROR_EOF) 
	{
		// 注意空间释放
		// av_packet_free(&pkt);
		return nullptr;
	}

	// 失败后释放pkt释放空间
	if (re < 0) 
	{
		PrintErr(re);
		// return nullptr;
	}

	return nullptr;
}

///////////////////////////////////////////////
/// 创建一个 AVFrame，根据 AVCodecContext，需要调用
/// 者释放
///////////////////////////////////////////////
AVFrame* XEncode::CreateFrame()
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
///////////////////////////////////////////////
/// 返回所有编码缓冲中的 AVPacket
///////////////////////////////////////////////
std::vector<AVPacket*> XEncode::End()
{
	std::vector<AVPacket*> res{};
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return res;
	auto re = avcodec_send_frame(c_, NULL);
	if (re != 0) return res;
	// 发送 NULL， 获取缓冲区内的数据
	while (re >= 0)  // AVERROR(EAGAIN) || AVERROR_EOF
	{ 
		auto pkt = av_packet_alloc();
		re = avcodec_receive_packet(c_,pkt);

		if (re != 0) // 出错
		{
			av_packet_free(&pkt);
			break;
		}
		res.push_back(pkt);
	}
	return res;
}