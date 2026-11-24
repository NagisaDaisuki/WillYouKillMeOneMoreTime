#include <iostream>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

// 预处理命令导入库
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

void PrintErr(int err) 
{
	char buf[1024] = {0};
	av_strerror(err, buf, sizeof(buf) - 1);
	std::cerr << buf << std::endl;
}
#define CHKERR(err) if (err!=0) {PrintErr(err);return -1;}

int main(int argc, char* argv[]) 
{
	int ret = 0;
	
	//////////////////////////////////////////
	// 解封装部分
	//////////////////////////////////////////
	
	// 全局上下文
	AVFormatContext* fmt_ctx = NULL;
	std::string filename = "Pensil.mp4";
	// 打开媒体文件，解封装输入上下文
	ret = avformat_open_input(&fmt_ctx, filename.c_str(),
		NULL,	// 解封装封装器格式 NULL 自动探测(根据后缀名或文件头探测)
		NULL);	// 参数设置, rtsp设置
	CHKERR(ret);
	

	// 获取媒体信息 无头部格式
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	CHKERR(ret);

	// 打印封装信息函数
	av_dump_format(fmt_ctx, 0, filename.c_str(),
		0 // 0 表示上下文是输入 1 表示上下文输出
	);

	AVStream* as = nullptr;	// 音频流
	AVStream* vs = nullptr;	// 视频流
	// 拿到音视频流信息
	for (int i = 0; i < fmt_ctx->nb_streams; ++i)
	{
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			as = fmt_ctx->streams[i];
			std::cout << "=========音频=========" << std::endl;
			std::cout << as->codecpar->sample_rate << std::endl;
		}
		else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vs = fmt_ctx->streams[i];
			std::cout << "=========视频=========" << std::endl;
			std::cout << vs->codecpar->width << std::endl;
			std::cout << vs->codecpar->height << std::endl;

		}
	}

	//////////////////////////////////////////
	// 封装部分
	//////////////////////////////////////////
	const char* out_filename = "out.mp4";
	// 编码器上下文
	AVFormatContext* ec = nullptr;
	ret = avformat_alloc_output_context2(&ec, NULL, NULL, 
		out_filename); // 根据文件名推测封装格式
	CHKERR(ret); // 判断返回值的错误

	// 添加视频流、音频流
	auto mvs = avformat_new_stream(ec, NULL); // 视频流
	auto mas = avformat_new_stream(ec, NULL); // 音频流
	
	// 打开输入IO
	ret = avio_open(&ec->pb, out_filename, AVIO_FLAG_WRITE);
	CHKERR(ret);

	// 设置编码音视频流参数
	// ec->streams[0]
	if (vs) 
	{
		mvs->time_base = vs->time_base; // 时间基数与原视频一致
		// 从解封装复制参数
		avcodec_parameters_copy(mvs->codecpar, vs->codecpar);
	}
	if (as) 
	{
		mas->time_base = as->time_base;
		// 从解封装复制参数
		avcodec_parameters_copy(mas->codecpar, as->codecpar);
	}
	
	// 写入文件头
	ret = avformat_write_header(ec, NULL);
	CHKERR(ret);
	
	// 打印输出上下文
	av_dump_format(ec, 0, out_filename, 1);
	
	//////////////////////////////////////////
	// 截取 10 ~ 20秒之间的音视频 
	// 取多不取少
	// 假定 9 11 秒有关键帧 取 9秒（约定俗成）
	//////////////////////////////////////////
	double begin_sec = 10.0; // 截取开始时间
	double end_sec = 20.0;	// 截取结束时间
	long long begin_pts = 0; // 开始显示时间戳
	long long end_pts = 0;	// 结束显示时间戳
	long long begin_audio_pts = 0; // 音频开始时间戳
	long long end_audio_pts = 0;	// 音频结束时间戳
	// 换算成 pts 换算成输入 ic 的 pts, 以视频流为准
	if (vs && vs->time_base.num > 0) // 分子大于0
	{
		// sec / time_base = pts //除以分子乘以分母(所以分子不能为0)
		// pts = sec / (num/den) = sec * (den/num)
		double t = (double)vs->time_base.den 
			/ (double)vs->time_base.num;	// 分母 / 分子
		begin_pts = begin_sec * t;
		end_pts = end_sec * t;
	}
	if (as && as->time_base.num > 0) 
	{
		begin_audio_pts = begin_sec * ((double)as->time_base.den / (double)as->time_base.num);
		end_audio_pts = end_sec * ((double)as->time_base.den / (double)as->time_base.num);
	}

	// seek 输入媒体文件 移动到第10秒的关键帧位置 向后关键帧
	if (vs)
		ret = av_seek_frame(fmt_ctx,vs->index,begin_pts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);// 默认就是 AVSEEK_FLAG_FRAME 
	CHKERR(ret);

	AVPacket* pkt = av_packet_alloc();
	for (;;) 
	{
		ret = av_read_frame(fmt_ctx,pkt);
		if (ret != 0)
		{
			PrintErr(ret);
			break;
		}
		CHKERR(ret);
		AVStream* in_stream = fmt_ctx->streams[pkt->stream_index];
		AVStream* out_stream = nullptr;
		long long offset_pts = 0;	// 偏移 pts, 用于 turncate 开头 pts运算
		if (vs && pkt->stream_index == vs->index) 
		{
			std::cout << "视频流信息: ";

			// 超过 第20秒退出，只存 10秒数据（10~20）
			if (pkt->pts > end_pts)
			{
				av_packet_unref(pkt); // 手动清理 pkt
				break;
			}
			out_stream = ec->streams[0];	// 在上面输出上下文
											// 打开时设置好流打开先后顺序了
			offset_pts = begin_pts;
		}
		else if (as && pkt->stream_index == as->index)
		{
			std::cout << "音频流信息: ";
			out_stream = ec->streams[1];
			offset_pts = begin_audio_pts;
		}

		// 重新计算 pts, dts, duration
		// 保证兼容性问题
		// a * bq(输入time_base) / cq(输出time_base)
		if (out_stream)
		{
			// 这里当调用 av_seek_frame() 跳转到原视频第 10 秒时，
			// 后续 av_read_frame() 读出来的数据包依然保留着原视
			// 频的绝对时间戳（例如 pts 从 10s 对应的刻度开始，一直到 20s）。
			/*
				如果不减 begin_pts（直接写原时间戳）：
				封装器写入 out.mp4 时，音视频轨道的起始
				时间就是 10s。多数播放器打开该文件时，会认为
				前 0 ~ 10s 是空白时间段——表现为总时长显示为
				20 秒，前 10 秒画面死死定格在第一帧、没有声音，
				直到进度条走到第 10 秒才开始正常播放；部分严格
				校验时间戳的播放器或流媒体协议甚至会直接报错拒播。
				减去 begin_pts 之后：
				把第 10 秒基准点平移为新文件的 0 秒：
				$$\text{新时间戳} = \text{av\_rescale\_q\_rnd}(\text{原时间戳} - \text{begin\_pts},\ \text{in\_time\_base},\ \text{out\_time\_base},\ \dots)$$
				这样 10s ~ 20s 的原始时间戳就会被映射为 0s ~ 10s，
				生成的 out.mp4 总时长为标准的 10 秒，点开即播。
			*/
			pkt->pts = av_rescale_q_rnd(pkt->pts - offset_pts, in_stream->time_base,
				out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt->dts = av_rescale_q_rnd(pkt->dts - offset_pts, in_stream->time_base,
				out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			
			pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
			pkt->pos = -1;
		}

		// 正常释放 pkt 空间
		std::cout << pkt->pts << " : " << pkt->dts <<
			" : " << pkt->size << std::endl;
		
		// 写入音视频帧 会清理 pkt
		ret = av_interleaved_write_frame(ec,pkt); 
		if (ret != 0)
		{
			PrintErr(ret);
		}

		// av_packet_unref(&pkt);
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
	// 写入 尾部trailer
	// MP4 的 moov 索引头是在写入文件尾时才正式生成并刷入磁盘
	av_write_trailer(ec);
	
	// 释放 pkt
	av_packet_free(&pkt);

	// 清理全局上下文
	avformat_close_input(&fmt_ctx);
	
	avio_closep(&ec->pb);

	avformat_free_context(ec);
	ec = nullptr;

	return EXIT_SUCCESS;
}