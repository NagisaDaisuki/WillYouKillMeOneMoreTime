#include "stdafx.h"
#include "sdlqtrgb.h"
#include <SDL.h>

#pragma comment(lib, "SDL2.lib")
static SDL_Window* sdl_win = NULL;
static SDL_Renderer* sdl_render = NULL;
static SDL_Texture* sdl_texture = NULL;
static int sdl_width = 0;
static int sdl_height = 0;
static unsigned char* rgb = NULL;
static int pix_size = 4;


void SdlQtRgb::timerEvent(QTimerEvent* ev)
{
    static int tmp = 255;
    tmp--;
    for (int j = 0; j < sdl_height; ++j)
    {
        int b = j * sdl_width * pix_size;
        for (int i = 0; i < sdl_width * pix_size; i += pix_size)
        {
            rgb[b + i] = 0; // B
            rgb[b + i + 1] = tmp; // G
            rgb[b + i + 2] = 0; // R
            rgb[b + i + 3] = 0; // A
        }
    }

    //if (ev->timerId() == 2)
    // {
        //SDL_UpdateTexture(SDL_GetRenderTarget(sdl_render), NULL, rgb, sdl_width * 4);
        SDL_UpdateTexture(sdl_texture, NULL, rgb, sdl_width * pix_size);
        SDL_RenderClear(sdl_render);
        SDL_Rect rect;
        rect.x = 0;
        rect.y = 0; 
        rect.w = sdl_width;
        rect.h = sdl_height;

        SDL_RenderCopy(sdl_render, sdl_texture, NULL, &rect);
        SDL_RenderPresent(sdl_render);
    // }
}

SdlQtRgb::SdlQtRgb(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    qreal dpr = ui.label->devicePixelRatioF();
    sdl_width = ui.label->width() * dpr;
    sdl_height = ui.label->height() * dpr;
    SDL_Init(SDL_INIT_VIDEO);
	sdl_win = SDL_CreateWindowFrom((void*)ui.label->winId());
    sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    sdl_texture = SDL_CreateTexture(sdl_render,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        sdl_width, sdl_height);
    // 多线程不能在当前析构函数中直接释放内存
    // 线程不能保证当前主线程退出时子线程也退出
    // 在析构函数中 wait 等待子线程退出
    rgb = new unsigned char[sdl_width * sdl_height * pix_size];
    startTimer(10);
}

SdlQtRgb::~SdlQtRgb()
{}

