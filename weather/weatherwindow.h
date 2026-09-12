#ifndef WEATHERWINDOW_H
#define WEATHERWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class WeatherWindow;
}
QT_END_NAMESPACE

class WeatherWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WeatherWindow(QWidget *parent = nullptr);
    ~WeatherWindow();

private:
    void loadTemperatureSummary();

    Ui::WeatherWindow *ui;
};

#endif // WEATHERWINDOW_H
