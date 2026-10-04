#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../../booth/booth.h"
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
    , booth(new Booth(this))
    , weatherApi(new WeatherApi(this))
    , weatherWindow(nullptr)
    , settingsWindow(nullptr)
    , m_trayIcon(new QSystemTrayIcon(QIcon(QStringLiteral(":/images/tray-icon.png")), this))
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
        showMainWindow();
    });
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->setToolTip(QStringLiteral("安阳二中定时关机"));

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon->show();
    }
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
    // 抓取时刻取 settings.json 的 startTime 前 3 分钟（默认参数即自动读取）
    weatherApi->startDailyFetchSchedule();
}

void MainWindow::showMainWindow()
{
    showNormal();
    raise();
    activateWindow();
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
    // 关窗口不退出程序：到点流程（提示音/音乐/关机）必须继续跑。
    // 这里统一最小化到任务栏——任务栏按钮在任何机器上都能把窗口叫回来；
    // 托盘图标若可用，双击图标同样能显示/隐藏窗口。要真正退出请用设置界面里的“退出”。
    showMinimized();
    event->ignore();

    if (m_trayIcon && m_trayIcon->isSystemTrayAvailable()) {
        m_trayIcon->showMessage(QStringLiteral("窗口已最小化到任务栏"),
                                QStringLiteral("双击系统托盘图标也能显示/隐藏窗口；退出请打开设置界面点“退出”。"),
                                QSystemTrayIcon::Information,
                                5000);
    }
}
