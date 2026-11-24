#pragma once
#include "xformat.h"

void PrintErr(int err);

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
};

