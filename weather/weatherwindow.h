#ifndef WEATHERWINDOW_H
#define WEATHERWINDOW_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class QAudioOutput;
class QJsonArray;
class QMediaPlayer;

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
    void loadWeatherTip(int weatherCode, const QJsonArray &temperatureHistory);
    void applyWeatherBackground(int conditionCode);
    void updatePanelLayout();

    Ui::WeatherWindow *ui;
    QPixmap backgroundPixmap;
    QFrame *glassPanel = nullptr;
    QFrame *tipGlassFrame = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *tipLabel = nullptr;
    QLabel *weatherIconLabel = nullptr;
    QPushButton *closeButton = nullptr;
    QMediaPlayer *tipPlayer = nullptr;
    QAudioOutput *tipAudioOutput = nullptr;
};

#endif // WEATHERWINDOW_H
