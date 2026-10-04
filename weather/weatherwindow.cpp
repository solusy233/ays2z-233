#include "weatherwindow.h"
#include "weather_judge.h"
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
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QTextCursor>
#include <QDate>
#include <QWidget>
#include <QVBoxLayout>
#include <QSvgRenderer>


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

void applyCustomWeatherFont(QLabel *titleLabel, QPushButton *closeButton,
                            QPlainTextEdit *weatherText, int weatherTextPointSize)
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
        textFormat.setFontPointSize(weatherTextPointSize);
        cursor.setCharFormat(textFormat);

        QString blackPath = QStringLiteral("Fout/HarmonyOS_Sans_Black.ttf");
        QString blackFamily;
        if (loadFontFamilyFromFile(blackPath, &blackFamily)) {
            QTextCharFormat numberFormat;
            numberFormat.setFontFamilies(QStringList{blackFamily});
            numberFormat.setFontWeight(QFont::Black);
            numberFormat.setFontPointSize(weatherTextPointSize);
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

QPixmap renderSvgToPixmap(const QString &svgPath, const QSize &size)
{
    if (svgPath.isEmpty() || size.isEmpty() || !QFileInfo::exists(svgPath))
        return QPixmap();

    QFile svgFile(svgPath);
    if (!svgFile.open(QIODevice::ReadOnly))
        return QPixmap();

    QByteArray svgData = svgFile.readAll();
    svgData.replace("currentColor", "#1e374b");

    QSvgRenderer renderer(svgData);
    if (!renderer.isValid())
        return QPixmap();

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter, QRect(QPoint(0, 0), size));
    painter.end();

    return QPixmap::fromImage(image);
}

void WeatherWindow::updateWeatherIcon(int conditionCode)
{
    QString iconDirectory = QStringLiteral("weather/icons");
    const QString settingsPath = QDir(QCoreApplication::applicationDirPath())
                                     .filePath(QStringLiteral("settings.json"));
    QFile settingsFile(settingsPath);
    if (settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument settingsDocument = QJsonDocument::fromJson(settingsFile.readAll());
        if (settingsDocument.isObject()) {
            const QJsonObject settings = settingsDocument.object();
            if (!settings.value(QStringLiteral("showWeatherSvg")).toBool(true)) {
                if (weatherIconLabel)
                    weatherIconLabel->hide();
                return;
            }

            const QString configuredDirectory = settings.value(QStringLiteral("weatherSvgDirectory")).toString();
            if (!configuredDirectory.isEmpty())
                iconDirectory = configuredDirectory;
        }
    }

    if (!weatherIconLabel) {
        weatherIconLabel = new QLabel(this);
        weatherIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        weatherIconLabel->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        weatherIconLabel->setScaledContents(true);
    }

    const QString iconName = QString::number(conditionCode);
    const QString weatherDir = QDir::isAbsolutePath(iconDirectory)
                                   ? iconDirectory
                                   : QDir(QCoreApplication::applicationDirPath()).filePath(iconDirectory);
    const QString primaryIconPath = QDir(weatherDir).filePath(QStringLiteral("%1-fill.svg").arg(iconName));
    const QString fallbackIconPath = QDir(weatherDir).filePath(QStringLiteral("qweather-fill.svg"));
    const QString iconPath = QFileInfo::exists(primaryIconPath) ? primaryIconPath : fallbackIconPath;

    if (!QFileInfo::exists(iconPath)) {
        weatherIconLabel->hide();
        return;
    }

    QPixmap iconPixmap = renderSvgToPixmap(iconPath, QSize(72, 72));
    if (iconPixmap.isNull()) {
        weatherIconLabel->hide();
        return;
    }

    weatherIconLabel->setPixmap(iconPixmap);
    if (ui && ui->weatherText) {
        const QRect textRect = ui->weatherText->geometry();
        weatherIconLabel->setGeometry(textRect.right() - 100,
                                     textRect.top() + 16,
                                     72,
                                     72);
    }
    weatherIconLabel->raise();
    weatherIconLabel->show();
}

void WeatherWindow::applyWeatherBackground(int conditionCode)
{
    backgroundPixmap = WeatherJudge::randomBackgroundForWeatherCode(
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
    if (backgroundPixmap.isNull() || size().isEmpty())
        return;

    QPixmap scaledBackground = backgroundPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                        Qt::SmoothTransformation);
    const QRect sourceRect((scaledBackground.width() - width()) / 2,
                           (scaledBackground.height() - height()) / 2,
                           width(), height());
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(rect(), scaledBackground, sourceRect);
}

/**
 * @brief 更新天气面板的布局
 * 该函数负责创建和配置天气面板的UI元素，包括面板样式、阴影效果、标题栏、关闭按钮和天气文本区域
 */
void WeatherWindow::updatePanelLayout()
{
    // 如果玻璃面板不存在，则创建一个新的面板
    if (!glassPanel) {
        // 创建玻璃面板框架
        glassPanel = new QFrame(this);
        // 设置面板样式表，包括背景颜色、边框和圆角
        glassPanel->setStyleSheet(QStringLiteral(
            "QFrame {"
            " background-color: rgba(255, 255, 255, 92);"  // 半透明白色背景
            " border: 1px solid rgba(255, 255, 255, 185);"  // 白色边框
            " border-radius: 28px;"  // 圆角半径
            "}"));
        // 创建并设置面板阴影效果
        QGraphicsDropShadowEffect *panelShadow = new QGraphicsDropShadowEffect(glassPanel);
        panelShadow->setBlurRadius(32);  // 阴影模糊半径
        panelShadow->setOffset(0, 12);   // 阴影偏移量
        panelShadow->setColor(QColor(25, 55, 80, 90));  // 阴影颜色
        glassPanel->setGraphicsEffect(panelShadow);
        glassPanel->lower();  // 将面板置于底层
    }

    // 计算面板的尺寸和位置
    const QRect backgroundRect = rect();
    const int panelMargin = qBound(24, qMin(width(), height()) / 24, 48);
    const int panelWidth = qMin(backgroundRect.width() - panelMargin * 2,
                                qBound(360, backgroundRect.width() / 3, 620));
    const int panelHeight = qMin(backgroundRect.height() - panelMargin * 2,
                                 qBound(360, backgroundRect.height() - panelMargin * 2, 760));
    const QRect panelGeometry(backgroundRect.x() + backgroundRect.width() - panelWidth - panelMargin,
                              backgroundRect.y() + (backgroundRect.height() - panelHeight) / 2,
                              panelWidth,
                              panelHeight);
    glassPanel->setGeometry(panelGeometry);

    // 计算内容区域的起始X坐标和宽度
    const int panelPadding = qBound(20, panelWidth / 18, 32);
    const int contentX = panelGeometry.x() + panelPadding;
    const int contentWidth = panelGeometry.width() - panelPadding * 2;

    // 如果标题标签不存在，则创建标题
    if (!titleLabel) {
        const QString tomorrowDateStr = QDate::currentDate().addDays(1).toString(QStringLiteral("yyyy年MM月dd日天气预报"));
        titleLabel = new QLabel(tomorrowDateStr, this);
        // 设置标题样式，包括颜色、字体大小和粗细
        titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: rgba(30, 55, 75, 225); font-size: 26px; font-weight: 600; "
            "background: transparent; border: none; }"));
    }

    // 设置标题标签的位置和大小
    titleLabel->setGeometry(contentX, panelGeometry.y() + 26, contentWidth - 56, 42);

    if (!weatherTipLabel) {
        weatherTipLabel = new QLabel(this);
        weatherTipLabel->setAlignment(Qt::AlignCenter);
        weatherTipLabel->setWordWrap(true);
        weatherTipLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: rgba(30, 55, 75, 245); background: rgba(255, 255, 255, 150); "
            "border: 1px solid rgba(255, 255, 255, 210); border-radius: 16px; "
            "padding: 8px 14px; font-size: 36px; font-weight: 600; }"));
    }

    // 如果关闭按钮不存在，则创建关闭按钮
    if (!closeButton) {
        closeButton = new QPushButton(QStringLiteral("×"), this);
        closeButton->setCursor(Qt::PointingHandCursor);  // 设置鼠标指针为手型
        // 设置关闭按钮样式，包括颜色、背景、边框和悬停效果
        closeButton->setStyleSheet(QStringLiteral(
            "QPushButton { color: rgba(30, 55, 75, 210); background: rgba(255, 255, 255, 100); "
            "border: 1px solid rgba(255, 255, 255, 160); border-radius: 19px; "
            "font-size: 25px; font-weight: 400; padding-bottom: 3px; }"
            "QPushButton:hover { background: rgba(255, 255, 255, 180); }"  // 悬停时背景变亮
            "QPushButton:pressed { background: rgba(220, 235, 242, 180); }"));  // 点击时背景色变化
        // 连接关闭按钮的点击信号到窗口的关闭槽
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    }
    // 设置关闭按钮的位置和大小
    closeButton->setGeometry(panelGeometry.right() - 60, panelGeometry.y() + 22, 38, 38);
    // 应用自定义天气字体到标题、关闭按钮和天气文本
    applyCustomWeatherFont(titleLabel, closeButton, ui->weatherText,
                           qBound(22, ui->weatherText->width() / 16, 34));

    // 设置天气文本区域的位置和大小
    ui->weatherText->setGeometry(contentX, panelGeometry.y() + 86,
                                 contentWidth, panelGeometry.height() - 122);
    weatherTipLabel->setGeometry(panelGeometry.x(),
                                  qMax(12, panelGeometry.y() - 126),
                                  panelGeometry.width(),
                                  112);
    if (weatherIconLabel && ui && ui->weatherText) {
        const QRect textRect = ui->weatherText->geometry();
        weatherIconLabel->setGeometry(textRect.right() - 100,
                                     textRect.top() + 16,
                                     72,
                                     72);
    }
    // 设置天气文本区域的样式，包括颜色、背景、边框和字体
    ui->weatherText->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { color: rgba(25, 48, 65, 235); background: rgba(255, 255, 255, 72); "
        "border: 1px solid rgba(255, 255, 255, 150); border-radius: 18px; padding: 18px; "
        "font-size: %1px; font-weight: 600; "
        "selection-background-color: rgba(90, 145, 175, 150); }")
        .arg(qBound(22, ui->weatherText->width() / 16, 34)));
    // 设置滚动条策略为关闭
    ui->weatherText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->weatherText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

/**
 * @brief WeatherWindow类的构造函数
 * @param parent 指向父窗口的指针，默认为nullptr
 */
WeatherWindow::WeatherWindow(QWidget *parent)
    : QWidget(parent)           // 初始化基类QWidget
    , ui(new Ui::WeatherWindow)  // 初始化UI成员
{
    ui->setupUi(this);  // 设置UI界面
    // 设置窗口标志：无边框、置顶显示
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // 设置窗口关闭时自动删除
    setAttribute(Qt::WA_DeleteOnClose);

    applyWeatherBackground(100);  // 应用天气背景，参数100表示某种天气状态
    updatePanelLayout();         // 更新面板布局
    loadTemperatureSummary();    // 加载温度摘要信息
}

WeatherWindow::~WeatherWindow()
{
    delete ui;
}

/**
 * @brief 加载并显示天气摘要信息
 * 该函数从JSON文件中读取天气数据，解析并提取相关信息，然后更新UI显示
 */
void WeatherWindow::loadTemperatureSummary()
{
    // 构建天气数据文件的完整路径
    const QString filePath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("weather/weather.json"));
    QFile weatherFile(filePath);
    // 尝试打开文件，如果失败则显示错误信息
    if (!weatherFile.open(QIODevice::ReadOnly)) {
        const QString message = QStringLiteral("天气数据暂不可用\n%1")
                                    .arg(filePath, weatherFile.errorString());
        ui->weatherText->setPlainText(message);
        qWarning().noquote() << message;
        return;
    }

    // 读取文件全部内容
    const QByteArray data = weatherFile.readAll();
    QJsonParseError parseError;
    // 将JSON数据解析为QJsonDocument对象
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    // 检查JSON解析是否成功
    if (parseError.error != QJsonParseError::NoError) {
        const QString message = QStringLiteral("天气数据解析失败：%1").arg(parseError.errorString());
        ui->weatherText->setPlainText(message);
        qWarning().noquote() << message;
        return;
    }

    // 获取每日天气数据数组
    const QJsonArray daily = document.object().value(QStringLiteral("daily")).toArray();
    // 检查数据是否有效
    if (daily.isEmpty() || !daily.first().isObject()) {
        ui->weatherText->setPlainText(QStringLiteral("天气数据暂不可用"));
        return;
    }

    // 获取明天的天气数据对象
    const QJsonObject tomorrow = daily.first().toObject();

    // 初始化天气状况代码
    int conditionCode = 0;
    // 获取天气状况值
    const QJsonValue conditionValue = tomorrow.value(QStringLiteral("condition"));
    // 如果天气状况是对象类型
    if (conditionValue.isObject()) {
        // 获取天气状况代码字符串
        const QString codeString = conditionValue.toObject().value(QStringLiteral("code")).toString();
        // 如果代码字符串不为空，则转换为整数
        if (!codeString.isEmpty())
            conditionCode = codeString.toInt();
    }

    // 如果天气状况代码仍为0，尝试从iconDay字段获取
    if (conditionCode == 0) {
        const QString iconDay = tomorrow.value(QStringLiteral("iconDay")).toString();
        if (!iconDay.isEmpty())
            conditionCode = iconDay.toInt();
    }

    // 如果获取到有效的天气状况代码，应用对应的背景
    if (conditionCode > 0)
        applyWeatherBackground(conditionCode);

    // 获取各项天气数据
    const QString maximum = tomorrow.value(QStringLiteral("tempMax")).toString();
    const QString minimum = tomorrow.value(QStringLiteral("tempMin")).toString();
    const QString windSpeed = tomorrow.value(QStringLiteral("windSpeedDay")).toString();
    const QString windScale = tomorrow.value(QStringLiteral("windScaleDay")).toString();
    const QString windDirection = tomorrow.value(QStringLiteral("windDirDay")).toString();
    // 获取天气描述，优先使用白天描述，否则使用夜间描述
    const QString weatherCondition = tomorrow.value(QStringLiteral("textDay")).toString(
        tomorrow.value(QStringLiteral("textNight")).toString());
    // 构建天气摘要信息字符串
    QString summary = QStringLiteral(
        "天气情况  %1\n\n温度      %2 - %3 °C\n\n风向      %4\n\n风力      %5 级\n\n风速      %6 km/h")
                                .arg(weatherCondition, minimum, maximum,
                                     windDirection, windScale, windSpeed);
    const QString weatherTip = WeatherJudge::randomTipForWeatherCode(
        conditionCode, QCoreApplication::applicationDirPath());
    if (weatherTipLabel) {
        weatherTipLabel->setText(weatherTip);
        weatherTipLabel->setVisible(!weatherTip.isEmpty());
        weatherTipLabel->raise();
    }
    // 将摘要信息显示在UI上
    ui->weatherText->setPlainText(summary);
    // 应用自定义天气字体到相关控件
    applyCustomWeatherFont(titleLabel, closeButton, ui->weatherText,
                           qBound(22, ui->weatherText->width() / 16, 34));

    updateWeatherIcon(conditionCode);
}
