#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>

class AutoPoweroff;
class WeatherWindow;

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsWindow;
}
QT_END_NAMESPACE

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(AutoPoweroff *autoPoweroff, QWidget *parent = nullptr);
    ~SettingsWindow();

private:
    Ui::SettingsWindow *ui;
    WeatherWindow *weatherWindow;
};

#endif // SETTINGSWINDOW_H