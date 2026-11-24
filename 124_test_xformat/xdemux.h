#pragma once
#include "xformat.h"

class XDemux : public XFormat
{
public:
	/// <summary>
	/// 打开解封装
	/// </summary>
	/// <param name="url">解封装地址，支持rtsp</param>
	/// <returns>失败返回 nullptr, 成功返回对象空间</returns>
	static AVFormatContext* Open(const char* url);

	/// <summary>
	/// 读取一帧数据
	/// </summary>
	/// <param name="pkt">输出数据</param>
	/// <returns>是否成功</returns>
	bool Read(AVPacket *pkt);
	
	/// <summary>
	/// seek 输入媒体文件 移动到指定pts关键帧位置 向后关键帧
	/// </summary>
	/// <param name="pts">指定pts关键帧时间戳</param>
	/// <param name="stream_index">指定流索引</param>
	/// <returns>执行成功返回true</returns>
	bool Seek(long long pts, int stream_index);
};

