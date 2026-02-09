// Copyright (c) 2023-2025 Manuel Schneider

#include "primitives.h"
#include "windowframe.h"
#include <QPaintEvent>
#include <QPixmapCache>


WindowFrame::WindowFrame(QWidget *parent):
    Frame(parent)
{
    setWindowFlags( Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
}


void WindowFrame::paintEvent(QPaintEvent *event)
{
    // CRIT << "Window::paintEvent" << event->rect();

    QPixmap pm;

    if (!QPixmapCache::find(cacheKey(), &pm))
    {
        auto dpr = devicePixelRatioF();

        auto frame_pixmap = pixelPerfectRoundedRect(contentsRect().size() * dpr,
                                                    backgroundBrush(),
                                                    (int)(borderRadius() * dpr),
                                                    borderBrush(),
                                                    (int)(borderWidth() * dpr));
        frame_pixmap.setDevicePixelRatio(dpr);
        // WARN << "FRAME >>>" << frame_pixmap.size() << frame_pixmap.deviceIndependentSize() << frame_pixmap.devicePixelRatio();

        QImage img(size() * dpr, QImage::Format_ARGB32_Premultiplied);
        img.setDevicePixelRatio(dpr);
        img.fill(Qt::transparent);
        // WARN << "IMAGE >>>" << img.size() << img.deviceIndependentSize() << img.devicePixelRatio();

        auto shadow_rect = contentsRect().translated(0, shadow_offset_);

        QPainter img_painter(&img);
        img_painter.drawPixmap(shadow_rect, frame_pixmap);
        img_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        img_painter.fillRect(shadow_rect, shadow_brush_);
        img_painter.end();

        pm = QPixmap(size() * dpr);
        pm.fill(Qt::transparent);
        pm.setDevicePixelRatio(dpr);

        QPainter pm_painter(&pm);
        pm_painter.save(); // needed qt_blurImage changes painter
        // Alpha only is faster but somehow shrinks the shadow,
        qt_blurImage(&pm_painter, img, shadow_size_ * dpr * 1.75, true, false);
        pm_painter.restore();
        pm_painter.drawPixmap(contentsRect().topLeft(), frame_pixmap);
        // WARN << "COMPOSITE >>>" << pm.size() << pm.deviceIndependentSize() << pm.devicePixelRatio();

        QPixmapCache::insert(cacheKey(), pm);
    }

    QPainter p(this);
    p.drawPixmap(0, 0, pm);
    event->accept();
}

QString WindowFrame::cacheKey() const
{
    return QStringLiteral("_WindowFrame_%1x%2").arg(width()).arg(height());
}

bool WindowFrame::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
        QPixmapCache::remove(cacheKey());
        break;
    default:
        break;
    }
    return Frame::event(event);
}

uint WindowFrame::shadowSize() const { return shadow_size_; }

void WindowFrame::setShadowSize(uint v)
{
    if (shadow_size_ == v)
        return;
    shadow_size_ = v;
    setContentsMargins(v, std::max(v - shadow_offset_, 0u),
                       v, v + shadow_offset_);
    update();
}

uint WindowFrame::shadowOffset() const { return shadow_offset_; }

void WindowFrame::setShadowOffset(uint v)
{
    if (shadow_offset_ == v)
        return;
    shadow_offset_ = v;
    setContentsMargins(shadow_size_, std::max(shadow_size_ - v, 0u),
                       shadow_size_, shadow_size_ + v);
    update();
}

QBrush WindowFrame::shadowBrush() const { return shadow_brush_; }

void WindowFrame::setShadowBrush(const QBrush &v)
{
    if (shadow_brush_ == v)
        return;
    shadow_brush_ = v;
    update();
}
