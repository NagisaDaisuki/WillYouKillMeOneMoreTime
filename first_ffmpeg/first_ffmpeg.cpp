#include <iostream>
// 引用 ffmpeg 头文件
// 用 extern "C" 包裹 FFmpeg 的 C头文件, 函数名不包含重载标注
extern "C" {
#include <libavcodec/avcodec.h>
}

// 预处理指令导入库
#pragma comment(lib,"avcodec.lib")

int main(int argc, char* argv[]) {
	std::cout << "First ffmpeg: " << '\n';
	std::cout << avcodec_configuration() << '\n';
	return 0;
}