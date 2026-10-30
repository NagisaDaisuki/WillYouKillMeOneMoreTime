#include <iostream>
#include <ctime>
#include <thread>
#include <chrono>
#include <windows.h>

#pragma comment(lib, "winmm.lib")

using namespace std::chrono_literals;

void MSleep(unsigned int ms) 
{
	auto beg = clock();
	for (int i = 0; i < ms; i++) 
	{
		std::this_thread::sleep_for(1ms);
		if ((clock() - beg) / (CLOCKS_PER_SEC / 1000) >= ms)
			break;
	}
}

int main(int argc, char* argv[])
{
	timeBeginPeriod(1); // 将系统时钟精度提升到 1ms
	// 测试 C++11 的 sleep
	// 测试 sleep 10毫秒 100fps
	auto beg = clock();		// 开始时间，CPU跳数
	int fps = 0; // 帧率
	for (;;) 
	{
		fps++;
		auto tmp = clock();
		// std::this_thread::sleep_for(10ms);
		// std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::this_thread::sleep_for(10ms);
		// 精度不够，每次刷新帧率都会浪费 20ms以上
		std::cout << clock() - tmp << " " << std::flush;
		// 一秒钟开始统计 CLOCKS_PER_SEC 表示每秒跳数
		if ((clock() - beg) / (CLOCKS_PER_SEC / 1000) > 1000) {// 间隔毫秒数
			std::cout << "sleep for fps: " << fps << std::endl;
			break;
		}
	}
	
	// 测试 wait 事件超时控制 帧率
	auto handle = CreateEvent(NULL,FALSE,FALSE, NULL);
	fps = 0;
	beg = clock();
	for (;;)
	{
		fps++;
		WaitForSingleObject(handle, 10);
		if ((clock() - beg) / (CLOCKS_PER_SEC / 1000) > 1000) { // 间隔毫秒数
			std::cout << "wait fps: " << fps << std::endl;
			break;
		}
	}
	fps = 0;
	beg = clock();
	for (;;)
	{
		fps++;
		MSleep(10);
		if ((clock() - beg) / (CLOCKS_PER_SEC / 1000) > 1000) { // 间隔毫秒数
			std::cout << "MSleep fps: " << fps << std::endl;
			break;
		}
	}
	

	// 退出前恢复默认设置（虽然进程结束系统也会恢复）
	timeEndPeriod(1);
	return EXIT_SUCCESS;
}