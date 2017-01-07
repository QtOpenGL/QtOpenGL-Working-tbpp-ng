// 文明模块 by hrg

#ifndef CIVIL_H
#define CIVIL_H

#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include "space.h"

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

// 舰队需要经常从中间删除，因此使用list，而不是vector
extern list<Fleet> fleets;

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

extern vector<Civil> civils;

#endif
