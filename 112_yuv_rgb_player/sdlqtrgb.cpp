#include "stdafx.h"
#include "sdlqtrgb.h"
#include "xvideoview.h"

// #include <SDL.h>
#include <fstream>
#include <QMessageBox>
#include <iostream>
#include <sstream>
#include <QSpinBox> // 用户输出数字
#include <QFileDialog> // 打开文件对话框
#include <vector>

static std::vector<XVideoView*> views;

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
}
void SdlQtRgb::timerEvent(QTimerEvent* ev)
{
}

// 指定打开的窗口
void SdlQtRgb::Open(int i)
{
    QFileDialog fd;
    auto filename = fd.getOpenFileName();  // QString
    if (filename.isEmpty()) return;     // 没有选择文件直接返回
    std::cout << filename.toLocal8Bit().data() << std::endl;
    // 调用初始化 view对应窗口的文件
    if (!views[i]->Open(filename.toLocal8Bit().toStdString()))
    {
        return;
    }

    int w = 0;
    int h = 0;
    QString pix = 0; // YUV420P RGBA ARGB BGRA
    XVideoView::PixFormat pixfmt = XVideoView::YUV420P;

    if (i == 0)
    {
		w = ui.width1->value();
        h = ui.height1->value();
        pix = ui.pix1->currentText(); // 像素格式
    }
    else if (i == 1)
	{
        w = ui.width2->value();
		h = ui.height2->value();
        pix = ui.pix2->currentText();
    }

    if (pix == "YUV420P") 
    {
        
    }
    else if (pix == "RGBA")
    {
        pixfmt = XVideoView::RGBA;
    }
    else if (pix == "ARGB")
    {
        pixfmt = XVideoView::ARGB;
    }
    else if (pix == "BGRA")
    {
        pixfmt = XVideoView::BGRA;
    }
    else if (pix == "RGB24")
    {
        pixfmt = XVideoView::RGB24;
    }

    // 初始化窗口和材质
    views[i]->Init(w, h, pixfmt);
    
}

void SdlQtRgb::Open1()
{
    Open(0);
}
void SdlQtRgb::Open2()
{
    Open(1);
}

// 槽函数
void SdlQtRgb::View()
{
    // 存放上次渲染的时间戳
	// if 当前时间戳 - 上次渲染时间戳 < 40ms 就不渲染
    // > 40ms 渲染
    static int last_pts[32] = {0};
    static int fps_arr[32] = {0};
    fps_arr[0] = ui.set_fps1->value();
    fps_arr[1] = ui.set_fps2->value();
    
    for (int i = 0; i < views.size(); ++i) 
    {
        if (fps_arr[i] <= 0) continue; // 防御性编程
        // 需要间隔时间
		int ms = 1000 / fps_arr[i];
        // 判断是否到了可渲染时间
        if (NowMs() - last_pts[i] <= ms)
            continue;
        last_pts[i] = NowMs();

        auto frame = views[i]->Read();
        if (!frame) continue;
        views[i]->DrawFrame(frame);
        // 显示 fps
        std::stringstream ss;
		ss << "FPS: " << views[i]->render_fps();
        if (i == 0)
            ui.fps1->setText(QString::fromStdString(ss.str()));
        else if (i == 1)
            ui.fps2->setText(QString::fromStdString(ss.str()));
    }
}

// 线程函数
void SdlQtRgb::Main()
{
    while (!is_exit_)
    {
        ViewS();
        MSleep(10);
    }
}

SdlQtRgb::SdlQtRgb(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    // 绑定渲染函数信号槽
    connect(this,SIGNAL(ViewS()), this, SLOT(View()));
    views.push_back(XVideoView::Create());
    views.push_back(XVideoView::Create());
    views[0]->set_win_id((void*)ui.video1->winId());
    views[1]->set_win_id((void*)ui.video2->winId());
    th_ = std::thread(&SdlQtRgb::Main,this);
}

SdlQtRgb::~SdlQtRgb()
{ 
    is_exit_ = true;
    // 等待渲染线程退出 
    th_.join();
}


