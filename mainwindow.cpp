#include "mainwindow.h"
#include <QDebug>
#include "backend.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->myOpenGLWidget, &MyOpenGLWidget::msg, this,
            &MainWindow::receiveMsg);

    gTime.start();

    backend = new Backend();
    backend->moveToThread(&backendThread);
    connect(this, &MainWindow::backendInit, backend, &Backend::init);
    connect(this, &MainWindow::backendWork, backend, &Backend::work);
    connect(backend, &Backend::msg, this, &MainWindow::receiveMsg);
    backendThread.start();
    emit backendInit();

    timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &MainWindow::animate);
    connect(timer, &QTimer::timeout, ui->myOpenGLWidget,
            &MyOpenGLWidget::animate);
    timer->start(33);
}

MainWindow::~MainWindow()
{
    delete ui;
    backendThread.terminate();
    backendThread.wait();
}

void MainWindow::animate()
{
    ui->statusBar->showMessage(
        QString("%1 frame/s, %2 clock/s")
            .arg(QString::number(ui->myOpenGLWidget->fps, 'f', 3))
            .arg(QString::number(backend->fps, 'f', 3)));
}

void MainWindow::receiveMsg(QString s)
{
    ui->statusBar->showMessage(s);
    if (s == "后端初始化完成")
    {
        emit backendWork();
        ui->myOpenGLWidget->paused = false;
    }
}

void MainWindow::on_actionPause_triggered()
{
    backend->paused = !backend->paused;
}

// 星图复位
void MainWindow::on_actionReset_triggered()
{
    ui->myOpenGLWidget->xPos = 0.0;
    ui->myOpenGLWidget->yPos = 0.0;
    ui->myOpenGLWidget->zPos = 0.0;
    ui->myOpenGLWidget->xSpd = 0.0;
    ui->myOpenGLWidget->ySpd = 0.0;
    ui->myOpenGLWidget->zSpd = 0.0;
}
