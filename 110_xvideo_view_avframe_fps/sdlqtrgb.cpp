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
#include <sstream>
#include <QSpinBox> // 用户输出数字

static int sdl_width = 0;
static int sdl_height = 0;
// static unsigned char* yuv = NULL;
static std::ifstream yuv_file;
static XVideoView* view = nullptr;
static AVFrame* frame = nullptr;
static long long file_size = 0;
static QLabel* view_fps = nullptr; // 显示 fps 控件
static QSpinBox* set_fps = nullptr; // 设置 fps 控件
int fps = 25; // 默认播放帧率

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

}

// 槽函数
void SdlQtRgb::View()
{
    // 防御性判断
    if (!frame) return;

    // 读取 Y 平面
    yuv_file.read((char*)frame->data[0], sdl_width * sdl_height);
    // 读取 U 平面
    yuv_file.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    // 读取 V 平面
    yuv_file.read((char*)frame->data[2], sdl_width * sdl_height / 4);

    if (yuv_file.tellg() == file_size) 
    {
        yuv_file.clear();
        yuv_file.seekg(0, std::ios::beg);
        yuv_file.read((char*)frame->data[0], sdl_width * sdl_height);
        yuv_file.read((char*)frame->data[1], sdl_width * sdl_height / 4);
        yuv_file.read((char*)frame->data[2], sdl_width * sdl_height / 4);
        
    }


    // 循环读取核心逻辑：如果读出来的字节数不够
    // 一帧，说明到文件尾了
    // yuv_file.gcount() 获取到本次的读取字节数
    // if (yuv_file.gcount() < (sdl_width * sdl_height / 4)) {
    //     yuv_file.clear();       // 清楚 EOF 标志位
    //     yuv_file.seekg(0, std::ios::beg); // 游标回到文件头

    //     // 重新读取第一帧
    //     yuv_file.read((char*)frame->data[0], sdl_width * sdl_height);
    //     yuv_file.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    //     yuv_file.read((char*)frame->data[2], sdl_width * sdl_height / 4);

    // }

    if (view->IsExit())
    {
        view->Close();
        exit(0);
    }
    view->DrawFrame(frame);

    // 显式 XVideoView 实时计算的 fps
    std::stringstream ss;
    ss << "fps:" << view->render_fps();

    // 只能在槽函数中调用 
    view_fps->setText(ss.str().c_str());
    fps = set_fps->value(); // 拿到播放帧率
}

// 线程函数
/*
    窗口最大化/最小化，多窗口切换
    导致的渲染窗口问题
    -> 把渲染的部分放在 Qt的信号槽当中
    窗口大小发生变化之后进行修改
*/
void SdlQtRgb::Main()
{
    while (!is_exit_)
    {
        ViewS();
        if (fps > 0) 
        {
            MSleep(1000 / fps);
        }
        else
        {
            MSleep(40);
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
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
    
    yuv_file.seekg(0, std::ios::end);   // 将文件流指针移动到文件结尾
    file_size = yuv_file.tellg();       // 文件指针位置
    yuv_file.seekg(0, std::ios::beg);
    // 绑定渲染函数信号槽
    connect(this,SIGNAL(ViewS()), this, SLOT(View()));
    
    // 显式 fps 控件 
    view_fps = new QLabel(this);
    // 1. 强制设为原生窗口，才能盖住 SDL 窗口
    view_fps->setAttribute(Qt::WA_NativeWindow);
    // 2. 设置一个足够大的固定大小，防止被挤压
    view_fps->setFixedSize(100,25);
    // 3. 设置 view_fps样式
    view_fps->setStyleSheet("background-color: rgba(0,0,0,0); color: cyan; font-size: 16px;" 
        " font-weight: bold; ");
    //  强制设置位置为绝对 0,0
    view_fps->move(0,0);

    // 设置 fps 控件
    set_fps = new QSpinBox(this);
    set_fps->move(200, 0);
    set_fps->setValue(25);
    set_fps->setRange(1, 300);

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
    ui.label->setText("");
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
    // startTimer(33);// 大约 30 帧/秒，比 10ms (100fps) 更合理，防止读取太快
    
    // 主线程退出后，子线程不会退
    // 在析构函数阻塞等待子线程释放
    th_ = std::thread(&SdlQtRgb::Main,this);
}

SdlQtRgb::~SdlQtRgb()
{ 
    is_exit_ = true;
    // 等待渲染线程退出 
    th_.join();
}


