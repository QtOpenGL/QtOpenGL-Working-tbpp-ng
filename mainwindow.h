// 主窗口

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QThread>
#include <QTimer>
#include "backend.h"
#include "statwindow.h"

const int AUTO_SAVE_INTERVAL = 60000;  // 自动保存的间隔（毫秒）

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

   private:
    Ui::MainWindow *ui;
    StatWindow statWindow;
    QThread backendThread;
    QTimer *timer;
    QString fileName;  // 当前打开的数据路径
    int lastAutoSaveTime;

    void loadData();
    void saveData();

   public slots:
    void animate();
    void showMsg(QString s);
    void on_actionPause_toggled(bool b);        // 开始/暂停模拟
    void on_actionOpen_triggered();             // 打开数据
    void on_actionSave_triggered(bool saveAs);  // 保存数据
    void on_actionSaveAs_triggered();           // 另存为数据
    void on_actionReset_triggered();            // 星图复位
    void on_actionStat_triggered();             // 显示统计图
    void on_actionAbout_triggered();            // 版权信息

   signals:
    backendInit();
    backendWork();
};

#endif
