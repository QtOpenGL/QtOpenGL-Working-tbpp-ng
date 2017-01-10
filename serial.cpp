// 序列化

#ifndef SERIAL_CPP
#define SERIAL_CPP

#include <array>
#include <cstdio>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

using namespace std;

// 将对象在内存中的数据转换为array
// 不包括动态分配的内存
// 不能用于数组和STL容器
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

// 从array恢复对象
template <typename T>
void deserialize(const array<unsigned char, sizeof(T)>& bytes, T& object)
{
    copy(begin(bytes), end(bytes),
         reinterpret_cast<unsigned char*>(addressof(object)));
}

// 将对象写入文件
template <typename T>
void serialize(const char* fileName, const T& object)
{
    FILE* fbin = fopen(fileName, "wb");
    fwrite(addressof(object), sizeof(T), 1, fbin);
    fclose(fbin);
}

// 从文件恢复对象
template <typename T>
void deserialize(const char* fileName, T& object)
{
    FILE* fbin = fopen(fileName, "rb");
    fread(addressof(object), sizeof(T), 1, fbin);
    fclose(fbin);
}

// 将数组中的对象依次写入文件
void serializeArray(const char* fileName, const void* objectArray, int count,
                    int size)
{
    FILE* fbin = fopen(fileName, "wb");
    fwrite(objectArray, size, count, fbin);
    fclose(fbin);
}

// 从文件依次恢复数组中的对象
void deserializeArray(const char* fileName, void* objectArray, int count,
                      int size)
{
    FILE* fbin = fopen(fileName, "rb");
    fread(objectArray, size, count, fbin);
    fclose(fbin);
}

// 将list中的对象依次写入文件
template <typename T>
void serializeList(const char* fileName, const list<T>& objectList)
{
    FILE* fbin = fopen(fileName, "wb");
    for (auto i = objectList.begin(); i != objectList.end(); ++i)
        fwrite(addressof(*i), sizeof(T), 1, fbin);
    fclose(fbin);
}

// 从文件依次恢复list中的对象
// 要求T有默认构造函数
template <typename T>
void deserializeList(const char* fileName, list<T>& objectList)
{
    objectList.clear();
    T tObject;
    FILE* fbin = fopen(fileName, "rb");
    while (fread(addressof(tObject), sizeof(T), 1, fbin) == sizeof(T))
        objectList.push_back(tObject);
    fclose(fbin);
}

// 将vector中的对象依次写入文件
template <typename T>
void serializeList(const char* fileName, const vector<T>& objectList)
{
    FILE* fbin = fopen(fileName, "wb");
    for (auto i = objectList.begin(); i != objectList.end(); ++i)
        fwrite(addressof(*i), sizeof(T), 1, fbin);
    fclose(fbin);
}

// 从文件依次恢复vector中的对象
// 要求T有默认构造函数
template <typename T>
void deserializeList(const char* fileName, vector<T>& objectList)
{
    objectList.clear();
    T tObject;
    FILE* fbin = fopen(fileName, "rb");
    while (fread(addressof(tObject), sizeof(T), 1, fbin) == sizeof(T))
        objectList.push_back(tObject);
    fclose(fbin);
}

// 将元素为map的数组写入文件
// 用于保存aiMap
template <typename T, typename U>
void serializeMapArray(const char* fileName, const map<T, U>* objectList,
                       int count)
{
    FILE* fbin = fopen(fileName, "wb");
    for (int i = 0; i < count; ++i)
    {
        size_t mapSize = objectList[i].size();
        fwrite(addressof(mapSize), sizeof(size_t), 1, fbin);
        for (auto j : objectList[i])
        {
            T tFirst = j.first;
            U tSecond = j.second;
            fwrite(addressof(tFirst), sizeof(T), 1, fbin);
            fwrite(addressof(tSecond), sizeof(U), 1, fbin);
        }
    }
    fclose(fbin);
}

// 从文件恢复元素为map的数组
template <typename T, typename U>
void deserializeMapArray(const char* fileName, map<T, U>* objectList, int count)
{
    FILE* fbin = fopen(fileName, "rb");
    for (int i = 0; i < count; ++i)
    {
        objectList[i].clear();
        size_t mapSize;
        fread(addressof(mapSize), sizeof(size_t), 1, fbin);
        for (size_t j = 0; j < mapSize; ++j)
        {
            T tFirst;
            U tSecond;
            fread(addressof(tFirst), sizeof(T), 1, fbin);
            fread(addressof(tSecond), sizeof(U), 1, fbin);
            objectList[i][tFirst] = tSecond;
        }
    }
    fclose(fbin);
}

#endif
