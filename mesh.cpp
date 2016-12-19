// 网格模块 by wd

#ifndef MESH_CPP
#define MESH_CPP

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int MAX_MESH = 101;
const double MESH_SMALL_SCALE = 10.0;
const int MAX_MESH_SMALL = ceil(double(MAX_MESH) / MESH_SMALL_SCALE);

#define forMesh(i, j)                       \
    for (int(i) = 0; (i) < MAX_MESH; ++(i)) \
        for (int(j) = 0; (j) < MAX_MESH; ++(j))

// 接下来可能要对网格进行自适应细分
template <class T>
class Mesh
{
   public:
    T mesh[MAX_MESH][MAX_MESH];

    // 不用初始化
    Mesh()
    {
    }

    // 重载()，方便读写mesh的值
    T& operator()(int i, int j)
    {
        return mesh[i][j];
    }

    // 取实数位置的值，用周围四个点线性插值
    T interpolar(double i, double j)
    {
        int iFloor = floor(i);
        int jFloor = floor(j);
        double iDecimal = i - iFloor;
        double jDecimal = j - jFloor;
        return (mesh[iFloor][jFloor] * (1 - iDecimal) +
                mesh[iFloor + 1][jFloor] * iDecimal) *
                   (1 - jDecimal) +
               (mesh[iFloor][jFloor + 1] * (1 - iDecimal) +
                mesh[iFloor + 1][jFloor + 1] * iDecimal) *
                   jDecimal;
    }

    // 实数位置的值只读，flag为重载标记
    T operator()(double i, double j, bool flag)
    {
        return interpolar(i, j);
    }

    Mesh<T> scale(double s)
    {
        Mesh<T> res;
        double invS = 1.0 / s;
        forMesh(i, j) res(i, j) = interpolar(i * invS, j * invS);
        return res;
    }

    // 将数组转换为单行字符串，与Mathematica的list格式兼容
    string serialize()
    {
        string res = "";
        stringstream ss;
        ss << "{";
        ss << "{";
        ss << setprecision(3) << mesh[0][0];
        for (int j = 1; j < MAX_MESH; ++j)
            ss << "," << setprecision(3) << mesh[0][j];
        ss << "}";
        for (int i = 0; i < MAX_MESH; ++i)
        {
            ss << ",{";
            ss << setprecision(3) << mesh[i][0];
            for (int j = 1; j < MAX_MESH; ++j)
                ss << "," << setprecision(3) << mesh[i][j];
            ss << "}";
        }
        ss << "}";
        ss >> res;
        return res;
    }

    // 在流中输出数组，用空格和换行符分隔，列宽度固定
    void printTable(ostream& ss)
    {
        for (int i = 0; i < MAX_MESH; ++i)
        {
            for (int j = 0; j < MAX_MESH; ++j)
                ss << setprecision(3) << setw(6) << mesh[i][j];
            ss << endl;
        }
        ss << endl;
    }
};

#endif
