#ifndef WEATHERWINDOW_H
#define WEATHERWINDOW_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void loadTemperatureSummary();
    void applyWeatherBackground(int conditionCode);
    void updatePanelLayout();
    void updateWeatherIcon(int conditionCode);

    Ui::WeatherWindow *ui;
    QPixmap backgroundPixmap;
    QFrame *glassPanel = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *weatherIconLabel = nullptr;
    QPushButton *closeButton = nullptr;
};

#endif // WEATHERWINDOW_H
