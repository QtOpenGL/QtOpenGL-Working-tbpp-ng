// 主窗口

#include "mainwindow.h"
#include "serial.cpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      fileName("autosave/auto.sav"),
      lastAutoSaveTime(0)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setFixedSize(width(), height());

    // 创建自动存档用的文件夹
    QDir dir("autosave");
    if (!dir.exists()) dir.mkpath(".");

    showMsg("正在读取星球数据...");
    int inPlanetCount = 0;
    QFile file("in.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            double x, y, mass;
            in >> x >> y >> mass;
            planets[inPlanetCount] =
                Planet(inPlanetCount, inPlanetCount, Point(x, y), mass);
            ++inPlanetCount;
        }
        file.close();
    }
    showMsg("正在初始化剩余星球数据...");
    for (int i = inPlanetCount; i < MAX_PLANET; ++i)
        planets[i] =
            Planet(i, i, newRandom.getPoint() * 100.0, newRandom.get() * 10.0);
    showMsg("正在计算时空曲率...");
    space.calcCurv();
    showMsg("正在计算星球距离...");
    space.calcPlanetDis();
    showMsg("正在初始化文明数据...");
    for (int i = 0; i < MAX_PLANET; ++i)
    {
        Civil c(i, i);
        civils.push_back(c);
    }
    Civil::initFriendship();
    showMsg("初始化完成");

    gTime.start();

    backend = new Backend();
    backend->moveToThread(&backendThread);
    connect(this, &MainWindow::backendWork, backend, &Backend::work);
    backendThread.start();
    emit backendWork();

    timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &MainWindow::animate);
    timer->start(33);
    ui->myOpenGLWidget->paused = false;
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
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
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
}

void MainWindow::on_actionOpen_triggered()
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    fileName = QFileDialog::getOpenFileName(
        this, tr("打开"), "", tr("二进制格式 (*.sav);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
    {
        // 读取用于标记存档位置的文件
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
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

void MainWindow::on_actionSave_triggered()
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    if (fileName.isEmpty())
        fileName = QFileDialog::getSaveFileName(
            this, tr("保存"), "", tr("二进制格式 (*.sav);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
    {
        // 写入用于标记存档位置的文件
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
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
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    // 无论是否设置过fileName，都要重新设置
    fileName = QFileDialog::getSaveFileName(
        this, tr("另存为"), "", tr("二进制格式 (*.sav);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
    {
        // 写入用于标记存档位置的文件
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
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

void MainWindow::on_actionImportStg_triggered()
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    QString tmpFileName = QFileDialog::getOpenFileName(
        this, tr("导入策略参数"), "", tr("JSON格式 (*.json);;所有文件 (*.*)"));
    if (!tmpFileName.isEmpty())
    {
        QFile file(tmpFileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, tr("错误"), tr("打开文件失败"));
        }
        else
        {
            QString qs = "";
            QTextStream in(&file);
            while (!in.atEnd()) qs += in.readLine();
            file.close();
            string str = qs.toLocal8Bit().constData();
            Json::Reader reader;
            Json::Value js;
            reader.parse(str, js);

            for (int i = 0; i < MAX_PLANET; ++i)
            {
                Civil& c = civils[planets[i].civilId];
                c.rateDev = js[i]["rateDev"].asDouble();
                c.rateAtk = js[i]["rateAtk"].asDouble();
                c.rateCoop = js[i]["rateCoop"].asDouble();
                for (auto j : Civil::aiMap[i])
                    for (int k = 0; k < MAX_AI_PARAM + 1; ++k)
                        Civil::aiMap[i][j.first][k] =
                            js[i][to_string(j.first)][k].asDouble();
            }
        }
    }
    backend->unlock();
    ui->myOpenGLWidget->paused = false;
}

void MainWindow::on_actionExportStg_triggered()
{
    // 暂停模拟和星图显示
    ui->myOpenGLWidget->paused = true;
    backend->lock();
    QString tmpFileName = QFileDialog::getSaveFileName(
        this, tr("导出策略参数"), "", tr("JSON格式 (*.json);;所有文件 (*.*)"));
    if (!tmpFileName.isEmpty())
    {
        QFile file(tmpFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, tr("错误"), tr("保存文件失败"));
        }
        else
        {
            Json::Value js;
            for (int i = 0; i < MAX_PLANET; ++i)
            {
                Civil& c = civils[planets[i].civilId];
                js[i]["rateDev"] = c.rateDev;
                js[i]["rateAtk"] = c.rateAtk;
                js[i]["rateCoop"] = c.rateCoop;
                for (auto j : Civil::aiMap[i])
                    for (int k = 0; k < MAX_AI_PARAM + 1; ++k)
                        js[i][to_string(j.first)][k] =
                            Civil::aiMap[i][j.first][k];
            }

            Json::StreamWriterBuilder builder;
            builder["commentStyle"] = "None";
            builder["indentation"] = "";
            builder["precision"] = 3;
            QTextStream out(&file);
            out << QString::fromStdString(Json::writeString(builder, js));
            file.close();
        }
    }
    backend->unlock();
    ui->myOpenGLWidget->paused = false;
}

void MainWindow::on_actionSpot_triggered()
{
    if (ui->myOpenGLWidget->selectedPlanetId >= 0)
    {
        Planet& p = planets[ui->myOpenGLWidget->selectedPlanetId];
        ui->myOpenGLWidget->xPos = -(ui->myOpenGLWidget->planetToDrawPos(p.x));
        ui->myOpenGLWidget->yPos = -(ui->myOpenGLWidget->planetToDrawPos(p.y));
        ui->myOpenGLWidget->xSpd = 0.0;
        ui->myOpenGLWidget->ySpd = 0.0;
        ui->myOpenGLWidget->zSpd = 0.0;
    }
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
