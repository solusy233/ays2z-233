#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class QCloseEvent;
class AutoPoweroff;
class WeatherApi;
class WeatherWindow;
class SettingsWindow;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    AutoPoweroff *autoPoweroff;
    WeatherApi *weatherApi;
    WeatherWindow *weatherWindow;
    SettingsWindow *settingsWindow;
    QSystemTrayIcon *m_trayIcon;

protected:
    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};
#endif // MAINWINDOW_H
