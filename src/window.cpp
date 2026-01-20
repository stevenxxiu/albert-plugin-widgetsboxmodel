// Copyright (c) 2022-2025 Manuel Schneider

#include "actionslist.h"
#include "debugoverlay.h"
#include "frame.h"
#include "inputline.h"
#include "resizinglist.h"
#include "resultitemmodel.h"
#include "resultslist.h"
#include "settingsbutton.h"
#include "statetransitions.h"
#include "style.h"
#include "util.h"
#include "window.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QPixmapCache>
#include <QPropertyAnimation>
#include <QSettings>
#include <QStateMachine>
#include <QStringListModel>
#include <QStyleFactory>
#include <QTimer>
#include <QWindow>
#include <albert/app.h>
#include <albert/logging.h>
#include <albert/messagebox.h>
#include <albert/plugininstance.h>
#include <albert/pluginloader.h>
#include <albert/pluginmetadata.h>
#include <albert/query.h>
#include <albert/queryexecution.h>
#include <albert/queryhandler.h>
#include <albert/queryresults.h>
#include <albert/systemutil.h>
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;

namespace  {

const double settings_button_rps_idle = 0.2;
const double settings_button_rps_busy = 0.5;
const unsigned settings_button_rps_animation_duration = 3000;
const unsigned settings_button_fade_animation_duration = 500;
const unsigned settings_button_highlight_animation_duration = 1000;

const struct {
    const bool      always_on_top                               = true;
    const bool      centered                                    = true;
    const bool      clear_on_hide                               = true;
    const bool      debug                                       = false;
    const bool      display_scrollbar                           = false;
    const bool      follow_cursor                               = true;
    const bool      hide_on_focus_loss                          = true;
    const bool      history_search                              = true;
    const bool      quit_on_close                               = false;
    const bool      shadow_client                               = true;
    const bool      shadow_system                               = false;
    const bool      disable_input_method                        = true;
    const uint      max_results                                 = 5;
} defaults;


const struct {

    const char* window_position                        = "windowPosition";

    const char *always_on_top                          = "alwaysOnTop";
    const char *centered                               = "showCentered";
    const char *clear_on_hide                          = "clearOnHide";
    const char *debug                                  = "debug";
    const char *display_scrollbar                      = "displayScrollbar";
    const char *follow_cursor                          = "followCursor";
    const char *hide_on_focus_loss                     = "hideOnFocusLoss";
    const char *history_search                         = "historySearch";
    const char *max_results                            = "itemCount";
    const char *quit_on_close                          = "quitOnClose";
    const char *shadow_client                          = "clientShadow";
    const char *shadow_system                          = "systemShadow";
    const char *style_dark                             = "darkStyle";
    const char *style_light                            = "lightStyle";
    const char *disable_input_method                   = "disable_input_method";

} keys;

constexpr Qt::KeyboardModifier mods_mod[] = {
   Qt::ShiftModifier,
   Qt::MetaModifier,
   Qt::ControlModifier,
   Qt::AltModifier
};

constexpr Qt::Key mods_keys[] = {
    Qt::Key_Shift,
    Qt::Key_Meta,
    Qt::Key_Control,
    Qt::Key_Alt
};

}

Window::Window(PluginInstance &p) :
    plugin(p),
    styles(StyleReader(styleDirectories()).styles),
    input_frame(new Frame(this)),
    input_line(new InputLine(input_frame)),
    settings_button(new SettingsButton(input_frame)),
    results_list(new ResultsList(this)),
    actions_list(new ActionsList(this)),
    dark_mode(haveDarkSystemPalette()),
    current_query{nullptr},
    edit_mode_(false)
{
    initializeUi();
    initializeWindowActions();
    initializeStatemachine();

    // Reproducible UX
    auto *style = QStyleFactory::create(u"Fusion"_s);
    style->setParent(this);
    setStyleRecursive(this, style);

    connect(input_line, &InputLine::textChanged,
            this, [this]{ emit inputChanged(input_line->text()); });

    connect(settings_button, &SettingsButton::clicked,
            this, &Window::onSettingsButtonClick);

    connect(&style_watcher, &QFileSystemWatcher::fileChanged,
            this, &Window::onStyleFileChanged);

    QPixmapCache::setCacheLimit(1024 * 50);  // 50 MB
}

Window::~Window() {}

void Window::initializeUi()
{
    // Identifiers

    setObjectName("window");
    input_frame->setObjectName("inputFrame");
    settings_button->setObjectName("settingsButton");
    input_line->setObjectName("inputLine");
    results_list->setObjectName("resultsList");
    actions_list->setObjectName("actionList");


    // Structure

    auto *input_frame_layout = new QHBoxLayout(input_frame);
    input_frame_layout->addWidget(input_line, 0, Qt::AlignTop);  // Needed to remove ui flicker
    input_frame_layout->addWidget(settings_button, 0, Qt::AlignTop);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(input_frame);
    layout->addWidget(results_list);
    layout->addWidget(actions_list);
    layout->addStretch(0);


    // Properties

    // contentsMargins: setShadowSize
    // layout.contentsMargins: setWindowPadding
    // input_frame.contentsMargins: setInputPadding
    // input_frame_layout.contentsMargins: Have to be set to zero

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    input_frame->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);

    input_frame_layout->setContentsMargins(0,0,0,0);
    input_frame_layout->setSpacing(0);

    input_line->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);

    settings_button->setFocusPolicy(Qt::NoFocus);
    settings_button->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);

    results_list->setFocusPolicy(Qt::NoFocus);
    results_list->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    results_list->setAutoFillBackground(false);

    actions_list->setFocusPolicy(Qt::NoFocus);
    actions_list->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);


    // Misc

    input_line->installEventFilter(this);
    input_frame->installEventFilter(this);  // Proper leave enter events
    settings_button->installEventFilter(this);  // Proper leave enter events
    results_list->installEventFilter(this);  // Edge move detection

    settings_button->hide();
    results_list->hide();
    actions_list->hide();
    actions_list->setMaxItems(100);


    // Preferences

    auto s = plugin.settings();

    setAlwaysOnTop(s->value(keys.always_on_top,
                            defaults.always_on_top).toBool());

    setClearOnHide(s->value(keys.clear_on_hide,
                            defaults.clear_on_hide).toBool());

    setDisplayScrollbar(s->value(keys.display_scrollbar,
                                 defaults.display_scrollbar).toBool());

    setFollowCursor(s->value(keys.follow_cursor,
                             defaults.follow_cursor).toBool());

    setHideOnFocusLoss(s->value(keys.hide_on_focus_loss,
                                defaults.hide_on_focus_loss).toBool());

    setHistorySearchEnabled(s->value(keys.history_search,
                                     defaults.history_search).toBool());

    setMaxResults(s->value(keys.max_results,
                           defaults.max_results).toUInt());

    setShowCentered(s->value(keys.centered,
                             defaults.centered).toBool());

    setDisableInputMethod(s->value(keys.disable_input_method,
                                   defaults.disable_input_method).toBool());

    setDebugMode(s->value(keys.debug,
                          defaults.debug).toBool());

    if (auto t = s->value(keys.style_light).toString(); styles.contains(t))
        style_light_ = t;
    if (auto t = s->value(keys.style_dark).toString(); styles.contains(t))
        style_dark_ = t;

    // applyStyle requires a valid window for the message box. Set style later.
    QTimer::singleShot(0, this, [this]{ applyStyle(dark_mode ? style_dark_ : style_light_); });

    s = plugin.state();
    if (!showCentered() && s->contains(keys.window_position)
        && s->value(keys.window_position).canConvert<QPoint>())
        move(s->value(keys.window_position).toPoint());
}

void Window::initializeWindowActions()
{
    auto *a = new QAction(tr("Settings"), this);
    a->setShortcuts({QKeySequence("Ctrl+,"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    connect(a, &QAction::triggered, this, [] { App::instance().showSettings(); });
    addAction(a);

    a = new QAction(tr("Hide on focus out"), this);
    a->setShortcuts({QKeySequence("Meta+h"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    a->setCheckable(true);
    a->setChecked(hideOnFocusLoss());
    connect(a, &QAction::toggled, this, &Window::setHideOnFocusLoss);
    connect(this, &Window::hideOnFocusLossChanged, a, &QAction::setChecked);
    addAction(a);

    a = new QAction(tr("Show centered"), this);
    a->setShortcuts({QKeySequence("Meta+c"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    a->setCheckable(true);
    a->setChecked(showCentered());
    connect(a, &QAction::toggled, this, &Window::setShowCentered);
    connect(this, &Window::showCenteredChanged, a, &QAction::setChecked);
    addAction(a);

    a = new QAction(tr("Clear on hide"), this);
    a->setShortcuts({QKeySequence("Meta+i"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    a->setCheckable(true);
    a->setChecked(clearOnHide());
    connect(a, &QAction::toggled, this, &Window::setClearOnHide);
    connect(this, &Window::clearOnHideChanged, a, &QAction::setChecked);
    addAction(a);

    a = new QAction(tr("Input edit mode"), this);
    a->setShortcuts({QKeySequence("Meta+e"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    a->setCheckable(true);
    a->setChecked(editModeEnabled());
    connect(a, &QAction::toggled, this, &Window::setEditModeEnabled);
    connect(this, &Window::editModeEnabledChanged, a, &QAction::setChecked);
    addAction(a);

    a = new QAction(tr("Debug mode"), this);
    a->setShortcuts({QKeySequence("Meta+d"_L1)});
    a->setShortcutVisibleInContextMenu(true);
    a->setCheckable(true);
    a->setChecked(debugMode());
    connect(a, &QAction::toggled, this, &Window::setDebugMode);
    connect(this, &Window::debugModeChanged, a, &QAction::setChecked);
    addAction(a);
}

static void setModelMemorySafe(QAbstractItemView *v, QAbstractItemModel *m)
{
    // See QAbstractItemView::setModel documentation
    auto dm = v->model();
    auto sm = v->selectionModel();
    v->setModel(m);
    if (m)
        m->setParent(v);
    if (sm)
        delete sm;
    if (dm)
        delete dm;
}

inline static bool isActive(detail::Query *query) { return query && query->execution().isActive(); }

inline static bool isGlobal(detail::Query *query) { return query &&query->trigger().isEmpty(); }

inline static bool hasMatches(detail::Query *query) { return query &&query->matches().count() > 0; }

inline static bool hasFallbacks(detail::Query *query) { return query &&query->fallbacks().count() > 0; }

void Window::initializeStatemachine()
{
    //
    // States
    //

    auto s_root = new QState(QState::ParallelStates);

    auto s_settings_button_appearance = new QState(s_root);
    auto s_settings_button_hidden = new QState(s_settings_button_appearance);
    auto s_settings_button_visible = new QState(s_settings_button_appearance);
    auto s_settings_button_highlight = new QState(s_settings_button_appearance);
    auto s_settings_button_highlight_delay = new QState(s_settings_button_appearance);
    s_settings_button_appearance->setInitialState(s_settings_button_hidden);

    auto s_settings_button_spin = new QState(s_root);
    auto s_settings_button_slow = new QState(s_settings_button_spin);
    auto s_settings_button_fast = new QState(s_settings_button_spin);
    s_settings_button_spin->setInitialState(s_settings_button_slow);

    auto s_results = new QState(s_root);
    auto s_results_hidden = new QState(s_results);
    auto s_results_disabled = new QState(s_results);
    auto s_results_matches = new QState(s_results);
    auto s_results_fallbacks = new QState(s_results);
    s_results->setInitialState(s_results_hidden);

    auto s_actions = new QState(s_root);
    auto s_actions_hidden = new QState(s_actions);
    auto s_actions_visible = new QState(s_actions);
    s_actions->setInitialState(s_actions_hidden);

    auto display_delay_timer = new QTimer(this);
    display_delay_timer->setInterval(250);
    display_delay_timer->setSingleShot(true);

    auto busy_delay_timer = new QTimer(this);
    busy_delay_timer->setInterval(250);
    busy_delay_timer->setSingleShot(true);


    //
    // Debug
    //

    // for (auto &[state, name] : initializer_list<pair<QState*, QString>>{
    //          {s_settings_button_hidden, u"SB hidden"_s},
    //          {s_settings_button_visible, u"SB visible"_s},
    //          {s_settings_button_highlight, u"SB higlight"_s},
    //          {s_settings_button_highlight_delay, u"SB delay"_s},
    //          {s_settings_button_slow, u"SB slow"_s},
    //          {s_settings_button_fast, u"SB fast"_s},

    //          {s_results_hidden, u"RESULTS hidden"_s},
    //          {s_results_disabled, u"RESULTS disabled"_s},
    //          {s_results_matches, u"RESULTS matches"_s},
    //          {s_results_fallbacks, u"RESULTS fallbacks"_s},

    //          {s_actions_hidden, u"ACTIONS hidden"_s},
    //          {s_actions_visible, u"ACTIONS visible"_s}
    //      })
    // {
    //     QObject::connect(state, &QState::entered, this, [name]{ CRIT << ">>>> ENTER" << name; });
    //     QObject::connect(state, &QState::exited, this, [name]{ CRIT << "<<<< EXIT" << name; });
    // }

    // connect(input_line, &InputLine::textChanged, this, []{ CRIT << "InputLine::textChanged";});
    // connect(this, &Window::queryChanged, this, [](albert::Query*q){ CRIT << "Window::queryChanged" << q;});
    // connect(this, &Window::queryActiveChanged, this, [](bool b){ CRIT << "Window::queryActiveChanged" << b;});
    // connect(this, &Window::queryHasMatches, this, []{ CRIT << "Window::queryHasMatches";});

    //
    // Transitions
    //

    // settingsbutton hidden ->

    addTransition(s_settings_button_hidden, s_settings_button_visible, InputFrameEnter);

    addTransition(s_settings_button_hidden, s_settings_button_highlight, SettingsButtonEnter);

    addTransition(s_settings_button_hidden, s_settings_button_highlight, this, &Window::queryActiveChanged,
                  [this] { return isActive(current_query) && settings_button->isVisible(); });  // visible ??

    addTransition(s_settings_button_hidden, s_settings_button_highlight_delay, this, &Window::queryActiveChanged,
                  [this] { return isActive(current_query) && settings_button->isHidden(); });  // visible ??


    // settingsbutton visible ->

    addTransition(s_settings_button_visible, s_settings_button_hidden, InputFrameLeave);

    addTransition(s_settings_button_visible, s_settings_button_highlight, SettingsButtonEnter);

    addTransition(s_settings_button_visible, s_settings_button_highlight, this, &Window::queryActiveChanged,
                  [this]{ return isActive(current_query); });


    // settingsbutton highlight ->

    addTransition(s_settings_button_highlight, s_settings_button_hidden, this, &Window::queryActiveChanged,
                  [this] { return !isActive(current_query) && !input_frame->underMouse() && !settings_button->underMouse(); });

    addTransition(s_settings_button_highlight, s_settings_button_visible, this, &Window::queryActiveChanged,
                  [this] { return !isActive(current_query) && input_frame->underMouse() && !settings_button->underMouse(); });

    addTransition(s_settings_button_highlight, s_settings_button_visible, SettingsButtonLeave,
                  [this] { return input_frame->underMouse(); });

    addTransition(s_settings_button_highlight, s_settings_button_hidden, SettingsButtonLeave,
                  [this] { return !input_frame->underMouse(); });


    // settingsbutton delay highlight ->

    addTransition(s_settings_button_highlight_delay, s_settings_button_highlight,
                  busy_delay_timer, &QTimer::timeout);

    addTransition(s_settings_button_highlight_delay, s_settings_button_highlight, InputFrameEnter);

    addTransition(s_settings_button_highlight_delay, s_settings_button_highlight, SettingsButtonEnter);

    addTransition(s_settings_button_highlight_delay, s_settings_button_hidden, this, &Window::queryActiveChanged,
                  [this]{ return !isActive(current_query); });


    // settingsbutton spin

    addTransition(s_settings_button_slow, s_settings_button_fast, this, &Window::queryActiveChanged,
                  [this]{ return isActive(current_query); });

    addTransition(s_settings_button_fast, s_settings_button_slow, this, &Window::queryActiveChanged,
                  [this]{ return !isActive(current_query); });

    // hidden ->

    addTransition(s_results_hidden, s_results_matches, this, &Window::queryHasMatches);

    addTransition(s_results_hidden, s_results_fallbacks, ShowFallbacks,
                  [this]{ return hasFallbacks(current_query); });

    addTransition(s_results_hidden, s_results_fallbacks, this, &Window::queryActiveChanged,
                  [this]{ return !isActive(current_query) && hasFallbacks(current_query) && isGlobal(current_query); });


    // matches ->

    addTransition(s_results_matches, s_results_hidden, this, &Window::queryChanged,
                  [this]{ return !current_query; });

    addTransition(s_results_matches, s_results_disabled, this, &Window::queryChanged,
                  [this]{ return current_query; });

    addTransition(s_results_matches, s_results_fallbacks, ShowFallbacks,
                  [this]{ return hasFallbacks(current_query); });


    // fallbacks ->

    addTransition(s_results_fallbacks, s_results_hidden, this, &Window::queryChanged,
                  [this]{ return !current_query; });

    addTransition(s_results_fallbacks, s_results_disabled, this, &Window::queryChanged,
                  [this]{ return current_query; });

    addTransition(s_results_fallbacks, s_results_matches, HideFallbacks,
                  [this]{ return hasMatches(current_query); });

    addTransition(s_results_fallbacks, s_results_hidden, HideFallbacks,
                  [this]{ return !hasMatches(current_query) && isActive(current_query); });


    // disabled ->

    addTransition(s_results_disabled, s_results_hidden, this, &Window::queryChanged,
                  [this]{ return !current_query; });

    addTransition(s_results_disabled, s_results_hidden, display_delay_timer, &QTimer::timeout);

    addTransition(s_results_disabled, s_results_hidden, this, &Window::queryActiveChanged,
                  [this]{ return !isActive(current_query) && (!hasFallbacks(current_query) || !isGlobal(current_query)); });

    addTransition(s_results_disabled, s_results_fallbacks, this, &Window::queryActiveChanged,
                  [this]{ return !isActive(current_query) && hasFallbacks(current_query) && isGlobal(current_query); });

    addTransition(s_results_disabled, s_results_matches, this, &Window::queryHasMatches);


    // actions ->

    const auto valid_current_index_has_actions = [=, this]{
        const auto current_index = results_list->currentIndex();
        return current_index.isValid()
               && !current_index.data(ItemRoles::ActionsListRole).toStringList().isEmpty();
    };

    addTransition(s_actions_hidden, s_actions_visible, ShowActions,
                  valid_current_index_has_actions);

    addTransition(s_actions_hidden, s_actions_visible, ToggleActions,
                  valid_current_index_has_actions);

    addTransition(s_actions_visible, s_actions_hidden, HideActions);

    addTransition(s_actions_visible, s_actions_hidden, ToggleActions);

    addTransition(s_actions_visible, s_actions_hidden, s_results_matches, &QState::exited);

    addTransition(s_actions_visible, s_actions_hidden, s_results_fallbacks, &QState::exited);


    //
    // Behavior
    //

    // BUTTON

    QObject::connect(s_settings_button_hidden, &QState::entered, this, [this]{
        auto c = settings_button->color;
        c.setAlpha(0);
        color_animation_ = make_unique<QPropertyAnimation>(settings_button, "color");
        color_animation_->setEndValue(c);
        color_animation_->setEasingCurve(QEasingCurve::OutQuad);
        color_animation_->setDuration(500);
        connect(color_animation_.get(), &QPropertyAnimation::finished,
                settings_button, &SettingsButton::hide);
        color_animation_->start();
    });

    QObject::connect(s_settings_button_highlight_delay, &QState::entered,
                     this, [busy_delay_timer]{ busy_delay_timer->start(); });

    QObject::connect(s_settings_button_highlight_delay, &QState::exited,
                     this, [busy_delay_timer]{ busy_delay_timer->stop(); });

    QObject::connect(s_settings_button_visible, &QState::entered, this, [this]{
        settings_button->show();
        color_animation_ = make_unique<QPropertyAnimation>(settings_button, "color");
        color_animation_->setEndValue(settings_button_color_);
        color_animation_->setEasingCurve(QEasingCurve::OutQuad);
        color_animation_->setDuration(settings_button_fade_animation_duration);
        color_animation_->start();
    });

    QObject::connect(s_settings_button_highlight, &QState::entered, this, [this]{
        settings_button->show();
        color_animation_ = make_unique<QPropertyAnimation>(settings_button, "color");
        color_animation_->setEndValue(settings_button_color_highlight_);
        color_animation_->setEasingCurve(QEasingCurve::OutQuad);
        color_animation_->setDuration(settings_button_highlight_animation_duration);
        color_animation_->start();
    });

    QObject::connect(s_settings_button_slow, &QState::entered, this, [this]{
        speed_animation_ = make_unique<QPropertyAnimation>(settings_button, "speed");
        speed_animation_->setEndValue(settings_button_rps_idle);
        speed_animation_->setEasingCurve(QEasingCurve::OutQuad);
        speed_animation_->setDuration(settings_button_rps_animation_duration);
        speed_animation_->start();
    });

    QObject::connect(s_settings_button_fast, &QState::entered, this, [this]{
        speed_animation_ = make_unique<QPropertyAnimation>(settings_button, "speed");
        speed_animation_->setEndValue(settings_button_rps_busy);
        speed_animation_->setEasingCurve(QEasingCurve::InOutQuad);
        speed_animation_->setDuration(settings_button_rps_animation_duration);
        speed_animation_->start();
    });


    // RESULTS

    QObject::connect(s_results_hidden, &QState::entered, this, [this]{
        keyboard_navigation_receiver = nullptr;
        results_list->hide();
        setModelMemorySafe(results_list, nullptr);
    });

    QObject::connect(s_results_disabled, &QState::entered, this, [this, display_delay_timer]{
        // disable user interaction withough using enabled property (flickers)
        results_list->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        keyboard_navigation_receiver = nullptr;
        display_delay_timer->start();
    });

    QObject::connect(s_results_disabled, &QState::exited, this, [this]{
        // enable user interaction withough using enabled property (flickers)
        results_list->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    });

    QObject::connect(s_results_matches, &QState::entered, this, [this]{
        keyboard_navigation_receiver = results_list;
        setModelMemorySafe(results_list, new MatchItemsModel(current_query->matches(), current_query->execution()));

        connect(results_list, &ResizingList::activated, this, &Window::onMatchActivation);
        connect(actions_list, &ResizingList::activated, this, &Window::onMatchActionActivation);

        // let selection model currentChanged set input hint
        connect(results_list->selectionModel(), &QItemSelectionModel::currentChanged,
                this, [this](const QModelIndex &current, const QModelIndex&) {
            if (current.isValid())
                input_line->setCompletion(current.data(ItemRoles::InputActionRole).toString());
        });

        // Initialize if we have a selected item
        if (results_list->currentIndex().isValid())
            input_line->setCompletion(results_list->currentIndex()
                                          .data(ItemRoles::InputActionRole).toString());
        else
            input_line->setCompletion();

        results_list->show();
    });

    QObject::connect(s_results_matches, &QState::exited, this, [this]{
        disconnect(results_list, &ResizingList::activated, this, &Window::onMatchActivation);
        disconnect(actions_list, &ResizingList::activated, this, &Window::onMatchActionActivation);
    });

    QObject::connect(s_results_fallbacks, &QState::entered, this, [this]{
        keyboard_navigation_receiver = results_list;
        setModelMemorySafe(results_list, new ResultItemsModel(current_query->fallbacks()));

        connect(results_list, &ResizingList::activated, this, &Window::onFallbackActivation);
        connect(actions_list, &ResizingList::activated, this, &Window::onFallbackActionActivation);

        results_list->show();
    });

    QObject::connect(s_results_fallbacks, &QState::exited, this, [this]{
        disconnect(results_list, &ResizingList::activated, this, &Window::onFallbackActivation);
        disconnect(actions_list, &ResizingList::activated, this, &Window::onFallbackActionActivation);
    });

    QObject::connect(s_actions_visible, &QState::entered, this, [this]{
        keyboard_navigation_receiver = actions_list;
        auto m = new QStringListModel(results_list->currentIndex().data(ItemRoles::ActionsListRole)
                                     .toStringList(),
                                 actions_list);  // takes ownership
        setModelMemorySafe(actions_list, m);
        actions_list->show();
    });

    QObject::connect(s_actions_visible, &QState::exited, this, [this]{
        keyboard_navigation_receiver = results_list;
        actions_list->hide();
        setModelMemorySafe(actions_list, nullptr);
    });

    state_machine = new QStateMachine(this);
    state_machine->addState(s_root);
    state_machine->setInitialState(s_root);
    state_machine->start();
}

void Window::installEventFilterKeepThisPrioritized(QObject *watched, QObject *filter)
{
    // Eventfilters are processed in reverse order
    watched->removeEventFilter(this);
    watched->installEventFilter(filter);
    watched->installEventFilter(this);
}

void Window::postCustomEvent(EventType event_type)
{ state_machine->postEvent(new Event(event_type)); } // takes ownership

void Window::onSettingsButtonClick(Qt::MouseButton button)
{
    if (button == Qt::LeftButton)
        App::instance().showSettings();

    else if (button == Qt::RightButton)
    {
        auto *menu = new QMenu(this);
        menu->addActions(actions());
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(QCursor::pos());
    }
}

void Window::onMatchActivation(const QModelIndex &index)
{
    if (index.isValid())
        if (auto should_hide = current_query->matches().activate(index.row(), 0);
            should_hide != QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
            hide();
}

void Window::onMatchActionActivation(const QModelIndex &index)
{
    if (index.isValid())
        if (auto should_hide = current_query->matches().activate(results_list->currentIndex().row(),
                                                                 index.row());
            should_hide != QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
            hide();
}

void Window::onFallbackActivation(const QModelIndex &index)
{
    if (index.isValid())
        if (auto should_hide = current_query->fallbacks().activate(index.row(), 0);
            should_hide != QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
            hide();
}

void Window::onFallbackActionActivation(const QModelIndex &index)
{
    if (index.isValid())
        if (auto should_hide = current_query->fallbacks()
                                   .activate(results_list->currentIndex().row(), index.row());
            should_hide != QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
            hide();
}

void Window::onStyleFileChanged()
{
    try {
        INFO << "Style file changed, reloading style";
        StyleReader reader(styleDirectories());
        applyStyle(reader.read((dark_mode) ? style_dark_ : style_light_));
    } catch (const runtime_error &e) {
        WARN << e.what();
    }
}

QString Window::input() const { return input_line->text(); }

void Window::setInput(const QString &text) { input_line->setText(text); }

void Window::setQuery(detail::Query *q)
{
    if(current_query)
    {
        disconnect(&current_query->matches(), nullptr, this, nullptr);
        disconnect(&current_query->execution(), nullptr, this, nullptr);
    }

    current_query = q;
    emit queryChanged(q);

    if(q)
    {
        input_line->setTriggerLength(q->trigger().length());
        input_line->setSynopsis(q->handler().synopsis(q->query()));
        input_line->setCompletion();

        // Statemachine active state synchronization
        connect(&current_query->execution(), &QueryExecution::activeChanged,
                this, &Window::queryActiveChanged);
        emit current_query->execution().activeChanged(current_query->execution().isActive());

        // Statemachine hasMatches state synchronization
        if (q->matches().count() > 0)
            emit queryHasMatches();
        else
            connect(&current_query->matches(), &QueryResults::resultsInserted,
                    this, &Window::queryHasMatches,
                    Qt::SingleShotConnection);
    }
}

vector<filesystem::path> Window::styleDirectories() const
{
    auto v = plugin.dataLocations()
             | views::transform([](const auto &p) { return p / "styles"; })
             | views::filter([](const auto &p) { return exists(p); });

    return {begin(v), end(v)};  // TODO ranges::to
}

void Window::applyStyle(const QString& name)
{
    if (name.isNull())
        applyStyle(Style{});
    else
    {
        try {
            StyleReader reader(styleDirectories());
            applyStyle(reader.read(name));
            if (!style_watcher.files().isEmpty())
                style_watcher.removePaths(style_watcher.files());
            style_watcher.addPath(toQString(reader.styles.at(name)));
        } catch (const runtime_error &e) {
            applyStyle(Style());
            WARN << e.what();
            albert::warning(u"%1:%2\n\n%3"_s.arg(tr("Failed loading style"),
                                                 name,
                                                 QString::fromUtf8(e.what())));
        }
    }
}

void Window::applyStyle(const Style &style)
{
    QPixmapCache::clear();

    setPalette(style.palette);

    // this (Frame)
    this->setBackgroundBrush(style.window_background_brush);
    this->setBorderBrush(style.window_border_brush);
    this->setBorderWidth(style.window_border_width);
    this->setBorderRadius(style.window_border_radius);
    input_frame->setFixedWidth(style.window_width); // set on frame because it defines frame size

    // this (WindowFrame)

    this->setShadowSize(style.window_shadow_size);
    this->setShadowOffset(style.window_shadow_offset);
    this->setShadowBrush(style.window_shadow_brush);

    // this (layout)
    layout()->setContentsMargins(style.window_padding, style.window_padding,
                                 style.window_padding, style.window_padding);
    layout()->setSpacing(style.window_spacing);


    // input_frame (Frame)
    input_frame->setBackgroundBrush(style.input_background_brush);
    input_frame->setBorderBrush(style.input_border_brush);
    input_frame->setBorderWidth(style.input_border_width);
    input_frame->setBorderRadius(style.input_border_radius);

    // input_line (InputLine)
    auto f = input_line->font();
    f.setPointSize(style.input_font_size);
    input_line->setFont(f);
    input_line->setTriggerColor(style.input_trigger_color);
    input_line->setInputActionColor(style.input_action_color);
    input_line->setInputHintColor(style.input_hint_color);
    // input_frame->setContentsMargins(style.input_padding, style.input_padding,
    //                                 style.input_padding, style.input_padding);

    // input_line->setContentsMargins(-5,-5,-5,-5);
    // input_frame->setContentsMargins(-5,-5,-5,-5);

    // settings_button (SettingsButton)
    settings_button_color_ = style.settings_button_color; // ??
    settings_button->color = style.settings_button_color;
    settings_button->color.setAlpha(0);
    settings_button_color_highlight_ = style.settings_button_highlight_color;

    const auto fm = input_line->fontMetrics();
    auto input_line_height = fm.lineSpacing() + 2;  // 1px document margins

    // Let settingsbutton have same size as a capital 'M'
    // TODO: This is an ugly prototyped fix and should rather be solved by a proper input line class
    auto font_margin_fix = (fm.lineSpacing() - fm.capHeight()
                            - fm.tightBoundingRect(u"|"_s).width())/2+1;
    font_margin_fix = 0;

    settings_button->setFixedSize(4, 4);
    settings_button->setContentsMargins(font_margin_fix, font_margin_fix,
                                        font_margin_fix, font_margin_fix);

    results_list->setIconSize(style.result_item_icon_size);
    results_list->setTextFontSize(style.result_item_text_font_size);
    results_list->setSubtextFontSize(style.result_item_subtext_font_size);
    results_list->setHorizontalSpacing(style.result_item_horizontal_space);
    results_list->setVerticalSpacing(style.result_item_vertical_space);
    results_list->setPadding(style.result_item_padding);
    results_list->setTextColor(style.result_item_text_color);
    results_list->setSubtextColor(style.result_item_subtext_color);
    results_list->setSelectionTextColor(style.result_item_selection_text_color);
    results_list->setSelectionSubtextColor(style.result_item_selection_subtext_color);
    results_list->setSelectionBackgroundBrush(style.result_item_selection_background_brush);
    results_list->setSelectionBorderBrush(style.result_item_selection_border_brush);
    results_list->setSelectionBorderRadius(style.result_item_selection_border_radius);
    results_list->setSelectionBorderWidth(style.result_item_selection_border_width);

    actions_list->setTextFontSize(style.action_item_font_size);
    actions_list->setPadding(style.action_item_padding);
    actions_list->setTextColor(style.action_item_text_color);
    actions_list->setSelectionTextColor(style.action_item_selection_text_color);
    actions_list->setSelectionBackgroundBrush(style.action_item_selection_background_brush);
    actions_list->setSelectionBorderBrush(style.action_item_selection_border_brush);
    actions_list->setSelectionBorderRadius(style.action_item_selection_border_radius);
    actions_list->setSelectionBorderWidth(style.action_item_selection_border_width);



    updateGeometry();
    update();
}

bool Window::darkMode() const { return dark_mode; }

bool Window::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        switch (auto *ke = static_cast<QKeyEvent *>(event);
                ke->key())
        {
        case Qt::Key_Escape:
            if (editModeEnabled())
                setEditModeEnabled(false);
            else
                setVisible(false);
            break;
        }
    }

    else if (event->type() == QEvent::MouseButtonPress)
        windowHandle()->startSystemMove();

    else if (event->type() == QEvent::Show)
    {
        // If showCentered or off screen (e.g. display disconnected) move into visible area
        if (showCentered_ || !screen())
        {
            QScreen *screen = nullptr;
            if (followCursor_){
                if (screen = QGuiApplication::screenAt(QCursor::pos()); !screen){
                    WARN << "Could not retrieve screen for cursor position. Using primary screen.";
                    screen = QGuiApplication::primaryScreen();
                }
            }
            else
                screen = QGuiApplication::primaryScreen();

            move(screen->geometry().center().x() - frameSize().width() / 2,
                 screen->geometry().top() + screen->geometry().height() / 5);
        }

#if not defined Q_OS_MACOS // steals focus on macos
        raise();
        activateWindow();
#endif
        emit visibleChanged(true);
    }

    else if (event->type() == QEvent::Hide)
    {
        plugin.state()->setValue(keys.window_position, pos());

        setEditModeEnabled(false);
        QPixmapCache::clear();

        emit visibleChanged(false);
    }

    else if (event->type() == QEvent::ThemeChange)
    {
#ifdef Q_OS_LINUX
        // No automatic palette update on GNOME
        QApplication::setPalette(QApplication::style()->standardPalette());
#endif
        dark_mode = haveDarkSystemPalette();
        applyStyle((dark_mode) ? style_dark_ : style_light_);
    }

    else if (event->type() == QEvent::Close)
        hide();

    else if (event->type() == QEvent::WindowActivate)
    {
        //
        // Hiding/Showing a window does not generate a Leave/Enter event. As such QWidget does not
        // update the internal underMouse property on show if the window is has been hidden and the
        // mouse pointer moved outside the widget.
        //
        QEvent synth(QEvent::Enter);
        for (auto w = QApplication::widgetAt(QCursor::pos()); w;  w = w->parentWidget())
            QApplication::sendEvent(w, &synth);

    }

    else if (event->type() == QEvent::WindowDeactivate)
    {
        //
        // Hiding/Showing a window does not generate a Leave/Enter event. As such QWidget does not
        // update the internal underMouse property on show if the window is has been hidden and the
        // mouse pointer moved outside the widget.
        //
        QEvent synth(QEvent::Leave);
        for (auto w = QApplication::widgetAt(QCursor::pos()); w;  w = w->parentWidget())
            QApplication::sendEvent(w, &synth);

        if(hideOnFocusLoss_)
            setVisible(false);
    }

    // DEBG << event->type();

    return QWidget::event(event);
}

bool Window::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == input_line)
    {
        if (event->type() == QEvent::KeyPress)
        {
            auto *ke = static_cast<QKeyEvent *>(event);
            switch (ke->key()) {

            // Emacs/Vim key synth

            case Qt::Key_P:
            case Qt::Key_K:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_Up,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_N:
            case Qt::Key_J:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_Down,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_H:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_Left,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_L:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_Right,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_F:
            case Qt::Key_D:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_PageDown,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_B:
            case Qt::Key_U:
                if (ke->modifiers().testFlag(Qt::ControlModifier)) {
                    QKeyEvent syn(QEvent::KeyPress,
                                  Qt::Key_PageUp,
                                  ke->modifiers().setFlag(Qt::ControlModifier, false),
                                  ke->text(),
                                  ke->isAutoRepeat());
                    return QApplication::sendEvent(input_line, &syn);
                }
                break;

            case Qt::Key_W:
                if (ke->modifiers().testFlag(Qt::ControlModifier)){
                    input_line->deleteWordBackwards();
                }
                break;

            // Keyboard interaction of lists and edit mode relevant keys

            case Qt::Key_Tab:
                if (!edit_mode_)
                {
                    if (!input_line->completion().isEmpty())
                        input_line->setText(input_line->text().left(input_line->triggerLength())
                                            + input_line->completion());
                    return true; // Always consume in non edit mode
                }
                break;

            case Qt::Key_Up:
                if (!edit_mode_)
                {
                    if (ke->modifiers().testFlag(Qt::ShiftModifier)
                        || (keyboard_navigation_receiver != actions_list
                            && results_list->currentIndex().row() < 1 && !ke->isAutoRepeat()))
                        input_line->next();
                    else if (keyboard_navigation_receiver)
                        QApplication::sendEvent(keyboard_navigation_receiver, event);
                    return true; // Always consume in non edit mode
                }
                break;

            case Qt::Key_Down:
                if (!edit_mode_)
                {
                    if (ke->modifiers().testFlag(Qt::ShiftModifier))
                        input_line->previous();
                    else if (keyboard_navigation_receiver)
                        QApplication::sendEvent(keyboard_navigation_receiver, event);
                    return true; // Always consume in non edit mode
                }
                break;

            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
                if (!edit_mode_ && keyboard_navigation_receiver)
                    QApplication::sendEvent(keyboard_navigation_receiver, event);
                return true; // Always consume in non edit mode

            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (ke->modifiers() == mods_mod[mod_command]) {
                    postCustomEvent(ToggleActions);
                    return true;
                }
                else if (!edit_mode_)
                {
                    if (ke->modifiers().testFlag(Qt::ShiftModifier))
                    {
                        input_line->insertPlainText(u"\n"_s);
                        return true;
                    }
                    else if (keyboard_navigation_receiver
                             && keyboard_navigation_receiver->currentIndex().isValid()){
                        emit keyboard_navigation_receiver->activated(
                            keyboard_navigation_receiver->currentIndex());
                        return true;
                    }
                }
                break;

            case Qt::Key_O:
                if (!edit_mode_ && keyboard_navigation_receiver
                    && ke->modifiers().testFlag(Qt::ControlModifier)
                    && keyboard_navigation_receiver->currentIndex().isValid()){
                    emit keyboard_navigation_receiver->activated(
                        keyboard_navigation_receiver->currentIndex());
                    return true;
                }
                break;

            }

            // State changes by modifiers

            if (ke->key() == mods_keys[mod_actions])
            {
                postCustomEvent(ShowActions);
                return true;
            }

            if (ke->key() == mods_keys[mod_fallback])
            {
                postCustomEvent(ShowFallbacks);
                return true;
            }
        }

        else if (event->type() == QEvent::KeyRelease)
        {
            auto *ke = static_cast<QKeyEvent *>(event);

            if (ke->key() == mods_keys[mod_actions])
            {
                postCustomEvent(HideActions);
                return true;
            }

            else if (ke->key() == mods_keys[mod_fallback])
            {
                postCustomEvent(HideFallbacks);
                return true;
            }
        }
    }

    else if (watched == input_frame)
    {
        if (event->type() == QEvent::Enter)
            postCustomEvent(InputFrameEnter);

        else if (event->type() == QEvent::Leave)
            postCustomEvent(InputFrameLeave);
    }

    else if (watched == settings_button)
    {
        if (event->type() == QEvent::Enter)
            postCustomEvent(SettingsButtonEnter);

        else if (event->type() == QEvent::Leave)
            postCustomEvent(SettingsButtonLeave);
    }

    return false;
}

//
//  PROPERTIES
//

const QString &Window::styleLight() const { return style_light_; }
void Window::setStyleLight(const QString &name)
{
    if (styleLight() == name)
        return;

    if (!styles.contains(name) && !name.isNull())
    {
        WARN << "Style does not exist:" << name;
        return;
    }

    if (!dark_mode)
        applyStyle(name);

    style_light_ = name;
    plugin.settings()->setValue(keys.style_light, name);
    emit styleLightChanged(name);
}

const QString &Window::styleDark() const { return style_dark_; }
void Window::setStyleDark(const QString &name)
{
    if (styleDark() == name)
        return;

    if (!styles.contains(name) && !name.isNull())
    {
        WARN << "Style does not exist:" << name;
        return;
    }

    if (dark_mode)
        applyStyle(name);

    style_dark_ = name;
    plugin.settings()->setValue(keys.style_dark, name);
    emit styleDarkChanged(name);
}

bool Window::alwaysOnTop() const { return windowFlags() & Qt::WindowStaysOnTopHint; }
void Window::setAlwaysOnTop(bool val)
{
    if (alwaysOnTop() == val)
        return;

    setWindowFlags(windowFlags().setFlag(Qt::WindowStaysOnTopHint, val));
    plugin.settings()->setValue(keys.always_on_top, val);
    emit alwaysOnTopChanged(val);
}

bool Window::clearOnHide() const { return input_line->clear_on_hide; }
void Window::setClearOnHide(bool val)
{
    if (clearOnHide() == val)
        return;

    input_line->clear_on_hide = val;
    plugin.settings()->setValue(keys.clear_on_hide, val);
    emit clearOnHideChanged(val);
}

bool Window::displayScrollbar() const
{ return results_list->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff; }
void Window::setDisplayScrollbar(bool val)
{
    if (displayScrollbar() == val)
        return;

    results_list->setVerticalScrollBarPolicy(val ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    plugin.settings()->setValue(keys.display_scrollbar, val);
    emit displayScrollbarChanged(val);
}

bool Window::followCursor() const { return followCursor_; }
void Window::setFollowCursor(bool val)
{
    if (followCursor() == val)
        return;

    followCursor_ = val;
    plugin.settings()->setValue(keys.follow_cursor, val);
    emit followCursorChanged(val);
}

bool Window::hideOnFocusLoss() const { return hideOnFocusLoss_; }
void Window::setHideOnFocusLoss(bool val)
{
    if (hideOnFocusLoss() == val)
        return;

    hideOnFocusLoss_ = val;
    plugin.settings()->setValue(keys.hide_on_focus_loss, val);
    emit hideOnFocusLossChanged(val);
}

bool Window::historySearchEnabled() const { return input_line->history_search; }
void Window::setHistorySearchEnabled(bool val)
{
    if (historySearchEnabled() == val)
        return;

    input_line->history_search = val;
    plugin.settings()->setValue(keys.history_search, val);
    emit historySearchEnabledChanged(val);
}

uint Window::maxResults() const { return results_list->maxItems(); }
void Window::setMaxResults(uint val)
{
    if (maxResults() == val)
        return;

    results_list->setMaxItems(val);
    plugin.settings()->setValue(keys.max_results, val);
    emit maxResultsChanged(val);
}

bool Window::showCentered() const { return showCentered_; }
void Window::setShowCentered(bool val)
{
    if (showCentered() == val)
        return;

    showCentered_ = val;
    plugin.settings()->setValue(keys.centered, val);
    emit showCenteredChanged(val);
}

bool Window::debugMode() const { return debug_overlay_.get(); }
void Window::setDebugMode(bool val)
{
    if (debugMode() == val)
        return;

    results_list->setDebugMode(val);
    actions_list->setDebugMode(val);

    if (val)
    {
        debug_overlay_ = make_unique<DebugOverlay>();
        debug_overlay_->recursiveInstallEventFilter(this);
    }
    else
        debug_overlay_.reset();

    plugin.settings()->setValue(keys.debug, val);
    update();
    emit debugModeChanged(val);
}

bool Window::editModeEnabled() const { return edit_mode_; }
void Window::setEditModeEnabled(bool v)
{
    if (edit_mode_ != v)
    {
        edit_mode_ = v;
        emit editModeEnabledChanged(v);
    }
}

bool Window::disableInputMethod() const { return input_line->disable_input_method_; }
void Window::setDisableInputMethod(bool val)
{
    if (disableInputMethod() != val)
    {
        input_line->disable_input_method_ = val;
        plugin.settings()->setValue(keys.disable_input_method, val);
    }
}
