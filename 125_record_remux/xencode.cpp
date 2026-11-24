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
		av_packet_free(&pkt);
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