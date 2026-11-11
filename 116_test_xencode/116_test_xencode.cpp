#include <iostream>
#include <fstream>
#include "xencode.h"
// 引用 ffmpeg 头文件
// 用 extern "C" 包裹 FFmpeg 的 C头文件, 函数名不包含重载标注
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

// 预处理指令导入库
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")

// ==========================================================
// 🌟 测试开关：取消注释你想测试的模式（一次只能开一个）
// ==========================================================
// #define TEST_ABR        // 平均码率 (避免使用，画质最差)
// #define TEST_CQP        // 恒定 QP (画质好，体积不可控)
// #define TEST_CRF        // 恒定速率因子 (点播首选，纯 VBR)
// #define TEST_CBR        // 恒定比特率 (直播首选，带填充)
#define TEST_CRF_VBV    // CRF + 约束 (高质量在线点播)

int main(int argc, char* argv[])
{
	int re;
	std::string filename = "400_300_25_preset_";
	AVCodecID codec_id = AV_CODEC_ID_H264;
	if (argc > 1)
	{
		std::string codec{ argv[1] };
		if (codec == "h265" || codec == "hevc")
		{
			codec_id = AV_CODEC_ID_HEVC;
		}
	}

#if defined(TEST_ABR)
	filename += "abr";
#elif defined(TEST_CQP)
	filename += "cqp";
#elif defined(TEST_CRF)
	filename += "crf";
#elif defined(TEST_CBR)
	filename += "cbr";
#elif defined(TEST_CRF_VBV)
	filename += "crf_vbv";
#endif

	if (codec_id == AV_CODEC_ID_H264)
		filename += ".h264";
	else if (codec_id == AV_CODEC_ID_HEVC)
		filename += ".h265";

	// 输出文件
	std::ofstream ofs;
	ofs.open(filename, std::ios::binary);

	// 创建上下文
	XEncode en;
	auto c = en.Create(codec_id);
	en.set_c(c);

	c->width = 400;
	c->height = 300;
	en.SetOpt("crf", 18); // 恒定速率因子

	en.Open();

	auto frame = en.CreateFrame();
	// auto pkt = 
	int count = 0; // 写入文件的帧数 SPS PPS IDR 放在一帧中
	for (int i = 0; i < 500; ++i) {
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
		auto pkt = en.Encode(frame);
		if (pkt) // 成功得到压缩后的Packet
		{
			count++;
			ofs.write((char*)pkt->data, pkt->size);
			//av_packet_unref(pkt);
			av_packet_free(&pkt);
		}
		
	}

	auto pkts = en.End();	// 拿到pkts
	for (auto pkt : pkts)
	{
		count++;
		ofs.write((char*)pkt->data, pkt->size);
		av_packet_free(&pkt);
	}

	// 关闭输出流
	ofs.close();

	// 手动释放 AVPacket
	// av_packet_free(&pkt);

	// 手动释放 AVFrame
	av_frame_free(&frame);

	// AVCodecContext由 XEncode 类对象管理
	en.set_c(nullptr);

	std::cout << "encode " << count << std::endl;
	return 0;
}


//	// 1. 找到编码器 AV_CODEC_ID_HEVC(H265)
//	auto codec = avcodec_find_encoder(codec_id);
//	if (!codec) 
//	{
//		std::cerr << "cannot find codec!" << std::endl;
//		return -1;
//	}
//
//	// 2. 编码上下文的创建
//	AVCodecContext* c = avcodec_alloc_context3(codec);
//	if (!c)
//	{
//		std::cerr << "avcodec_alloc_context3 failed!" << std::endl;
//		return -1;
//	}
//
//	// 3. 设定上下文参数
//	c->width = 400;
//	c->height = 300;
//	c->pix_fmt = AV_PIX_FMT_YUV420P;
//	// 帧时间戳的时间单位 pts * time_base == 播放时间（秒）
//	c->time_base = {1, 25}; // 分数 1/25
//	// 时间基必须由用户设置
//	// 帧的时间戳一秒所占比重
//
//	c->thread_count = 16; // 编码线程数，可以通过调用系统接口获取cpu核心数量
//	
//	// 预设编码器参数
//	c->max_b_frames = 0; // B 帧设为0，降低延时，增大空间
//	re = av_opt_set(c->priv_data,"preset", "ultrafast", 0);
//	if (re != 0) 
//	{
//		std::cout << "preset failed." << std::endl;
//	}
//	av_opt_set(c->priv_data, "tune", "zerolatency", 0); // 零延时
//	if (re != 0) 
//	{
//		std::cout << "tune failed." << std::endl;
//	}
//
//	int target_bitrate = 400000;	// 测试用目标码率 400k
//
//	// ==========================================================
//	// 🌟 4. 核心：根据宏定义配置码率控制算法
//	// ==========================================================
//#if defined(TEST_ABR)
//	// 模式 1：ABR 平均比特率 (硬凑目标码率，画质波动大)
//	c->bit_rate = target_bitrate;
//	c->rc_max_rate = target_bitrate;
//	c->rc_buffer_size = target_bitrate * 2;
//	std::cout << "当前模式: ABR (平均码率)" << std::endl;
//#elif defined(TEST_CQP)
//	// 模式 2：CQP 恒定 QP (每一帧的压缩力度恒定，不管体积)
//	c->flags |= AV_CODEC_FLAG_QSCALE;
//	c->global_quality = FF_QP2LAMBDA * 23;
//	// av_opt_set_int(c->priv_data, "qp", 23, 0);
//	std::cout << "当前模式: CQP (恒定 QP 23)" << std::endl;
//#elif defined(TEST_CRF)
//	// 模式 3：CRF 恒定质量 (最常用，根据画质决定码率)
//	av_opt_set(c->priv_data, "crf", "23", 0);
//	std::cout << "当前模式: CRF (恒定质量 CRF 23)" << std::endl;
//#elif defined(TEST_CBR)
//	// 模式 4：CBR 恒定比特率 (加填充，适合推流。注意：封装成 MP4 会丢弃填充数据)
//	av_opt_set(c->priv_data, "nal-hrd", "cbr", 0);
//	c->bit_rate = target_bitrate;
//	c->rc_min_rate = target_bitrate;
//	c->rc_max_rate = target_bitrate;
//	c->rc_buffer_size = target_bitrate * 2;
//	std::cout << "当前模式: CBR (恒定码率 400k)" << std::endl;
//#elif defined(TEST_CRF_VBV)
//	// 模式 5：CRF + VBV 约束 (B站/YouTube 首选，保画质且防卡顿)
//	av_opt_set(c->priv_data, "crf", "23", 0);
//	c->rc_max_rate = target_bitrate;	// 限制瞬间最大码率
//	c->rc_buffer_size = target_bitrate;	// 缓冲区
//	std::cout << "当前模式: CRF + VBV 约束" << std::endl;
//	
//#endif
//
//	// 打开编码器上下文
//	re = avcodec_open2(c, codec, NULL);
//	if (re != 0)
//	{
//		std::cerr << "Cannot open codec!" << std::endl;
//		return -1;
//	}
//
//	// 创建好 AVFrame 空间 存放未压缩数据
//	auto frame = av_frame_alloc();
//	frame->width = c->width;
//	frame->height = c->height;
//	frame->format = c->pix_fmt;
//
//	re = av_frame_get_buffer(frame, 0);
//	if (re != 0) {
//		char buf[1024] = { 0 };
//		av_strerror(re, buf, sizeof(buf) - 1);
//		std::cerr << "av_frame_get_buffer failed!" << buf << std::endl;
//		return -1;
//	}
//	
//	auto pkt = av_packet_alloc();
//	// 10 秒视频 250 帧
//	for (int i = 0; i < 250; ++i) {
//
//		// 生成AVFrame 数据
//		// 每帧数据不同
//		// Y
//		for (int y = 0; y < c->height; ++y) {
//			for (int x = 0; x < c->width; ++x) {
//				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
//			}
//		}
//		// UV
//		for (int y = 0; y < c->height / 2; ++y) {
//			for (int x = 0; x < c->width / 2; ++x) {
//				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
//				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
//			}
//		}
//		frame->pts = i; // 设置帧的时间戳
//
//		// 发送未压缩帧到线程中压缩
//		re = avcodec_send_frame(c, frame);
//		if (re != 0) 
//		{
//			break; // 偶尔一帧会发送失败，可能是编码器内部缓冲区满了
//		}
//
//		while (re >= 0)  // 返回多帧
//		{
//			// 接收压缩帧，一般前几次调用返回空
//			// 缓冲立刻返回编码未完成
//			// 编码是在独立的线程中进行的，
//			// 编码器内部有一个缓冲区，
//			// 发送帧到编码器后，
//			// 编码器会在独立线程中进行压缩，
//			// 压缩完成后会将压缩后的数据放入缓冲区中，
//			// 接收压缩帧时，如果缓冲区中没有数据，则返回空	
//			re = avcodec_receive_packet(c, pkt);
//			if (re == AVERROR(EAGAIN) || re == AVERROR_EOF)
//				break;
//			if (re < 0) // 编码出错
//			{
//				char buf[1024] = { 0 };
//				av_strerror(re, buf, sizeof(buf) - 1);
//				std::cerr << "avcodec_receive_packet failed!" << buf << std::endl;
//				break;
//			}
//			// std::cout << pkt->size << " " << std::flush;
//			
//			// 分析 NALU
//			// 一个AVPacket 中包含多个 NALU 以0001 间隔，
//			// 多个是以 001 间隔
//			///// 0001[NALU_HEAD]
//			///// [NALU_HEAD] 1 个字节
//			///// orbidden_bit(1bit), nal_reference_bit(2bits)(优先级)
//			///// nal_unit_type(5bits)
//
//			int nal_unit_type = 0;
//			unsigned char nal_head = *(pkt->data + 4); // +4 去掉开头的 0001
//			// 取后五位
//			nal_unit_type = nal_head & 0x1f;	
//			std::cout << nal_unit_type << " " << std::flush;
//			for (int i = 4; i < pkt->size - 4; ++i) { // 一个 data 中有多条nalu
//				if (pkt->data[i] == 0 &&
//					pkt->data[i + 1] == 0 &&
//					pkt->data[i + 2] == 1) {
//					nal_unit_type = pkt->data[i + 3] & 0x1f;
//					std::cout << "(" << nal_unit_type << ")" << std::flush;
//				}
//			}
//			ofs.write((char*)pkt->data, pkt->size);
//			// 每次调用 avcodec_receive_packet 都会返回一个新的 AVPacket
//			// 也就是 重新分配 pkt内部的空间
//			av_packet_unref(pkt);
//		}
//	}
//
//	// 关闭 ofs
//	ofs.close();
//
//	// 释放 AVPacket
//	av_packet_free(&pkt);
//
//	// 释放AVFrame
//	av_frame_free(&frame);
//
//	// 释放编码上下文
//	avcodec_free_context(&c);
//
//	// std::cout << "First ffmpeg: " << '\n';
//	// std::cout << avcodec_configuration() << '\n';
//	return 0;