#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../../booth/booth.h"
#include "../settings/settingswindow.h"
#include "../../weather/weatherapi.h"
#include "../../weather/weatherwindow.h"
#include "../../Easter-egg/Easter-egg.h"
#include <QCloseEvent>
#include <QDebug>
#include <QFont>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QAction>
#include <QPushButton>
#include <QScreen>

namespace {
// 主界面按 1774x887 的设计稿等比排布（背景图尺寸一致）
constexpr int kDesignWidth = 1774;
constexpr int kDesignHeight = 887;
// 缩放时给窗口标题栏、边框预留的余量
constexpr qreal kScreenFitRatio = 0.95;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , autoPoweroff(new AutoPoweroff(this))
    , booth(new Booth(this))
    , weatherApi(new WeatherApi(this))
    , weatherWindow(nullptr)
    , settingsWindow(nullptr)
    , easterEggWindow(nullptr)
    , m_trayIcon(new QSystemTrayIcon(QIcon(QStringLiteral(":/images/tray-icon.jpg")), this))
{
    ui->setupUi(this);

    // 高 DPI 缩放后屏幕的逻辑可用区域可能小于设计稿尺寸，直接使用设计稿尺寸会
    // 导致窗口超出屏幕、界面显示不完整。这里按可用区域等比缩小整体界面。
    QScreen *targetScreen = screen();
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }
    qreal uiScale = 1.0;
    if (targetScreen) {
        const QRect available = targetScreen->availableGeometry();
        const qreal fitWidth = available.width() * kScreenFitRatio / kDesignWidth;
        const qreal fitHeight = available.height() * kScreenFitRatio / kDesignHeight;
        uiScale = qMin<qreal>(1.0, qMin(fitWidth, fitHeight));
    }

    resize(qRound(kDesignWidth * uiScale), qRound(kDesignHeight * uiScale));
    setFixedSize(size());

    const int settingsButtonWidth = qRound(ui->settingsButton->width() * uiScale);
    const int settingsButtonHeight = qRound(ui->settingsButton->height() * uiScale);
    ui->settingsButton->setFixedSize(settingsButtonWidth, settingsButtonHeight);
    ui->settingsButton->setParent(this);
    ui->settingsButton->setGeometry(width() - qRound(200 * uiScale) - settingsButtonWidth,
                                    qRound(70 * uiScale),
                                    settingsButtonWidth, settingsButtonHeight);
    QFont settingsButtonFont = ui->settingsButton->font();
    if (settingsButtonFont.pointSizeF() > 0.0) {
        settingsButtonFont.setPointSizeF(settingsButtonFont.pointSizeF() * uiScale);
        ui->settingsButton->setFont(settingsButtonFont);
    }
    ui->settingsButton->setEnabled(true);
    ui->settingsButton->setCursor(Qt::PointingHandCursor);
    ui->settingsButton->raise();
    ui->centralwidget->setAttribute(Qt::WA_TranslucentBackground);
    ui->centralwidget->setAutoFillBackground(false);
    ui->centralwidget->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *easterEggButton = new QPushButton(this);
    easterEggButton->setObjectName(QStringLiteral("easterEggButton"));
    easterEggButton->setGeometry(qRound(width() * 0.325), qRound(height() * 0.345),
                                 qRound(width() * 0.114), qRound(height() * 0.232));
    easterEggButton->setStyleSheet(QStringLiteral(
        "QPushButton#easterEggButton { background: transparent; border: none; }"));
    easterEggButton->setCursor(Qt::PointingHandCursor);
    easterEggButton->setFocusPolicy(Qt::NoFocus);
    easterEggButton->raise();
    connect(easterEggButton, &QPushButton::clicked, this, [this]() {
        if (!easterEggWindow) {
            easterEggWindow = new EasterEggWindow(this);
            connect(easterEggWindow, &QObject::destroyed, this, [this]() {
                easterEggWindow = nullptr;
            });
        }
        easterEggWindow->showFullScreen();
        easterEggWindow->raise();
        easterEggWindow->activateWindow();
    });

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
            settingsWindow = new SettingsWindow(autoPoweroff, booth, weatherApi);
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
