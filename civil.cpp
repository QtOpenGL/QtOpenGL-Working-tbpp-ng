// 文明模块 by hrg

#ifndef CIVIL_CPP
#define CIVIL_CPP

#include <initializer_list>
#include <iostream>
#include <map>
#include "space.cpp"

using namespace std;

const double MAX_INIT_TECH = 3.0;
const double TECH_LIMIT = 100.0;
const double TECH_STEP = 10.0;       // 两次技术爆炸之间的科技
const double TECH_EXPL_RANGE = 1.0;  // 判定技术爆炸时的误差范围
const double CHILD_CIVIL_FRIENDSHIP = 100.0;
const double RUIN_TECH_REDUCE = 1.0;  //废墟造成的技术爆炸随时间减少的速度
const double MAX_AI_MIX = 1.0e10;

// 包装double使其初始化为一个随机数
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

    operator+(double d)
    {
        return n + d;
    }

    operator-(double d)
    {
        return n - d;
    }

    operator*(double d)
    {
        return n * d;
    }

    operator+=(double d)
    {
        return n += d;
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

    Fleet(int _fleetId, double _leftDis, double _initDis, double _initTech)
        : fleetId(_fleetId),
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
    // 根据civilId查找Civil对象
    static Civil* getByCivilId(int civilId);

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

    // 以下是与策略有关的方法
    void action();
    void mutate();
};

vector<Civil> civils;
vector<Civil> civilMuseum;
// TODO：根据civilId查找Civil对象用的索引
map<int, Civil*> civilIndex;

int Civil::civilCount = 0;

void Civil::initCivils()
{
    for (int i = 0; i < civils.size(); ++i)
        for (int j = 0; j < civils.size(); ++j) civils[i].friendship[j] = 0;
}

Civil* Civil::getByCivilId(int civilId)
{
    for (size_t i = 0; i < civils.size(); ++i)
        if (civils[i].civilId == civilId) return &civils[i];
    for (size_t i = 0; i < civilMuseum.size(); ++i)
        if (civilMuseum[i].civilId == civilId) return &civilMuseum[i];
    // 默认返回值，不应该出现
    throw "Civil not found";
    return nullptr;
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
    cout << id << " " << x << " " << y << " " << mass << endl;
    cout << civilId << " " << parentCivilId << " " << childCivilCount << " "
         << space.clock - birthTime << endl;
    cout << tech << " " << timeScale << endl;
    cout << rateDev << " " << rateAtk << " " << rateCoop << endl;
    for (int i = 0; i < civils.size(); ++i) cout << friendship[i] << " ";
    cout << endl;
    for (auto i : aiMap) cout << i.first << " " << i.second << endl;
    cout << endl;
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
    // 防止数据过大
    if (res > MAX_AI_MIX) res = 0.0;
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
            cout << civilId << " colonize " << target.civilId << endl;
            // 殖民
            civilMuseum.push_back(target);
            target.civilId = civilCount;
            ++civilCount;
            target.parentCivilId = civilId;
            target.childCivilCount = 0;
            childCivilCount += 1;
            target.birthTime = space.clock;
            target.deathTime = -1;

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

            // 变异
            target.mutate();
        }
        else
        {
            cout << civilId << " ruin " << target.civilId << endl;
            // 成为废墟。放置废墟标志，记录科技值，产生新文明
            // TODO：废墟被探测到时探测方技术爆炸
            target.ruinMark = true;
            target.recordTech = target.tech;
            civilMuseum.push_back(target);

            // 产生随机文明
            target.civilId = civilCount;
            ++civilCount;
            target.parentCivilId = -1;
            target.childCivilCount = 0;
            target.birthTime = space.clock;
            target.deathTime = -1;
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
}

// TODO：计算自己做各个动作的概率，然后选一个
void Civil::action()
{
    // 废墟星球就不参与行动了
    // if (civils[id].ruinMark) return;

    double invSize = 1.0 / civils.size();
    // 目前有1/3概率发展，1/3概率攻击，1/3概率合作
    double choice = newRandom.get();
    if (choice < 0.33)
    {
        cout << id << " develop" << endl;
        develop();
    }
    else if (choice < 0.67)
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
                cout << id << " attack " << i << endl;
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
            if (civils[i].ruinMark) continue;
            if (newRandom.get() < invSize &&
                exp(rateCoop) * (friendship[i] + 1.0) > 1.0)
            {
                cout << id << " cooperate " << i << endl;
                cooperate(civils[i]);
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
    Civil* parentList[MAX_PARENT];
    int parentCount = 0;
    Civil* nowParent = this;
    for (int i = 0; i < MAX_PARENT; ++i)
    {
        if (nowParent->parentCivilId == -1) break;
        nowParent = getByCivilId(nowParent->parentCivilId);
        parentList[parentCount] = nowParent;
        ++parentCount;
    }

    for (auto stgPara : aiMap)
    {
        double stepCumu = 1.0;
        for (int i = 0; i < parentCount - 1; ++i)
        {
            stepCumu = stepCumu * STEP_WEIGHT_REDUCE +
                       parentList[i]->aiMap[stgPara.first] -
                       parentList[i + 1]->aiMap[stgPara.first];
        }
        stgPara.second += (newRandom.getNormal() + 0.33) * stepCumu;
    }
}

#endif
