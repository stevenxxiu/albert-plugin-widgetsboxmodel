// Copyright (c) 2024-2026 Manuel Schneider

#include "style.h"
#include <QApplication>
#include <QDir>
#include <QFont>
#include <QLinearGradient>
#include <QRegularExpression>
#include <QSettings>
#include <QStyle>
#include <albert/logging.h>
#include <albert/systemutil.h>
#include <expected>
#include <filesystem>
#include <map>
#include <ranges>
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;

namespace
{

struct {
    QString base                                   = "palette/base"_L1;
    QString text                                   = "palette/text"_L1;
    QString window                                 = "palette/window"_L1;
    QString window_text                            = "palette/window_text"_L1;
    QString button                                 = "palette/button"_L1;
    QString button_text                            = "palette/button_text"_L1;
    QString light                                  = "palette/light"_L1;
    QString mid                                    = "palette/mid"_L1;
    QString dark                                   = "palette/dark"_L1;
    QString placeholder_text                       = "palette/placeholder_text"_L1;
    QString highlight                              = "palette/highlight"_L1;
    QString highlight_text                         = "palette/highlight_text"_L1;
    QString link                                   = "palette/link"_L1;
    QString link_visited                           = "palette/link_visited"_L1;

    QString action_item_font_size                  = "window/action_item_font_size"_L1;
    QString action_item_padding                    = "window/action_item_padding"_L1;
    QString action_item_selection_background_brush = "window/action_item_selection_background_brush"_L1;
    QString action_item_selection_border_brush     = "window/action_item_selection_border_brush"_L1;
    QString action_item_selection_border_radius    = "window/action_item_selection_border_radius"_L1;
    QString action_item_selection_border_width     = "window/action_item_selection_border_width"_L1;
    QString action_item_selection_text_color       = "window/action_item_selection_text_color"_L1;
    QString action_item_text_color                 = "window/action_item_text_color"_L1;

    QString input_background_brush                 = "window/input_background_brush"_L1;
    QString input_border_brush                     = "window/input_border_brush"_L1;
    QString input_border_radius                    = "window/input_border_radius"_L1;
    QString input_border_width                     = "window/input_border_width"_L1;
    QString input_font_size                        = "window/input_font_size"_L1;
    QString input_trigger_color                    = "window/input_trigger_color"_L1;
    QString input_action_color                     = "window/input_action_color"_L1;
    QString input_hint_color                       = "window/input_hint_color"_L1;
    QString input_padding                          = "window/input_padding"_L1;

    QString result_item_horizontal_space           = "window/result_item_horizontal_space"_L1;
    QString result_item_icon_size                  = "window/result_item_icon_size"_L1;
    QString result_item_padding                    = "window/result_item_padding"_L1;
    QString result_item_selection_background_brush = "window/result_item_selection_background_brush"_L1;
    QString result_item_selection_border_brush     = "window/result_item_selection_border_brush"_L1;
    QString result_item_selection_border_radius    = "window/result_item_selection_border_radius"_L1;
    QString result_item_selection_border_width     = "window/result_item_selection_border_width"_L1;
    QString result_item_selection_subtext_color    = "window/result_item_selection_subtext_color"_L1;
    QString result_item_selection_text_color       = "window/result_item_selection_text_color"_L1;
    QString result_item_subtext_color              = "window/result_item_subtext_color"_L1;
    QString result_item_subtext_font_size          = "window/result_item_subtext_font_size"_L1;
    QString result_item_text_color                 = "window/result_item_text_color"_L1;
    QString result_item_text_font_size             = "window/result_item_text_font_size"_L1;
    QString result_item_vertical_space             = "window/result_item_vertical_space"_L1;

    QString settings_button_color                  = "window/settings_button_color"_L1;
    QString settings_button_highlight_color        = "window/settings_button_highlight_color"_L1;

    QString window_background_brush                = "window/window_background_brush"_L1;
    QString window_border_brush                    = "window/window_border_brush"_L1;
    QString window_border_radius                   = "window/window_border_radius"_L1;
    QString window_border_width                    = "window/window_border_width"_L1;
    QString window_padding                         = "window/window_padding"_L1;
    QString window_shadow_brush                    = "window/window_shadow_brush"_L1;
    QString window_shadow_offset                   = "window/window_shadow_offset"_L1;
    QString window_shadow_size                     = "window/window_shadow_size"_L1;
    QString window_spacing                         = "window/window_spacing"_L1;
    QString window_width                           = "window/window_width"_L1;
} const key;

const uint general_spacing                         = 6;

}

//--------------------------------------------------------------------------------------------------

Style::Style():
    Style(QApplication::palette())
{}

Style::Style(const QPalette &p):
    palette(p),
    // Do sync with template.ini

    input_font_size                          (QApplication::font().pointSize() + 9),
    input_trigger_color                      (p.color(QPalette::Highlight)),
    input_action_color                       (p.color(QPalette::PlaceholderText)),
    input_hint_color                         (p.color(QPalette::PlaceholderText)),
    input_background_brush                   (p.brush(QPalette::Base)),
    input_border_brush                       (p.brush(QPalette::Highlight)),
    input_border_width                       (0),
    input_padding                            (0),  // Plaintextedit has a margin of 1
    input_border_radius                      (general_spacing + input_padding),

    window_border_brush                      (p.brush(QPalette::Highlight)),
    window_border_width                      (1),
    window_padding                           (general_spacing + (int)window_border_width),
    window_border_radius                     (window_padding + input_border_radius),
    window_background_brush                  (p.brush(QPalette::Window)),
    window_shadow_brush                      (QColor(0, 0, 0, 128)),
    window_shadow_size                       (32),
    window_shadow_offset                     (8),
    window_spacing                           (general_spacing),
    window_width                             (640),

    settings_button_color                    (p.color(QPalette::Button)),
    settings_button_highlight_color          (p.color(QPalette::Highlight)),

    result_item_icon_size                    (36),
    result_item_text_font_size               (QApplication::font().pointSize() + 4),
    result_item_subtext_font_size            (QApplication::font().pointSize() - 1),
    result_item_horizontal_space             (general_spacing),
    result_item_vertical_space               (1),
    result_item_text_color                   (p.color(QPalette::WindowText)),
    result_item_subtext_color                (p.color(QPalette::PlaceholderText)),
    result_item_selection_text_color         (p.color(QPalette::HighlightedText)),
    result_item_selection_subtext_color      (p.color(QPalette::PlaceholderText)),
    result_item_selection_background_brush   (p.brush(QPalette::Highlight)),
    result_item_selection_border_brush       (p.brush(QPalette::Highlight)),
    result_item_selection_border_radius      (input_border_radius),
    result_item_selection_border_width       (0),
    result_item_padding                      (general_spacing),

    action_item_font_size                    (QApplication::font().pointSize()),
    action_item_text_color                   (p.color(QPalette::WindowText)),
    action_item_selection_text_color         (p.color(QPalette::HighlightedText)),
    action_item_selection_background_brush   (p.brush(QPalette::Highlight)),
    action_item_selection_border_brush       (p.brush(QPalette::Highlight)),
    action_item_selection_border_radius      (input_border_radius),
    action_item_selection_border_width       (0),
    action_item_padding                      (general_spacing)
{}

//--------------------------------------------------------------------------------------------------

static optional<pair<QString, multimap<QString, QString>>> parseFunction(const QString &s)
{
    static const auto re_fn = QRegularExpression(R"(([\w-]+)\((.+)\))"_L1);
    static const auto re_arg = QRegularExpression(R"(^(?:\s*(\w+)\s*:\s*(.+))\s*$)"_L1);

    auto match = re_fn.match(s);
    if (!match.hasMatch())
        return nullopt;

    const auto fn_name = match.captured(1);
    multimap<QString, QString> args;
    for (const auto &arg : match.captured(2).simplified().split(","_L1))
    {
        match = re_arg.match(arg);
        if (!match.hasMatch())
        {
            WARN << "Invalid argument:" << arg;
            return nullopt;
        }
        args.emplace(match.captured(1), match.captured(2));
    }

    return pair{fn_name, args};
}

static QBrush parseBrush(const QString &s, const filesystem::path &style_path)
{
    if (const auto opt = parseFunction(s); opt)
    {
        const auto &[fn, args] = *opt;
        if (fn == "linear-gradient"_L1)
        {
            const auto x1 = args.equal_range(u"x1"_s).first->second.toDouble();
            const auto y1 = args.equal_range(u"y1"_s).first->second.toDouble();
            const auto x2 = args.equal_range(u"x2"_s).first->second.toDouble();
            const auto y2 = args.equal_range(u"y2"_s).first->second.toDouble();

            QLinearGradient lg(x1, y1, x2, y2);

            for (const auto &[beg, end] = args.equal_range(u"stop"_s);
                 const auto &stop_arg : ranges::subrange(beg, end) | views::values)
            {
                const auto stop_args = stop_arg.split(u" "_s, Qt::SkipEmptyParts);
                if (stop_args.size() != 2)
                {
                    WARN << "Invalid gradient stop:" << stop_arg;
                    return Qt::red;
                }
                else
                    lg.setColorAt(stop_args[0].toDouble(), QColor(stop_args[1]));
            }

            lg.setCoordinateMode(QGradient::ObjectMode);
            return lg;
        }
        // else if (fn == "radial-gradient"_L1)
        // {
        // }
        else if (fn == "image"_L1)
        {
            filesystem::path img_path = args.equal_range(u"src"_s).first->second.toStdString();
            if (img_path.is_absolute())
                return QBrush(QImage(toQString(img_path)));
            else
                return QBrush(QImage(toQString(style_path.parent_path() / img_path)));
        }
        else
            WARN << "Invalid brush function:" << s;

        return {};
    }

    else if (QColor c(s); c.isValid())
        return c;

    else
        return {};
}


static map<QString, filesystem::path> searchStyles(const vector<filesystem::path> &directories)
{
    map<QString, filesystem::path> t;
    for (const auto &path : directories)
        for (const auto ini_files = QDir(path).entryInfoList({u"*.ini"_s},
                                                               QDir::Files | QDir::NoSymLinks);
             const auto &ini_file_info : ini_files)
            t.emplace(ini_file_info.baseName(),
                      ini_file_info.canonicalFilePath().toStdString());
    return t;
}

struct Parser
{
    map<QString, QString> &raw_entries;
    const std::filesystem::path &path;

    expected<QString, QString> resolve(const QString &k)
    {
        if (const auto raw_value_it = raw_entries.find(k);
            raw_value_it == raw_entries.end())
            return unexpected(u"Key not found: %1"_s.arg(k));
        else if (!raw_value_it->second.startsWith(u"$"_s))
            return raw_value_it->second;
        else
            return resolve(raw_value_it->second.mid(1))
                .transform_error([&](auto err){ return u"%1 > %2"_s.arg(k, err); });
    }

    QColor mandatoryColor(const QString &k)
    {
        if (const auto raw_value = resolve(k);
            !raw_value.has_value())
            throw runtime_error(format("Mandatory key missing: {} ({})",
                                       k.toStdString(), raw_value.error().toStdString()));
        else if (QColor c(*raw_value); !c.isValid())
            throw runtime_error(format("Invalid color: {}", raw_value->toStdString()));
        else
            return c;
    }

    void optional(const QString &k, std::integral auto *out)
    {
        using T = std::remove_pointer_t<decltype(out)>;
        if (const auto raw_value = resolve(k);
            raw_value.has_value())
        {
            bool ok = true;
            if (const auto v = raw_value->toLongLong(&ok); !ok)
                throw runtime_error(format("Invalid integer: {}", raw_value->toStdString()));

            else if (v < std::numeric_limits<T>::min() || v > std::numeric_limits<T>::max())
                throw runtime_error(format("Integer out of range: {}", raw_value->toStdString()));
            else
                *out = static_cast<T>(v);
        }
    };

    void optional(const QString &k, double *out)
    {
        if (const auto raw_value = resolve(k);
            raw_value.has_value())
        {
            bool ok = true;
            if (const auto real = raw_value->toDouble(&ok); ok)
                *out = real;
            else
                throw runtime_error(format("Invalid float: {}", raw_value->toStdString()));
        }
    };

    void optional(const QString &k, QColor *out)
    {
        if (const auto raw_value = resolve(k);
            !raw_value.has_value())
            return;
        else if (const auto c = QColor::fromString(*raw_value); c.isValid())
            *out = c;
        else
            throw runtime_error(format("Invalid color: {}", raw_value->toStdString()));
    };

    void optional(const QString &k, QBrush *out)
    {
        if (const auto raw_value = resolve(k);
            !raw_value.has_value())
            return;
        else if (const auto b = parseBrush(*raw_value, path);
                 b == Qt::NoBrush)
            throw runtime_error(format("Invalid brush: {}", raw_value->toStdString()));
        else
            *out = b;
    };

};

StyleReader::StyleReader(vector<filesystem::path> dirs)
    : style_directories(::move(dirs))
    , styles(searchStyles(style_directories))
{}

Style StyleReader::read(const QString &name) const { return read(styles.at(name)); }

Style StyleReader::read(const std::filesystem::path &path) const
{
    map<QString, QString> raw_entries;
    readRawEntriesRecursive(path, raw_entries);



    // Resolve and read values

    // map<QString, QBrush> brushes;

    // while(raw_values.size() > 0)
    // {
    //     auto c = raw_values.size();

    //     // Parse all non references
    //     for (auto it = raw_values.begin(); it != raw_values.end();)
    //     {
    //         auto &[k, v] = *it;

    //         if (v[0] == u'$')
    //             ++it;

    //         else if (auto b = parseBrush(v, path); b.style() == Qt::NoBrush)
    //             throw runtime_error(QStringLiteral("Invalid brush for %1: %2").arg(k, v).toStdString());

    //         else
    //         {
    //             brushes.emplace(k, b);
    //             it = raw_values.erase(it);
    //         }
    //     }

    //     if (c == raw_values.size())
    //         throw runtime_error(QStringLiteral("Cyclic reference: %1").arg(raw_values.begin()->first).toStdString());

    //     // Resolve references (only references in kv)
    //     for (auto it = raw_values.begin(); it != raw_values.end();)
    //     {
    //         auto &[k, v] = *it;

    //         // if ref is already parsed resolve directly
    //         if (auto b_it = brushes.find(raw_value); b_it != brushes.end())
    //         {
    //             brushes.emplace(k, b_it->second);
    //             it = raw_values.erase(it);
    //         }

    //         // if not already parsed and the key exists update reference
    //         else if (auto ref_it = raw_values.find(v.mid(1)); ref_it != raw_values.end())
    //         {
    //             v = ref_it->second;
    //             ++it;
    //         }

    //         // If neither is the case we have a dangling refernce;
    //         else
    //             throw runtime_error(QStringLiteral("Dangling reference: %1").arg(v).toStdString());
    //     }
    // }

     Parser parse(raw_entries, path);


    // Read palette

    Style style;

    // If any palette overrides use it
    if (const auto keys = raw_entries | views::keys;
        ranges::find_if(keys, [](const auto &k){ return k.startsWith(u"palette/"_s); }) != keys.end())
    {

        auto base             = parse.mandatoryColor(key.base);
        auto text             = parse.mandatoryColor(key.text);
        auto window           = parse.mandatoryColor(key.window);
        auto window_text      = parse.mandatoryColor(key.window_text);
        auto button           = parse.mandatoryColor(key.button);
        auto button_text      = parse.mandatoryColor(key.button_text);
        auto highlight        = parse.mandatoryColor(key.highlight);
        auto highlight_text   = parse.mandatoryColor(key.highlight_text);
        auto placeholder_text = parse.mandatoryColor(key.placeholder_text);
        auto link             = parse.mandatoryColor(key.link);
        auto link_visited     = parse.mandatoryColor(key.link_visited);

        QColor light, mid, dark;
        try
        {
            light = parse.mandatoryColor(key.light);
            mid   = parse.mandatoryColor(key.mid);
            dark  = parse.mandatoryColor(key.dark);
        }
        catch (...)
        {
            light = button.lighter();
            mid   = button.darker();
            dark  = mid.darker();
        }

        QPalette palette(window_text, button, light, mid, dark, text, button_text, base, window);
        palette.setColor(QPalette::All, QPalette::Highlight, highlight);
        palette.setColor(QPalette::All, QPalette::HighlightedText, highlight_text);
        palette.setColor(QPalette::All, QPalette::Link, link);
        palette.setColor(QPalette::All, QPalette::LinkVisited, link_visited);
        palette.setColor(QPalette::All, QPalette::PlaceholderText, placeholder_text);

        style = Style(palette);
    }


    // Read optional values

    parse.optional(key.action_item_font_size,
                   &style.action_item_font_size);
    parse.optional(key.action_item_padding,
                   &style.action_item_padding);
    parse.optional(key.action_item_selection_background_brush,
                   &style.action_item_selection_background_brush);
    parse.optional(key.action_item_selection_border_brush,
                   &style.action_item_selection_border_brush);
    parse.optional(key.action_item_selection_border_radius,
                   &style.action_item_selection_border_radius);
    parse.optional(key.action_item_selection_border_width,
                   &style.action_item_selection_border_width);
    parse.optional(key.action_item_selection_text_color,
                   &style.action_item_selection_text_color);
    parse.optional(key.action_item_text_color,
                   &style.action_item_text_color);

    parse.optional(key.input_background_brush,
                   &style.input_background_brush);
    parse.optional(key.input_border_brush,
                   &style.input_border_brush);
    parse.optional(key.input_border_radius,
                   &style.input_border_radius);
    parse.optional(key.input_border_width,
                   &style.input_border_width);
    parse.optional(key.input_font_size,
                   &style.input_font_size);
    parse.optional(key.input_trigger_color,
                   &style.input_trigger_color);
    parse.optional(key.input_hint_color,
                   &style.input_hint_color);
    parse.optional(key.input_action_color,
                   &style.input_action_color);
    parse.optional(key.input_padding,
                   &style.input_padding);

    parse.optional(key.result_item_horizontal_space,
                   &style.result_item_horizontal_space);
    parse.optional(key.result_item_icon_size,
                   &style.result_item_icon_size);
    parse.optional(key.result_item_padding,
                   &style.result_item_padding);
    parse.optional(key.result_item_selection_background_brush,
                   &style.result_item_selection_background_brush);
    parse.optional(key.result_item_selection_border_brush,
                   &style.result_item_selection_border_brush);
    parse.optional(key.result_item_selection_border_radius,
                   &style.result_item_selection_border_radius);
    parse.optional(key.result_item_selection_border_width,
                   &style.result_item_selection_border_width);
    parse.optional(key.result_item_selection_subtext_color,
                   &style.result_item_selection_subtext_color);
    parse.optional(key.result_item_selection_text_color,
                   &style.result_item_selection_text_color);
    parse.optional(key.result_item_subtext_color,
                   &style.result_item_subtext_color);
    parse.optional(key.result_item_subtext_font_size,
                   &style.result_item_subtext_font_size);
    parse.optional(key.result_item_text_color,
                   &style.result_item_text_color);
    parse.optional(key.result_item_text_font_size,
                   &style.result_item_text_font_size);
    parse.optional(key.result_item_vertical_space,
                   &style.result_item_vertical_space);

    parse.optional(key.settings_button_color,
                   &style.settings_button_color);
    parse.optional(key.settings_button_highlight_color,
                   &style.settings_button_highlight_color);

    parse.optional(key.window_background_brush,
                   &style.window_background_brush);
    parse.optional(key.window_border_brush,
                   &style.window_border_brush);
    parse.optional(key.window_border_radius,
                   &style.window_border_radius);
    parse.optional(key.window_border_width,
                   &style.window_border_width);
    parse.optional(key.window_padding,
                   &style.window_padding);
    parse.optional(key.window_shadow_brush,
                   &style.window_shadow_brush);
    parse.optional(key.window_shadow_offset,
                   &style.window_shadow_offset);
    parse.optional(key.window_shadow_size,
                   &style.window_shadow_size);
    parse.optional(key.window_spacing,
                   &style.window_spacing);
    parse.optional(key.window_width,
                   &style.window_width);

    return style;

}

map<QString, QString> StyleReader::readRawEntriesRecursive(const filesystem::path &path,
                                                           map<QString, QString> &raw_entries) const
{

    QSettings ini(toQString(path), QSettings::IniFormat);

    if (ini.contains(u"bases"_s))
    {
        const auto bases = ini.value(u"bases"_s).toString().split(u","_s, Qt::SkipEmptyParts)
                           | ranges::views::transform([](const QString &s){ return s.trimmed(); });
        for (const auto &base : bases)
            if (const auto it = styles.find(base); it != styles.end())
                readRawEntriesRecursive(it->second, raw_entries);
            else if (auto base_path = filesystem::path(base.toStdString());
                     base_path.is_absolute() && filesystem::exists(base_path))
                readRawEntriesRecursive(base_path, raw_entries);
            else if (base_path = path.parent_path() / base.toStdString();
                     filesystem::exists(base_path))
                readRawEntriesRecursive(base_path, raw_entries);
            else
                throw runtime_error(format("Could not find base style: {}", base.toStdString()));
    }

    for (const auto &k : ini.allKeys())
        if (auto v = ini.value(k);
            v.typeId() == qMetaTypeId<QStringList>())
            raw_entries.insert_or_assign(k, v.toStringList().join(u","_s));
        else if(v.typeId() == qMetaTypeId<QString>())
            raw_entries.insert_or_assign(k, v.toString().trimmed());
        else
            throw runtime_error(format("Unsupported entry {}", k.toStdString()));

    return raw_entries;
}
