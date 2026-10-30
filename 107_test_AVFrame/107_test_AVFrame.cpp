#include <iostream>

extern "C" { // 指定函数是c 语言函数，函数名不包含重载标注
	// 引用 ffmpeg 头文件
#include <libavcodec/avcodec.h>
}

// 预处理指令导入库
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

int main(int argc, char* argv[]) 
{
	std::cout << "First FFmpeg: " << std::endl;
	std::cout << avcodec_configuration() << std::endl;

	// 创建 frame 对象
	auto frame1 = av_frame_alloc();

	// 图像参数
	frame1->width = 401;
	frame1->height = 300;
	frame1->format = AV_PIX_FMT_ARGB;
	
	// 分配空间
	// 0 是默认对齐方式
	// int re = av_frame_get_buffer(frame1, 0);
	int re = av_frame_get_buffer(frame1, 16);
	if (re != 0) 
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf));
		std::cout << buf << std::endl;
	}

	std::cout << "frame1 linesize:" << frame1->linesize[0] << '\n';
	// frame1->width = 400; -> linesize[0] = 1600
	// frame1->width = 401; -> linesize[0] = 1632
	// 一行像素 400，一个像素占用 4 个字节
	// 1600 表示一行字节数，401 时，字节数为 1632
	// 表示 按照32字节对齐(默认对齐方式) -> av_frame_get_buffer(frame1, 0)
	// 如果 av_frame_get_buffer(frame1,16) 表示
	// 按照 16字节对齐 linesize[0] = 1616
	// 最少是 4 字节对齐 
	// 在 Qt 的 QImage 中默认是 16字节对齐的
	// YUV 格式的对齐方式
	
	// 读取 AVFrame 引用计数
	if (frame1->buf[0]) {
		std::cout << "frame1 ref count = " <<
			av_buffer_get_ref_count(frame1->buf[0]); // 线程安全
		std::cout << '\n';
	}
	
	// 创建另一个 AVFrame 对象
	AVFrame* frame2 = av_frame_alloc();
	av_frame_ref(frame2,frame1); 
	// 两个 AVFrame 指向同一个空间
	if (frame1->buf[0]) {
		std::cout << "frame1 ref count = " <<
			av_buffer_get_ref_count(frame1->buf[0]); // 线程安全
		std::cout << '\n';
	}
	if (frame2->buf[0]) {
		std::cout << "frame2 ref count = " <<
			av_buffer_get_ref_count(frame2->buf[0]); // 线程安全
		std::cout << '\n';
	}

	// 引用计数减一 并将 frame2 的 buf 清零
	av_frame_unref(frame2);
	std::cout << "av_frame_unref(frame2)" << std::endl;

	if (frame1->buf[0]) {
		std::cout << "frame1 ref count = " <<
			av_buffer_get_ref_count(frame1->buf[0]); // 线程安全
		std::cout << '\n';
	}

	// 引用计数为1时 直接删除 buf 空间 引用计数变为 0(无法访问)
	av_frame_unref(frame1);

	// 释放 frame 对象空间并且引用计数减一
	// buf已为空，只删除frame 对象空间
	av_frame_free(&frame1);
	av_frame_free(&frame2);
	// getchar();
	return 0;
}
