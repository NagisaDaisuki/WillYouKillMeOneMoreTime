#pragma once
#include <mutex>
#include <vector>
#include "xcodec.h"
// 前置声明
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVPacket;

class XEncode : public XCodec
{
public:
	///////////////////////////////////////////////
	/// 编码数据 线程安全 每次新创建 AVPacket
	/// @para frame 空间由用户维护
	/// @return 失败返回 nullptr, 返回的 AVPacket 用户需要
	/// av_packet_free() 清理
	///////////////////////////////////////////////
	AVPacket* Encode(const AVFrame* frame);

	
	///////////////////////////////////////////////
	/// 返回所有编码缓冲中的 AVPacket
	///////////////////////////////////////////////
	std::vector<AVPacket*> End();
};

