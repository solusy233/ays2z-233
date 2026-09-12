#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../settings/settingswindow.h"
#include "../../weather/weatherapi.h"
#include "../../weather/weatherwindow.h"
#include <QCloseEvent>
#include <QDebug>
#include <QPainter>

namespace {
const QStringList backgroundResources = {
    QStringLiteral(":/images/background-dark.png"),
    QStringLiteral(":/images/background-light.png")
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , autoPoweroff(new AutoPoweroff(this))
    , weatherApi(new WeatherApi(this))
    , weatherWindow(nullptr)
    , settingsWindow(nullptr)
    , backgroundIndex(0)
{
    ui->setupUi(this);
    setFixedSize(size());
    ui->settingsButton->setParent(this);
    ui->settingsButton->setGeometry(width() - 200 - ui->settingsButton->width(), 70,
                                    ui->settingsButton->width(), ui->settingsButton->height());
    ui->settingsButton->setEnabled(true);
    ui->settingsButton->setCursor(Qt::PointingHandCursor);
    ui->settingsButton->raise();
    ui->centralwidget->setAttribute(Qt::WA_TranslucentBackground);
    ui->centralwidget->setAutoFillBackground(false);
    ui->centralwidget->setStyleSheet(QStringLiteral("background: transparent;"));
    connect(ui->settingsButton, &QPushButton::clicked, this, [this]() {
        if (!settingsWindow) {
            settingsWindow = new SettingsWindow(autoPoweroff);
            connect(settingsWindow, &SettingsWindow::backgroundChangeRequested, this, [this]() {
                backgroundIndex = (backgroundIndex + 1) % backgroundResources.size();
                update();
            });
            connect(settingsWindow, &QObject::destroyed, this, [this]() {
                settingsWindow = nullptr;
            });
        }
        settingsWindow->move(frameGeometry().center() - settingsWindow->rect().center());
        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });
    connect(weatherApi, &WeatherApi::requestFailed, this, [](const QString &message) {
        qWarning() << "天气请求失败:" << message;
    });
    connect(autoPoweroff, &AutoPoweroff::playbackStarted, this, [this]() {
        if (!weatherWindow) {
            weatherWindow = new WeatherWindow(this);
            connect(weatherWindow, &QObject::destroyed, this, [this]() {
                weatherWindow = nullptr;
            });
        }
        weatherWindow->show();
        weatherWindow->raise();
        weatherWindow->activateWindow();
    });
    autoPoweroff->start();
    weatherApi->fetchAnyangWeather();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    QPixmap bgPixmap(backgroundResources.at(backgroundIndex));
    if (bgPixmap.isNull()) {
        return;
    }

    QPixmap scaledBg = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (width() - scaledBg.width()) / 2;
    int y = (height() - scaledBg.height()) / 2;
    painter.drawPixmap(x, y, scaledBg);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    showMinimized();
    event->ignore();
}
