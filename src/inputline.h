// Copyright (c) 2022-2025 Manuel Schneider

#pragma once
#include "resizingqplaintextedit.h"
#include <albert/inputhistory.h>

class InputLine : public ResizingQPlainTextEdit
{
    Q_OBJECT

public:

    InputLine(QWidget *parent = nullptr);

    const QString& synopsis() const;
    void setSynopsis(const QString&);

    const QString& completion() const;
    void setCompletion(const QString& = {});

    uint triggerLength() const;
    void setTriggerLength(uint);

    QString text() const;
    void setText(QString);

    void deleteWordBackwards();

    const QColor &triggerColor() const;
    void setTriggerColor(const QColor &);

    const QColor &inputActionColor() const;
    void setInputActionColor(const QColor &);

    const QColor &inputHintColor() const;
    void setInputHintColor(const QColor &);

    void next();
    void previous();

    bool clear_on_hide;
    bool history_search;
    bool disable_input_method_;


private:
    bool event(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    albert::detail::InputHistory history_;
    QString completion_;
    QString synopsis_;
    QString user_text_;
    uint trigger_length_;
    class TriggerHighlighter;
    TriggerHighlighter *highlighter_;

    // Style
    QColor trigger_color_;
    QColor input_action_color_;
    QColor input_hint_color_;

signals:

    void textEdited();

};


