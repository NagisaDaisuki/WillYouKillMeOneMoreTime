#include "stdafx.h"
#include "sdlrgbqt.h"
#include <SDL.h>
#pragma comment(lib, "SDL2.lib")
static int sdl_width = 0;
static int sdl_height = 0;
static int pix_size = 4;
static SDL_Renderer* sdl_renderer = nullptr;
static SDL_Window* sdl_window = nullptr;
static SDL_Texture* sdl_texture = nullptr;
static unsigned char* rgb = nullptr;
SdlRgbQt::SdlRgbQt(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    // 将 SDL 初始化延迟到事件循环空闲时
    QTimer::singleShot(0,this, &SdlRgbQt::initSDL);
}

void SdlRgbQt::timerEvent(QTimerEvent* ev)
{
    static int tmp = 255;
    tmp--;
    for (int j = 0; j < sdl_height; j++)
    {
        int b = j * sdl_width * pix_size;
        for (int i = 0; i < sdl_width * pix_size; i += pix_size)
        {
            rgb[b + i + 0] = (unsigned char)(i / pix_size); // R
            rgb[b + i + 1] = (unsigned char)(tmp);             // G
            rgb[b + i + 2] = (unsigned char)(255 - tmp);       // B
			rgb[b + i + 3] = 255;                             // A
        }
    }

	SDL_UpdateTexture(sdl_texture, nullptr, rgb, sdl_width * pix_size);
    SDL_RenderClear(sdl_renderer);
    SDL_Rect sdl_rect;
    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = sdl_width;
	sdl_rect.h = sdl_height;
	SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &sdl_rect);
	SDL_RenderPresent(sdl_renderer);
}
void SdlRgbQt::initSDL() 
{
    // 获取物理像素尺寸
    qreal dpr = ui.label->devicePixelRatioF();
    // 这里获得的是真实的 QLabel 的尺寸
    sdl_width = ui.label->width() * dpr;
	sdl_height = ui.label->height() * dpr;
    
    // 初始化SDL视频
    SDL_Init(SDL_INIT_VIDEO);
    // 创建 SDL 窗口，使用 SDL_WINDOW_FOREIGN 标志
    sdl_window = SDL_CreateWindowFrom((void*)ui.label->winId());
    if (!sdl_window) {
        qDebug() << "SDL_CreateWindowFrom Error: " << SDL_GetError();
        return;
    }
    // 创建 SDL 渲染器
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl_renderer) {
        qDebug() << "SDL_CreateRenderer Error: " << SDL_GetError();
        return;
    }
    // 创建纹理
    sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, sdl_width, sdl_height);
    if (!sdl_texture) {
        qDebug() << "SDL_CreateTexture Error: " << SDL_GetError();
        return;
    }
    // 分配内存
    rgb = new unsigned char[sdl_width * sdl_height * pix_size];

    // 设置渲染器的逻辑尺寸为 QLabel 的尺寸
    //SDL_RenderSetLogicalSize(sdl_renderer, sdl_width, sdl_height);
	// 这里可以开始渲染内容到纹理，然后显示在 QLabel 上
    startTimer(10);
}
SdlRgbQt::~SdlRgbQt()
{}

