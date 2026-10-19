#pragma once

#include <QtWidgets/QWidget>
#include "ui_sdlrgbqt.h"

class SdlRgbQt : public QWidget
{
    Q_OBJECT

public:
    SdlRgbQt(QWidget *parent = nullptr);
    ~SdlRgbQt();
    void timerEvent(QTimerEvent* ev) override;
    void initSDL();

private:
    Ui::SdlRgbQtClass ui;
};

