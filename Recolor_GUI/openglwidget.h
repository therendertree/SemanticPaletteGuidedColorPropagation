#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include<QWindow>
#include<QOpenGLFunctions>
#include<QOpenGLPaintDevice>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLFramebufferObjectFormat>
#include "data.h"

class OpenGLWidget:public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit OpenGLWidget(QWidget *parent = 0);
    void setData(Data *value);

public slots:
    void update();
    void setTime(int t);
    void setShowImageData(bool _showImageData);
    void setPreview(int percentage);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;

protected:
    Data *data = nullptr;
    int time = 0;
    bool showImageData = true;
    double preview =.25;
};

#endif // OPENGLWIDGET_H
