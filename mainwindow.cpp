// 主窗口

#include "mainwindow.h"
#include "serial.cpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      fileName("autosave/auto.sav"),
      lastAutoSaveTime(0)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setFixedSize(width(), height());
    connect(ui->myOpenGLWidget, &MyOpenGLWidget::msg, this,
            &MainWindow::showMsg);

    gTime.start();

    backend = new Backend();
    backend->moveToThread(&backendThread);
    connect(this, &MainWindow::backendInit, backend, &Backend::init);
    connect(this, &MainWindow::backendWork, backend, &Backend::work);
    connect(backend, &Backend::msg, this, &MainWindow::showMsg);
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

void MainWindow::loadData()
{
    showMsg("正在读取星球数据..");
    deserializeArray((fileName + ".planets").toLocal8Bit().data(), planets,
                     MAX_PLANET, sizeof(Planet));
    showMsg("正在读取空间数据..");
    deserialize((fileName + ".space").toLocal8Bit().data(), space);
    showMsg("正在读取文明数据..");
    deserializeList((fileName + ".civils").toLocal8Bit().data(), civils);
    showMsg("正在读取舰队数据..");
    deserializeList((fileName + ".fleets").toLocal8Bit().data(), fleets);
    showMsg("正在读取外交数据..");
    deserializeArray((fileName + ".friendship").toLocal8Bit().data(),
                     Civil::friendship, MAX_PLANET * MAX_PLANET,
                     sizeof(double));
    showMsg("正在读取策略数据..");
    deserializeMapArray((fileName + ".aimap").toLocal8Bit().data(),
                        Civil::aiMap, MAX_PLANET);
    showMsg("读取数据完成");
}

void MainWindow::saveData()
{
    showMsg("正在写入星球数据..");
    serializeArray((fileName + ".planets").toLocal8Bit().data(), planets,
                   MAX_PLANET, sizeof(Planet));
    showMsg("正在写入空间数据..");
    serialize((fileName + ".space").toLocal8Bit().data(), space);
    showMsg("正在写入文明数据..");
    serializeList((fileName + ".civils").toLocal8Bit().data(), civils);
    showMsg("正在写入舰队数据..");
    serializeList((fileName + ".fleets").toLocal8Bit().data(), fleets);
    showMsg("正在写入外交数据..");
    serializeArray((fileName + ".friendship").toLocal8Bit().data(),
                   Civil::friendship, MAX_PLANET * MAX_PLANET, sizeof(double));
    showMsg("正在写入策略数据..");
    serializeMapArray((fileName + ".aimap").toLocal8Bit().data(), Civil::aiMap,
                      MAX_PLANET);
    showMsg("写入数据完成");
}

void MainWindow::animate()
{
    if (ui->myOpenGLWidget->paused) return;

    ui->myOpenGLWidget->animate();

    QString status;
    if (space.clock < MAX_CLOCK)
    {
        if (backend->paused)
            status = "模拟已暂停";
        else
            status = "正在模拟...";
    }
    else
        status = "模拟完成";

    ui->statusBar->showMessage(
        QString("%1    第 %2 轮，%3 个文明，%4 支舰队    %5 轮/秒，%6 帧/秒")
            .arg(status)
            .arg(QString::number(space.clock))
            .arg(QString::number(civils.size()))
            .arg(QString::number(fleets.size()))
            .arg(QString::number(backend->fps, 'f', 1))
            .arg(QString::number(ui->myOpenGLWidget->fps, 'f', 1)));

    // 自动保存
    if (!backend->paused)
    {
        int nowAutoSaveTime = gTime.elapsed();
        if (nowAutoSaveTime - lastAutoSaveTime > AUTO_SAVE_INTERVAL)
        {
            ui->myOpenGLWidget->paused = true;
            backend->lock();
            // 写入用于标记存档位置的文件
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly))
            {
                file.close();
                saveData();
            }
            backend->unlock();
            ui->myOpenGLWidget->paused = false;
            lastAutoSaveTime = nowAutoSaveTime;
        }
    }
}

void MainWindow::showMsg(QString s)
{
    ui->statusBar->showMessage(s);
    if (s == "后端初始化完成")
    {
        emit backendWork();
        ui->myOpenGLWidget->paused = false;
    }
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
            loadData();
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
            saveData();
        }
    }
    backend->unlock();
    ui->myOpenGLWidget->paused = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    on_actionSave_triggered(true);
}

void MainWindow::on_actionPause_toggled(bool b)
{
    backend->paused = b;
}

void MainWindow::on_actionSlow_toggled(bool b)
{
    backend->slow = b;
}

void MainWindow::on_actionShowFriendship_toggled(bool b)
{
    ui->myOpenGLWidget->showFriendship = b;
}

void MainWindow::on_actionShowParent_toggled(bool b)
{
    ui->myOpenGLWidget->showParentCivil = b;
}

void MainWindow::on_actionShowFleet_toggled(bool b)
{
    ui->myOpenGLWidget->showFleet = b;
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
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(
        this, tr("关于"),
        tr("三体++  宇宙社会学模拟软件\n制作：Team 630\n          吴典 何若谷 梅杰 段明阳"));
}
