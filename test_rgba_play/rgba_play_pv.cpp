// 基于 std::mutex 和 
// std::condition_variable 
// 的生产者-消费者模型
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

#undef main

const char* filename = "Pensil.rgba";

const int RGBA_WIDTH = 1280;
const int RGBA_HEIGHT = 720;
const int FRAME_SIZE = RGBA_WIDTH * RGBA_HEIGHT * 4;

// 线程安全的数据队列 （存放读出来的帧数据）
std::queue<unsigned char*> frameQueue;
std::mutex queueMutex;
std::condition_variable queueCV;

// 用于退出子线程的标志
bool isExit = false;

// 子线程函数：只负责读文件
void ReadThreadFunc(const char* filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		std::cerr << "Failed to open file in read thread!" << std::endl;
		return;
	}

	while (!isExit)
	{
		// 限制队列最大长度，防止读的太快爆内存（最多存 10 帧）
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCV.wait(lock, [] { return frameQueue.size() < 10 || isExit; });
			if (isExit) break;
		}

		unsigned char* buffer = new unsigned char[FRAME_SIZE];
		file.read((char*)buffer, FRAME_SIZE);
		
		// 循环播放逻辑
		if (file.gcount() < FRAME_SIZE)
		{
			file.clear(); // 清除 EOF 标志
			file.seekg(0, std::ios::beg); // 回到文件开头
			file.read((char*)buffer, FRAME_SIZE);
		}

		// 将读到的数据压入队列
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			frameQueue.push(buffer);
		}
		queueCV.notify_one(); // 唤醒主线程
	}
}

int main(int argc, char* argv[])
{
	std::cout << "Multi-thread SDL RGBA Player Start..." << std::endl;
	
	// 1. 初始化 SDL（必须在主线程）
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
	
	SDL_Window* window = SDL_CreateWindow("Multi-thread RGBA Player",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		RGBA_WIDTH, RGBA_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (!window)
	{
		std::cerr << "SDL Create Window Failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return -1;
	}

	// 在 SDL_CreateRenderer 之前调用
	// "0" 或 "nearest"：最近邻（像素化，适合像素游戏）
	// "1" 或 "linear"：线性插值（默认，平滑但稍微模糊）
	// "2" 或 "best"：各向异性过滤（仅部分后端支持，画质最好，但最吃性能）
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	// 2. 创建渲染器（去掉垂直同步，防止拖动时 RenderPresent死等）
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED);

	if (!renderer)
	{
		std::cerr << "SDL Create Renderer Failed: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// 3. 创建 RGBA 纹理
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING, RGBA_WIDTH, RGBA_HEIGHT);
	if (!texture)
	{
		std::cerr << "SDL Create Texture Failed: " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// 4. 启动子线程读取 RGBA 文件
	std::thread readThread(ReadThreadFunc,filename);
	
	// 变量准备
	bool quit = false;
	SDL_Event event;
	const int FPS = 25;
	const int FRAME_DELAY = 1000 / FPS;
	Uint32 frameStart;
	
	// 5. 主线程循环渲染
	while (!quit)
	{
		frameStart = SDL_GetTicks(); // 记录这一帧开始的时间

		// --- 事件处理 ---
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) quit = true;
		}

		// 从队列获取数据
		unsigned char* renderBuffer = nullptr;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			if (!frameQueue.empty())
			{
				renderBuffer = frameQueue.front();
				frameQueue.pop();
			}
		}
		queueCV.notify_one();	// 告诉子线程队列有空位了，继续读

		// 如果拿到了新的一帧，就更新纹理并释放旧内存
		if (renderBuffer) 
		{
			SDL_UpdateTexture(texture, NULL, renderBuffer, RGBA_WIDTH * 4);
			delete[] renderBuffer; // 用完就删，防止内存泄露
		}

		// --- 渲染 ---
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer,texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		// 动态延时控制（精确控帧）
		Uint32 elapsed = SDL_GetTicks() - frameStart;
		if (elapsed < FRAME_DELAY)
		{
			SDL_Delay(FRAME_DELAY - elapsed);
		}
		
	}

	// 6. 清理与退出
	isExit = true;
	queueCV.notify_all(); // 唤醒可能还在等待的子线程，让它们退出
	if (readThread.joinable()) readThread.join(); // 等待子线程结束

	// 清空队列残留内存
	while (!frameQueue.empty())
	{
		delete[] frameQueue.front();
		frameQueue.pop();
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return EXIT_SUCCESS;
}
