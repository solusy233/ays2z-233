#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "../../Auto_poweroff233/Auto_poweroff.h"
#include "../../booth/booth.h"
#include "../../weather/weatherapi.h"
#include "../../weather/weatherwindow.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>

namespace {
QString settingsJsonPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.json");
}

QString readPasswordFromJson()
{
    QFile file(settingsJsonPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringLiteral("123456");

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return QStringLiteral("123456");

    const QString password = document.object().value(QStringLiteral("password")).toString();
    return password.isEmpty() ? QStringLiteral("123456") : password;
}

void writePasswordToJson(const QString &password)
{
    QFile file(settingsJsonPath());
    QJsonObject object;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument currentDocument = QJsonDocument::fromJson(file.readAll());
        if (currentDocument.isObject())
            object = currentDocument.object();
        file.close();
    }

    object.insert(QStringLiteral("password"), password);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

}

SettingsWindow::SettingsWindow(AutoPoweroff *autoPoweroff, Booth *booth, WeatherApi *weatherApi,
                               QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWindow)
    , weatherWindow(nullptr)
{
    ui->setupUi(this);
    setFixedSize(size());
    QString fontFamily = QStringLiteral("HarmonyOS Sans");
    const int fontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/HarmonyOS_Sans_Black.ttf"));
    const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    if (!fontFamilies.isEmpty())
        fontFamily = fontFamilies.constFirst();
    setStyleSheet(QStringLiteral(
        "QWidget#SettingsWindow { background: transparent; }"
        "QLabel { color: #f2d28c; background: transparent; font-family: '%1'; font-size: 12pt; }"
        "QTimeEdit { color: #f2d28c; background: rgba(3, 34, 72, 220); border: 1px solid #e7bd65;"
        " border-radius: 2px; padding: 1px 4px; font-family: '%1'; font-size: 19pt; }"
        "QCheckBox { color: #f2d28c; background: transparent; font-family: '%1'; font-size: 12pt; }"
        "QPushButton { color: #f2d28c; background: rgba(3, 34, 72, 205); border: 1px solid #e7bd65;"
        " border-radius: 3px; padding: 2px 6px; font-family: '%1'; font-size: 12pt; }"
        "QPushButton:hover { background: rgba(22, 66, 116, 230); }"
        "QPushButton:pressed { background: rgba(2, 24, 54, 240); }"
        "QStatusBar { color: #f2d28c; background: transparent; font-family: '%1'; font-size: 9pt; }"
        "QStatusBar::item { border: none; }").arg(fontFamily));
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    if (!QFileInfo::exists(settingsJsonPath()))
        writePasswordToJson(QStringLiteral("123456"));

    bool ok = false;
    const QString enteredPassword = QInputDialog::getText(this,
        QStringLiteral("密码验证"),
        QStringLiteral("请输入设置密码："),
        QLineEdit::Password,
        QString(),
        &ok);
    if (!ok || enteredPassword != readPasswordFromJson()) {
        QMessageBox::critical(this,
            QStringLiteral("密码错误"),
            QStringLiteral("密码错误，无法打开设置界面。"));
        close();
        return;
    }

    ui->startTimeEdit->setTime(autoPoweroff->configuredStartTime());
    connect(ui->startTimeEdit, &QTimeEdit::timeChanged, autoPoweroff, &AutoPoweroff::setStartTime);
    connect(autoPoweroff, &AutoPoweroff::statusChanged, this, [this](const QString &status) {
        ui->statusbar->showMessage(status);
    });
    connect(ui->cancelShutdownButton, &QPushButton::clicked, autoPoweroff, &AutoPoweroff::cancelShutdown);
    connect(ui->openWeatherButton, &QPushButton::clicked, this, [this, weatherApi]() {
        weatherApi->fetchAnyangWeather();
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
    connect(ui->selectExeButton, &QPushButton::clicked, this, [this, booth]() {
        booth->selectAndStartExe(this);
    });
    connect(booth, &Booth::statusMessage, this, [this](const QString &message) {
        ui->statusbar->showMessage(message);
    });
    connect(ui->exitButton, &QPushButton::clicked, []() {
        QCoreApplication::quit();
    });
    ui->autoStartCheckBox->setChecked(autoPoweroff->isAutoStartEnabled());
    connect(ui->autoStartCheckBox, &QCheckBox::toggled, autoPoweroff, &AutoPoweroff::setAutoStartEnabled);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const QPixmap background(QStringLiteral(":/images/settings-background.png"));
    if (!background.isNull())
        painter.drawPixmap(rect(), background);
}
