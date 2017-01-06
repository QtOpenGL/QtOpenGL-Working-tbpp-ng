#include "civil.cpp"

using namespace std;

const int MAX_MAIN_LOOP = 100;

int main()
{
    // 初始化星球
    planets.push_back(Planet(Point(50.0, 50.0), 100.0));
    for (int i = 1; i < MAX_PLANET; ++i)
        planets.push_back(
            Planet(newRandom.getPoint() * 100.0, newRandom.get() * 1.0));
    space.calcCurv();

    // 初始化文明
    for (auto& i : planets) civils.push_back(Civil(i));
    civils[0].tech = 10.0;
    Civil::initCivils();

    // 主循环
    for (int mainLoopCount = 0; mainLoopCount < MAX_MAIN_LOOP; ++mainLoopCount)
    {
        cout << "round " << mainLoopCount << " museum " << civilMuseum.size() << endl
             << endl;
        for (auto& i : civils) i.action();
        cout << endl;
        for (auto& i : civils) i.debugPrint();
        cout << endl;
        ++space.clock;
    }

    return 0;
}
