// Copyright (c) 2014-2025 Manuel Schneider

#pragma once
#include "resizinglist.h"
class ResultsListDelegate;

class ResultsList : public ResizingList
{
public:
    ResultsList(QWidget *parent = nullptr);
    ~ResultsList();

    int iconSize() const;
    void setIconSize(int);

    int subtextFontSize() const;
    void setSubtextFontSize(int);

    const QColor &subtextColor() const;
    void setSubtextColor(const QColor &);

    const QColor &selectionSubtextColor() const;
    void setSelectionSubtextColor(const QColor &);

    int horizontalSpacing() const;
    void setHorizontalSpacing(int);

    int verticalSpacing() const;
    void setVerticalSpacing(int);

private:
    ItemDelegateBase *delegate() const override;
    ResultsListDelegate *delegate_;
};
