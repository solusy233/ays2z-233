#ifndef EASTER_EGG_H
#define EASTER_EGG_H

#include <QPixmap>
#include <QWidget>

class QKeyEvent;
class QPaintEvent;

class EasterEggWindow : public QWidget
{
    Q_OBJECT

public:
    explicit EasterEggWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap image;
};

#endif // EASTER_EGG_H