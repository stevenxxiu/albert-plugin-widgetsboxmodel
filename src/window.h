// Copyright (c) 2022-2026 Manuel Schneider

#pragma once
#include "windowframe.h"
#include <QEvent>
#include <QFileSystemWatcher>
#include <QPoint>
#include <QTimer>
#include <QWidget>
#include <filesystem>

namespace albert
{
class PluginInstance;
namespace detail { class Query; }
}  // namespace albert
class ActionDelegate;
class ActionsList;
class DebugOverlay;
class Frame;
class InputLine;
class ItemDelegate;
class Plugin;
class QEvent;
class QFrame;
class QKeyCombination;
class QListView;
class QPropertyAnimation;
class QSpacerItem;
class QStateMachine;
class ResultItemsModel;
class ResultsList;
class SettingsButton;
class Style;

class Window : public WindowFrame
{
    Q_OBJECT

public:

    Window(albert::PluginInstance &plugin);
    ~Window();

    albert::PluginInstance const &plugin;

    QString input() const;
    void setInput(const QString&);

    void setQuery(albert::detail::Query *query);

    const std::map<QString, std::filesystem::path> styles;

    bool darkMode() const;

private:

    void initializeUi();
    void initializeWindowActions();
    void initializeStatemachine();
    void installEventFilterKeepThisPrioritized(QObject *watched, QObject *filter);
    std::vector<std::filesystem::path> styleDirectories() const;
    void applyStyle(const QString& name);  // only for valid names, throws runtime_errors
    void applyStyle(const Style &);

    void onSettingsButtonClick(Qt::MouseButton button);
    void onMatchActivation(const QModelIndex &);
    void onMatchActionActivation(const QModelIndex &);
    void onFallbackActivation(const QModelIndex &);
    void onFallbackActionActivation(const QModelIndex &);
    void onStyleFileChanged();

    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    QStateMachine *state_machine;

    Frame *input_frame;
    InputLine *input_line;
    SettingsButton *settings_button;
    ResultsList *results_list;
    ActionsList *actions_list;
    bool dark_mode;

    albert::detail::Query *current_query;
    QListView *keyboard_navigation_receiver;

    enum Mod {Shift, Meta, Contol, Alt};
    Mod mod_command = Mod::Contol;
    Mod mod_actions = Mod::Alt;
    Mod mod_fallback = Mod::Meta;

    QString style_light_;  // null or exists in styles
    QString style_dark_;   // null or exists in styles
    QFileSystemWatcher style_watcher;
    bool hideOnFocusLoss_;
    bool showCentered_;
    bool followCursor_;
    bool edit_mode_;
    QColor settings_button_color_;
    QColor settings_button_color_highlight_;
    std::unique_ptr<DebugOverlay> debug_overlay_;
    std::unique_ptr<QPropertyAnimation> color_animation_;
    std::unique_ptr<QPropertyAnimation> speed_animation_;

    enum EventType {
        ShowActions = QEvent::User,
        HideActions,
        ToggleActions,
        ShowFallbacks,
        HideFallbacks,
        SettingsButtonEnter,
        SettingsButtonLeave,
        InputFrameEnter,
        InputFrameLeave,
    };

    struct Event : public QEvent {
        Event(EventType eventType) : QEvent((QEvent::Type)eventType) {}
    };

    void postCustomEvent(EventType type);

signals:

    void inputChanged(QString);
    void visibleChanged(bool);
    void queryChanged(albert::detail::Query*);
    void queryActiveChanged(bool);  // Convenience signal to avoid reconnects
    void queryHasMatches();  // Convenience signal to avoid reconnects

public:

    const QString &styleLight() const;
    void setStyleLight(const QString& name);

    const QString &styleDark() const;
    void setStyleDark(const QString& name);

    bool alwaysOnTop() const;
    void setAlwaysOnTop(bool alwaysOnTop);

    bool clearOnHide() const;
    void setClearOnHide(bool b = true);

    bool displayScrollbar() const;
    void setDisplayScrollbar(bool value);

    bool followCursor() const;
    void setFollowCursor(bool b = true);

    bool hideOnFocusLoss() const;
    void setHideOnFocusLoss(bool b = true);

    bool historySearchEnabled() const;
    void setHistorySearchEnabled(bool b = true);

    uint maxResults() const;
    void setMaxResults(uint max);

    bool showCentered() const;
    void setShowCentered(bool b = true);

    bool disableInputMethod() const;
    void setDisableInputMethod(bool b = true);

    bool debugMode() const;
    void setDebugMode(bool b = true);

    bool editModeEnabled() const;
    void setEditModeEnabled(bool v);

signals:

    void alwaysOnTopChanged(bool);
    void clearOnHideChanged(bool);
    void displayClientShadowChanged(bool);
    void displayScrollbarChanged(bool);
    void displaySystemShadowChanged(bool);
    void followCursorChanged(bool);
    void hideOnFocusLossChanged(bool);
    void historySearchEnabledChanged(bool);
    void maxResultsChanged(uint);
    void quitOnCloseChanged(bool);
    void showCenteredChanged(bool);
    void debugModeChanged(bool);
    void styleDarkChanged(QString);
    void styleLightChanged(QString);
    void editModeEnabledChanged(bool);
};
