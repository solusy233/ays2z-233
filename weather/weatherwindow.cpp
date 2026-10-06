#include "weatherwindow.h"
#include "weather_background.h"
#include "ui_weatherwindow.h"

#include <QCoreApplication>
#include <QDate>
#include <QAudioOutput>
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
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScreen>
#include <QMediaPlayer>
#include <QSvgRenderer>
#include <QTextCursor>
#include <QUrl>

namespace {
QRect fittedBackgroundRect(const QPixmap &pixmap, const QSize &size)
{
    if (pixmap.isNull() || size.isEmpty())
        return QRect();

    QSize scaledSize = pixmap.size();
    scaledSize.scale(size, Qt::KeepAspectRatioByExpanding);
    return QRect((size.width() - scaledSize.width()) / 2,
                 (size.height() - scaledSize.height()) / 2,
                 scaledSize.width(),
                 scaledSize.height());
}

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

        QString blackPath = QStringLiteral("Fout/HarmonyOS_Sans_Black.ttf");
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
    backgroundPixmap = WeatherBackground::randomBackgroundForWeatherCode(
        conditionCode, QCoreApplication::applicationDirPath());
    if (backgroundPixmap.isNull())
        return;

    setAttribute(Qt::WA_TranslucentBackground);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        setGeometry(screen->geometry());
    }

    update();
    updatePanelLayout();
}

void WeatherWindow::paintEvent(QPaintEvent *)
{
    const QRect targetRect = fittedBackgroundRect(backgroundPixmap, size());
    if (targetRect.isEmpty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(targetRect, backgroundPixmap);
}

void WeatherWindow::updatePanelLayout()
{
    if (!tipGlassFrame) {
        tipGlassFrame = new QFrame(this);
        tipGlassFrame->setStyleSheet(QStringLiteral(
            "QFrame { background-color: rgba(255, 255, 255, 72); "
            "border: 1px solid rgba(255, 255, 255, 155); border-radius: 20px; }"));
        QGraphicsDropShadowEffect *tipShadow = new QGraphicsDropShadowEffect(tipGlassFrame);
        tipShadow->setBlurRadius(24);
        tipShadow->setOffset(0, 6);
        tipShadow->setColor(QColor(25, 55, 80, 75));
        tipGlassFrame->setGraphicsEffect(tipShadow);

        tipLabel = new QLabel(tipGlassFrame);
        tipLabel->setAlignment(Qt::AlignCenter);
        tipLabel->setWordWrap(true);
        tipLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: rgba(25, 48, 65, 235); background: transparent; "
            "border: none; font-size: 32px; font-weight: 600; }"));
    }

    const int tipWidth = qMin(width() - 80, qMax(480, width() * 65 / 100));
    tipGlassFrame->setGeometry((width() - tipWidth) / 2, 20, tipWidth, 70);
    tipLabel->setGeometry(tipGlassFrame->rect().adjusted(18, 4, -18, -4));

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

    const QRect backgroundRect = rect();
    const int panelWidth = qMin(backgroundRect.width() - 48,
                                qMax(420, qMin(backgroundRect.width() / 3, 620)));
    const int panelHeight = qMin(backgroundRect.height() - 48,
                                 qMax(360, qMin(backgroundRect.height() - 100, 760)));
    const QRect panelGeometry(backgroundRect.x() + backgroundRect.width() - panelWidth - 40,
                              backgroundRect.y() + (backgroundRect.height() - panelHeight) / 2,
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

    if (weatherIconLabel) {
        const int iconSize = qMin(96, ui->weatherText->height() / 4);
        weatherIconLabel->setGeometry(ui->weatherText->geometry().right() - iconSize - 20,
                                      ui->weatherText->geometry().y() + 12,
                                      iconSize, iconSize);
    }
}

WeatherWindow::WeatherWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WeatherWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    tipPlayer = new QMediaPlayer(this);
    tipAudioOutput = new QAudioOutput(this);
    tipPlayer->setAudioOutput(tipAudioOutput);
    tipAudioOutput->setVolume(1.0);

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
    const QJsonObject weatherDocument = document.object();
    const QDate forecastDate = QDate::fromString(
        tomorrow.value(QStringLiteral("fxDate")).toString(), Qt::ISODate);
    const QDate displayDate = forecastDate.isValid() ? forecastDate : QDate::currentDate().addDays(1);
    titleLabel->setText(QStringLiteral("%1 天气预报")
                            .arg(displayDate.toString(QStringLiteral("yyyy年M月d日"))));

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
    loadWeatherTip(conditionCode, weatherDocument.value(QStringLiteral("temperatureHistory")).toArray());

    const QString maximum = tomorrow.value(QStringLiteral("tempMax")).toString();
    const QString minimum = tomorrow.value(QStringLiteral("tempMin")).toString();
    const QString windSpeed = tomorrow.value(QStringLiteral("windSpeedDay")).toString();
    const QString windScale = tomorrow.value(QStringLiteral("windScaleDay")).toString();
    const QString windDirection = tomorrow.value(QStringLiteral("windDirDay")).toString();
    const QString weatherCondition = tomorrow.value(QStringLiteral("textDay")).toString(
        tomorrow.value(QStringLiteral("textNight")).toString());
    const QString iconCode = tomorrow.value(QStringLiteral("iconDay")).toString();
    if (!weatherIconLabel) {
        weatherIconLabel = new QLabel(this);
        weatherIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        weatherIconLabel->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        updatePanelLayout();
    }

    const QString iconPath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("weather/icons/%1.svg").arg(iconCode));
    QSvgRenderer iconRenderer(iconPath);
    if (iconRenderer.isValid()) {
        QPixmap iconPixmap(96, 96);
        iconPixmap.fill(Qt::transparent);
        QPainter iconPainter(&iconPixmap);
        iconRenderer.render(&iconPainter, QRectF(iconPixmap.rect()));
        iconPainter.end();

        QPainter tintPainter(&iconPixmap);
        tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tintPainter.fillRect(iconPixmap.rect(), QColor(25, 48, 65, 235));
        weatherIconLabel->setPixmap(iconPixmap);
        weatherIconLabel->show();
    } else {
        weatherIconLabel->hide();
    }

    const QString summary = QStringLiteral(
        "天气情况  %1\n\n温度      %2 - %3 °C\n\n风向      %4\n\n风力      %5 级\n\n风速      %6 km/h")
                                .arg(weatherCondition, minimum, maximum,
                                     windDirection, windScale, windSpeed);
    ui->weatherText->setPlainText(summary);
    applyCustomWeatherFont(titleLabel, closeButton, ui->weatherText);
}

void WeatherWindow::loadWeatherTip(int weatherCode, const QJsonArray &temperatureHistory)
{
    const QString tipsPath = QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral("weather/weathertips.json"));
    QFile tipsFile(tipsPath);
    if (!tipsFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开天气提示文件:" << tipsPath << tipsFile.errorString();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument tipsDocument = QJsonDocument::fromJson(tipsFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !tipsDocument.isObject()) {
        qWarning() << "天气提示文件解析失败:" << parseError.errorString();
        return;
    }

    const QDate today = QDate::currentDate();
    double todayAverage = 0.0;
    double yesterdayAverage = 0.0;
    bool hasTodayAverage = false;
    bool hasYesterdayAverage = false;
    for (const QJsonValue &entry : temperatureHistory) {
        if (!entry.isObject())
            continue;
        const QJsonObject historyEntry = entry.toObject();
        const QDate entryDate = QDate::fromString(
            historyEntry.value(QStringLiteral("date")).toString(), Qt::ISODate);
        const QJsonValue averageValue = historyEntry.value(QStringLiteral("average"));
        if (!averageValue.isDouble())
            continue;
        const double average = averageValue.toDouble();
        if (entryDate == today) {
            todayAverage = average;
            hasTodayAverage = true;
        } else if (entryDate == today.addDays(-1)) {
            yesterdayAverage = average;
            hasYesterdayAverage = true;
        }
    }

    QString category;
    if (hasTodayAverage && hasYesterdayAverage) {
        const double temperatureChange = todayAverage - yesterdayAverage;
        if (temperatureChange > 6.0)
            category = QStringLiteral("Heating_up");
        else if (temperatureChange < -6.0)
            category = QStringLiteral("Cooling_down");
    }

    if (category.isEmpty()) {
        if (weatherCode >= 100 && weatherCode < 200)
            category = QStringLiteral("normor_tips");
        else if (weatherCode >= 300 && weatherCode < 400)
            category = QStringLiteral("Rain");
        else if (weatherCode >= 400 && weatherCode < 500)
            category = QStringLiteral("Snow");
        else if ((weatherCode >= 500 && weatherCode <= 502)
                 || (weatherCode >= 509 && weatherCode <= 515))
            category = QStringLiteral("Fog");
        else
            category = QStringLiteral("Default");
    }

    const QJsonObject tipsObject = tipsDocument.object();
    QJsonArray candidates = category == QStringLiteral("normor_tips")
                                ? tipsObject.value(category).toArray()
                                : tipsObject.value(QStringLiteral("weather_tips")).toObject()
                                      .value(category).toArray();
    if (candidates.isEmpty() && category != QStringLiteral("Default")) {
        category = QStringLiteral("Default");
        candidates = tipsObject.value(QStringLiteral("weather_tips")).toObject()
                         .value(category).toArray();
    }
    if (candidates.isEmpty())
        return;

    const int selectedIndex = QRandomGenerator::global()->bounded(candidates.size());
    const QString tipText = candidates.at(selectedIndex).toString();
    if (tipText.isEmpty())
        return;
    tipLabel->setText(tipText);

    QString soundFolder = QStringLiteral("others");
    if (weatherCode >= 300 && weatherCode < 400)
        soundFolder = QStringLiteral("rainy");
    else if (weatherCode >= 400 && weatherCode < 500)
        soundFolder = QStringLiteral("snow");
    else if (weatherCode >= 500 && weatherCode < 600)
        soundFolder = QStringLiteral("fog");

    const QDir soundDirectory(QDir(QCoreApplication::applicationDirPath())
                                  .filePath(QStringLiteral("weather/tips_sound/%1").arg(soundFolder)));
    const QStringList soundFiles = soundDirectory.entryList(
        {QStringLiteral("*.wav"), QStringLiteral("*.mp3"), QStringLiteral("*.m4a")},
        QDir::Files, QDir::Name);
    if (!soundFiles.isEmpty()) {
        const QString soundPath = soundDirectory.filePath(
            soundFiles.at(QRandomGenerator::global()->bounded(soundFiles.size())));
        tipPlayer->setSource(QUrl::fromLocalFile(soundPath));
        tipPlayer->play();
    }
}
