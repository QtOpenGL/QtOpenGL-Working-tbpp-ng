#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

class MyOpenGLWidget : public QOpenGLWidget, public QOpenGLExtraFunctions
{
    Q_OBJECT

   public:
    bool paused;
    float xPos, yPos, zPos, xSpd, ySpd, zSpd;
    QPoint lastMousePoint;
    bool mouseInLeftEdge, mouseInRightEdge, mouseInTopEdge, mouseInBottomEdge;

    int frameCount, lastFpsTime;
    float fps;

    // 星球、发光效果、合成的着色器
    QOpenGLShaderProgram shader, shaderBlur, shaderPost;
    // 星球、屏幕的顶点数组
    QOpenGLVertexArrayObject vertexArray, scrVertexArray;
    GLuint vertexBuffer,
        // 绘制发光效果用的缓冲区
        frameBuffer, colorBuffers[2], depthBuffer,
        // 通过迭代获得较好的发光效果，需交替使用两个缓冲区
        pingpongFrameBuffers[2], pingpongColorBuffers[2],
        // 屏幕的顶点
        scrVertexBuffer;

    // 投影矩阵
    QMatrix4x4 projMat;

    MyOpenGLWidget(QWidget *parent = 0);
    ~MyOpenGLWidget();

    void initializeGL();
    void paintGL();
    void resizeGL();
    void drawCircle(float x, float y, float r, float colorR, float colorG,
                    float colorB);
    void keyPressEvent(QKeyEvent *event);
    //    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

   public slots:
    void animate();

   signals:
    msg(QString s);
};

#endif
