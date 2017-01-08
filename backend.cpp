// 后端，用于在独立的线程中进行模拟文明的运算

#include "backend.h"
#include <QThread>

Backend *backend;

// 初始暂停运算，用户手动开始
Backend::Backend()
    : paused(true), locked(false), lastClock(0), lastFpsTime(0), fps(0.0)
{
}

void Backend::init()
{
    emit msg("后端正在初始化...");
    planets.push_back(Planet(0, 0, Point(50.0, 50.0), 100.0));
    // 初始化星球
    for (int i = 1; i < MAX_PLANET; ++i)
        planets.push_back(Planet(planets.size(), planets.size(),
                                 newRandom.getPoint() * 100.0,
                                 newRandom.get() * 1.0));
    space.calcCurv();
    space.calcPlanetDis();

    // 初始化文明
    for (int i = 0; i < MAX_PLANET; ++i)
        civils.push_back(Civil(civils.size(), civils.size()));
    // civils[0].tech = 10.0;
    Civil::initFriendship();

    emit msg("后端初始化完成");
}

void Backend::lock()
{
    while (locked)
    {
        emit msg("等待后端响应...");
        QThread::msleep(1);
    }
    locked = true;
}

void Backend::unlock()
{
    locked = false;
}

void Backend::work()
{
    while (true)
    {
        // TODO：将这句注释掉则全速运行
        //        QThread::msleep(100);

        if (paused || locked)
        {
            QThread::msleep(1);
            continue;
        }

        //        cout << "clock " << space.clock << " civils " << civils.size()
        //             << " fleets " << fleets.size() << endl;

        // 文明执行动作时不改变星球数量，会改变文明和舰队数量
        // 舰队执行动作时不改变舰队数量，需要删除的舰队标记为deleteLater
        // 因此可以放在for循环里
        //        cout << "civils action" << endl;
        for (size_t i = 0; i < planets.size(); ++i)
            civils[planets[i].civilId].action();

        //        cout << "fleets action" << endl;
        for (auto i = fleets.begin(); i != fleets.end(); ++i) i->action();

        // 删除需要删除的舰队
        auto i = fleets.begin();
        while (i != fleets.end())
        {
            if (i->deleteLater)
                i = fleets.erase(i);
            else
                ++i;
        }

        ++space.clock;
        int nowFpsTime = gTime.elapsed();
        if (nowFpsTime - lastFpsTime > 100)
        {
            fps = float(space.clock - lastClock) /
                  float(nowFpsTime - lastFpsTime) * 1000.0;
            lastClock = space.clock;
            lastFpsTime = nowFpsTime;
        }
    }
}
