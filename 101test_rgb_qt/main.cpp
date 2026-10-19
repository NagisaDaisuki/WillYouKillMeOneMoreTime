#include "stdafx.h"
#include "testrgb.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestRGB window;
    window.show();
    return app.exec();
}
