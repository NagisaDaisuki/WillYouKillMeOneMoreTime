#pragma once
#include <mutex>

// 前置声明
struct AVFormat;
struct AVFormatContext;
struct AVCodecParameters;
struct AVPacket;

struct XRational { // 1:1 复刻 
	int num;	// Numerator
	int den;	// Denominator
};

void PrintErr(int err);
#define BERR(err) if (err!=0) {PrintErr(err);return 0;} // 返回0 false

/// <summary>
/// 封转和解封转的基类
/// </summary>
class XFormat
{
public:
	/// <summary>
	/// 复制参数，线程安全
	/// </summary>
	/// <param name="stream_index">对应c_streams 下标</param>
	/// <param name="dst">输出参数</param>
	/// <returns>是否成功</returns>
	bool CopyPara(int stream_index, AVCodecParameters* dst);

	/// <summary>
	/// 设置上下文，并且清理上次设置的值，如果传递NULL相当于
	/// 关闭上下文，线程安全
	/// </summary>
	/// <param name="c">全局上下文</param>
	void set_c(AVFormatContext* c);
	
	int audio_index() { return audio_index_; }
	int video_index() { return video_index_; }
	XRational video_time_base() { return video_time_base_; }
	XRational audio_time_base() { return audio_time_base_; }
	
	// 根据 timebase 换算时间
	bool RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base);
protected:
	AVFormatContext* c_ = nullptr;	// 封装解封装上下文
	std::mutex mux_;	// 针对c_上下文资源 互斥访问
	// video audio 在 stream 中的索引
	int audio_index_ = 1;	// 音频默认值1
	int video_index_ = 0;	// 视频默认值0
	XRational video_time_base_ = {1,25}; // 视频时间基数
	XRational audio_time_base_ = {1,9000}; // 音频时间基数
};

