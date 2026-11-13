#include "xvideoview.h"
#include "xsdl.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <thread>
#include <chrono>
#include <iostream>

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

long long NowMs() 
{
	return clock() / (CLOCKS_PER_SEC / 1000);
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

bool XVideoView::Open(std::string filepath) 
{
	if (ifs_.is_open())
		ifs_.close();
	ifs_.open(filepath, std::ios::binary);
	return ifs_.is_open();
}

AVFrame* XVideoView::Read() 
{
	if (width_ <= 0 || height_ <= 0 || !ifs_)
	{
		return NULL;
	}
	// AVFrame 空间已经申请，如果参数发生变化，需要释放空间
	if (frame_) 
	{
		if (frame_->width != width_ ||
			frame_->height != height_ ||
			frame_->format != fmt_) 
		{
			// 释放 AVFrame 对象空间，并且
			// buf 引用计数减一

			av_frame_free(&frame_);
		}
	}

	if (!frame_)
	{
		frame_ = av_frame_alloc();
		// 分配对象空间和像素空间
		frame_->width = width_;
		frame_->height = height_;
		frame_->format = fmt_;
		frame_->linesize[0] = width_ * 4; // 默认 RGBA 4字节对齐
		if (frame_->format == AV_PIX_FMT_YUV420P)
		{
			frame_->linesize[0] = width_;		// Y
			frame_->linesize[1] = width_ / 2;	// U
			frame_->linesize[2] = width_ / 2;	// V
		}
		else if (frame_->format == AV_PIX_FMT_RGB24)
		{
			frame_->linesize[0] = width_ * 3;	// RGB24 每像素 3 字节
		}
		// 生成 AVFrame空间，使用默认对齐
		auto re = av_frame_get_buffer(frame_, 0); // 32 字节对齐
		if (re != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			std::cout << "av_frame_get_buffer failed: " << buf << std::endl;
			// 分配内存失败也要释放AVFrame 对象
			av_frame_free(&frame_);
			return NULL;
		}
	}
	if (!frame_) return NULL;

	// 读取一帧数据
	if (frame_->format == AV_PIX_FMT_YUV420P)
	{
		ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_);
		ifs_.read((char*)frame_->data[1], frame_->linesize[1] * height_ / 2);
		ifs_.read((char*)frame_->data[2], frame_->linesize[2] * height_ / 2);
	}
	// else if (frame_->format == AV_PIX_FMT_RGBA || frame_->format == AV_PIX_FMT_BGRA
	// 	|| frame_->format == AV_PIX_FMT_ARGB) {
	// 	ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_);
	// }
	else 
	{
			ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_);
	}

	if (ifs_.gcount() == 0)
	{
		return nullptr;
		// ifs_.clear();
		// ifs_.seekg(0, std::ios::beg);
		// if (frame_->format == AV_PIX_FMT_YUV420P)
		// {
		// 	ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_); 
		// 	ifs_.read((char*)frame_->data[1], frame_->linesize[0] * height_ / 2); 
		// 	ifs_.read((char*)frame_->data[2], frame_->linesize[0] * height_ / 2);
		// }
		// else 
		// {
		// 	ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_);
		// }
	}
	return frame_;
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
	case AV_PIX_FMT_ARGB: 
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_RGB24:
		return Draw(frame->data[0], frame->linesize[0]);
		break;
	default:
		break;
	}
	return true;
}
