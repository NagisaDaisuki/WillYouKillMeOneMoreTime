#include "stdafx.h"
#include "xvideoview.h"
#include "xsdl.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
# pragma comment(lib,"avutil.lib")

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
