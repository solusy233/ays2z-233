#include "aboutwindows/mainwindows/mainwindow.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QDebug>

int main(int argc, char *argv[])
{
    // 允许启用高 DPI 自动缩放
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // 使用高 DPI 属性（例如图标选择正确的 DPR）
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // 设置非整数缩放策略（解决 125%、150% 缩放下的字体/边框错位）
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    QApplication a(argc, argv);
    
    a.setQuitOnLastWindowClosed(false);

    // 1. 加载资源文件中的字体
    int fontId = QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
    
    if (fontId == -1) {
        qWarning() << "字体加载失败，请检查 qrc 路径是否正确！";
    } else {
        // 2. 获取字体族名称
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            // 设置全局默认字体
            QFont font(fontFamilies.at(0));
            font.setPointSize(10); // 可选：设置默认字号
            a.setFont(font);
        }
    }

    MainWindow w;
    w.showNormal();
    w.raise();
    w.activateWindow();
    return a.exec();
}