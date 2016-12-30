// 文明模块 by hrg

#ifndef CIVIL_CPP
#define CIVIL_CPP

#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include "newrandom.cpp"
#include "space.cpp"

using namespace std;

const double MAX_INIT_TECH = 3.0;
const double TECH_LIMIT = 100.0;
const double TECH_STEP = 10.0;       // 两次技术爆炸之间的科技
const double TECH_EXPL_RANGE = 1.0;  // 判定技术爆炸时的误差范围
const double CHILD_CIVIL_FRIENDSHIP = 100.0;
const double RUIN_TECH_REDUCE = 1.0;  //废墟造成的技术爆炸随时间减少的速度

// 包装double使其初始化为一个随机数
class AiDouble
{
   public:
    double n;

    AiDouble()
    {
        n = (newRandom.get() - 0.5) * 0.1;
    }

    operator double()
    {
        return n;
    }
};

// 舰队用来储存出发时的科技值，以及计算到目标的距离，便于触发事件
class Fleet
{
   public:
    int fleetId;
    double leftDis;
    double initDis;
    double initTech;
    Fleet(int _id, double _leftDis, double _initDis, double _initTech)
        : fleetId(_id),
          leftDis(_leftDis),
          initDis(_initDis),
          initTech(_initTech)
    {
    }
};

class Civil : public Planet
{
   public:
    static int civilCount;

    // 初始化与多个文明有关的参数，如friendship
    static void initCivils();

    // id为星球编号，继承自Planet类，friendship的下标是id
    // civilId为文明编号，一个星球上可以先后有多个不同的文明
    // parentCivilId == -1表示初始或手动增加的文明
    // deathTime < 0表示还没死
    int civilId, parentCivilId, childCivilCount, birthTime, deathTime;

    // 以下是状态参数
    double tech, timeScale;

    // 以下是策略参数
    double rateDev, rateAtk, rateCoop;
    double friendship[MAX_PLANET];  // 好感度
    map<int, AiDouble> aiMap;  // 存储在各种情境下修改rate...用的参数

    bool ruinMark;
    double recordTech;

    vector<Fleet> fleets;

    Civil(Planet& _p);

    void debugPrint();
    double aiMix(int mainKey, initializer_list<double> list);
    bool detect(Civil& target);
    void develop();
    void attack(Civil& target);
    void cooperate(Civil& target);
    void action();
};

vector<Civil> civils;
vector<Civil> civilMuseum;
// 如果开战文明间科技差值不大，势必你死我活，星球被破坏，不再适合殖民
// vector<Civil> civilRuins;

int Civil::civilCount = 0;

void Civil::initCivils()
{
    for (int i = 0; i < civils.size(); ++i)
        for (int j = 0; j < civils.size(); ++j) civils[i].friendship[j] = 0;
}

// 随机初始化
Civil::Civil(Planet& _p)
    : Planet(_p),
      civilId(civilCount),
      parentCivilId(-1),
      childCivilCount(0),
      birthTime(space.clock),
      deathTime(-1),
      tech(newRandom.get() * MAX_INIT_TECH),
      timeScale(space.curvtt(x, y)),
      rateDev(newRandom.get()),
      rateAtk(newRandom.get()),
      rateCoop(newRandom.get()),
      ruinMark(false)
{
    ++civilCount;
}

// 目前写了详细和简略两种输出方式，调试时选一种
void Civil::debugPrint()
{
    //     cout << id << endl;
    //     cout << civilId << " " << parentCivilId << " " << childCivilCount <<
    //     " "
    //     << space.clock - birthTime << endl;
    //     cout << tech << " " << timeScale << endl;
    //     cout << rateDev << " " << rateAtk << " " << rateCoop << endl;
    //     for (int i = 0; i < civils.size(); ++i) cout << friendship[i] << " ";
    //     cout << endl;
    //     for (auto iter = aiMap.begin(); iter != aiMap.end(); ++iter)
    //     cout << iter->first << " " << iter->second << endl;
    //     cout << endl;
    //
    //    cout << id << " " << civilId << " " << tech << " "
    //         << space.clock - birthTime << " " << int(ruinMark) << endl;
}

double Civil::aiMix(int mainKey, initializer_list<double> list)
{
    int key = mainKey << 16;
    double res = aiMap[key];
    for (int i = 0; i < list.size(); ++i)
    {
        key = mainKey << 16 & 1 << 8 & i;
        res += aiMap[key] * (*(list.begin() + i));
    }
    return res;
}

bool Civil::detect(Civil& target)
{
    // 探测距离与科技水平有关。具体函数待定。
    double detectDis = tech;
    for (int i = 0; i < MAX_PLANET; ++i)
    {
        if (space.getDis(civils[id].x, civils[id].y, target.x, target.y) <
            detectDis)
        {
            double remainTech =
                target.recordTech *
                exp(RUIN_TECH_REDUCE * (space.clock - target.deathTime));
            if (target.ruinMark && remainTech > tech)
            {
                // 探测到废墟且废墟剩余科技大于己方科技则科技加成，加成值为剩余科技值。
                // 剩余科技值随时间而减少。
                tech += remainTech;
                target.ruinMark = false;
            }
            return true;
        }
    }
    return false;
}

void Civil::develop()
{
    // 当科技累积到一定程度触发技术爆炸
    // TODO：探测到文明遗址时，若对方文明比自己高，或者科技走向不同（发展策略不同）则技术爆炸
    for (int i = 0; i < TECH_LIMIT / TECH_STEP; ++i)
    {
        if (abs(tech - TECH_STEP * i) < TECH_EXPL_RANGE)
        {
            tech += 2.0;
            return;
        }
    }
    tech += 1.0;
}

void Civil::attack(Civil& target)
{
// 根据aiMap修改自己的rate...
// 参数为自己的rate...，双方科技的比值，时间曲率的比值
// 用行号表示情境
#define tempMacro(a)                                                      \
    a += aiMix(__LINE__, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
// 修改对方的rate...和好感度
#define tempMacro(a)                                                       \
    a += aiMix(__LINE__, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(target.friendship[id]);
#undef tempMacro

    // 判定是否攻击成功
    double atkChance = newRandom.get();
    if (1.0 - exp(-tech / target.tech) > atkChance)
    {
        target.deathTime = space.clock;
        civilMuseum.push_back(target);

        // 判定殖民或成为废墟
        double desChance = newRandom.get();
        if (exp(-tech / target.tech) < desChance)
        {
            // 殖民
            civilMuseum.push_back(target);
            target.civilId = civilCount;
            ++civilCount;
            target.parentCivilId = civilId;
            target.childCivilCount = 0;
            childCivilCount += 1;
            target.birthTime = space.clock;
            target.deathTime = -1.0;

#define copyData(a) target.a = a
            copyData(tech);
            copyData(rateAtk);
            copyData(rateCoop);
            copyData(rateDev);
            copyData(aiMap);
#undef copyData

            for (int i = 0; i < civils.size(); ++i)
            {
                target.friendship[i] = friendship[i];
                civils[i].friendship[target.id] = civils[i].friendship[id];
            }
            // 自己与子文明好感度很高
            friendship[target.id] = CHILD_CIVIL_FRIENDSHIP;
            target.friendship[id] = CHILD_CIVIL_FRIENDSHIP;
        }
        else
        {
            // 成为废墟。放置废墟标志，记录科技值，产生新文明
            // TODO：废墟被探测到时探测方技术爆炸
            target.ruinMark = true;
            target.recordTech = target.tech;
            //            target.lastCivilId = civilMuseum.size()+1;
            civilMuseum.push_back(target);
            // civilRuins.push_back(target);

            // 产生随机文明
            target.civilId = civilCount;
            ++civilCount;
            target.parentCivilId = -1;
            target.childCivilCount = 0;
            target.birthTime = space.clock;
            target.deathTime = -1.0;
            target.tech = newRandom.get() * MAX_INIT_TECH;
            target.rateAtk = newRandom.get();
            target.rateCoop = newRandom.get();
            target.rateDev = newRandom.get();
            for (int i = 0; i < civils.size(); ++i)
            {
                target.friendship[i] = 0.0;
                civils[i].friendship[target.id] = 0.0;
            }
        }
    }
}

void Civil::cooperate(Civil& target)
{
#define tempMacro(a)                                                      \
    a += aiMix(__LINE__, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
#define tempMacro(a)                                                       \
    a += aiMix(__LINE__, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(target.friendship[id]);
#undef tempMacro

    double t = tech / target.tech;
    tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
    t = 1.0 / t;
    target.tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
    // double x = target.tech - tech;
    // tech += 2.0 + 0.25 * (tanh(x) + 1.0) * abs(x);
    // target.tech += 2.0 + 0.25 * (tanh(-x) + 1.0) * abs(x);
}

// TODO：计算自己做各个动作的概率，然后选一个
void Civil::action()
{
    // 废墟星球就不参与行动了
    //    if (civils[id].ruinMark) return;

    //  先对内动作：发展
    double invSize = 1.0 / civils.size();
    // 目前有1/3概率发展，1/3概率攻击，1/3概率合作
    double choice = newRandom.get();
    if (choice < 0.33)
    {
        // cout << id << " develop" << endl;
        develop();
    }

    // 再进行对外动作：攻击、合作。载体为舰队。
    // 解决舰队航行时对目标文明重复动作问题：行动时检查已经存在的舰队组，
    // 如果有相应舰队则继续该舰队行动，否则派出新舰队。

    for (int i = 0; i < civils.size(); i++)
    {
        for (int j = 0; j < fleets.size(); j++)
        {
            // 舰队已经存在
            if (fleets[j].fleetId == civils[i].id)
            {
                fleets[j].leftDis -= 1;
                //已经到达，执行行动
                if (fleets[j].leftDis == fleets[j].initDis)
                {
                    if (choice < 0.67 && choice >= 0.33)
                    {
                        // 找一个目标攻击
                        // 没有找到合适的目标就什么都不做
                        for (int i = 0; i < civils.size(); ++i)
                        {
                            if (i == id) continue;
                            if (civils[i].ruinMark) continue;
                            if (newRandom.get() < invSize &&
                                exp(rateAtk) * (-friendship[i] + 1.0) > 1.0)
                            {
                                // cout << id << " attack " << i << endl;
                                attack(civils[i]);
                                break;
                            }
                        }
                    }
                    if (choice >= 0.67)
                    {
                        // 找一个目标合作
                        // 没有找到合适的目标就什么都不做
                        for (int i = 0; i < civils.size(); ++i)
                        {
                            if (i == id) continue;
                            if (civils[i].ruinMark) continue;
                            if (newRandom.get() < invSize &&
                                exp(rateCoop) * (friendship[i] + 1.0) > 1.0)
                            {
                                // cout << id << " cooperate " << i << endl;
                                cooperate(civils[i]);
                                break;
                            }
                        }
                    }
                }
            }
            // 舰队不存在，产生舰队
            else
            {
                fleets.push_back(Fleet(
                    civils[i].id, space.getDis(x, y, civils[i].x, civils[i].y),
                    space.getDis(x, y, civils[i].x, civils[i].y), tech));
            }
        }
    }
}

#endif
