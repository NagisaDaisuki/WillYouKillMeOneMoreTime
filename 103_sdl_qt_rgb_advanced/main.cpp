#include "stdafx.h"
#include "sdlrgbqt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SdlRgbQt window;
    window.show();
    return app.exec();
}
