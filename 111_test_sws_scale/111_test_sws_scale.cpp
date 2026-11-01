#include <iostream>
#include <fstream>
extern "C" { // 指定函数 是 c 语言函数，函数名不包含重载标注
// #include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

const char* yuv_file = "Pensil.yuv";
const char* rgba_file = "Pensil.rgba";
const char* output_yuv_file = "Pensil_Reverse.yuv";
// #pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swscale.lib")
int main(int argc, char* argv[])
{
	std::cout << "111_test_sws_scale" << std::endl;
	// 1920 x 1080 YUV 转 1280 x 720 RGBA
	int width = 1920;
	int height = 1080;
	int rgba_width = 1280;
	int rgba_height = 720;
	// 平面存储YUV420P  yyyy uu vv
	unsigned char* yuv[3] = {0};
	int yuv_linesize[3] = {width, width / 2, width / 2};
	yuv[0] = new unsigned char[width * height];		// Y
	yuv[1] = new unsigned char[width * height / 4];	// U
	yuv[2] = new unsigned char[width * height / 4];	// V
	
	// 交错存储RGBA rgba rgba
	unsigned char* rgba = new unsigned char[rgba_width * rgba_height * 4];
	int rgba_linesize = rgba_width * 4;	// 未对齐

	// 获取源文件
	std::ifstream ifs;
	ifs.open(yuv_file, std::ios::binary);
	if (!ifs) { 
		std::cerr << "Open yuv file " << yuv_file << " failed." << std::endl;
		return -1;
	}
	
	// 输出 rgba 文件
	std::ofstream ofs; 
	ofs.open(rgba_file, std::ios::binary);
	if (!ofs) {
		std::cerr << "Open rgba file " << rgba_file << " failed." << std::endl;
		return -1;
	}
	

	SwsContext* yuvToRgb = nullptr;
	while (true) 
	{
		// 读取 YUV 帧
		ifs.read((char*)yuv[0], width * height);
		ifs.read((char*)yuv[1], width * height / 4);
		ifs.read((char*)yuv[2], width * height / 4);
		
		if (ifs.gcount() == 0) break;

		// YUV 转 RGBA
		// 转换上下文，NULL 新创建，非NULL判断与现有参数是否
		// 一致，一致直接返回，不一致先清理当前然后再次创建
		yuvToRgb = sws_getCachedContext(yuvToRgb,
			width, height,				// 输入 宽高
			AV_PIX_FMT_YUV420P,			// 输入 像素格式
			rgba_width, rgba_height,		// 输出 宽高
			AV_PIX_FMT_RGBA,			// 输出 像素格式
			SWS_BILINEAR,				// 选择支持变换的 算法，双线性插值
			0,0,0);						// 过滤器参数
		if (!yuvToRgb)		
		{
			std::cerr << "sws_getCachedContext failed" << std::endl;
		}
		
		unsigned char* data[1];
		data[0] = rgba;
		int lines[1] = {rgba_linesize};
		int re = sws_scale(yuvToRgb,
			yuv,			// 输入数据
			yuv_linesize,	// 输入数据行字节数
			0,				// 从源图像的第几行开始处理。通常传 0，表示从顶部开始处理整帧。
			height,			// 输出高度
			data,			// 输出数据
			lines			// 输出行大小
			);
		std::cout << re << " " << std::flush;
		ofs.write((char*)rgba, rgba_width * rgba_height * 4);
	}

	// RGBA 转 YUV420P
	ofs.close();
	ifs.close();
	ifs.open(rgba_file, std::ios::binary); // 一定要以二进制模式打开
	// 要写入的输出文件(应该是和原 yuv文件一样的)
	std::ofstream out_yuv(output_yuv_file, std::ios::binary);


	SwsContext* RgbaToYuv = nullptr;
	for (;;)
	{
		// 读取 RGBA 帧
		ifs.read((char*)rgba, rgba_width * rgba_height * 4);
		if (ifs.gcount() == 0) break;
		
		RgbaToYuv = sws_getCachedContext(RgbaToYuv,
			rgba_width, rgba_height,
			AV_PIX_FMT_RGBA,
			width, height,
			AV_PIX_FMT_YUV420P,
			SWS_BILINEAR,
			0,0,0);

		if (!RgbaToYuv) 
		{
			std::cerr << "sws_getCachedContext failed" << std::endl;
		}
		
		unsigned char* data[1];
		data[0] = rgba;
		int linesize[1] = {rgba_linesize};
		auto re = sws_scale(RgbaToYuv,
			data,
			linesize,
			0,
			rgba_height, // 输入数据高度
			yuv,
			yuv_linesize);
		std::cout << "(" << re << ")" << " " << std::flush;

		// 把转换出来的 yuv 写进文件
		out_yuv.write((char*)yuv[0], width * height);
		out_yuv.write((char*)yuv[1], width * height / 4);
		out_yuv.write((char*)yuv[2], width * height / 4);
	}


	delete[] yuv[0];
	delete[] yuv[1];
	delete[] yuv[2];
	delete[] rgba;
	ifs.close();
	out_yuv.close();
	return 0;
}