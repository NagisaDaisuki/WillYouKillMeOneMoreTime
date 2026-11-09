#include <iostream>
#include <fstream>
// 引用 ffmpeg 头文件
// 用 extern "C" 包裹 FFmpeg 的 C头文件, 函数名不包含重载标注
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// 预处理指令导入库
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")

int main(int argc, char* argv[]) {
	int re;
	std::string filename= "400_300_25";
	AVCodecID codec_id = AV_CODEC_ID_H264;
	if (argc > 1) 
	{
		std::string codec{argv[1]};
		if (codec == "h265" || codec == "hevc") 
		{
			codec_id = AV_CODEC_ID_HEVC;
		}
	}

	if (codec_id == AV_CODEC_ID_H264) 
	{
		filename += ".h264";
	}
	else if (codec_id == AV_CODEC_ID_HEVC)
	{
		filename += ".h265";
	}
	std::ofstream ofs;
	ofs.open(filename, std::ios::binary);


	// 1. 找到编码器 AV_CODEC_ID_HEVC(H265)
	auto codec = avcodec_find_encoder(codec_id);
	if (!codec) 
	{
		std::cerr << "cannot find codec!" << std::endl;
		return -1;
	}

	// 2. 编码上下文的创建
	AVCodecContext* c = avcodec_alloc_context3(codec);
	if (!c)
	{
		std::cerr << "avcodec_alloc_context3 failed!" << std::endl;
		return -1;
	}

	// 3. 设定上下文参数
	c->width = 400;
	c->height = 300;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	// 帧时间戳的时间单位 pts * time_base == 播放时间（秒）
	c->time_base = {1, 25}; // 分数 1/25
	// 时间基必须由用户设置
	// 帧的时间戳一秒所占比重
	
	// 打开编码器
	re = avcodec_open2(c, codec, NULL);
	if (re < 0)
	{
		std::cerr << "Cannot open codec!" << std::endl;
		return -1;
	}

	// 创建好 AVFrame 空间 存放未压缩数据
	auto frame = av_frame_alloc();
	frame->width = c->width;
	frame->height = c->height;
	frame->format = c->pix_fmt;

	re = av_frame_get_buffer(frame, 0);
	if (re != 0) {
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		std::cerr << "av_frame_get_buffer failed!" << buf << std::endl;
		return -1;
	}
	
	auto pkt = av_packet_alloc();
	// 10 秒视频 250 帧
	for (int i = 0; i < 250; ++i) {

		// 生成AVFrame 数据
		// 每帧数据不同
		// Y
		for (int y = 0; y < c->height; ++y) {
			for (int x = 0; x < c->width; ++x) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}
		// UV
		for (int y = 0; y < c->height / 2; ++y) {
			for (int x = 0; x < c->width / 2; ++x) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}
		frame->pts = i; // 设置帧的时间戳

		// 发送未压缩帧到线程中压缩
		re = avcodec_send_frame(c, frame);
		if (re != 0) 
		{
			break; // 偶尔一帧会发送失败，可能是编码器内部缓冲区满了
		}

		while (re >= 0)  // 返回多帧
		{
			// 接收压缩帧，一般前几次调用返回空
			// 缓冲立刻返回编码未完成
			// 编码是在独立的线程中进行的，
			// 编码器内部有一个缓冲区，
			// 发送帧到编码器后，
			// 编码器会在独立线程中进行压缩，
			// 压缩完成后会将压缩后的数据放入缓冲区中，
			// 接收压缩帧时，如果缓冲区中没有数据，则返回空	
			re = avcodec_receive_packet(c, pkt);
			if (re == AVERROR(EAGAIN) || re == AVERROR_EOF)
				break;
			if (re < 0) // 编码出错
			{
				char buf[1024] = { 0 };
				av_strerror(re, buf, sizeof(buf) - 1);
				std::cerr << "avcodec_receive_packet failed!" << buf << std::endl;
				break;
			}
			std::cout << pkt->size << " " << std::flush;
			ofs.write((char*)pkt->data, pkt->size);
			// 每次调用 avcodec_receive_packet 都会返回一个新的 AVPacket
			// 也就是 重新分配 pkt内部的空间
			av_packet_unref(pkt);
		}
	}

	// 关闭 ofs
	ofs.close();

	// 释放 AVPacket
	av_packet_free(&pkt);

	// 释放AVFrame
	av_frame_free(&frame);

	// 释放编码上下文
	avcodec_free_context(&c);

	// std::cout << "First ffmpeg: " << '\n';
	// std::cout << avcodec_configuration() << '\n';
	return 0;
}