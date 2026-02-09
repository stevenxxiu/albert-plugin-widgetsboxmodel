// Copyright (c) 2023-2025 Manuel Schneider

#include "frame.h"
#include "primitives.h"
#include <QPaintEvent>

Frame::Frame(QWidget *parent) :
    QWidget(parent),
    background_brush_(palette().color(QPalette::Window)),
    border_radius_(0),
    border_width_(0),
    border_brush_(Qt::transparent)
{
    setMinimumSize(0,0);
}

void Frame::paintEvent(QPaintEvent*)
{
    auto dpr = devicePixelRatioF();
    QPixmap pm = pixelPerfectRoundedRect(size() * dpr,
                                         background_brush_,
                                         (int)(border_radius_ * dpr),
                                         border_brush_,
                                         (int)(border_width_ * dpr));
    pm.setDevicePixelRatio(dpr);

    QPainter p(this);
    p.drawPixmap(rect(), pm);
}

QSize Frame::minimumSizeHint() const { return {-1, -1}; }

const QBrush &Frame::backgroundBrush() const { return background_brush_; }

void Frame::setBackgroundBrush(const QBrush &v)
{
    if (background_brush_ == v)
        return;
    background_brush_ = v;
    update();
}

const QBrush &Frame::borderBrush() const { return border_brush_; }

void Frame::setBorderBrush(const QBrush &v)
{
    if (border_brush_ == v)
        return;
    border_brush_ = v;
    update();
}

double Frame::borderRadius() const { return border_radius_; }

void Frame::setBorderRadius(double v)
{
    if (border_radius_ == v)
        return;
    border_radius_ = v;
    update();
}

double Frame::borderWidth() const { return border_width_; }

void Frame::setBorderWidth(double v)
{
    if (border_width_ == v)
        return;
    border_width_ = v;
    update();
}
