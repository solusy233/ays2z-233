#include "weatherwindow.h"
#include "weather_background.h"
#include "ui_weatherwindow.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QTextCursor>

namespace {
bool loadFontFamilyFromFile(const QString &fontPath, QString *familyName)
{
    if (!QFileInfo::exists(fontPath))
        return false;

    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1)
        return false;

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty())
        return false;

    if (familyName)
        *familyName = families.at(0);
    return true;
}

void applyCustomWeatherFont(QLabel *titleLabel, QPushButton *closeButton, QPlainTextEdit *weatherText)
{
    QString boldFamily;
    const QString boldPath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("Fout/HarmonyOS_Sans_Black.ttf"));
    const bool hasBoldFont = loadFontFamilyFromFile(boldPath, &boldFamily);
    if (!hasBoldFont)
        return;

    QFont boldFont(boldFamily);
    boldFont.setWeight(QFont::Black);
    boldFont.setBold(true);
    boldFont.setStyleStrategy(QFont::PreferQuality);
    if (titleLabel)
        titleLabel->setFont(boldFont);
    if (closeButton)
        closeButton->setFont(boldFont);
    if (weatherText) {
        weatherText->setFont(boldFont);
        QString text = weatherText->toPlainText();
        if (text.isEmpty())
            return;

        QTextCursor cursor(weatherText->document());
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(QTextCharFormat());

        QTextCharFormat textFormat;
        textFormat.setFontFamilies(QStringList{boldFamily});
        textFormat.setFontWeight(QFont::Black);
        textFormat.setFontPointSize(34);
        cursor.setCharFormat(textFormat);

        QString blackPath = QStringLiteral("D:/xiangmu/untitled/Fout/HarmonyOS_Sans_Black.ttf");
        QString blackFamily;
        if (loadFontFamilyFromFile(blackPath, &blackFamily)) {
            QTextCharFormat numberFormat;
            numberFormat.setFontFamilies(QStringList{blackFamily});
            numberFormat.setFontWeight(QFont::Black);
            numberFormat.setFontPointSize(34);
            const QRegularExpression numberPattern(QStringLiteral(R"(\d+|°C|km/h|级)"));
            QRegularExpressionMatchIterator it = numberPattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                cursor.setPosition(match.capturedStart());
                cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
                cursor.setCharFormat(numberFormat);
            }
        }
    }
}
}

void WeatherWindow::applyWeatherBackground(int conditionCode)
{
    const QPixmap backgroundPixmap = WeatherBackground::randomBackgroundForWeatherCode(
        conditionCode, QCoreApplication::applicationDirPath());
    if (backgroundPixmap.isNull())
        return;

    resize(backgroundPixmap.size());
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect availableGeometry = screen->availableGeometry();
        move(availableGeometry.center() - rect().center());
    }

    QPalette windowPalette = palette();
    windowPalette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
    setAutoFillBackground(true);
    setPalette(windowPalette);

    updatePanelLayout();
}

void WeatherWindow::updatePanelLayout()
{
    if (!glassPanel) {
        glassPanel = new QFrame(this);
        glassPanel->setStyleSheet(QStringLiteral(
            "QFrame {"
            " background-color: rgba(255, 255, 255, 92);"
            " border: 1px solid rgba(255, 255, 255, 185);"
            " border-radius: 28px;"
            "}"));
        QGraphicsDropShadowEffect *panelShadow = new QGraphicsDropShadowEffect(glassPanel);
        panelShadow->setBlurRadius(32);
        panelShadow->setOffset(0, 12);
        panelShadow->setColor(QColor(25, 55, 80, 90));
        glassPanel->setGraphicsEffect(panelShadow);
        glassPanel->lower();
    }

    const int panelWidth = qMax(420, qMin(width() / 3, 620));
    const int panelHeight = qMax(360, qMin(height() - 100, 760));
    const QRect panelGeometry(width() - panelWidth - 80,
                              (height() - panelHeight) / 2,
                              panelWidth,
                              panelHeight);
    glassPanel->setGeometry(panelGeometry);

    const int contentX = panelGeometry.x() + 28;
    const int contentWidth = panelGeometry.width() - 56;

    if (!titleLabel) {
        titleLabel = new QLabel(QStringLiteral("天气预报"), this);
        titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: rgba(30, 55, 75, 225); font-size: 26px; font-weight: 600; "
            "background: transparent; border: none; }"));
    }
    titleLabel->setGeometry(contentX, panelGeometry.y() + 26, contentWidth - 56, 42);

    if (!closeButton) {
        closeButton = new QPushButton(QStringLiteral("×"), this);
        closeButton->setCursor(Qt::PointingHandCursor);
        closeButton->setStyleSheet(QStringLiteral(
            "QPushButton { color: rgba(30, 55, 75, 210); background: rgba(255, 255, 255, 100); "
            "border: 1px solid rgba(255, 255, 255, 160); border-radius: 19px; "
            "font-size: 25px; font-weight: 400; padding-bottom: 3px; }"
            "QPushButton:hover { background: rgba(255, 255, 255, 180); }"
            "QPushButton:pressed { background: rgba(220, 235, 242, 180); }"));
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    }
    closeButton->setGeometry(panelGeometry.right() - 60, panelGeometry.y() + 22, 38, 38);
    applyCustomWeatherFont(titleLabel, closeButton, ui->weatherText);

    ui->weatherText->setGeometry(contentX, panelGeometry.y() + 86,
                                 contentWidth, panelGeometry.height() - 122);
    ui->weatherText->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { color: rgba(25, 48, 65, 235); background: rgba(255, 255, 255, 72); "
        "border: 1px solid rgba(255, 255, 255, 150); border-radius: 18px; padding: 18px; "
        "font-size: 32px; font-weight: 600; "
        "selection-background-color: rgba(90, 145, 175, 150); }"));
    ui->weatherText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->weatherText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

WeatherWindow::WeatherWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WeatherWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    applyWeatherBackground(100);
    updatePanelLayout();
    loadTemperatureSummary();
}

WeatherWindow::~WeatherWindow()
{
    delete ui;
}

void WeatherWindow::loadTemperatureSummary()
{
    const QString filePath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("weather/weather.json"));
    QFile weatherFile(filePath);
    if (!weatherFile.open(QIODevice::ReadOnly)) {
        const QString message = QStringLiteral("天气数据暂不可用\n%1")
                                    .arg(filePath, weatherFile.errorString());
        ui->weatherText->setPlainText(message);
        qWarning().noquote() << message;
        return;
    }

    const QByteArray data = weatherFile.readAll();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        const QString message = QStringLiteral("天气数据解析失败：%1").arg(parseError.errorString());
        ui->weatherText->setPlainText(message);
        qWarning().noquote() << message;
        return;
    }

    const QJsonArray daily = document.object().value(QStringLiteral("daily")).toArray();
    if (daily.isEmpty() || !daily.first().isObject()) {
        ui->weatherText->setPlainText(QStringLiteral("天气数据暂不可用"));
        return;
    }

    const QJsonObject tomorrow = daily.first().toObject();

    int conditionCode = 0;
    const QJsonValue conditionValue = tomorrow.value(QStringLiteral("condition"));
    if (conditionValue.isObject()) {
        const QString codeString = conditionValue.toObject().value(QStringLiteral("code")).toString();
        if (!codeString.isEmpty())
            conditionCode = codeString.toInt();
    }

    if (conditionCode == 0) {
        const QString iconDay = tomorrow.value(QStringLiteral("iconDay")).toString();
        if (!iconDay.isEmpty())
            conditionCode = iconDay.toInt();
    }

    if (conditionCode > 0)
        applyWeatherBackground(conditionCode);

    const QString maximum = tomorrow.value(QStringLiteral("tempMax")).toString();
    const QString minimum = tomorrow.value(QStringLiteral("tempMin")).toString();
    const QString windSpeed = tomorrow.value(QStringLiteral("windSpeedDay")).toString();
    const QString windScale = tomorrow.value(QStringLiteral("windScaleDay")).toString();
    const QString windDirection = tomorrow.value(QStringLiteral("windDirDay")).toString();
    const QString weatherCondition = tomorrow.value(QStringLiteral("textDay")).toString(
        tomorrow.value(QStringLiteral("textNight")).toString());
    const QString summary = QStringLiteral(
        "天气情况  %1\n\n温度      %2 - %3 °C\n\n风向      %4\n\n风力      %5 级\n\n风速      %6 km/h")
                                .arg(weatherCondition, minimum, maximum,
                                     windDirection, windScale, windSpeed);
    ui->weatherText->setPlainText(summary);
    applyCustomWeatherFont(titleLabel, closeButton, ui->weatherText);
}
