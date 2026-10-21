#include <QMessageBox>
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
    if (!rgb || !sdl_texture) return;
    // static int tmp = 255;
    // tmp--;
    // for (int j = 0; j < sdl_height; ++j)
    // {
    //     int b = j * sdl_width * pix_size;
    //     for (int i = 0; i < sdl_width * pix_size; i += pix_size)
    //     {
    //         rgb[b + i] = 0; // B
    //         rgb[b + i + 1] = tmp; // G
    //         rgb[b + i + 2] = 0; // R
    //         rgb[b + i + 3] = 0; // A
    //     }
    // }

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

SdlQtRgb::SdlQtRgb(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    // 加载图片
    QImage img1("001.jpg");
    QImage img2("002.png");
    if (img1.isNull() || img2.isNull()) {
        QMessageBox::information(this, "", "open image failed!");
        return;
    }
    img1 = img1.convertToFormat(QImage::Format_ARGB32);
    img2 = img2.convertToFormat(QImage::Format_ARGB32);

    qreal dpr = devicePixelRatioF();


    // int out_w = img1.width() * dpr + img2.width() * dpr;
    // int out_h = img1.height() > img2.height() ? img1.height() * dpr : img2.height() * dpr;
    sdl_width = img1.width() + img2.width();
    sdl_height = qMax(img1.height(), img2.height());
    int logical_w = sdl_width / dpr;
    int logical_h = sdl_height / dpr;

    // 先调整窗口和 Label 的大小，再去让 SDL绑定窗口
    // 设置 Label 的大小
    ui.label->setFixedSize(logical_w, logical_h);
    ui.label->move(0, 0);

    // 调整主窗口大小 (稍微加一点宽度防止出现滚动条)
    this->resize(logical_w, logical_h);
    // resize(sdl_width, sdl_height);
    // ui.label->move(0,0);
    // ui.label->resize(sdl_width, sdl_height);

    // 初始化 SDL 和纹理
    SDL_Init(SDL_INIT_VIDEO);
    sdl_win = SDL_CreateWindowFrom((void*)ui.label->winId());
    // 使用软件渲染或硬件加速
    sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    sdl_texture = SDL_CreateTexture(sdl_render,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        sdl_width, sdl_height);
    // 多线程不能在当前析构函数中直接释放内存
    // 线程不能保证当前主线程退出时子线程也退出
    // 在析构函数中 wait 等待子线程退出

    // 设置混合纹理模式，防止 Alpha为 0 时什么都看不见
    SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_NONE);


    // 分配内存并初始化为黑色（A = 255, R = 0, G = 0, B = 0）
    rgb = new unsigned char[sdl_width * sdl_height * pix_size];
    memset(rgb, 0, sdl_width * sdl_height * pix_size);
    for (int i = 0; i < sdl_width * sdl_height; ++i) {
        rgb[i * pix_size + 3] = 0; // 改成透明Alpha通道
    }


    for (int i = 0; i < sdl_height; ++i) {
        int b = i * sdl_width * pix_size;
        if (i < img1.height())
            memcpy(rgb + b, img1.scanLine(i), img1.width() * pix_size);
        b += img1.width() * pix_size;
        if (i < img2.height())
            memcpy(rgb + b, img2.scanLine(i), img2.width() * pix_size);
    }   
    QImage out(rgb, sdl_width, sdl_height, QImage::Format_RGBA8888);
    out.save("output.png");
    startTimer(10);
}

SdlQtRgb::~SdlQtRgb()
{
    if (rgb) delete[] rgb;
}

