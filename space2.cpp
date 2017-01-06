// 空间模块 by dmy

#include <cmath>
#include <iostream>

using namespace std;

// const double RS_CONST = 1.484942765e-27;
const double RS_CONST = 1.0;
const int MAX_BISEC_ITER = 10;
const double BISEC_RANGE = 1.0e-6;

struct Point
{
    double r;
    double theta;
    double t;
};

double f(double m, double rg, double rs, double r)
{
    return r * sqrt(1.0 - rs / r) +
           0.5 * rs * log(-rs + 2.0 * r * (1.0 + sqrt(1.0 - rs / r))) -
           0.5 * rs *
               log(-rs +
                   2.0 * rg * sqrt(rg / rs) *
                       (1.0 + sqrt(1.0 - (rs / (sin(sqrt(rs / rg)) *
                                                sqrt(rg * rg * rg / rs))))) *
                       sin(sqrt(rs / rg))) -
           rg * sqrt(rg / rs) *
               sqrt(1.0 -
                    (rs / (sin(sqrt(rs / rg)) * sqrt(rg * rg * rg / rs)))) *
               sin(sqrt(rs / rg)) +
           rg;
}

double solve(double m, double rg, double rs, double r0)
{
    double r1, r2, rMid, rNew;
    r1 = 0.0;
    r2 = r0;
    for (int i = 0; i < MAX_BISEC_ITER; ++i)
    {
        rMid = (r1 + r2) * 0.5;
        rNew = f(m, rg, rs, rMid);
        if (rNew - r0 > BISEC_RANGE)
            r2 = rMid;
        else if (rNew - r0 < -BISEC_RANGE)
            r1 = rMid;
        else
            break;
    }
    return rNew;
}

Point relativity(double m, double rg, Point& A)
{
    const double rs = RS_CONST * m;
    Point B;
    B.r = solve(m, rg, rs, A.r);
    B.theta = A.theta;
    B.t = A.t / sqrt(1 - rs / B.r);
    return B;
}

double d(Point& A, Point& B)
{
    return sqrt(A.r * A.r + B.r * B.r - 2.0 * A.r * B.r * cos(A.theta - B.theta));
}

int main()
{
    cout << "hello, world" << endl;
    int n;
    double m, rg;
    cin >> n;
    cin >> m;
    cin >> rg;
    Point A[n], B[n];
    for (int i = 0; i < n; ++i) A[i] = {double(i), 6.28 * i / n, 1.0};
    for (int i = 0; i < n; ++i) B[i] = relativity(m, rg, A[i]);
    for (int i = 0; i < n - 1; ++i)
        for (int j = i + 1; j < n; ++j)
            cout << "d(B[" << i << "], B[" << j << "]) = " << d(B[i], B[j])
                 << endl;
    for (int i = 0; i < n; ++i) cout << B[i].t << endl;
    return 0;
}
