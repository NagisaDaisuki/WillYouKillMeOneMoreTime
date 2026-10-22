#include "stdafx.h"
#include "sdlqtrgb.h"
#include "xvideoview.h"
// #include <SDL.h>
#include <fstream>
#include <QMessageBox>

// #pragma comment(lib, "SDL2.lib")
// static SDL_Window* sdl_win = NULL;
// static SDL_Renderer* sdl_render = NULL;
// static SDL_Texture* sdl_texture = NULL;
static int sdl_width = 0;
static int sdl_height = 0;
static unsigned char*  yuv = NULL;
// static int pix_size = 2;
static std::ifstream yuv_file;
static XVideoView* view = nullptr;

void SdlQtRgb::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (ui.label) {
        ui.label->setGeometry(0,0,width(), height());
    }
}

void SdlQtRgb::timerEvent(QTimerEvent* ev)
{
    // 防御性判断
    // if (!yuv || !sdl_texture) return;


    // yuv 420格式 pix占内存大小 为像素 总大小 * 1.5
    // y * 1 + u * 0.25 + v * 0.25 = y * 1.5
    int frame_size = sdl_width * sdl_height * 3 / 2;
    // 
        view->Draw(yuv,sdl_width);
    // 读取一帧数据
    yuv_file.read((char*)yuv, frame_size);
    //if (ev->timerId() == 2)
    // {
        //SDL_UpdateTexture(SDL_GetRenderTarget(sdl_render), NULL, rgb, sdl_width * 4);
        // // YUV420p (IYUV) 的 pitch 参数是 Y 分量的每行字节数，也就是 sdl_width

    // 如果文件读完了，重置文件指针（循环播放）
    if (yuv_file.gcount() < frame_size) {
        yuv_file.clear(); // 清除 EOF 标志
        yuv_file.seekg(0, std::ios::beg); 
        // Sets input position indicator of the current associated streambuf object.
        view->Draw(yuv,sdl_width);
        // yuv_file.read((char*)yuv, frame_size);
    }
        // SDL_UpdateTexture(sdl_texture, NULL, yuv, sdl_width); // sdl_width 是一行y大小
        // SDL_RenderClear(sdl_render);
        // SDL_Rect rect;
        // rect.x = 0;
        // rect.y = 0; 
        // rect.w = sdl_width;
        // rect.h = sdl_height;

        // SDL_RenderCopy(sdl_render, sdl_texture, NULL, &rect);
        // SDL_RenderPresent(sdl_render);
    // }
}

SdlQtRgb::SdlQtRgb(QWidget *parent)
    : QWidget(parent)
{
	// 打开 yuv 文件
    yuv_file.open("Pensil.yuv", std::ios::binary);
    if (!yuv_file) {
        QMessageBox::information(this, "", "open yuv failed!");
        return;
    }
    ui.setupUi(this);
    resize(1920,1080);
    // qreal dpr = ui.label->devicePixelRatioF();
    // sdl_width = ui.label->width() * dpr;
    // sdl_height = ui.label->height() * dpr;
    // 让 label 铺满整个 widget:
    ui.label->setGeometry(0,0, width(), height());

    // 视频的实际分辨率
    sdl_width = 1920;
    sdl_height = 1080;
    // 初始化 SDL，重点：传 label 的真实物理尺寸给SDL
    qreal dpr = ui.label->devicePixelRatioF();
    int phys_w = ui.label->width() * dpr;
    int phys_h = ui.label->height() * dpr;
    //ui.label->resize(sdl_width, sdl_height);

    // 构造一个 view 对象
    view = XVideoView::Create(XVideoView::SDL);
    // 传入物理尺寸，保证 SDL 的纹理能覆盖整个 Label 区域
    view->Init(sdl_width,sdl_height, XVideoView::YUV420P, (void*)ui.label->winId());
    

    // SDL_Init(SDL_INIT_VIDEO);
	// sdl_win = SDL_CreateWindowFrom((void*)ui.label->winId());
    // if (!sdl_win) {
    //     qDebug() << "SDL_CreateWindowFrom failed:" << SDL_GetError();
    //     return;
    // }
    // sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    // if (!sdl_render) {
    //     qDebug() << "SDL_CreateRenderer failed:" << SDL_GetError();
    //     return;
    // }
    // // 创建材质（显存）
    // unsigned int sdl_fmt = SDL_PIXELFORMAT_RGBA8888;
    // switch (fmt) {
    //     
    // }
    // sdl_texture = SDL_CreateTexture(sdl_render,
    //     SDL_PIXELFORMAT_IYUV,
    //     SDL_TEXTUREACCESS_STREAMING,
    //     sdl_width, sdl_height);
    // 多线程不能在当前析构函数中直接释放内存
    // 线程不能保证当前主线程退出时子线程也退出
    // 在析构函数中 wait 等待子线程退出
    
	int yuv_size = sdl_width * sdl_height * 3 / 2;
    yuv = new unsigned char[yuv_size];
    memset(yuv, 0, yuv_size);
    startTimer(33);// 大约 30 帧/秒，比 10ms (100fps) 更合理，防止读取太快
}

SdlQtRgb::~SdlQtRgb()
{
    if (yuv) {
        delete[] yuv;
        yuv = nullptr;
    }
    // if (sdl_texture) SDL_DestroyTexture(sdl_texture);
    // if (sdl_render) SDL_DestroyRenderer(sdl_render);
	// if (sdl_win) SDL_DestroyWindow(sdl_win);
    // 
    // SDL_Quit();

}


