#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../../weather/weatherwindow.h"
#include <QCoreApplication>

SettingsWindow::SettingsWindow(AutoPoweroff *autoPoweroff, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWindow)
    , weatherWindow(nullptr)
{
    ui->setupUi(this);
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->startTimeEdit->setTime(autoPoweroff->configuredStartTime());
    connect(ui->startTimeEdit, &QTimeEdit::timeChanged, autoPoweroff, &AutoPoweroff::setStartTime);
    connect(autoPoweroff, &AutoPoweroff::statusChanged, this, [this](const QString &status) {
        ui->statusbar->showMessage(status);
    });
    connect(ui->cancelShutdownButton, &QPushButton::clicked, autoPoweroff, &AutoPoweroff::cancelShutdown);
    connect(ui->backgroundButton, &QPushButton::clicked, this, &SettingsWindow::backgroundChangeRequested);
    connect(ui->openWeatherButton, &QPushButton::clicked, this, [this]() {
        if (!weatherWindow) {
            weatherWindow = new WeatherWindow();
            connect(weatherWindow, &QObject::destroyed, this, [this]() {
                weatherWindow = nullptr;
            });
        }
        weatherWindow->show();
        weatherWindow->raise();
        weatherWindow->activateWindow();
    });
    connect(ui->exitButton, &QPushButton::clicked, []() {
        QCoreApplication::quit();
    });
    ui->autoStartCheckBox->setChecked(autoPoweroff->isAutoStartEnabled());
    connect(ui->autoStartCheckBox, &QCheckBox::toggled, autoPoweroff, &AutoPoweroff::setAutoStartEnabled);
    if (!autoPoweroff->isAutoStartEnabled())
        autoPoweroff->setAutoStartEnabled(true);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}