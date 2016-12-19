// 空间模块 by wd

#ifndef SPACE_CPP
#define SPACE_CPP

#include <cmath>
#include <vector>
#include "fside.cpp"
#include "mesh.cpp"

using namespace std;

const int MAX_PLANET = 100;
const double G_CONST = 0.1;   // 引力强度
const double RG_CONST = 1.2;  // 与黑洞形状有关，必须大于9/8
const int MAX_ITER = 10;
const double MIN_LOSS = 1.0e-7;
const double MIN_DIS_SEG_LEN = 1.0;

#define sqr(x) ((x) * (x))

class Planet
{
   public:
    static int planetCount;

    int id;
    double x, y, mass;

    Planet(double _x, double _y, double _mass)
        : id(planetCount), x(_x), y(_y), mass(_mass)
    {
        ++planetCount;
    }
};

vector<Planet> planets;

int Planet::planetCount = 0;

class Space
{
   public:
    // 度规张量
    Mesh<double> curvxx, curvxy, curvyy, curvtt;
    // curv..2为更新度规张量时的临时变量
    Mesh<double> curvxx2, curvxy2, curvyy2, curvtt2;

    int clock;

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
                segLen *
                sqrt(
                    sqr(curvxx(px, py, true) * ex + curvxy(px, py, true) * ey) +
                    sqr(curvxy(px, py, true) * ex + curvyy(px, py, true) * ey));
        }
        return res;
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

        for (int k = 0; k < planets.size(); ++k)
        {
            Planet& p = planets[k];

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

        // cout << "{";
        for (int iterCount = 0; iterCount < MAX_ITER; ++iterCount)
        {
            updateCurv();

            double loss = 0.0;
            forMesh(i, j)
            {
                loss += sqr(curvxx(i, j) - curvxx2(i, j)) +
                        sqr(curvyy(i, j) - curvyy2(i, j));
            }
            loss /= sqr(MAX_MESH);

            // fSide << loss << endl;

            curvxx = curvxx2;
            curvxy = curvxy2;
            curvyy = curvyy2;
            curvtt = curvtt2;

            // cout << "{";
            // cout << curvxx.serialize() << "," << endl;
            // cout << curvxy.serialize() << "," << endl;
            // cout << curvyy.serialize() << "," << endl;
            // cout << curvtt.serialize() << "}," << endl;

            if (loss < MIN_LOSS) break;
        }
        // cout << "{}}" << endl;
    }
};

Space space;

#endif
