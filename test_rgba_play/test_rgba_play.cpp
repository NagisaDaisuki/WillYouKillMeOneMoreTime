#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "SDL2.lib")
#undef main // 避免 SDL 重定义 main 函数

// 假设输出的 RGBA 分辨率是 1280x720 
const int RGBA_WIDTH = 1280;
const int RGBA_HEIGHT = 720;
const int FRAME_SIZE = RGBA_WIDTH * RGBA_HEIGHT * 4;


int main(int argc, char* argv[])
{
	std::cout << "Pure SDL RGBA Player Start..." << std::endl;

	// 1. 打开 RGBA 裸流文件 
	std::ifstream rgbaFile("Pensil.rgba", std::ios::binary);
	if (!rgbaFile)
	{
		std::cerr << "Failed to open RGBA file!" << std::endl;
		return -1;
	}

	// 2. 初始化 SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
		return -1;
	}

	// 3. 创建窗口和渲染器
	SDL_Window* window = SDL_CreateWindow("SDL RGBA Player",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		RGBA_WIDTH, RGBA_HEIGHT, SDL_WINDOW_SHOWN);
	if (!window)
	{
		std::cerr << "SDL Create Window Failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
	{
		std::cerr << "SDL Create Renderer Failed: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// 4. 创建 RGBA 纹理
	 // 注意：这里使用 SDL_PIXELFORMAT_ABGR8888 是因为在主流小端机器上，
	// RGBA 的字节排列在内存中是 R, G, B, A，对应 SDL 的 ABGR8888 格式。
	// 如果画面颜色不对（比如红蓝颠倒），可以换成 SDL_PIXELFORMAT_RGBA8888 试试。
	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING, // 频繁修改的渲染
		RGBA_WIDTH, RGBA_HEIGHT);
	if (!texture)
	{
		std::cerr << "SDL Create Texture Failed: " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// 分配内存，用于读取文件
	unsigned char* rgbaBuffer = new unsigned char[FRAME_SIZE];

	bool quit = false;
	SDL_Event event;

	// 用于计算帧率的简单时间控制（假设 30 帧）
	const int FPS = 25;
	const int FRAME_DELAY = 1000 / FPS;

	// 5. 主循环 
	while (!quit)
	{
		// 处理退出事件
		while (SDL_PollEvent(&event)) 
		{
			if (event.type == SDL_QUIT)
				quit = true;
		}
		
		// 读取 一帧数据
		rgbaFile.read((char*)rgbaBuffer, FRAME_SIZE);
		
		// 如果文件读完了，回到文件开头重新读（循环播放）
		if (rgbaFile.gcount() < FRAME_SIZE)
		{
			rgbaFile.clear(); // 清除 EOF 标志
			rgbaFile.seekg(0, std::ios::beg); // 回到文件开头
			rgbaFile.read((char*)rgbaBuffer, FRAME_SIZE);
		}

		// 6. 将内存中的 RGBA 数据上传到 GPU纹理中
		// 这里的 pitch 是每行的字节数，即 RGBA_WIDTH * 4
		SDL_UpdateTexture(texture, NULL, rgbaBuffer, RGBA_WIDTH * 4);

		// 7. 清屏并渲染纹理
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL); // nullptr 表示铺满整个窗口
		SDL_RenderPresent(renderer);

		// 简单的帧率控制
		SDL_Delay(FRAME_DELAY);
	}
	
	// 8. 释放资源
	delete[] rgbaBuffer;
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
