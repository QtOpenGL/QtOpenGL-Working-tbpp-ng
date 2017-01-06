// 文明模块 by hrg

#ifndef CIVIL_CPP
#define CIVIL_CPP

#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include "space.cpp"

using namespace std;

const double MAX_INIT_TECH = 3.0;
const double TECH_LIMIT = 100.0;
const double TECH_STEP = 10.0;       // 两次技术爆炸之间的科技
const double TECH_BOOM_RANGE = 1.0;  // 判定技术爆炸时的误差范围
const double CHILD_CIVIL_FRIENDSHIP = 100.0;
const double RUIN_TECH_REDUCE = 1.0;  // 废墟造成的技术爆炸随时间减少的速度
const double MAX_AI_MIX = 1.0e10;
const bool PRINT_ACTION = true;  // 是否在文明执行动作时输出内容，用于调试
const bool PRINT_FULL = false;  // 是否输出详细内容

// 包装double，使其初始化为一个0.1数量级的随机数
class AiDouble
{
   public:
    double n;

    AiDouble() : n(newRandom.getNormal() * 0.1)
    {
    }

    AiDouble(double _n) : n(_n)
    {
    }

    operator double()
    {
        return n;
    }

    AiDouble operator*(double d)
    {
        return AiDouble(n * d);
    }

    AiDouble operator+=(double d)
    {
        return AiDouble(n += d);
    }
};

enum ActType
{
    ACT_DEV,
    ACT_ATK,
    ATK_COOP
};

class Fleet
{
   public:
    int fromCivilId, targetPlanetId, initTime;
    ActType actType;
    double initDis, remainDis, initTech;
    bool deleteLater;

    Fleet(int _fromCivilId, int _targetPlanetId, ActType _actType,
          double _initDis, double _initTech)
        : fromCivilId(_fromCivilId),
          targetPlanetId(_targetPlanetId),
          initTime(space.clock),
          actType(_actType),
          initDis(_initDis),
          remainDis(_initDis),
          initTech(_initTech),
          deleteLater(false)
    {
    }

    void debugPrint()
    {
        cout << fromCivilId << " " << targetPlanetId << " " << initTime << " "
             << actType << " " << initDis << " " << remainDis << " " << initTech
             << endl;
    }

    void action()
    {
    }
};

// 需要经常从中间删除，因此使用list，而不是vector
list<Fleet> fleets;

class Civil
{
   public:
    // friendship[a][b]表示星球a对星球b的好感度
    // 星球上的文明改变时好感度需要重置
    // 已死的文明的好感度不会储存
    static double friendship[MAX_PLANET][MAX_PLANET];

    static void initFriendship();

    // 殖民时复制数据
    static void colonize(Civil& oldCivil, Civil& newCivil);

    // planetId为星球编号，即Planet对象在planets中的位置
    // civilId为文明编号，即Civil对象在civils中的位置
    // 一个星球可能先后有多个文明，一个文明只对应一个星球
    // parentCivilId == -1表示没有母文明
    // deathTime == -1表示没有死亡
    int planetId, civilId, parentCivilId, childCivilCount, birthTime, deathTime;

    // 以下是状态参数
    double tech, timeScale;

    // 以下是策略参数
    double rateDev, rateAtk, rateCoop;
    map<int, AiDouble> aiMap;  // 存储在各种情境下修改rate...用的参数

    Civil(int _planetId, int _civilId);

    void debugPrint();
    // 处理策略参数
    double aiMix(int mainKey, initializer_list<double> list);
    // 将rate...的绝对值归一化，否则会越来越大
    void normalizeRate();

    // 以下是与模拟规则有关的函数
    // 发展，探测，攻击，合作
    // 目标为星球
    void develop();
    void detect(Planet& targetPlanet);
    void attack(Planet& targetPlanet);
    void cooperate(Planet& targetPlanet);

    // 以下是与策略有关的函数
    void action();
    void mutate();
    // 平凡的进化策略，用于与厉害的进化策略对比
    void mutateNaive();
};

vector<Civil> civils;

double Civil::friendship[MAX_PLANET][MAX_PLANET];

void Civil::initFriendship()
{
    for (size_t i = 0; i < planets.size(); ++i)
        for (size_t j = 0; j < planets.size(); ++j) friendship[i][j] = 0.0;
}

// 随机初始化
Civil::Civil(int _planetId, int _civilId)
    : planetId(_planetId),
      civilId(_civilId),
      parentCivilId(-1),
      childCivilCount(0),
      birthTime(space.clock),
      deathTime(-1),
      tech(newRandom.get() * MAX_INIT_TECH),
      timeScale(space.curvtt(planets[_planetId].x, planets[_planetId].y)),
      rateDev(newRandom.get()),
      rateAtk(newRandom.get()),
      rateCoop(newRandom.get())
{
}

void Civil::debugPrint()
{
    if (PRINT_FULL)
    {
        cout << planetId << " " << planets[planetId].x << " "
             << planets[planetId].y << " " << planets[planetId].mass << endl;
        cout << civilId << " " << parentCivilId << " " << childCivilCount << " "
             << birthTime << " " << space.clock - birthTime << endl;
        cout << tech << " " << timeScale << endl;
        cout << rateDev << " " << rateAtk << " " << rateCoop << endl;
        for (size_t i = 0; i < planets.size(); ++i)
            cout << friendship[planetId][i] << " ";
        cout << endl;
        for (auto i : aiMap) cout << i.first << " " << i.second << endl;
        cout << endl;
    }
    else
    {
        cout << planetId << " " << civilId << " " << tech << " "
             << space.clock - birthTime << endl;
    }
}

double Civil::aiMix(int mainKey, initializer_list<double> list)
{
    int key = mainKey << 16;
    double res = aiMap[key];
    for (size_t i = 0; i < list.size(); ++i)
    {
        key = mainKey << 16 & 1 << 8 & i;
        res += aiMap[key] * (*(list.begin() + i));
    }
    // 防止数值过大
    if (res > MAX_AI_MIX) res = 0.0;
    return res;
}

void Civil::normalizeRate()
{
    double sum = abs(rateDev) + abs(rateAtk) + abs(rateCoop);
    rateDev /= sum;
    rateAtk /= sum;
    rateCoop /= sum;
}

void Civil::develop()
{
    if (PRINT_ACTION) cout << civilId << " develop" << endl;
    // 科技累积到一定程度时触发技术爆炸
    for (int i = 1; i < TECH_LIMIT / TECH_STEP; ++i)
    {
        if (abs(tech - TECH_STEP * i) < TECH_BOOM_RANGE)
        {
            if (PRINT_ACTION) cout << civilId << " tech boom" << endl;
            tech += 2.0;
            return;
        }
    }
    tech += 1.0;
}

// 探测的功能还要再考虑，先不要写到action里
// 不用返回值
// 探测到废墟则科技增加，增加量与上一个文明的科技成正比，随时间指数衰减
// TODO：探测到废墟时，若对方科技比自己高，或者科技走向不同（发展策略不同）则技术爆炸
void Civil::detect(Planet& targetPlanet)
{
    if (PRINT_ACTION)
        cout << civilId << " detect " << targetPlanet.planetId << endl;
    if (targetPlanet.ruinMark)
    {
        tech += targetPlanet.lastTech *
                exp(-RUIN_TECH_REDUCE *
                    (space.clock - civils[targetPlanet.civilId].deathTime));
        targetPlanet.ruinMark = false;
    }
}

void Civil::attack(Planet& targetPlanet)
{
    Civil& target = civils[targetPlanet.civilId];
    if (PRINT_ACTION) cout << civilId << " attack " << target.civilId << endl;

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
    tempMacro(friendship[target.planetId][planetId]);
#undef tempMacro

    // 判定是否攻击成功
    double atkChance = newRandom.get();
    if (1.0 - exp(-tech / target.tech) > atkChance)
    {
        target.deathTime = space.clock;

        // 判定殖民或成为废墟
        double desChance = newRandom.get();
        if (exp(-tech / target.tech) < desChance)
        {
            // 殖民
            if (PRINT_ACTION)
                cout << civilId << " colonize " << target.civilId << endl;
            // push_back之后原来的文明在内存中的位置可能发生变化，需要重新引用
            int oldCivilId = civilId;
            civils.push_back(Civil(target.planetId, civils.size()));
            Civil& oldCivil = civils[oldCivilId];
            Civil& newCivil = civils[civils.size() - 1];
            targetPlanet.civilId = newCivil.civilId;
            // 复制参数
            colonize(oldCivil, newCivil);
            // 变异
            newCivil.mutate();
        }
        else
        {
            // 成为废墟
            if (PRINT_ACTION)
                cout << civilId << " ruin " << target.civilId << endl;
            targetPlanet.ruinMark = true;
            targetPlanet.lastTech = target.tech;

            // 产生随机初始化的新文明
            civils.push_back(Civil(target.planetId, civils.size()));
            Civil& newCivil = civils[civils.size() - 1];
            targetPlanet.civilId = newCivil.civilId;

            // 重置好感度
            for (size_t i = 0; i < planets.size(); ++i)
            {
                friendship[newCivil.planetId][i] = 0.0;
                friendship[i][newCivil.planetId] = 0.0;
            }
        }
    }
}

void Civil::colonize(Civil& oldCivil, Civil& newCivil)
{
    ++oldCivil.childCivilCount;

// 复制自己的科技与策略参数
#define copyData(a) newCivil.a = oldCivil.a
    copyData(tech);
    copyData(rateAtk);
    copyData(rateCoop);
    copyData(rateDev);
    copyData(aiMap);
#undef copyData

    // 复制自己对其他星球的好感度，其他星球对自己的好感度
    for (size_t i = 0; i < planets.size(); ++i)
    {
        friendship[newCivil.planetId][i] = friendship[oldCivil.planetId][i];
        friendship[i][newCivil.planetId] = friendship[i][oldCivil.planetId];
    }
    // 自己与子文明好感度很高
    friendship[oldCivil.planetId][newCivil.planetId] = CHILD_CIVIL_FRIENDSHIP;
    friendship[newCivil.planetId][oldCivil.planetId] = CHILD_CIVIL_FRIENDSHIP;
}

void Civil::cooperate(Planet& targetPlanet)
{
    Civil& target = civils[targetPlanet.civilId];
    if (PRINT_ACTION)
        cout << civilId << " cooperate " << target.civilId << endl;

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
    tempMacro(friendship[target.planetId][planetId]);
#undef tempMacro

    // 双方科技增加
    double t = tech / target.tech;
    tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
    t = 1.0 / t;
    target.tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
}

// TODO：计算自己做各个动作的概率，然后选一个
void Civil::action()
{
    // 回合开始前把rate...归一化
    normalizeRate();
    double invSize = 1.0 / civils.size();
    // 目前有1/3概率发展，1/3概率攻击，1/3概率合作
    double choice = newRandom.get();
    if (choice < 0.33)
    {
        develop();
    }
    else if (choice < 0.67)
    {
        // 找一个目标攻击
        // 没有找到合适的目标就什么都不做
        for (size_t i = 0; i < planets.size(); ++i)
        {
            if (i == size_t(planetId)) continue;
            if (newRandom.get() < invSize &&
                exp(rateAtk) * (-friendship[planetId][i] + 1.0) > 1.0)
            {
                attack(planets[i]);
                break;
            }
        }
    }
    else
    {
        // 找一个目标合作
        // 没有找到合适的目标就什么都不做
        for (size_t i = 0; i < planets.size(); ++i)
        {
            if (i == size_t(planetId)) continue;
            if (newRandom.get() < invSize &&
                exp(rateCoop) * (friendship[planetId][i] + 1.0) > 1.0)
            {
                cooperate(planets[i]);
                break;
            }
        }
    }
}

void Civil::mutate()
{
    const int MAX_PARENT = 5;
    const double STEP_WEIGHT_REDUCE = 0.5;

    // 将各代母文明存入parentList，不包括自己
    // 达到MAX_PARENT或没有母文明时结束
    int parentIdList[MAX_PARENT];
    int parentCount = 0;
    int nowCivilId = civilId;
    for (int i = 0; i < MAX_PARENT; ++i)
    {
        if (civils[nowCivilId].parentCivilId == -1) break;
        nowCivilId = civils[nowCivilId].parentCivilId;
        parentIdList[parentCount] = nowCivilId;
        ++parentCount;
    }

    for (auto stgPara : aiMap)
    {
        // stepCumu是各代母文明的策略参数变化量，按指数衰减加权平均的结果
        // 之前的变异往策略参数较好的方向进行，所以之后的变异也要往这个方向进行，
        // 同时要有一定随机性
        double stepCumu = 1.0;
        for (int i = 0; i < parentCount - 1; ++i)
        {
            stepCumu = stepCumu * STEP_WEIGHT_REDUCE +
                       civils[parentIdList[i]].aiMap[stgPara.first] -
                       civils[parentIdList[i + 1]].aiMap[stgPara.first];
        }
        // 修改策略参数，变化量的数量级与stepCumu相同，方向也偏向stepCumu
        aiMap[stgPara.first] += (newRandom.getNormal() + 0.33) * stepCumu;
    }
}

void Civil::mutateNaive()
{
    for (auto stgPara : aiMap)
        aiMap[stgPara.first] += newRandom.getNormal() * stgPara.second;
}

#endif
