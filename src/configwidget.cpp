// Copyright (c) 2022-2025 Manuel Schneider

#include "configwidget.h"
#include "ui_configwidget.h"
#include "window.h"
#include <QDialog>
#include <QGroupBox>
#include <albert/widgetsutil.h>
using namespace albert;
using namespace std;

template<typename T>
static QSpinBox *createSpinBox(QFormLayout *form_layout,
                               QString label,
                               T *t,
                               uint (T::*get)() const,
                               void (T::*set)(uint))
{
    auto *spin_box = new QSpinBox;
    spin_box->setValue((t->*get)());
    QObject::connect(spin_box, QOverload<int>::of(&QSpinBox::valueChanged), t, set);
    spin_box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    form_layout->addRow(label, spin_box);
    return spin_box;
}

template<typename T>
static auto addFontSpinBox(QFormLayout *form_layout,
                           QString label,
                           T *t,
                           uint (T::*get)() const,
                           void (T::*set)(uint))
{
    auto *spin_box = createSpinBox(form_layout, label, t, get, set);
    spin_box->setMinimum(6);
    spin_box->setSuffix(QStringLiteral(" pt"));
    return spin_box;
}

template<typename T>
static auto addPixelMetricSpinBox(QFormLayout *form_layout,
                                  QString label,
                                  T *t,
                                  uint (T::*get)() const,
                                  void (T::*set)(uint))
{
    auto *spin_box = createSpinBox(form_layout, label, t, get, set);
    spin_box->setSuffix(QStringLiteral(" px"));
    return spin_box;
}

template<typename T>
static auto addPixelMetricSpinBox(QFormLayout *form_layout,
                                  QString label,
                                  T *t,
                                  double (T::*get)() const,
                                  void (T::*set)(double))
{

    auto *spin_box = new QDoubleSpinBox;
    spin_box->setValue((t->*get)());
    spin_box->setSingleStep(0.5);
    spin_box->setDecimals(1);
    spin_box->setSuffix(QStringLiteral(" px"));
    spin_box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QObject::connect(spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), t, set);
    form_layout->addRow(label, spin_box);
    return spin_box;
}


ConfigWidget::ConfigWidget(Window &_window):
    window(_window)
{
    Ui::ConfigWidget ui;
    ui.setupUi(this);

    ui.comboBox_style_light->addItem(tr("System"), QString());
    ui.comboBox_style_light->insertSeparator(1);
    for (const auto&[name, _] : window.styles)
    {
        ui.comboBox_style_light->addItem(name, name);
        if (name == window.styleLight())
            ui.comboBox_style_light->setCurrentIndex(ui.comboBox_style_light->count()-1);
    }
    connect(ui.comboBox_style_light,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this, cb=ui.comboBox_style_light](int i)
            { window.setStyleLight(cb->itemData(i).toString()); });

    connect(&window, &Window::styleLightChanged, this, [cb=ui.comboBox_style_light](QString name){
        if (auto i = cb->findData(name); i != -1)
            cb->setCurrentIndex(i);
    });


    ui.comboBox_style_dark->addItem(tr("System"), QString());
    ui.comboBox_style_dark->insertSeparator(1);
    for (const auto&[name, _] : window.styles)
    {
        ui.comboBox_style_dark->addItem(name, name);
        if (name == window.styleDark())
            ui.comboBox_style_dark->setCurrentIndex(ui.comboBox_style_dark->count()-1);
    }
    connect(ui.comboBox_style_dark,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this, cb=ui.comboBox_style_dark](int i)
            { window.setStyleDark(cb->itemData(i).toString()); });
    connect(&window, &Window::styleDarkChanged, this, [cb=ui.comboBox_style_dark](QString name){
        if (auto i = cb->findData(name); i != -1)
            cb->setCurrentIndex(i);
    });

    bindWidget(ui.checkBox_onTop,
               &window,
               &Window::alwaysOnTop,
               &Window::setAlwaysOnTop,
               &Window::alwaysOnTopChanged);

    bindWidget(ui.checkBox_clearOnHide,
               &window,
               &Window::clearOnHide,
               &Window::setClearOnHide,
               &Window::clearOnHideChanged);

    bindWidget(ui.checkBox_scrollbar,
               &window,
               &Window::displayScrollbar,
               &Window::setDisplayScrollbar,
               &Window::displayScrollbarChanged);

    bindWidget(ui.checkBox_followCursor,
               &window,
               &Window::followCursor,
               &Window::setFollowCursor,
               &Window::followCursorChanged);

    bindWidget(ui.checkBox_hideOnFocusOut,
               &window,
               &Window::hideOnFocusLoss,
               &Window::setHideOnFocusLoss,
               &Window::hideOnFocusLossChanged);

    bindWidget(ui.checkBox_history_search,
               &window,
               &Window::historySearchEnabled,
               &Window::setHistorySearchEnabled,
               &Window::historySearchEnabledChanged);

    ui.spinBox_results->setValue((int)window.maxResults());
    connect(ui.spinBox_results, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            &window, &Window::setMaxResults);
    connect(&window, &Window::maxResultsChanged,
            ui.spinBox_results, &QSpinBox::setValue);

    bindWidget(ui.checkBox_input_method,
               &window,
               &Window::disableInputMethod,
               &Window::setDisableInputMethod);

    bindWidget(ui.checkBox_center,
               &window,
               &Window::showCentered,
               &Window::setShowCentered,
               &Window::showCenteredChanged);

    bindWidget(ui.checkBox_debug,
               &window,
               &Window::debugMode,
               &Window::setDebugMode,
               &Window::debugModeChanged);
}
