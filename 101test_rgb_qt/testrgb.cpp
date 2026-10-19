#include "stdafx.h"
#include "testrgb.h"
#include <QPainter> // 绘制QImage图像内容
#include <QImage> // 存储图像内容

static int w = 1280;
static int h = 720;

TestRGB::TestRGB(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    resize(w,h); // 设置窗口大小
}

TestRGB::~TestRGB()
{}

void TestRGB::paintEvent(QPaintEvent* ev)
{
    QImage img(w, h, QImage::Format_RGB888); // 用三个字节表示一个画面
    auto d = img.bits(); // unsigned char * 指向图像原始字节数据的指针
    unsigned char r = 255; // 红色
    // 先遍历高度，宽度大小不固定（不满足对齐条件，为了让计算更快高度会被填充）
    for (int i = 0; i < h; ++i)
    {
        r--; // 做渐变色
        int b = i * w * 3;
        for (int j = 0; j < w * 3; j += 3)
        {
            d[b + j] = r;         // R
            d[b + j + 1] = 0;       // G
            d[b + j + 2] = 0;       // B
        }
    }
    QPainter p;
    // p.begin(this);
    // p.end(); // 固定渲染用法
    p.begin(this);
    p.drawImage(0,0,img); // 屏幕坐标从左上角开始计算
    p.end(); // 固定渲染用法
}

