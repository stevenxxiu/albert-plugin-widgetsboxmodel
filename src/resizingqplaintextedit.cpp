// Copyright (c) 2022-2025 Manuel Schneider

#include "resizingqplaintextedit.h"
#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QSignalBlocker>
#include <QSyntaxHighlighter>
#include <albert/logging.h>
using namespace Qt::StringLiterals;


ResizingQPlainTextEdit::ResizingQPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    document()->setDocumentMargin(1); // 0 would be optimal but clips bearing

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // auto fixHeight = [this]{
    //     INFO << "INPUTLINE fm lineSpacing" << fontMetrics().lineSpacing();
    //     INFO << "INPUTLINE doc height" << document()->size().height();
    //     INFO << "INPUTLINE doc margin" << document()->documentMargin();
    //     int height = document()->size().height() ;//+ document()->documentMargin();
    //     INFO << "Final height" << height;
    //     setFixedHeight(height);
    // };

    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
            this,[this](const QSizeF &newSize)
            {
                // Looks like there is some more space needed. The scrollarea reserves space in full
                // multiples of lines. Without the + 1 an additional line is reserved. Maybe some
                // rounding issues or such.
                setFixedHeight((int)newSize.height() * fontMetrics().lineSpacing()
                               + 2 * (int)document()->documentMargin() + 1); // see comment above
            });
}

bool ResizingQPlainTextEdit::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
    {
        // auto f = font();
        // f.setPointSize(v);
        // setFont(f);
        // setFixedHeight(fontMetrics().lineSpacing() + 2 * (int)document()->documentMargin());

                // Fix for proper text margins. The subjective margins (top, left, bottom) should equal.

                // The text should be idented by the distance of the cap line to the top.
        QFontMetricsF fm(font());

                // Linespacing is not really reliable
        const auto real_height = int(fm.lineSpacing() + .5) + 1;
        CRIT << "real_height" << real_height;
        const auto vertical_margin = int((real_height - fm.capHeight())/2 + .5);
        CRIT << "vertical_margin" << vertical_margin;


                // auto fm = fontMetrics();
        CRIT << "fm.height()" << fm.height();
        CRIT << "fm.lineSpacing()" << fm.lineSpacing();
        CRIT << "fm.capHeight()" << fm.capHeight();
        CRIT << "ls - ch" << fm.lineSpacing() - fm.capHeight();
        CRIT << "(ls - ch)/2" << (fm.lineSpacing() - fm.capHeight())/2;

        CRIT << "fm.ascent()" << fm.ascent();
        CRIT << "fm.descent()" << fm.descent();
        CRIT << "LB |" << fm.leftBearing(u'|');
        CRIT << "LB M" << fm.leftBearing(u'M');

        auto font_margin_fix = int(vertical_margin - fm.leftBearing(u'|'));

        INFO << "font_margin_fix" << font_margin_fix;
        // 1px document margins
        // setViewportMargins(-5,-5,-5,-5);
        setViewportMargins(font_margin_fix, 0, font_margin_fix, 0);
        // document()->setDocumentMargin(0);
        // setContentsMargins(-5,-5,-5,-5);

        CRIT << "height()" << height();
    }

    return QPlainTextEdit::event(event);
}
