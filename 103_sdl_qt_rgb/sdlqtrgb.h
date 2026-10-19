#pragma once

#include <QtWidgets/QWidget>
#include "ui_sdlqtrgb.h"

class SdlQtRgb : public QWidget
{
    Q_OBJECT

public:
    SdlQtRgb(QWidget *parent = nullptr);
    ~SdlQtRgb();
    void timerEvent(QTimerEvent* ev) override;
private:
    Ui::SdlQtRgbClass ui;
};

