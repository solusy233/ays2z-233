#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>

class AutoPoweroff;
class Booth;
class WeatherApi;
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
    explicit SettingsWindow(AutoPoweroff *autoPoweroff, Booth *booth, WeatherApi *weatherApi,
                            QWidget *parent = nullptr);
    ~SettingsWindow();

private:
    void paintEvent(QPaintEvent *event) override;

    Ui::SettingsWindow *ui;
    WeatherWindow *weatherWindow;
};

#endif // SETTINGSWINDOW_H