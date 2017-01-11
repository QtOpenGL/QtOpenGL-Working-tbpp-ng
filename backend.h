// 后端，在独立的线程中进行模拟文明的运算

#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <QThread>
#include "civil.h"
#include "globaltime.h"

const int MAX_CLOCK = 10000;

class Backend : public QObject
{
    Q_OBJECT

   public:
    // paused用于用户暂停
    // locked用于线程保护
    // slow为慢速模拟
    bool paused, locked, slow;

    int lastClock, lastFpsTime;
    float fps;

    Backend();

    void init();
    void lock();
    void unlock();

   public slots:
    void work();

   signals:
    msg(QString s);
};

extern Backend *backend;

#endif
