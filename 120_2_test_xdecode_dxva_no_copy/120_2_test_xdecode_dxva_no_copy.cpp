#include <string>
#include <fstream>
#include <iostream>
#include "xvideoview.h"
#include "xdecode.h"
#include <windows.h>
#include <d3d9.h>

extern "C" { // 指定函数是 c语言函数，函数名不包含重载标注
// 引用 FFmpeg 头文件 
#include <libavcodec/avcodec.h> 
#include <libavutil/opt.h>
}

struct DXVA2DevicePriv
{
	HMODULE d3dlib;
	HMODULE dxva2lib;
	HANDLE device_handle;
	IDirect3D9* d3d9;
	IDirect3DDevice9* d3d9device;
};

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib") 

void DrawFrame(AVFrame* frame, AVCodecContext* c)
{
	// frame的释放问题
	if (!frame->data[3] || !c) return;
	std::cout << "D" << std::flush;

	auto surface = (IDirect3DSurface9*)frame->data[3]; // 强制类型转换为 surface类型
	auto ctx = (AVHWDeviceContext*)c->hw_device_ctx->data;
	/*
		AVHWDeviceInternal *internal = (AVHWDeviceInternal*)device_ctx->internal;
		DXVA2DevicePriv *priv = (DXVA2DevicePriv*)internal->priv;
	*/
	auto priv = (DXVA2DevicePriv*)ctx->user_opaque;
	auto device = priv->d3d9device;
	static HWND hwnd = nullptr;
	static RECT viewport;
	
	if (!hwnd) // 第一次创建窗口
	{
		hwnd = CreateWindow(L"DX",L"TEST DXVA",
			WS_OVERLAPPEDWINDOW,
			200, 200, frame->width, frame->height,
			0,0,0,0); // 窗口创建
		// 显示窗口
		ShowWindow(hwnd, 1);
		UpdateWindow(hwnd);
		viewport.left = 0;
		viewport.right = frame->width;
		viewport.top = 0;
		viewport.bottom = frame->height;
	}
	// 设置显示窗口句柄
	device->Present(&viewport, &viewport, hwnd, 0);
	// 后台缓冲表面
	static IDirect3DSurface9* back = nullptr;
	if (!back)
		device->GetBackBuffer(0,0, D3DBACKBUFFER_TYPE_MONO, &back);
	device->StretchRect(surface, 0, back, &viewport, D3DTEXF_LINEAR);
}

int main(int argc, char* argv[])
{
	// win窗口注册
	WNDCLASSEX wc;
	// 置零
	memset(&wc,0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	// 设置回调函数
	wc.lpfnWndProc = DefWindowProc; // 消息函数
	wc.lpszClassName = L"DX"; // Win 宽字节L
	RegisterClassEx(&wc);
	

	int re{};
	auto view = XVideoView::Create();

	// 1. 分割 h264, 存入 AVPacket
	std::string filename = "Pensil.h264";
	std::ifstream ifs(filename,std::ios::binary);
	if (!ifs) return -1;
	unsigned char inbuf[4096] = {0};
	AVCodecID codec_id = AV_CODEC_ID_H264;
	
	XDecode de;
	// 根据编码器创建上下文
	auto c = de.Create(codec_id, false);
	de.set_c(c);

	// 初始化硬件加速上下文
	de.initHW();
	
	// 打开编解码器
	de.Open();


	// 初始化分割上下文
	auto parser = av_parser_init(codec_id);
	
	// pkt 创建
	auto pkt = av_packet_alloc();
	
	// frame 创建
	auto frame = av_frame_alloc();

	// 硬解码转换用 frame 创建
	auto hw_frame = av_frame_alloc();

	// 设置 Begin 时间(Ms)
	auto begin = NowMs();
	int count = 0;	// 解码统计
	bool is_init_win = false;
	while (!ifs.eof())
	{
		ifs.read((char*)inbuf, sizeof(inbuf));
		if (ifs.eof())  // 正好到结尾处
		{
			// ifs.clear();	// 清理EOF 标志
			// ifs.seekg(0, std::ios::beg);
			// ifs.read((char*)inbuf, sizeof(inbuf));
		}
		int data_size = ifs.gcount();	// 读取的字节数
		if (data_size <= 0) break;
		
		auto data = inbuf;

		while (data_size > 0)  // 一次有多帧数据
		{
			// 通过 0001 截断输出到 AVPacket, 返回帧大小
			int ret = av_parser_parse2(
				parser,
				c,
				&pkt->data,					// 输出数据
				&pkt->size,					// 输出数据大小
				data,						// 输入数据
				data_size,					// 输入数据大小
				AV_NOPTS_VALUE,
				AV_NOPTS_VALUE,
				0
			);
			// 移动指针位置
			data += ret;
			// 已处理的数据减去
			data_size -= ret;

			if (pkt->size) 
			{
				// 发送 Packet到解码线
				if (!de.Send(pkt)) // 如果发送失败直接退出
					break;
				// 设置 false 直接不拷贝直接在GPU上渲染
				while (de.Recv(frame, false)) 
				{
					///////////////////////////////////////////
					/// 第一帧初始化窗口，只在最初获取
					///////////////////////////////////////////
					if (!is_init_win)
					{
						// frame->format 获取实际格式
						// （AV_PIX_FMT_NV12 或 AV_PIX_FMT_YUV420P）
						// 动态初始化，动态传给 view->Init 初始化渲染器。
						// view->Init(frame->width, frame->height,(XVideoView::PixFormat)frame->format);
						is_init_win = true;
					}
					DrawFrame(frame, c);
					// view->DrawFrame(frame);
					count++;
					auto cur = NowMs();
					if (cur - begin >= 100) // 1/10 秒钟计算一次
					{
						std::cout << "\nfps = " << count * 10 << std::endl; 
						count = 0;
						begin = cur;
					}
				}
			}
		}
	}

	// 完成结尾帧的获取
	auto frames = de.End();
	for (auto f : frames) 
	{
		view->DrawFrame(f);
		av_frame_free(&f);
	}

	// 释放 pkt
	av_packet_free(&pkt);

	// 释放 frame
	av_frame_free(&frame);

	// 释放上下文
	// avcodec_free_context(&c);

	// 关闭分割上下文
	av_parser_close(parser);

	return EXIT_SUCCESS;
}