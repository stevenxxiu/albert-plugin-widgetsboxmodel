// Copyright (c) 2024-2025 Manuel Schneider

#pragma once
#include <filesystem>
#include <map>
#include <QColor>
#include <QBrush>
#include <QPalette>
class QPalette;


class Style
{
public:
    Style();
    Style(const QPalette &palette);

    QPalette palette;

    // not lexicographic, window metrics depend on input metrics
    uint   input_font_size;
    QColor input_trigger_color;
    QColor input_action_color;
    QColor input_hint_color;
    QBrush input_background_brush;
    QBrush input_border_brush;
    double input_border_width;
    uint   input_padding;
    double input_border_radius;

    QBrush window_border_brush;
    double window_border_width;
    uint   window_padding;
    double window_border_radius;
    QBrush window_background_brush;
    QBrush window_shadow_brush;
    uint   window_shadow_size;
    uint   window_shadow_offset;
    uint   window_spacing;
    uint   window_width;

    QColor settings_button_color;
    QColor settings_button_highlight_color;

    uint   result_item_icon_size;
    uint   result_item_text_font_size;
    uint   result_item_subtext_font_size;
    uint   result_item_horizontal_space;
    uint   result_item_vertical_space;
    QColor result_item_text_color;
    QColor result_item_subtext_color;
    QColor result_item_selection_text_color;
    QColor result_item_selection_subtext_color;
    QBrush result_item_selection_background_brush;
    QBrush result_item_selection_border_brush;
    double result_item_selection_border_radius;
    double result_item_selection_border_width;
    uint   result_item_padding;

    uint   action_item_font_size;
    QColor action_item_text_color;
    QColor action_item_selection_text_color;
    QBrush action_item_selection_background_brush;
    QBrush action_item_selection_border_brush;
    double action_item_selection_border_radius;
    double action_item_selection_border_width;
    uint   action_item_padding;
};

class StyleReader
{
public:
    StyleReader(std::vector<std::filesystem::path> directories);

    Style read(const QString &name) const;
    Style read(const std::filesystem::path &path) const;

    const std::vector<std::filesystem::path> style_directories;
    const std::map<QString, std::filesystem::path> styles;

    std::map<QString, QString> readRawEntriesRecursive(const std::filesystem::path &,
                                                       std::map<QString, QString> &) const;
};
