#include "Easter-egg.h"

#include <QKeyEvent>
#include <QPainter>

EasterEggWindow::EasterEggWindow(QWidget *parent)
	: QWidget(parent)
	, image(QStringLiteral(":/images/easter-egg.png"))
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_DeleteOnClose, false);
}

void EasterEggWindow::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);
	if (!image.isNull()) {
		const QPixmap scaledImage = image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		painter.drawPixmap((width() - scaledImage.width()) / 2,
						   (height() - scaledImage.height()) / 2, scaledImage);
	}
}

void EasterEggWindow::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		hide();
		return;
	}
	QWidget::keyPressEvent(event);
}
