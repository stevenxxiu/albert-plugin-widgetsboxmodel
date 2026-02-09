// Copyright (c) 2023-2026 Manuel Schneider

#pragma once
#include <QWidget>

class Frame : public QWidget
{
public:
    Frame(QWidget *parent = nullptr);

    const QBrush &backgroundBrush() const;
    void setBackgroundBrush(const QBrush &);

    const QBrush &borderBrush() const;
    void setBorderBrush(const QBrush &);

    double borderRadius() const;
    void setBorderRadius(double);

    double borderWidth() const;
    void setBorderWidth(double);

private:
    void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

    QBrush background_brush_;
    double border_radius_;
    double border_width_;
    QBrush border_brush_;
};
