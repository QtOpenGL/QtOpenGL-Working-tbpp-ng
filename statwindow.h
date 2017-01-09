#ifndef STATWINDOW_H
#define STATWINDOW_H

#include <QWidget>
#include "qcustomplot.h"

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

#endif  // STATWINDOW_H
