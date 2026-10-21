#include "stdafx.h"
#include "sdlqtrgb.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SdlQtRgb window;
    window.show();
    return app.exec();
}
