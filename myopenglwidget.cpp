#include "myopenglwidget.h"
#include <math.h>
#include <QDebug>
#include <QKeyEvent>
#include <QThread>
#include <QVector4D>
#include "backend.h"
#include "civil.h"
#include "color.cpp"
#include "globaltime.h"

const float PLANET_Z = 2.0;  // 星球的z坐标

const int MAX_BLUR_ITER = 4;

const float MOVE_SPD = 0.01;
const float MOVE_DUMP = 0.8;

const float EDGE_X = 2.0;
const float EDGE_Y = 1.5;
const float EDGE_Z_NEAR = PLANET_Z - 0.1;
const float EDGE_Z_FAR = -3.0;
const float EDGE_ELAS = -0.5;

const float MOVE_SPD_MOUSE = 0.02;
const int EDGE_MOUSE = 200;

// 初始暂停绘制，后端初始化完毕后才能开始绘制
MyOpenGLWidget::MyOpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      paused(true),
      xPos(0.0),
      yPos(0.0),
      zPos(0.0),
      xSpd(0.0),
      ySpd(0.0),
      zSpd(0.0),
      frameCount(0),
      lastFpsTime(0),
      fps(0.0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

MyOpenGLWidget::~MyOpenGLWidget()
{
    // 释放绘图用的缓冲区
    vertexArray.destroy();
    scrVertexArray.destroy();
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteFramebuffers(1, &frameBuffer);
    glDeleteTextures(2, colorBuffers);
    glDeleteRenderbuffers(1, &depthBuffer);
    glDeleteFramebuffers(2, pingpongFrameBuffers);
    glDeleteTextures(2, pingpongColorBuffers);
    glDeleteBuffers(1, &scrVertexBuffer);
}

void MyOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    const float retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glEnable(GL_DEPTH_TEST);

    // 初始化着色器
    shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
#include "shaders/planet.vert"
                                   );
    shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
#include "shaders/planet.frag"
                                   );
    shader.link();

    shaderBlur.addShaderFromSourceCode(QOpenGLShader::Vertex,
#include "shaders/blur.vert"
                                       );
    shaderBlur.addShaderFromSourceCode(QOpenGLShader::Fragment,
#include "shaders/blur.frag"
                                       );
    shaderBlur.link();

    shaderPost.addShaderFromSourceCode(QOpenGLShader::Vertex,
#include "shaders/post.vert"
                                       );
    shaderPost.addShaderFromSourceCode(QOpenGLShader::Fragment,
#include "shaders/post.frag"
                                       );
    shaderPost.link();

    shaderPost.bind();
    shaderPost.setUniformValue("image", 0);
    shaderPost.setUniformValue("blur", 1);
    shaderPost.release();

    // 初始化缓冲区
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenTextures(2, colorBuffers);
    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB,
                     GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // 边缘设为剪切
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(),
                          height());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer);

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFrameBuffers);
    glGenTextures(2, pingpongColorBuffers);
    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFrameBuffers[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB,
                     GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
    }

    // 初始化星球的顶点数组，未写入数据
    vertexArray.create();
    glGenBuffers(1, &vertexBuffer);

    // 初始化屏幕的顶点数组
    const GLfloat quadVertices[] = {
        // 坐标              // 材质坐标
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 0
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 1
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 2
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // 3
    };

    scrVertexArray.create();
    scrVertexArray.bind();
    glGenBuffers(1, &scrVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, scrVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid *)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    scrVertexArray.release();

    // 初始化投影矩阵
    projMat.perspective(60.0, float(width()) / float(height()), 0.1, 100.0);
}

void MyOpenGLWidget::paintGL()
{
    if (paused) return;

    // 在frameBuffer中绘制星球
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.bind();
    vertexArray.bind();

    while (backend->lock)
    {
        emit msg("等待后端响应...");
        QThread::msleep(1);
    }
    backend->lock = true;

    for (int i = 0; i < MAX_PLANET; ++i)
    {
        // 将星球坐标转换为绘图坐标
        float scrx = (planets[i].x - 50.0) * 0.05;
        float scry = (planets[i].y - 50.0) * 0.05;
        float r = planets[i].radius;
        // 根据星球发展、攻击、合作概率之比确定色相
        // 发展概率大则偏绿色
        // TODO：更合适的颜色
        Civil &p = civils[planets[i].civilId];
        float colorH = abs(p.rateDev) /
                       (abs(p.rateDev) + abs(p.rateAtk) + abs(p.rateCoop));
        // 根据星球科技确定饱和度与亮度
        float colorS = exp(-0.01 * p.tech);
        RgbColor tColor = hsvToRgb(
            HsvColor(round(127.0 * colorH), round(255.0 * pow(colorS, 0.1)),
                     63 + round(192.0 * (1.0 - colorS))));
        float colorR = float(tColor.r) / 255.0;
        float colorG = float(tColor.g) / 255.0;
        float colorB = float(tColor.b) / 255.0;

        drawCircle(scrx, scry, r, colorR, colorG, colorB);
    }

    backend->lock = false;

    vertexArray.release();
    shader.release();

    // 在pingpongFrameBuffers中绘制发光效果
    shaderBlur.bind();
    bool horizontal = true;
    for (int i = 0; i < MAX_BLUR_ITER; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFrameBuffers[horizontal]);
        shaderBlur.setUniformValue("horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        if (i == 0)
            glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
        else
            glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontal]);
        // 绘制屏幕
        scrVertexArray.bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        scrVertexArray.release();
        horizontal = !horizontal;
    }
    shaderBlur.release();

    // 将colorBuffers[0]中的暗区与pingpongColorBuffers中的亮区合成
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderPost.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontal]);
    // 绘制屏幕
    scrVertexArray.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    scrVertexArray.release();
    shaderPost.release();
}

// 留空
void MyOpenGLWidget::resizeGL()
{
}

void MyOpenGLWidget::drawCircle(float x, float y, float r, float colorR,
                                float colorG, float colorB)
{
    // 设置绘制位置
    // 半径大的星球略微往下移动，防止挡住半径小的星球
    QMatrix4x4 matrix = projMat;
    matrix.translate(xPos + x, yPos + y, zPos - PLANET_Z - r);
    // 用testVec测试星球是否在视线外，在视线外则不绘制
    QVector4D testVec(0.0, 0.0, 0.0, 1.0);
    testVec = matrix * testVec;
    if (abs(testVec.x()) > testVec.w()) return;
    if (abs(testVec.y()) > testVec.w()) return;
    shader.setUniformValue("matrix", matrix);

    // 根据半径和缩放调整画圆的线段数
    int segCount = floor(1000.0 * r / abs(PLANET_Z - zPos));
    if (segCount < 6) segCount = 6;
    if (segCount > 50) segCount = 50;

    // 画圆用到的临时变量
    float th = 2.0 * M_PI / float(segCount);
    float c = cosf(th);
    float s = sinf(th);
    float px = r;
    float py = 0;

    // 构造星球的顶点数组
    GLfloat vertices[300];
    for (int i = 0; i < segCount; ++i)
    {
        vertices[i * 6] = px;
        vertices[i * 6 + 1] = py;
        vertices[i * 6 + 2] = 0.0;
        vertices[i * 6 + 3] = colorR;
        vertices[i * 6 + 4] = colorG;
        vertices[i * 6 + 5] = colorB;

        float t = px;
        px = c * px - s * py;
        py = s * t + c * py;
    }

    // 写入星球的顶点数组
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, segCount * 6 * sizeof(GLfloat), &vertices,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          (GLvoid *)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_POLYGON, 0, segCount);
}

// z坐标离星球越近，移动速度越慢
void MyOpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    // qDebug() << "key" << event->key() << event->text();
    switch (event->key())
    {
        case Qt::Key_L:
            xSpd += MOVE_SPD * (PLANET_Z - zPos);
            break;
        case Qt::Key_J:
            xSpd -= MOVE_SPD * (PLANET_Z - zPos);
            break;
        case Qt::Key_I:
            ySpd += MOVE_SPD * (PLANET_Z - zPos);
            break;
        case Qt::Key_K:
            ySpd -= MOVE_SPD * (PLANET_Z - zPos);
            break;
        case Qt::Key_U:
            zSpd += MOVE_SPD * (PLANET_Z - zPos);
            break;
        case Qt::Key_O:
            zSpd -= MOVE_SPD * (PLANET_Z - zPos);
            break;
    }
}

void MyOpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{
    // qDebug() << "key" << event->key() << event->text();
    //    switch (event->key())
    //    {
    //        case Qt::Key_L:
    //            xSpd = 0;
    //            break;
    //        case Qt::Key_J:
    //            xSpd = 0;
    //            break;
    //        case Qt::Key_I:
    //            ySpd = 0;
    //            break;
    //        case Qt::Key_K:
    //            ySpd = 0;
    //            break;
    //        case Qt::Key_U:
    //            zSpd = 0;
    //            break;
    //        case Qt::Key_O:
    //            zSpd = 0;
    //            break;
    //    }
}

QPoint lastMousePoint;
void MyOpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::RightButton)
    {
        QPoint delta = event->pos() - lastMousePoint;
        if (delta.manhattanLength() < 10)
        {
            zSpd -= 0.002 * delta.y() * (PLANET_Z - zPos);
            // TODO：右键左右移动用来干什么
        }
        lastMousePoint = event->pos();
    }
    else
    {
        if (event->pos().x() > width() - EDGE_MOUSE)
            xSpd = -MOVE_SPD_MOUSE * (PLANET_Z - zPos);
        else if (event->pos().x() < EDGE_MOUSE)
            xSpd = MOVE_SPD_MOUSE * (PLANET_Z - zPos);
        if (event->pos().y() > height() - EDGE_MOUSE)
            ySpd = MOVE_SPD_MOUSE * (PLANET_Z - zPos);
        else if (event->pos().y() < EDGE_MOUSE)
            ySpd = -MOVE_SPD_MOUSE * (PLANET_Z - zPos);
    }
}

void MyOpenGLWidget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0)
    {
        zPos += 0.1;
    }
    if (event->delta() < 0)
    {
        zPos -= 0.1;
    }
}

void MyOpenGLWidget::animate()
{
    xPos += xSpd;
    yPos += ySpd;
    zPos += zSpd;
    xSpd *= MOVE_DUMP;
    ySpd *= MOVE_DUMP;
    zSpd *= MOVE_DUMP;
    if (xPos > EDGE_X)
    {
        xPos = EDGE_X;
        xSpd *= EDGE_ELAS;
    }
    if (xPos < -EDGE_X)
    {
        xPos = -EDGE_X;
        xSpd *= EDGE_ELAS;
    }
    if (yPos > EDGE_Y)
    {
        yPos = EDGE_Y;
        ySpd *= EDGE_ELAS;
    }
    if (yPos < -EDGE_Y)
    {
        yPos = -EDGE_Y;
        ySpd *= EDGE_ELAS;
    }
    if (zPos > EDGE_Z_NEAR)
    {
        zPos = EDGE_Z_NEAR;
        zSpd *= EDGE_ELAS;
    }
    if (zPos < EDGE_Z_FAR)
    {
        zPos = EDGE_Z_FAR;
        zSpd *= EDGE_ELAS;
    }

    update();

    ++frameCount;
    int nowFpsTime = gTime.elapsed();
    if (nowFpsTime - lastFpsTime > 100)
    {
        fps = float(frameCount) / float(nowFpsTime - lastFpsTime) * 1000.0;
        frameCount = 0;
        lastFpsTime = nowFpsTime;
    }
}