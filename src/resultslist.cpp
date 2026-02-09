// Copyright (c) 2014-2025 Manuel Schneider

#include "primitives.h"
#include "resultitemmodel.h"
#include "resultslist.h"
#include <QApplication>
#include <QPainter>
#include <QPixmapCache>
#include <albert/logging.h>
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;


class ResultsListDelegate : public ItemDelegateBase
{
public:
    ResultsListDelegate();

    QFont subtext_font;
    QColor subtext_color;
    QColor selection_subtext_color;
    QFontMetrics subtext_font_metrics;

    int icon_size;

    int horizontal_spacing;
    int vertical_spacing;

    QSize sizeHint(const QStyleOptionViewItem &o, const QModelIndex&) const override;
    void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override;

};

//--------------------------------------------------------------------------------------------------

ResultsList::ResultsList(QWidget *parent):
    ResizingList(parent)
{
    delegate_ = new ResultsListDelegate;
    setItemDelegate(delegate_);
}

ResultsList::~ResultsList() { delete delegate_; }

ItemDelegateBase *ResultsList::delegate() const { return delegate_; }

//--------------------------------------------------------------------------------------------------

ResultsListDelegate::ResultsListDelegate():
    subtext_font(QApplication::font()),
    subtext_font_metrics(subtext_font)
{

}

QSize ResultsListDelegate::sizeHint(const QStyleOptionViewItem &o, const QModelIndex &) const
{
    auto width = o.widget->width();
    auto height = std::max(icon_size,
                           text_font_metrics.height()
                               + subtext_font_metrics.height()
                               + vertical_spacing)
                  + 2 * padding;
    return {width, height};
}

void ResultsListDelegate::paint(QPainter *p,
                                const QStyleOptionViewItem &o,
                                const QModelIndex &i) const
{
    //
    // LAYOUT
    //

    const QRect icon_rect{padding,
                          o.rect.y() + (o.rect.height() - icon_size) / 2,
                          icon_size,
                          icon_size};

    const auto texts_x = padding + icon_size + horizontal_spacing;

    const auto texts_width = o.rect.width() - texts_x - padding;

    const auto texts_height = text_font_metrics.height()
                              + subtext_font_metrics.height()
                              + vertical_spacing;

    const QRect text_rect{texts_x,
                          o.rect.y() + (o.rect.height() - texts_height) / 2,
                          texts_width,
                          text_font_metrics.height()};

    const QRect subtext_rect{texts_x,
                             o.rect.y() + (o.rect.height() - texts_height) / 2
                                 + text_font_metrics.height()
                                 + vertical_spacing,
                             texts_width,
                             subtext_font_metrics.height()};

    //
    // DATA
    //

    using enum ItemRoles;

    const auto selected = o.state.testFlag(QStyle::State_Selected);

    const auto text = text_font_metrics.elidedText(i.data(TextRole).toString(),
                                                   o.textElideMode,
                                                   text_rect.width());

    const auto subtext = subtext_font_metrics.elidedText(i.data(SubTextRole).toString(),
                                                         o.textElideMode,
                                                         subtext_rect.width());

    QPixmap pm;
    if (const auto cache_key = u"%1@%2x%3"_s
                                   .arg(i.data(IdentifierRole).toString())
                                   .arg(icon_size)
                                   .arg(o.widget->devicePixelRatioF());
        !QPixmapCache::find(cache_key, &pm))
    {
        if (const auto icon = i.data(IconRole).value<QIcon>();
            icon.isNull())
            WARN << "Item retured null icon:"
                 << i.data(IdentifierRole).value<QString>();
        else
            pm = icon.pixmap(QSize(icon_size, icon_size), o.widget->devicePixelRatioF());
        QPixmapCache::insert(cache_key, pm);
    }

    //
    // PAINT
    //

    p->save();

    // Draw selection
    ItemDelegateBase::paint(p, o, i);

    // Draw icon (such that it is centered in the icon_rect)
    p->drawPixmap(
        icon_rect.x() + (icon_rect.width() - (int)pm.deviceIndependentSize().width()) / 2,
        icon_rect.y() + (icon_rect.height() - (int)pm.deviceIndependentSize().height()) / 2,
        pm);

    // Draw text
    p->setFont(text_font);
    p->setPen(QPen(selected ? selection_text_color : text_color, 0));
    p->drawText(text_rect, Qt::AlignTop | Qt::AlignLeft | Qt::TextDontClip, text);
    // // Clips. Adjust by descent sice origin seems to be the baseline
    // p->drawText(text_rect, text);
    // p->drawText(text_rect.bottomLeft() - QPoint(0, text_font_metrics.descent() - 1), text);

    // Draw subtext
    p->setFont(subtext_font);
    p->setPen(QPen(selected ? selection_subtext_color : subtext_color, 0));
    p->drawText(subtext_rect, Qt::AlignTop | Qt::AlignLeft | Qt::TextDontClip, subtext);
    // // Clips. Adjust by descent sice origin seems to be the baseline
    // p->drawText(subtext_rect, subtext);
    // p->drawText(subtext_rect.bottomLeft() - QPoint(0, subtext_font_metrics.descent() - 1), subtext);

    if (draw_debug_overlays)
    {
        drawDebugRect(*p, o.rect, u"ResultDelegate"_s);
        drawDebugRect(*p, icon_rect, u"icon_rect"_s);
        drawDebugRect(*p, text_rect, u"text_rect"_s);
        drawDebugRect(*p, subtext_rect, u"subtext_rect"_s);
    }

    p->restore();
}

int ResultsList::iconSize() const { return delegate_->icon_size; }

void ResultsList::setIconSize(int v)
{
    if (delegate_->icon_size == v)
        return;
    delegate_->icon_size = v;
    relayout();
}

int ResultsList::subtextFontSize() const { return delegate_->subtext_font.pointSize(); }

void ResultsList::setSubtextFontSize(int v)
{
    if (delegate_->subtext_font.pointSize() == v)
        return;
    delegate_->subtext_font.setPointSize(v);
    delegate_->subtext_font_metrics = QFontMetrics(delegate_->subtext_font);
    relayout();
}

const QColor &ResultsList::subtextColor() const { return delegate_->subtext_color; }

void ResultsList::setSubtextColor(const QColor &v)
{
    if (delegate_->subtext_color == v)
        return;
    delegate_->subtext_color = v;
    update();
}

const QColor &ResultsList::selectionSubtextColor() const
{ return delegate_->selection_subtext_color; }

void ResultsList::setSelectionSubtextColor(const QColor &v)
{
    if (delegate_->selection_subtext_color == v)
        return;
    delegate_->selection_subtext_color = v;
    update();
}

int ResultsList::horizontalSpacing() const { return delegate_->horizontal_spacing; }

void ResultsList::setHorizontalSpacing(int v)
{
    if (delegate_->horizontal_spacing == v)
        return;
    delegate_->horizontal_spacing = v;
    relayout();
}

int ResultsList::verticalSpacing() const { return delegate_->vertical_spacing; }

void ResultsList::setVerticalSpacing(int v)
{
    if (delegate_->vertical_spacing == v)
        return;
    delegate_->vertical_spacing = v;
    relayout();
}
