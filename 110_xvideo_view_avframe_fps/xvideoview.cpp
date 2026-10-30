#include "stdafx.h"
#include "xvideoview.h"
#include "xsdl.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <thread>
#include <chrono>

# pragma comment(lib,"avutil.lib")

void MSleep(unsigned int ms) 
{
	auto beg = clock();
	for (int i = 0; i < ms; ++i) 
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if ((clock() - beg) / (CLOCKS_PER_SEC / 1000) >= ms)
			break;
	}
}

XVideoView* XVideoView::Create(RenderType type)
{
	switch (type)
	{
	case XVideoView::SDL:
		return new XSDL();
		break;
	default:
		break;
	}

	return nullptr;
}

bool XVideoView::DrawFrame(AVFrame* frame)
{
	if (!frame || !frame->data[0]) return false;
	count_++;
	if (beg_ms_ <= 0)
	{
		beg_ms_ = clock();
	}
	// 计算显式帧率
	else if ((clock() - beg_ms_) / (CLOCKS_PER_SEC / 1000) >= 1000) // 一秒计算一次 fps
	{
		render_fps_ = count_;
		count_ = 0;
		beg_ms_ = clock();
	}
	switch (frame->format) {
	case AV_PIX_FMT_YUV420P:
		// 代码通用性 字节对齐问题（直接显式在Frame定义的时候设置好 linesize的参数）
		return Draw(frame->data[0], frame->linesize[0], // Y
			frame->data[1], frame->linesize[1], // U
			frame->data[2], frame->linesize[2]); // V
		break;

	case AV_PIX_FMT_BGRA:
		return Draw(frame->data[0], frame->linesize[0]);
		break;
	default:
		break;
	}
	return true;
}
