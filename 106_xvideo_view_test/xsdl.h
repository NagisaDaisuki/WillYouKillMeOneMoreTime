#pragma once
#include "XVideoView.h"
struct SDL_Window; // 前向声明
struct SDL_Renderer;
struct SDL_Texture;

class XSDL :
    public XVideoView
{
public:

	void Close() override;

	bool IsExit() override;
    /////////////////////////////////////////
	//// 初始化渲染窗口 线程安全 可多次调用
	//// @para w 窗口宽度
	//// @para h 窗口高度
	//// @para fmt 像素格式
	//// @para win_id 窗口句柄, 如果为空，创建新窗口
	//// @return 是否创建成功
	/////////////////////////////////////////
	bool Init(int w, int h,
		PixFormat fmt = RGBA, void* win_id = nullptr) override;
	
	/////////////////////////////////////////
	//// 渲染图像 线程安全
	//// @para data 渲染的二进制图像数据
	//// @para linesize 图像每行字节数, 对于 YUV420P就是Y一行字节数
	//// linesize <= 0 就根据宽度和像素格式自动算出大小
	//// @return 是否渲染成功
	/////////////////////////////////////////
	bool Draw(const unsigned char* data, int linesize = 0) override;

private:
	SDL_Window* win_ = nullptr;
	SDL_Renderer* render_ = nullptr;
	SDL_Texture* texture_ = nullptr;
};

