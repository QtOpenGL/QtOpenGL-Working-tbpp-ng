// 空间模块 by wd

#ifndef SPACE_H
#define SPACE_H

#include <cmath>
#include <vector>
#include "mesh.cpp"
#include "utils.h"

using namespace std;

const int MAX_PLANET = 1000;
const double G_CONST = 0.1;  // 引力强度
const double RG_CONST = 1.2;  // 描述星球密度，影响黑洞形状，必须大于9/8
const int MAX_CURV_ITER = 10;
const double MIN_LOSS = 1.0e-7;
const double MIN_DIS_SEG_LEN = 1.0;

class Planet : public Point
{
   public:
    // planetId为星球编号，即Planet对象在planets中的位置
    // civilId为文明编号，即Civil对象在civils中的位置
    // 一个星球可能先后有多个文明，一个文明只对应一个星球
    int planetId, civilId;
    double mass;
    // 根据星球质量确定半径，用于可视化
    // 0~100左右的质量对应0.01~0.1左右的半径
    float radius;

    bool ruinMark;    // 是否为废墟
    double lastTech;  // 上个文明的科技

    Planet(int _planetId, int _civilId, Point _p, double _mass)
        : Point(_p),
          planetId(_planetId),
          civilId(_civilId),
          mass(_mass),
          radius(log(log(_mass + 1.0) + 1.0) * 0.02 + 0.002),
          ruinMark(false),
          lastTech(0.0)
    {
    }
};

extern vector<Planet> planets;

class Space
{
   public:
    int clock;
    // 度规张量
    Mesh<double> curvxx, curvxy, curvyy, curvtt;
    // curv..2为更新度规张量时的临时变量
    Mesh<double> curvxx2, curvxy2, curvyy2, curvtt2;
    // 星球距离的缓存
    double planetDis[MAX_PLANET][MAX_PLANET];

    Space() : clock(0)
    {
    }

    // 两点间的距离
    // 在目前的近似下，测地线为平直时空中的直线
    double getDis(double x1, double y1, double x2, double y2)
    {
        double dx = x1 - x2;
        double dy = y1 - y2;
        double r = sqrt(sqr(x1 - x2) + sqr(y1 - y2));
        if (r == 0.0) return 0.0;

        // 连线方向矢量
        double ex = dx / r;
        double ey = dy / r;

        // 在测地线上每隔最多MIN_DIS_SEG_LEN取一个点
        int segCount = ceil(r / MIN_DIS_SEG_LEN);
        double invSegCount = 1.0 / segCount;
        double segLen = r / double(segCount);

        // 每个点附近的长度乘上该点的空间伸缩率
        double res = 0.0;
        for (double i = 0.5; i < segCount; ++i)
        {
            double px =
                x1 * i * invSegCount + x2 * (segCount - i) * invSegCount;
            double py =
                y1 * i * invSegCount + y2 * (segCount - i) * invSegCount;
            res +=
                segLen * sqrt(sqr(curvxx(px, py) * ex + curvxy(px, py) * ey) +
                              sqr(curvxy(px, py) * ex + curvyy(px, py) * ey));
        }
        return res;
    }

    // 两个星球间的距离
    double getDis(int p1, int p2)
    {
        return planetDis[p1][p2];
    }

    void updateCurv()
    {
        forMesh(i, j)
        {
            curvxx2(i, j) = 1.0;
            curvxy2(i, j) = 0.0;
            curvyy2(i, j) = 1.0;
            curvtt2(i, j) = 1.0;
        }

        //        for (size_t k = 0; k < planets.size(); ++k)
        for (size_t k = 0; k < 1; ++k)
        {
            const Planet& p = planets[k];

            // 计算当前星球到各个点的距离
            // 先在小网格中计算，再在大网格中插值
            Mesh<double> dis, disSmall;
            for (int i = 0; i < MAX_MESH_SMALL; ++i)
                for (int j = 0; j < MAX_MESH_SMALL; ++j)
                    disSmall(i, j) = getDis(p.x, p.y, i * MESH_SMALL_SCALE,
                                            j * MESH_SMALL_SCALE);
            dis = disSmall.scale(MESH_SMALL_SCALE);

            forMesh(i, j)
            {
                double dx = p.x - i;
                double dy = p.y - j;
                double dr = sqrt(sqr(dx) + sqr(dy));
                if (dr == 0.0)
                {
// 用i，j较小的五个点插值
#define fix(a)                            \
    a(i, j) = a(i - 1, j) + a(i, j - 1) - \
              (a(i - 2, j) + a(i - 1, j - 1) + a(i, j - 2)) / 3.0
                    fix(curvxx2);
                    fix(curvxy2);
                    fix(curvyy2);
                    fix(curvtt2);
                    continue;
#undef fix
                }

                // 连线方向矢量
                double ex = dx / dr;
                double ey = dy / dr;

                double spaceScale = 0.0;
                double timeScale = 0.0;
                double r = dis(i, j);
                double rs = G_CONST * p.mass;
                double rg = RG_CONST * rs;

                // Schwarzchild度规
                if (r >= rg)
                {
                    spaceScale = sqrt(1.0 - rs / r);
                    timeScale = 1.0 / spaceScale;
                }
                else
                {
                    spaceScale = sqrt(1.0 - sqr(r) / sqr(rg) / RG_CONST);
                    timeScale =
                        2.0 / (3.0 * sqrt(1.0 - 1.0 / RG_CONST) - spaceScale);
                }

                // 将当前星球与当前点连线方向的空间伸缩率转换为x，y方向
                double pCurvxx = spaceScale * sqr(ex) + sqr(ey);
                double pCurvxy = (spaceScale - 1.0) * ex * ey;
                double pCurvyy = (sqr(ex) + spaceScale * sqr(ey));

                // 把当前星球产生的空间伸缩率加到之前的度规上
                // 目前用的是矩阵乘法，不满足交换律
                // 不过空间伸缩率接近1时对易项是高阶小量
                // 接下来只要对质量较大的星球作特殊处理
                double newCurvxx =
                    curvxx2(i, j) * pCurvxx + curvxy2(i, j) * pCurvxy;
                double newCurvxy =
                    0.5 * ((curvxx2(i, j) + curvyy2(i, j)) * pCurvxy +
                           curvxy2(i, j) * (pCurvxx + pCurvyy));
                double newCurvyy =
                    curvxy2(i, j) * pCurvxy + curvyy2(i, j) * pCurvyy;
                curvxx2(i, j) = newCurvxx;
                curvxy2(i, j) = newCurvxy;
                curvyy2(i, j) = newCurvyy;
                curvtt2(i, j) *= timeScale;
            }
        }
    }

    void calcCurv()
    {
        forMesh(i, j)
        {
            curvxx(i, j) = 1.0;
            curvxy(i, j) = 0.0;
            curvyy(i, j) = 1.0;
            curvtt(i, j) = 1.0;
        }

        for (int iterCount = 0; iterCount < MAX_CURV_ITER; ++iterCount)
        {
            updateCurv();

            double loss = 0.0;
            forMesh(i, j)
            {
                loss += sqr(curvxx(i, j) - curvxx2(i, j)) +
                        sqr(curvyy(i, j) - curvyy2(i, j));
            }
            loss /= sqr(MAX_MESH);

            curvxx = curvxx2;
            curvxy = curvxy2;
            curvyy = curvyy2;
            curvtt = curvtt2;
            if (loss < MIN_LOSS) break;
        }
    }

    void calcPlanetDis()
    {
        for (size_t i = 0; i < planets.size(); ++i)
            for (size_t j = i; j < planets.size(); ++j)
            {
                if (i == j)
                    planetDis[i][j] = 0.0;
                else
                    planetDis[i][j] = planetDis[j][i] = getDis(
                        planets[i].x, planets[i].y, planets[j].x, planets[j].y);
            }
    }
};

extern Space space;

#endif
