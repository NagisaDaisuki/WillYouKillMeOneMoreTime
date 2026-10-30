#include "stdafx.h"
#include "sdlqtrgb.h"
#include <QtWidgets/QApplication>
#include <windows.h>
#pragma comment(lib, "winmm.lib")

int main(int argc, char *argv[])
{

    timeBeginPeriod(1); // 将系统时钟精度提升到 1ms
    QApplication app(argc, argv);
    SdlQtRgb window;
    window.show();
    int ret = app.exec();
    timeEndPeriod(1); // 恢复系统时钟精度

    return ret;
}
