#include "stdafx.h"
#include "xsdl.h"
#include <SDL.h>
#include <iostream>
#pragma comment(lib, "SDL2.lib")
static bool InitVideo()
{
	static bool is_first = true;
	static std::mutex mux;
	std::unique_lock<std::mutex> sdl_lock(mux);
	if (!is_first) return true;
	is_first = false;
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	// 设定缩放算法解决锯齿问题
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	// 1 是线性插值算法，0 是临近值缩放（默认会抗锯齿）
	return true;
}

bool XSDL::Init(int w, int h,
	PixFormat fmt, void* win_id)
{
	if (w <= 0 || h <= 0) return false;

	// 初始化 SDL 视频库 放在锁外
	InitVideo();

	// 考虑线程安全问题
	// mtx.lock(); 忘记写 unlock会造成死锁
	std::unique_lock<std::mutex> sdl_lock(mtx_);
	// 在栈中创建的一个锁，退出函数时自动析构调用 unlock
	// 确保线程安全

	width_ = w;
	height_ = h;
	fmt_ = fmt;
	

	//////////////////////////////////////////
	//// 1. 创建窗口
	//////////////////////////////////////////
	if (!win_) 
	{ // 如果窗口未创建
		// 没有传递 win_id
		if (!win_id) 
		{
			win_ = SDL_CreateWindow("",
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				w, h,
				SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

		}
		// 窗口未创建但是 有 winId
		else {
			// 渲染到控件窗口
			win_ = SDL_CreateWindowFrom(win_id);
		}
	}

	if (!win_)
	{
		// 创建窗口失败
		std::cerr << SDL_GetError() << std::endl;
		return false;
	}

	//////////////////////////////////////////
	//// 2. 创建渲染器
	//////////////////////////////////////////
	render_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
	if (!render_)
	{
		// 创建渲染器失败
		std::cerr << SDL_GetError() << std::endl;
		return false;

	}
	//////////////////////////////////////////
	//// 3. 创建材质（显存）
	////////////////////////////////////////////
	unsigned int sdl_fmt = SDL_PIXELFORMAT_UNKNOWN;
	switch (fmt) {
	case XVideoView::RGBA:
		sdl_fmt = SDL_PIXELFORMAT_RGBA8888; break;
	case XVideoView::ARGB:
		sdl_fmt = SDL_PIXELFORMAT_ARGB32; break;
	case XVideoView::YUV420P:
		sdl_fmt = SDL_PIXELFORMAT_IYUV; break;
	}

	texture_ = SDL_CreateTexture(render_,
		sdl_fmt, // 像素格式
		SDL_TEXTUREACCESS_STREAMING, // 频繁修改的渲染（带锁）
		w,h							// 材质大小 
	);
	if (!texture_) {
		std::cerr << SDL_GetError() << std::endl;
		return false;
	}
	return true;
}

bool XSDL::Draw(const unsigned char* data, int linesize)
{
	// 防御性编程
	if (!data) return false; 
	
	// 确保线程安全
	std::unique_lock<std::mutex> sdl_lock(mtx_);
	if (!texture_ || !render_ || !win_ || width_ <= 0 || height_ <= 0) {
		return false;
	}
	if (linesize <= 0)
	{
		switch (fmt_) {
		case XVideoView::RGBA:
		case XVideoView::ARGB:
			linesize = width_ * 4;
			break;
		case XVideoView::YUV420P:
			linesize = width_;
			break;
		default:
			break;
		}
	}
	if (linesize <= 0)
		return false;
	
	// 复制内存到显存
	auto re = SDL_UpdateTexture(texture_, NULL, data, linesize);
	if (re != 0) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}

	// 清空屏幕
	SDL_RenderClear(render_);


	// 获取渲染器（Label）的真实大小
	// int out_w, out_h;
	// SDL_GetRendererOutputSize(render_, &scale_w_, &scale_h_);
	// 材质复制到渲染器
	if (scale_w_ <= 0) scale_w_ = width_;
	if (scale_h_ <= 0) scale_h_ = height_;
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = scale_w_; // 渲染的宽高，可缩放 
	rect.h = scale_h_;

	re = SDL_RenderCopy(render_,texture_, NULL, &rect);
	if (re != 0) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	
	SDL_RenderPresent(render_);
	return true;
}