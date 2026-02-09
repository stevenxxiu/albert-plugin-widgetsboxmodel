// Copyright (c) 2022-2026 Manuel Schneider

#pragma once
#include <QPlainTextEdit>

class ResizingQPlainTextEdit : public QPlainTextEdit
{
public:
    ResizingQPlainTextEdit(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
};
