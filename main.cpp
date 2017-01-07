#include "civil.h"

using namespace std;

const int MAX_MAIN_LOOP = 100;

int main()
{
    // 初始化星球
    for (int i = 0; i < MAX_PLANET; ++i)
        planets.push_back(Planet(planets.size(), planets.size(),
                                 newRandom.getPoint() * 100.0,
                                 newRandom.get() * 1.0));
    planets[0].x = 50.0;
    planets[0].y = 50.0;
    planets[0].mass = 100.0;
    space.calcCurv();
    space.calcPlanetDis();

    // 初始化文明
    for (int i = 0; i < MAX_PLANET; ++i)
        civils.push_back(Civil(civils.size(), civils.size()));
    // civils[0].tech = 10.0;
    Civil::initFriendship();

    // 主循环
    for (int mainLoopCount = 0; mainLoopCount < MAX_MAIN_LOOP; ++mainLoopCount)
    {
        cout << "round " << mainLoopCount << " civils " << civils.size() << endl
             << endl;
        // 文明执行动作时不改变星球数量，会改变文明和舰队数量
        // 舰队执行动作时不改变舰队数量，需要删除的舰队标记为deleteLater
        // 因此可以放在for循环里
        cout << "civils action" << endl;
        for (auto i : planets) civils[i.civilId].action();
        cout << "fleets action" << endl;
        for (auto i : fleets) i.action();
        // 删除需要删除的舰队
        auto i = fleets.begin();
        while (i != fleets.end())
        {
            if (i->deleteLater)
                i = fleets.erase(i);
            else
                ++i;
        }
        cout << "civils print" << endl;
        for (auto i : planets) civils[i.civilId].debugPrint();
        cout << "fleets print" << endl;
        for (auto i : fleets) i.debugPrint();
        cout << endl;
        ++space.clock;
    }

    return 0;
}
