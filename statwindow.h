// 统计窗口

#ifndef STATWINDOW_H
#define STATWINDOW_H

#include <QVector>
#include <QWidget>
#include <cmath>
#include "civil.h"
#include "libs/qcustomplot.h"

namespace Ui
{
    class statWindow;
}

class StatWindow : public QWidget
{
    Q_OBJECT

   public:
    explicit StatWindow(QWidget *parent = 0);
    ~StatWindow();
    void paintStat();

   private:
    Ui::statWindow *ui;
};

#endif
