#pragma once
#include "xformat.h"

/// <summary>
/// 媒体封装类
/// </summary>

class XMux :
    public XFormat
{
public:
    /// <summary>
    /// 打开封装
    /// </summary>
    /// <param name="url">地址</param>
    /// <returns></returns>
    static AVFormatContext* Open(const char* url);
    
    /// <summary>
    /// 写入头部信息
    /// </summary>
    /// <returns>写入成功返回 true</returns>
    bool WriteHeader();

    /// <summary>
    /// 写入Pkt
    /// </summary>
    /// <param name="pkt">要写入的</param>
    /// <returns></returns>
    bool Write(AVPacket* pkt);

    bool WriteEnd();
};

