#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QPixmap>

class AutoPoweroff;
class Booth;
class QPaintEvent;
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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::SettingsWindow *ui;
    WeatherWindow *weatherWindow;
    QPixmap backgroundPixmap;
};

#endif // SETTINGSWINDOW_H