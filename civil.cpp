// 文明模块 by hrg

#ifndef CIVIL_CPP
#define CIVIL_CPP

#include <iostream>
#include <map>
#include "newrandom.cpp"
#include "space.cpp"

using namespace std;

const double MAX_INIT_TECH = 3.0;
const double CHILD_CIVIL_FRIENDSHIP = 100.0;

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

class Civil : public Planet
{
   public:
    static int civilCount;

    // 初始化与多个文明有关的参数，如friendship
    static void initCivils();

    // id为星球编号，继承自Planet类，friendship的下标是id
    // civilId为文明编号，一个星球上可以先后有多个不同的文明
    // parentCivilId为-1表示初始或手动增加的文明
    int civilId, parentCivilId, childCivilCount, birthTime, liveTime;

    // 以下是状态参数
    double tech, timeScale;

    // 以下是策略参数
    double rateDev, rateAtk, rateCoop;
    double friendship[MAX_PLANET];  // 好感度
    map<int, AiDouble> aiMap;  // 存储在各种情境下修改rate...用的参数

    Civil(Planet& _p);

    void debugPrint();
    void adjPara(int line, double& para, initializer_list<double> list);
    void develop();
    void attack(Civil& target);
    void cooperate(Civil& target);
    void action();
};

vector<Civil> civils;
vector<Civil> civilMuseum;

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
      liveTime(0),
      tech(newRandom.get() * MAX_INIT_TECH),
      timeScale(space.curvtt(x, y)),
      rateDev(newRandom.get()),
      rateAtk(newRandom.get()),
      rateCoop(newRandom.get())
{
    ++civilCount;
}

// 目前写了详细和简略两种输出方式，调试时选一种
void Civil::debugPrint()
{
    // cout << id << endl;
    // cout << civilId << " " << parentCivilId << " " << childCivilCount << " "
    // << space.clock - birthTime << endl;
    // cout << tech << " " << timeScale << endl;
    // cout << rateDev << " " << rateAtk << " " << rateCoop << endl;
    // for (int i = 0; i < civils.size(); ++i) cout << friendship[i] << " ";
    // cout << endl;
    // for (auto iter = aiMap.begin(); iter != aiMap.end(); ++iter)
    // cout << iter->first << " " << iter->second << endl;
    // cout << endl;

    cout << id << " " << civilId << " " << tech << " "
         << space.clock - birthTime << endl;
}

void Civil::adjPara(int line, double& para, initializer_list<double> list)
{
    int key;
    key = line << 16;
    para += aiMap[key];
    for (int i = 0; i < list.size(); ++i)
    {
        key = line << 16 & 1 << 8 & i;
        para += aiMap[key] * (*(list.begin() + i));
    }
}

void Civil::develop()
{
    tech += 1.0;
}

void Civil::attack(Civil& target)
{
// 根据aiMap修改自己的rate...
// 参数为自己的rate...，双方科技的比值，时间曲率的比值
// 用行号表示情境
#define tempMacro(a)                                                      \
    adjPara(__LINE__, a, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
// 修改对方的rate...和好感度
#define tempMacro(a)                                                       \
    adjPara(__LINE__, a, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(target.friendship[id]);
#undef tempMacro

    double atkChance = newRandom.get();
    // 判定是否攻击成功
    // 自己科技小于对方时一定不攻击吗？待定
    if (target.tech < tech && 1.0 - exp(-tech / target.tech) > atkChance)
    {
        // 攻击成功则直接殖民
        target.liveTime = space.clock;
        civilMuseum.push_back(target);

        target.civilId = civilCount;
        target.parentCivilId = civilId;
        target.childCivilCount = 0;
        childCivilCount += 1;
        target.birthTime = space.clock;
        target.liveTime = 0;

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

        ++civilCount;
    }
}

void Civil::cooperate(Civil& target)
{
#define tempMacro(a)                                                      \
    adjPara(__LINE__, a, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
#define tempMacro(a)                                                       \
    adjPara(__LINE__, a, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(target.friendship[id]);
#undef tempMacro

    double coopChance = newRandom.get();
    // 判定是否合作成功
    if (1.0 - exp(-tech / target.tech) > coopChance)
    {
        double x = target.tech - tech;
        tech += 2.0 + 0.25 * (tanh(x) + 1.0) * abs(x);
        target.tech += 2.0 + 0.25 * (tanh(-x) + 1.0) * abs(x);
    }
}

// TODO：计算自己做各个动作的概率，然后选一个
void Civil::action()
{
    double invSize = 1.0 / civils.size();
    // 目前有1/3概率发展，1/3概率攻击，1/3概率合作
    double choice = newRandom.get();
    if (choice < 0.33)
    {
        // cout << id << " develop" << endl;
        develop();
    }
    else if (choice < 0.67)
    {
        // 找一个目标攻击
        // 没有找到合适的目标就什么都不做
        for (int i = 0; i < civils.size(); ++i)
        {
            if (i == id) continue;
            if (newRandom.get() > invSize &&
                exp(rateAtk) * (-friendship[i] + 1.0) > 1.0)
            {
                // cout << id << " attack " << i << endl;
                attack(civils[i]);
                break;
            }
        }
    }
    else
    {
        // 找一个目标合作
        // 没有找到合适的目标就什么都不做
        for (int i = 0; i < civils.size(); ++i)
        {
            if (i == id) continue;
            if (newRandom.get() > invSize &&
                exp(rateCoop) * (friendship[i] + 1.0) > 1.0)
            {
                // cout << id << " cooperate " << i << endl;
                cooperate(civils[i]);
                break;
            }
        }
    }
}

#endif
