#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <thread>
#include "xdemux.h"
#include "xmux.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

#define CHKERR(err) if (err!=0) {PrintErr(err);return 0;} // 返回0 false

int main(int argc, char* argv[]) 
{
	// 使用说明
	char usage[256] = "使用：124_test_xformat 输入文件 输出文件 开始时间（秒） 结束时间（秒）\n";
	const char* example = "例子：124_text_xformat Pensil.mp4 Pensil_output.mp4\n\n\n";
	strncat(usage,example,strlen(example));
	fputs(usage, stdout);
	
	if (argc < 3) { // 使用错误
		return -1;
	}
	std::string in_file{argv[1]};
	std::string out_file{argv[2]}; 

	//////////////////////////////////////////
	/// 设置截断时间
	//////////////////////////////////////////
	int begin_sec{ 0 };		// 截取开始时间
	int end_sec{ 0 };			// 截取结束时间
	if (argc > 3) 
	{
		begin_sec = atoi(argv[3]);
	}
	if (argc > 4)
	{
		end_sec = atoi(argv[4]);
	}

	long long video_begin_pts{};
	long long audio_begin_pts{};
	long long video_end_pts{};
	// long long audio_end_pts{};
	
	
	int ret = 0;
	//////////////////////////////////////////
	// 解封装部分
	//////////////////////////////////////////
	
	XDemux demux;
	auto demux_c = XDemux::Open(in_file.c_str());
	demux.set_c(demux_c);

	AVStream* as = nullptr;	// 音频流
	AVStream* vs = nullptr;	// 视频流
	// 拿到音视频流信息
	for (int i = 0; i < demux_c->nb_streams; ++i)
	{
		if (demux_c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			as = demux_c->streams[i];
			std::cout << "=========音频=========" << std::endl;
			std::cout << as->codecpar->sample_rate << std::endl;
		}
		else if (demux_c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vs = demux_c->streams[i];
			std::cout << "=========视频=========" << std::endl;
			std::cout << vs->codecpar->width << std::endl;
			std::cout << vs->codecpar->height << std::endl;
		}
	}

	//////////////////////////////////////////
	// 封装部分
	//////////////////////////////////////////
	XMux mux;
	auto mux_c = XMux::Open(out_file.c_str());
	mux.set_c(mux_c);

	auto mvs = mux_c->streams[mux.video_index()];
	auto mas = mux_c->streams[mux.audio_index()];

	// 设置编码音视频流参数
	// 判断是否有视频流
	if (demux.video_index() >= 0)
	{
		mvs->time_base = {demux.video_time_base().num, demux.video_time_base().den};
		// 复制视频参数
		demux.CopyPara(demux.video_index(), mvs->codecpar);
	}

	// 判断是否有音频流
	if (demux.audio_index() >= 0) 
	{
		mas->time_base = {demux.audio_time_base().num, demux.audio_time_base().den};
		// 复制音频参数
		demux.CopyPara(demux.audio_index(),mas->codecpar);
	}
	
	// 写入文件头
	mux.WriteHeader();
	
	// 开始截断秒数，算出输入视频的pts
	if (begin_sec > 0) 
	{
		if (demux.video_index() >= 0 && demux.video_time_base().num > 0)
		{
			double t = (double)(demux.video_time_base().den / demux.video_time_base().num);
			video_begin_pts = begin_sec * t;
			video_end_pts = end_sec * t;
			demux.Seek(video_begin_pts, demux.video_index()); // 移动到开始帧
		}
		if (demux.audio_index() >= 0 && demux.audio_time_base().num > 0)
		{
			double t = (double)(demux.audio_time_base().den / demux.audio_time_base().num);
			audio_begin_pts = begin_sec * t;
			// audio_end_pts = end_sec * t; // 以视频时间作为基准不需要音频的结尾时间戳
			// demux.Seek(audio_begin_pts, demux.audio_index()); // 移动到开始帧
		}
	}
	
	int audio_frame_count = 0;
	int video_frame_count = 0;
	double total_sec = 0.0;
	AVPacket* pkt = av_packet_alloc();
	for (;;) 
	{
		if (!demux.Read(pkt))
			break;	// 到结尾处
		
		long long offset_pts = 0;
		// 判断是视频流并且视频流时间 大于结束时间
		if (video_end_pts > 0 &&
			pkt->stream_index == demux.video_index() && 
			pkt->pts > video_end_pts)
		{
			av_packet_unref(pkt); // 手动释放 pkt
			break;
		}
		
		if (pkt->stream_index == demux.video_index())
		{
			offset_pts = video_begin_pts;
			mux.RescaleTime(pkt,offset_pts, demux.video_time_base());
			video_frame_count++;
			// 根据 视频播放duration 计算总播放时长
			if (demux.video_time_base().den > 0)
				total_sec += pkt->duration * (double)demux.video_time_base().num / (double)demux.video_time_base().den;
		}
		else if (pkt->stream_index == demux.audio_index())
		{
			offset_pts = audio_begin_pts;
			mux.RescaleTime(pkt,offset_pts, demux.audio_time_base());
			audio_frame_count++;
		}
		
		// 写入音视频帧 会清理 pkt
		mux.Write(pkt);
	}
	
	// 写入 尾部trailer
	// MP4 的 moov 索引头是在写入文件尾时才正式生成并刷入磁盘
	mux.WriteEnd();

	// 释放 pkt
	av_packet_free(&pkt);

	// 清理全局上下文
	//avformat_close_input(&demux_c);
	mux.set_c(nullptr);
	demux.set_c(nullptr); // 资源清理
	std::cout << "\n输出文件: " << out_file << "\n";
	std::cout << "视频帧: " << video_frame_count << "\n";
	std::cout << "音频帧: " << audio_frame_count << "\n";
	std::cout << "总时长: " << total_sec << "\n";
	
	return EXIT_SUCCESS;
}
