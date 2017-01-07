// ����ģ�� by hrg

#include "civil.h"

using namespace std;

list<Fleet> fleets;

vector<Civil> civils;

double Civil::friendship[MAX_PLANET][MAX_PLANET];

void Civil::initFriendship()
{
    for (size_t i = 0; i < planets.size(); ++i)
        for (size_t j = 0; j < planets.size(); ++j) friendship[i][j] = 0.0;
}

// �����ʼ��
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
    // ��ֹ��ֵ����
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
    // �Ƽ��ۻ���һ���̶�ʱ����������ը
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

// ̽��Ĺ��ܻ�Ҫ�ٿ��ǣ��Ȳ�Ҫд��action��
// ���÷���ֵ
// ̽�⵽������Ƽ����ӣ�����������һ�������ĿƼ������ȣ���ʱ��ָ��˥��
// TODO��̽�⵽����ʱ�����Է��Ƽ����Լ��ߣ����߿Ƽ�����ͬ����չ���Բ�ͬ��������ը
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

// ����aiMap�޸��Լ���rate...
// ����Ϊ�Լ���rate...��˫���Ƽ��ı�ֵ��ʱ�����ʵı�ֵ
// ���кű�ʾ�龳
#define tempMacro(a)                                                      \
    a += aiMix(__LINE__, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
// �޸ĶԷ���rate...�ͺøж�
#define tempMacro(a)                                                       \
    a += aiMix(__LINE__, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(friendship[target.planetId][planetId]);
#undef tempMacro

    // �ж��Ƿ񹥻��ɹ�
    double atkChance = newRandom.get();
    if (1.0 - exp(-tech / target.tech) > atkChance)
    {
        target.deathTime = space.clock;

        // �ж�ֳ����Ϊ����
        double desChance = newRandom.get();
        if (exp(-tech / target.tech) < desChance)
        {
            // ֳ��
            if (PRINT_ACTION)
                cout << civilId << " colonize " << target.civilId << endl;
            // push_back֮��ԭ�����������ڴ��е�λ�ÿ��ܷ����仯����Ҫ��������
            int oldCivilId = civilId;
            civils.push_back(Civil(target.planetId, civils.size()));
            Civil& oldCivil = civils[oldCivilId];
            Civil& newCivil = civils[civils.size() - 1];
            targetPlanet.civilId = newCivil.civilId;
            // ���Ʋ���
            colonize(oldCivil, newCivil);
            // ����
            newCivil.mutate();
        }
        else
        {
            // ��Ϊ����
            if (PRINT_ACTION)
                cout << civilId << " ruin " << target.civilId << endl;
            targetPlanet.ruinMark = true;
            targetPlanet.lastTech = target.tech;

            // ���������ʼ����������
            civils.push_back(Civil(target.planetId, civils.size()));
            Civil& newCivil = civils[civils.size() - 1];
            targetPlanet.civilId = newCivil.civilId;

            // ���úøж�
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

// �����Լ��ĿƼ�����Բ���
#define copyData(a) newCivil.a = oldCivil.a
    copyData(tech);
    copyData(rateAtk);
    copyData(rateCoop);
    copyData(rateDev);
    copyData(aiMap);
#undef copyData

    // �����Լ�����������ĺøжȣ�����������Լ��ĺøж�
    for (size_t i = 0; i < planets.size(); ++i)
    {
        friendship[newCivil.planetId][i] = friendship[oldCivil.planetId][i];
        friendship[i][newCivil.planetId] = friendship[i][oldCivil.planetId];
    }
    // �Լ����������øжȺܸ�
    friendship[oldCivil.planetId][newCivil.planetId] = CHILD_CIVIL_FRIENDSHIP;
    friendship[newCivil.planetId][oldCivil.planetId] = CHILD_CIVIL_FRIENDSHIP;
}

void Civil::cooperate(Planet& targetPlanet)
{
    Civil& target = civils[targetPlanet.civilId];
    if (PRINT_ACTION)
        cout << civilId << " cooperate " << target.civilId << endl;

// ����aiMap�޸��Լ���rate...
// ����Ϊ�Լ���rate...��˫���Ƽ��ı�ֵ��ʱ�����ʵı�ֵ
// ���кű�ʾ�龳
#define tempMacro(a)                                                      \
    a += aiMix(__LINE__, {rateDev, rateAtk, rateCoop, tech / target.tech, \
                          timeScale / target.timeScale})
    tempMacro(rateDev);
    tempMacro(rateAtk);
    tempMacro(rateCoop);
#undef tempMacro
// �޸ĶԷ���rate...�ͺøж�
#define tempMacro(a)                                                       \
    a += aiMix(__LINE__, {target.rateDev, target.rateAtk, target.rateCoop, \
                          target.tech / tech, target.timeScale / timeScale})
    tempMacro(target.rateDev);
    tempMacro(target.rateAtk);
    tempMacro(target.rateCoop);
    tempMacro(friendship[target.planetId][planetId]);
#undef tempMacro

    // ˫���Ƽ�����
    double t = tech / target.tech;
    tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
    t = 1.0 / t;
    target.tech += pow((2.0 * t / (1.0 + pow(t, 4))), 4);
}

// TODO�������Լ������������ĸ��ʣ�Ȼ��ѡһ��
void Civil::action()
{
    // �غϿ�ʼǰ��rate...��һ��
    normalizeRate();
    double invSize = 1.0 / civils.size();
    // Ŀǰ��1/3���ʷ�չ��1/3���ʹ�����1/3���ʺ���
    double choice = newRandom.get();
    if (choice < 0.33)
    {
        develop();
    }
    else if (choice < 0.67)
    {
        // ��һ��Ŀ�깥��
        // û���ҵ����ʵ�Ŀ���ʲô������
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
        // ��һ��Ŀ�����
        // û���ҵ����ʵ�Ŀ���ʲô������
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

    // ������ĸ��������parentList���������Լ�
    // �ﵽMAX_PARENT��û��ĸ����ʱ����
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
        // stepCumu�Ǹ���ĸ�����Ĳ��Բ����仯������ָ��˥����Ȩƽ���Ľ��
        // ֮ǰ�ı��������Բ����Ϻõķ�����У�����֮��ı���ҲҪ�����������У�
        // ͬʱҪ��һ�������
        double stepCumu = 1.0;
        for (int i = 0; i < parentCount - 1; ++i)
        {
            stepCumu = stepCumu * STEP_WEIGHT_REDUCE +
                       civils[parentIdList[i]].aiMap[stgPara.first] -
                       civils[parentIdList[i + 1]].aiMap[stgPara.first];
        }
        // �޸Ĳ��Բ������仯������������stepCumu��ͬ������Ҳƫ��stepCumu
        aiMap[stgPara.first] += (newRandom.getNormal() + 0.33) * stepCumu;
    }
}

void Civil::mutateNaive()
{
    for (auto stgPara : aiMap)
        aiMap[stgPara.first] += newRandom.getNormal() * stgPara.second;
}
