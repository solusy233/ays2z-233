#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../settings/settingswindow.h"
#include "../../weather/weatherapi.h"
#include "../../weather/weatherwindow.h"
#include <QCloseEvent>
#include <QDebug>
#include <QMenu>
#include <QPainter>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , autoPoweroff(new AutoPoweroff(this))
    , weatherApi(new WeatherApi(this))
    , weatherWindow(nullptr)
    , settingsWindow(nullptr)
    , m_trayIcon(new QSystemTrayIcon(QIcon(QStringLiteral(":/images/tray-icon.jpg")), this))
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

    auto *trayMenu = new QMenu(this);
    auto *showAction = new QAction(QStringLiteral("显示窗口"), this);
    auto *quitAction = new QAction(QStringLiteral("退出"), this);
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible()) {
                hide();
            } else {
                showNormal();
                raise();
                activateWindow();
            }
        }
    });
    m_trayIcon->show();
    hide();

    connect(ui->settingsButton, &QPushButton::clicked, this, [this]() {
        if (!settingsWindow) {
            settingsWindow = new SettingsWindow(autoPoweroff);
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
    weatherApi->startDailyFetchSchedule(QTime(21, 0));
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    const QPixmap bgPixmap(QStringLiteral(":/images/background-dark.png"));
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
    if (m_trayIcon && m_trayIcon->isSystemTrayAvailable()) {
        hide();
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);
}
