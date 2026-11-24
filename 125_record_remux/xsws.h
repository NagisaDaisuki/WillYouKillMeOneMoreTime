#pragma once
#include <mutex>

extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")

class XSws
{
public:
	XSws() = default;
	~XSws();
	
	// 禁止拷贝
	XSws(const XSws&) = delete;
	XSws& operator=(const XSws&) = delete;
	
	/// <summary>
	/// 缩放并转换格式
	/// </summary>
	/// <param name="in_frame">输入的解码帧</param>
	/// <param name="out_frame">输出的缩放帧（空间由本类自动分配）</param>
	/// <param name="dst_w">目标宽度</param>
	/// <param name="dst_h">目标高度</param>
	/// <param name="dst_fmt">目标像素格式</param>
	/// <returns>成功返回 true</returns>
	bool Scale(const AVFrame* in_frame, AVFrame* out_frame, int dst_w,
		int dst_h, AVPixelFormat dst_fmt);
private:
	
	SwsContext* sws_ctx_ = nullptr;
	
	// 记录上一次的输入参数，用于判断还是否需要重新创建上下文
	int last_src_w_ = 0;
	int last_src_h_ = 0;
	AVPixelFormat last_src_fmt_ = AV_PIX_FMT_NONE;
	
	std::mutex mtx_;
};

