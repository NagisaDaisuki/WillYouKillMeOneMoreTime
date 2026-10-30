#include "stdafx.h"
#include "sdlqtrgb.h"
#include "xvideoview.h"
extern "C" {
#include <libavcodec/avcodec.h>
}

// #include <SDL.h>
#include <fstream>
#include <QMessageBox>
#include <iostream>

static int sdl_width = 0;
static int sdl_height = 0;
// static unsigned char* yuv = NULL;
static std::ifstream yuv_file;
static XVideoView* view = nullptr;
static AVFrame* frame = nullptr;

void SdlQtRgb::ReverseRow(unsigned char* row, size_t n)
{
    for (size_t i = 0,j = n - 1; i < n / 2; ++i, --j) {
        unsigned char tmp = row[i];
        row[i] = row[j];
        row[j] = tmp;
    }
}

void SdlQtRgb::resizeEvent(QResizeEvent* event)
{
    ui.label->resize(size());
    ui.label->move(0,0);
    // 传实际缩放过后的物理像素
    // qreal dpr = ui.label->devicePixelRatioF();
    // view->Scale(width() * dpr, height() * dpr);
    // QWidget::resizeEvent(event);
    // if (ui.label) {
    //     ui.label->setGeometry(0,0,width(), height());
    // }
}

void SdlQtRgb::timerEvent(QTimerEvent* ev)
{
    // 防御性判断
    if (!frame) return;
    // yuv 420格式 pix占内存大小 为像素 总大小 * 1.5
    // y * 1 + u * 0.25 + v * 0.25 = y * 1.5
    // 传入的是连续存储格式
    // int frame_size = sdl_width * sdl_height * 3 / 2;
    // 读取一帧数据
    // yuv_file.read((char*)yuv, frame_size);
    // yuv镜像操作
    // 考虑内存对齐
    // yyyyyyyy uu vv
    // y 平面镜像(planar)
    
    // 读取 Y 平面
    yuv_file.read((char*)frame->data[0], sdl_width * sdl_height);
    // 读取 U 平面
    yuv_file.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    // 读取 V 平面
    yuv_file.read((char*)frame->data[2],sdl_width * sdl_height / 4);
    
    // 循环读取核心逻辑：如果读出来的字节数不够
    // 一帧，说明到文件尾了
    if (yuv_file.gcount() < (sdl_width * sdl_height / 4)) {
        yuv_file.clear();       // 清楚 EOF 标志位
        yuv_file.seekg(0, std::ios::beg); // 游标回到文件头
        
        // 重新读取第一帧
    yuv_file.read((char*)frame->data[0], sdl_width * sdl_height);
    yuv_file.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    yuv_file.read((char*)frame->data[2],sdl_width * sdl_height / 4);
        
    }

    if (view->IsExit())
    {
        view->Close();
        exit(0);
    }
    view->DrawFrame(frame);


    // unsigned char* cur_yuv = nullptr;
    // size_t y_size = (size_t)sdl_width * sdl_height;
    // size_t uv_size = y_size / 4;
    // uint8_t* Y = yuv;
    // uint8_t* U = yuv + y_size;
    // uint8_t* V = yuv + y_size + uv_size;
    // for (int i = 0; i < sdl_height; ++i) {
    //     // cur_yuv = yuv + i * sdl_width;
    //     cur_yuv = Y + i * sdl_width;
    //     ReverseRow(cur_yuv, sdl_width);
    // }
    // // u 平面镜像 
    // for (int i = 0; i < sdl_height / 2; ++i) {
    //     // cur_yuv = yuv + sdl_width * sdl_height + i * sdl_width / 2;
    //     cur_yuv = U + i * sdl_width / 2;
    //     ReverseRow(cur_yuv, sdl_width / 2);
    // }
    // // v 平面镜像
    // for (int i = 0; i < sdl_height / 2; ++i) {
    //     // cur_yuv = yuv + sdl_width * sdl_height * 5 / 4 + i * sdl_width / 2;
    //     cur_yuv = V + i * sdl_width / 2;
    //     ReverseRow(cur_yuv, sdl_width / 2);
    // }

    // view->Draw(yuv,sdl_width);
    //if (ev->timerId() == 2)
    // {
        //SDL_UpdateTexture(SDL_GetRenderTarget(sdl_render), NULL, rgb, sdl_width * 4);
        // // YUV420p (IYUV) 的 pitch 参数是 Y 分量的每行字节数，也就是 sdl_width

    // 如果文件读完了，重置文件指针（循环播放）
    // if (yuv_file.gcount() < frame_size) {
    //     yuv_file.clear(); // 清除 EOF 标志
    //     yuv_file.seekg(0, std::ios::beg); 
    //     // Sets input position indicator of the current associated streambuf object.
    //     // view->Draw(yuv,sdl_width);
    //     // yuv_file.read((char*)yuv, frame_size);
    // }
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
    // ui.label->setGeometry(0,0, width(), height());

    // 视频的实际分辨率
    sdl_width = 1920;
    sdl_height = 1080;
    // 初始化 SDL，重点：传 label 的真实物理尺寸给SDL
    // qreal dpr = ui.label->devicePixelRatioF();
    // int phys_w = ui.label->width() * dpr;
    // int phys_h = ui.label->height() * dpr;
    ui.label->resize(sdl_width, sdl_height);
    ui.label->move(0,0);

    // 构造一个 view 对象
    view = XVideoView::Create();

    // 独立窗口
    view->Init(sdl_width,sdl_height, XVideoView::YUV420P, (void*)ui.label->winId());
    
    // 生成 AVFrame 对象空间
    frame = av_frame_alloc();
    frame->width = sdl_width;
    frame->height = sdl_height;
    frame->format = AV_PIX_FMT_YUV420P; 
    // 生成图像空间 默认32字节对齐(对于某些视频分辨率来说会出现padding添加的问题)
    // 用手动赋值然后在 DrawFrame时调用 frame的 linesize避免出现对齐问题
    frame->linesize[0] = sdl_width;
    frame->linesize[1] = sdl_width / 2; // 行大小单独除以 2
    frame->linesize[2] = sdl_width / 2;
    
    auto re = av_frame_get_buffer(frame, 0);
    if (re != 0) 
    {
        char buf[1024] = { 0 };
        av_strerror(re, buf, sizeof(buf));
        std::cout << buf << std::endl;
    }

    // int yuv_size = sdl_width * sdl_height * 3 / 2;
    // yuv = new unsigned char[yuv_size];
    // memset(yuv, 0, yuv_size);
    startTimer(33);// 大约 30 帧/秒，比 10ms (100fps) 更合理，防止读取太快
}

SdlQtRgb::~SdlQtRgb()
{
    // if (yuv) {
    //     delete[] yuv;
    //     yuv = nullptr;
    // }
    // view->Close();
    // if (sdl_texture) SDL_DestroyTexture(sdl_texture);
    // if (sdl_render) SDL_DestroyRenderer(sdl_render);
	// if (sdl_win) SDL_DestroyWindow(sdl_win);
    // 
    // SDL_Quit();

}


