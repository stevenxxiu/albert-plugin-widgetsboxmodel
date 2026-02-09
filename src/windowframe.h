// Copyright (c) 2023-2025 Manuel Schneider

#pragma once
#include "frame.h"
#include <QWidget>

class WindowFrame : public Frame
{
public:

    WindowFrame(QWidget *parent = nullptr);

    uint shadowSize() const;
    void setShadowSize(uint);

    uint shadowOffset() const;
    void setShadowOffset(uint);

    QBrush shadowBrush() const;
    void setShadowBrush(const QBrush &);

private:

    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QString cacheKey() const;

    uint shadow_size_;
    uint shadow_offset_;
    QBrush shadow_brush_;

};
