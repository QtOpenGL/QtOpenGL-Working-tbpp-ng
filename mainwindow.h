#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QThread>
#include <QTimer>
#include "statwindow.h"

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

   public slots:
    void animate();
    void receiveMsg(QString s);
    void on_actionPause_triggered();
    void on_actionReset_triggered();
    void on_actionStat_triggered();

   signals:
    backendInit();
    backendWork();
};

#endif
