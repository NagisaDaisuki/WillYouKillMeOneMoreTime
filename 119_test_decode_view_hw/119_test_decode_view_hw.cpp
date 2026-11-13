#include <string>
#include <fstream>
#include <iostream>
#include "xsdl.h"
extern "C" { // 指定函数是 c语言函数，函数名不包含重载标注
// 引用 FFmpeg 头文件 
#include <libavcodec/avcodec.h> 
#include <libavutil/opt.h>
}

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib") 

int main(int argc, char* argv[])
{
	int re{};
	auto view = XVideoView::Create();

	// 1. 分割 h264, 存入 AVPacket
	std::string filename = "Pensil1.h264";
	std::ifstream ifs(filename,std::ios::binary);
	if (!ifs) return -1;
	unsigned char inbuf[4096] = {0};
	AVCodecID codec_id = AV_CODEC_ID_H264;


	// 找解码器
	auto codec = avcodec_find_decoder(codec_id);
	if (!codec) 
	{
		std::cerr << "avcodec_find_decoder failed!" << std::endl;
		return -1;
	} 
	
	
	
	// 创建解码器上下文
	auto codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
	{
		std::cerr << "avcodec_alloc_context3 failed!" << std::endl;
		return -1;
	}	
	// 硬件加速格式
	auto hw_type = AV_HWDEVICE_TYPE_DXVA2;
	/////////////////////////////////////////////
	/// 打印所有支持的硬件加速方式
	/////////////////////////////////////////////
	for (int i = 0; ; i++)
	{
		auto config = avcodec_get_hw_config(codec, i);
		if (!config) break;	// 未取到任何支持方式
		if (config->device_type)
			std::cout << av_hwdevice_get_type_name(config->device_type) << std::endl;
	}
	// 初始化硬件加速上下文
	AVBufferRef* hw_ctx = nullptr;
	av_hwdevice_ctx_create(&hw_ctx,
							hw_type,
							NULL,
							NULL,
							0);
	codec_ctx->hw_device_ctx = av_buffer_ref(hw_ctx);

	// 添加多线程
	codec_ctx->thread_count = 16;

	// 打开解码器
	re = avcodec_open2(codec_ctx, NULL, NULL);
	if (re != 0)
	{
		std::cerr << "avcodec_open2 failed!" << std::endl;
		return -1;
	}
	

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
			ifs.clear();	// 清理EOF 标志
			ifs.seekg(0, std::ios::beg);
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
				codec_ctx,
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
				// std::cout << pkt->size << " " << std::flush;
				// 发送 Packet到解码线
				ret = avcodec_send_packet(codec_ctx, pkt);
				if (ret < 0)
					break;
				// 获取多帧解码数据
				while (ret >= 0) 
				{
					// 每次会调用 av_frame_unref()
					ret = avcodec_receive_frame(codec_ctx, frame);
					if (ret < 0)
						break;

					auto pframe = frame; // 同时支持硬解码和软解码
					
					// 区分硬解码和软解码
					if (codec_ctx->hw_device_ctx)
					{
						// 硬解码转换 GPU --> CPU
						// 显存 --> 内存
						av_hwframe_transfer_data(hw_frame,frame,0);
						pframe = hw_frame;
					}

					std::cout << frame->format << " " << std::flush;
					///////////////////////////////////////////
					/// 第一帧初始化窗口
					///////////////////////////////////////////
					if (!is_init_win)
					{
						view->Init(pframe->width, pframe->height,(XVideoView::PixFormat)pframe->format);
						is_init_win = true;
					}
					view->DrawFrame(pframe);
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

	// 取出缓存当中的数据
	int ret = avcodec_send_packet(codec_ctx, NULL);
	while (ret >= 0) {
		ret = avcodec_receive_frame(codec_ctx, frame);
		if (ret < 0) break;
		std::cout << frame->format << "-" << std::flush;
	}

	// 释放 pkt
	av_packet_free(&pkt);

	// 释放 frame
	av_frame_free(&frame);

	// 释放上下文
	avcodec_free_context(&codec_ctx);

	// 关闭分割上下文
	av_parser_close(parser);

	return EXIT_SUCCESS;
}