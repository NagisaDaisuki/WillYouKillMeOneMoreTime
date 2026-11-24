#include "xdecode.h"
#include <iostream>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

///////////////////////////////////////////
/// 发送解码
///////////////////////////////////////////
bool XDecode::Send(const AVPacket* pkt)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	auto re = avcodec_send_packet(c_, pkt);
	if (re != 0) return false;	// 可以不打印错误避免流媒体阻塞
	return true;
}

///////////////////////////////////////////
/// 获取编码
///////////////////////////////////////////
bool XDecode::Recv(AVFrame* frame)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;
	// 【软解】：直接解码到用户传进来的 frame 中
	if (!c_->hw_device_ctx)
	{
		auto re = avcodec_receive_frame(c_, frame);
		if (re == 0) return true;
		return false;	// 失败直接返回，不能动原frame
	}

	// 【硬解】：分配一个临时的硬件 frame 接收显存数据
	AVFrame* tmp_hw_frame = av_frame_alloc();
	auto re = avcodec_receive_frame(c_, tmp_hw_frame);
	if (re == 0)
	{
		// 显存转内存，转移到用户的 frame 里
		re = av_hwframe_transfer_data(frame, tmp_hw_frame, 0);
		av_frame_free(&tmp_hw_frame); // 临时硬件帧用完必须立刻释放
		if (re != 0)
		{
			PrintErr(re);
			return false;
		}
		return true;
	}
	// 硬解失败(如EAGAIN)的清理
	av_frame_free(&tmp_hw_frame);
	return false;

	// auto f = frame;
	// if (c_->hw_device_ctx) // 如果是硬件加速
	// 	f = av_frame_alloc();	// 一定要做分配 frame清理
	// auto re = avcodec_receive_frame(c_, f); // 接收到视频帧
	// if (re == 0) 
	// {
	// 	if (c_->hw_device_ctx) // GPU解码
	// 	{
	// 		// 显存转内存 GPU -> CPU
	// 		re = av_hwframe_transfer_data(frame, f, 0);
	// 		av_frame_free(&f);
	// 		if (re != 0) // 如果转换失败
	// 		{
	// 			PrintErr(re);
	// 			return false;
	// 		}
	// 	}
	// 	return true;
	// }
	// // 异常情况释放
	// av_frame_free(&f);
	// return false;
}

///////////////////////////////////////////
/// 获取缓存
///////////////////////////////////////////
std::vector<AVFrame*> XDecode::End()
{	
	std::vector<AVFrame*> res{};
	// 取出缓存当中的数据
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return res;

	int ret = avcodec_send_packet(c_, NULL);
	if (ret < 0 && ret != AVERROR_EOF) return res;

	while (true) {
		AVFrame* out_frame = nullptr;
		// 这里如果使用硬件加速要先把硬件帧(显存里)的
		// 调用 av_hwframe_transfer_data 转换后
		// 再 push_back 到 res 最后把原来的硬件帧
		// free掉
		if (!c_->hw_device_ctx) // 软解
		{
			out_frame = av_frame_alloc();
			ret = avcodec_receive_frame(c_, out_frame);
			if (ret < 0) 
			{
				av_frame_free(&out_frame);
				break;
			}
		}
		else // 硬解
		{
			AVFrame* hw_frame = av_frame_alloc();
			ret = avcodec_receive_frame(c_, hw_frame); 
			if (ret < 0)
			{
				av_frame_free(&hw_frame);// 失败，释放硬件帧
				break;
			}
			out_frame = av_frame_alloc();
			ret = av_hwframe_transfer_data(out_frame, hw_frame, 0);
			av_frame_free(&hw_frame); //  硬件帧用完立刻销毁，不管成功
			if (ret != 0)
			{
				PrintErr(ret);
				av_frame_free(&out_frame); // 转换失败，释放软件帧
				break;
			}
		}
		res.push_back(out_frame);
		std::cout << out_frame->format << "-" << std::flush;
	}

	return res;
}

///////////////////////////////////////////
/// 初始化硬件加速 初始值 4 -> AV_HWDEVICE_TYPE_DXVA2
///////////////////////////////////////////
bool XDecode::initHW(int type)
{
	std::unique_lock<std::mutex> lock(mtx_);
	if (!c_) return false;

	AVBufferRef* hw_device_ctx = nullptr;	// 硬件加速上下文
	auto re = av_hwdevice_ctx_create(&hw_device_ctx,(AVHWDeviceType)type, NULL, NULL, 0);
	if (re != 0)
	{
		PrintErr(re);
		return false;
	}
	c_->hw_device_ctx = hw_device_ctx; // 设置硬件加速
	std::cout << "Hardware Acceleration: " << type << std::endl;
	return true;
}

