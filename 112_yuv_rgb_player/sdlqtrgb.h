#pragma once

#include <QtWidgets/QWidget>
#include "ui_sdlqtrgb.h"

#include <thread>
class SdlQtRgb : public QWidget
{
    Q_OBJECT

public:
    SdlQtRgb(QWidget *parent = nullptr);
    ~SdlQtRgb();
    void timerEvent(QTimerEvent* ev) override;
    void resizeEvent(QResizeEvent* event) override;
    void ReverseRow(unsigned char* row, size_t n);
    void Main(); // 线程入口函数
signals:
    void ViewS(); // 信号函数，将任务放入列表
public slots:
    void View();    // 显示的槽函数
    void Open(int i); // 通过编号进行设定
    void Open1();   
    void Open2();
private:
    std::thread th_;
    bool is_exit_ = false;  // 处理线程退出
    Ui::SdlQtRgbClass ui;
};

