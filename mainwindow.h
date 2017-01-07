#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QThread>
#include <QTimer>

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

   public:
    QThread backendThread;
    QTimer *timer;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

   private:
    Ui::MainWindow *ui;

   public slots:
    void animate();
    void receiveMsg(QString s);

   private slots:
    void on_actionPause_triggered();
    void on_actionReset_triggered();

   signals:
    backendInit();
    backendWork();
};

#endif
