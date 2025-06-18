#include "imagewidget.h"
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

ImageWidget::ImageWidget(bool _isAfter, QWidget *parent) : QWidget(parent), isAfter(_isAfter)
{
}

void ImageWidget::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);

	if (data == nullptr) return;

	int width = data->GetFrameWidth();
	int height = data->GetFrameHeight();
	int depth = data->GetFrameDepth();
	cv::Mat const& recolored_img = data->GetImage(isAfter);

	QPainter painter(this);
	QImage im(width, height, QImage::Format_RGB888);

	for (int i = 0; i < height; i++) {
		uchar* const row = im.scanLine(i);
		for (int j = 0; j < width; j++) {
			cv::Vec3b pixel(recolored_img.at<cv::Vec3b>(i, j));
			row[j * 3] = pixel[0];
			row[j * 3 + 1] = pixel[1];
			row[j * 3 + 2] = pixel[2];
		}
	}
	painter.setPen(QPen(Qt::red, 3));
	painter.drawImage(QRectF(0, 0, width, height), im);
	painter.end();
}

void ImageWidget::mousePressEvent(QMouseEvent* event) {
	if (data == nullptr) return;
	//if (!(event->button() == Qt::LeftButton && (QApplication::keyboardModifiers() & Qt::ControlModifier) == Qt::ControlModifier)) return;
	if (event->button() == Qt::LeftButton)
		data->AddEditColor(event->pos());
	// emit update();
}

void ImageWidget::setTime(int t)
{
	time = t;

	update();
}

void ImageWidget::update()
{
	QWidget::update();

	int w = data->GetFrameWidth();
	int h = data->GetFrameHeight();

	setMaximumSize(w, h);
	setMinimumSize(w, h);
}

void ImageWidget::setData(Data *value)
{
	data = value;
	connect(value, &Data::updated, this, &ImageWidget::update);
}
