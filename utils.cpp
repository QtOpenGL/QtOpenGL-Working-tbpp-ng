// 杂项

#ifndef UTILS_CPP
#define UTILS_CPP

#include <array>
#include <ctime>
#include <fstream>
#include <memory>
#include <random>

using namespace std;

#define sqr(x) ((x) * (x))

// fout用于输出大量带格式的数据，然后用其他程序分析
// fout2用于输出少量调试数据
// 少量且实时的调试数据输出到cout
ofstream fout("out.txt");
ofstream fout2("out2.txt");

// 将任何对象转换为二进制数组
// 注意：不包括对象内的指针指向的内容！
template <typename T>
array<unsigned char, sizeof(T)> serialize(const T& object)
{
    array<unsigned char, sizeof(T)> bytes;
    const unsigned char* beginObj =
        reinterpret_cast<const unsigned char*>(addressof(object));
    const unsigned char* endObj = beginObj + sizeof(T);
    copy(beginObj, endObj, begin(bytes));
    return bytes;
}

// 从二进制数组恢复对象
template <typename T>
void deserialize(const array<unsigned char, sizeof(T)>& bytes, T& object)
{
    copy(begin(bytes), end(bytes),
         reinterpret_cast<unsigned char*>(addressof(object)));
}

// 包装一对double
class Point
{
   public:
    double x, y;

    Point(double _x, double _y) : x(_x), y(_y)
    {
    }

    Point operator*(double s)
    {
        return Point(x * s, y * s);
    }
};

// 包装与随机数有关的函数
class NewRandom
{
   private:
    // 使用Mersenne Twister算法生成随机数，统计性质比rand()更好
    mt19937 gen;
    uniform_real_distribution<double> dist;

    unsigned int hash(unsigned int x)
    {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

   public:
    // 将时间hash后作为Mersenne Twister算法的初始种子
    NewRandom() : gen(hash(time(nullptr))), dist(0.0, 1.0)
    {
    }

    // 返回(0, 1)之间的随机实数
    double get()
    {
        return dist(gen);
    }

    // 返回圆心为(0.5, 0.5)，半径为0.5的圆内的两个随机实数
    // TODO：用高斯采样提高效率
    Point getPoint()
    {
        while (true)
        {
            double x = get();
            double y = get();
            if (sqr(x - 0.5) + sqr(y - 0.5) < 0.25) return Point(x, y);
        }
    }
};

NewRandom newRandom;

#endif