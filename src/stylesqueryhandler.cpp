// Copyright (c) 2022-2025 Manuel Schneider

#include "stylesqueryhandler.h"
#include "window.h"
#include <albert/icon.h>
#include <albert/matcher.h>
#include <albert/standarditem.h>
#include <albert/systemutil.h>
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;

StylesQueryHandler::StylesQueryHandler(Window *w) : window(w) {}

QString StylesQueryHandler::id() const { return u"styles"_s; }

QString StylesQueryHandler::name() const { return Window::tr("Styles"); }

QString StylesQueryHandler::description() const { return Window::tr("Switch styles"); }

//: The trigger
QString StylesQueryHandler::defaultTrigger() const { return Window::tr("style") + QChar::Space; }

static vector<Action> makeActions(Window *window, const QString& name)
{
    vector<Action> actions;
    actions.emplace_back(u"setlight"_s,
                         Window::tr("Use in light mode"),
                         [window, name]{ window->setStyleLight(name); });

    actions.emplace_back(u"setdark"_s,
                         Window::tr("Use in dark mode"),
                         [window, name]{ window->setStyleDark(name); });

    if (window->darkMode())
        std::swap(actions[0], actions[1]);

    return actions;
}

static unique_ptr<Icon> makeIcon() { return Icon::grapheme(u"🎨"_s); }

vector<RankItem> StylesQueryHandler::rankItems(QueryContext &ctx)
{
    Matcher matcher(ctx);
    vector<RankItem> items;
    const auto sytem_title = Window::tr("System");

    if (const auto m = matcher.match(sytem_title); m)
        items.emplace_back(StandardItem::make(u"default"_s,
                                              sytem_title,
                                              Window::tr("The default style."),
                                              makeIcon,
                                              makeActions(window, {})),
                           m);

    for (const auto &[name, path] : window->styles)
        if (const auto m = matcher.match(name); m)
        {
            auto actions = makeActions(window, name);

            actions.emplace_back(u"open"_s, Window::tr("Open"),
                                 [path] { open(path); });

            actions.emplace_back(u"open"_s, Window::tr("Reveal in file manager"),
                                 [path] { open(path.parent_path()); });

            items.emplace_back(StandardItem::make(u"style_%1"_s.arg(name),
                                                  name,
                                                  toQString(path),
                                                  makeIcon,
                                                  ::move(actions)),
                               m);
        }

    return items;
}
