#include "mainwindow.h"
#include "serial.cpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setFixedSize(width(), height());
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
    ui->myOpenGLWidget->animate();
    if (space.clock < MAX_CLOCK)
    {
        if (backend->paused)
        {
            ui->statusBar->showMessage(
                QString("模拟已暂停 第 %1 轮，%2 轮/秒，%3 帧/秒")
                    .arg(QString::number(space.clock))
                    .arg(QString::number(backend->fps, 'f', 1))
                    .arg(QString::number(ui->myOpenGLWidget->fps, 'f', 1)));
        }
        else
        {
            ui->statusBar->showMessage(
                QString("正在模拟... 第 %1 轮，%2 轮/秒，%3 帧/秒")
                    .arg(QString::number(space.clock))
                    .arg(QString::number(backend->fps, 'f', 1))
                    .arg(QString::number(ui->myOpenGLWidget->fps, 'f', 1)));
        }
    }
    else
    {
        ui->statusBar->showMessage(
            QString("模拟完成，%1 帧/秒")
                .arg(QString::number(ui->myOpenGLWidget->fps, 'f', 1)));
    }
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

void MainWindow::on_actionPause_toggled(bool b)
{
    backend->paused = b;
}

void MainWindow::on_actionOpen_triggered()
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    fileName = QFileDialog::getOpenFileName(
        this, tr("打开"), "", tr("数据存档 (*.sav);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
    {
        // 读取用于标记存档位置的文件
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("错误"), tr("打开文件失败"));
        }
        else
        {
            file.close();
            receiveMsg("正在读取星球数据..");
            deserializeArrayFromFile(
                (fileName + ".planets").toLocal8Bit().data(), planets,
                MAX_PLANET, sizeof(Planet));
            receiveMsg("正在读取空间数据..");
            deserializeFromFile((fileName + ".space").toLocal8Bit().data(),
                                space);
            receiveMsg("正在读取舰队数据..");
            deserializeListFromFile((fileName + ".fleets").toLocal8Bit().data(),
                                    fleets);
            receiveMsg("正在读取外交数据..");
            deserializeArrayFromFile(
                (fileName + ".friendship").toLocal8Bit().data(),
                Civil::friendship, MAX_PLANET * MAX_PLANET, sizeof(double));
            receiveMsg("读取数据完成");
        }
    }
    backend->unlock();
    ui->myOpenGLWidget->paused = false;
}

void MainWindow::on_actionSave_triggered(bool saveAs)
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    if (fileName.isEmpty() || saveAs)
        fileName = QFileDialog::getSaveFileName(
            this, tr("保存"), "", tr("数据存档 (*.sav);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
    {
        // 写入用于标记存档位置的文件
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("错误"), tr("保存文件失败"));
        }
        else
        {
            file.close();
            receiveMsg("正在写入星球数据..");
            serializeArrayToFile((fileName + ".planets").toLocal8Bit().data(),
                                 planets, MAX_PLANET, sizeof(Planet));
            receiveMsg("正在写入空间数据..");
            serializeToFile((fileName + ".space").toLocal8Bit().data(), space);
            receiveMsg("正在写入舰队数据..");
            serializeListToFile((fileName + ".fleets").toLocal8Bit().data(),
                                fleets);
            receiveMsg("正在写入外交数据..");
            serializeArrayToFile(
                (fileName + ".friendship").toLocal8Bit().data(),
                Civil::friendship, MAX_PLANET * MAX_PLANET, sizeof(double));
            receiveMsg("写入数据完成");
        }
    }
    backend->unlock();
    ui->myOpenGLWidget->paused = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    on_actionSave_triggered(true);
}

void MainWindow::on_actionReset_triggered()
{
    ui->myOpenGLWidget->xPos = 0.0;
    ui->myOpenGLWidget->yPos = 0.0;
    ui->myOpenGLWidget->zPos = 0.0;
    ui->myOpenGLWidget->xSpd = 0.0;
    ui->myOpenGLWidget->ySpd = 0.0;
    ui->myOpenGLWidget->zSpd = 0.0;
}

void MainWindow::on_actionStat_triggered()
{
    statWindow.show();
    statWindow.paintStat();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(
        this, tr("关于"),
        tr("三体++  宇宙社会学模拟软件\n制作：Team 630\n          吴典 何若谷 梅杰 段明阳"));
}
