// 序列化

#ifndef SERIAL_CPP
#define SERIAL_CPP

#include <array>
#include <cstdio>
#include <list>
#include <memory>

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
void serializeToFile(const char* fileName, const T& object)
{
    FILE* fbin = fopen(fileName, "wb");
    fwrite(addressof(object), 1, sizeof(T), fbin);
    fclose(fbin);
}

// 从文件恢复对象
template <typename T>
void deserializeFromFile(const char* fileName, T& object)
{
    FILE* fbin = fopen(fileName, "rb");
    fread(addressof(object), 1, sizeof(T), fbin);
    fclose(fbin);
}

// 将数组中的对象依次写入文件
void serializeArrayToFile(const char* fileName, const void* objectArray,
                          int count, int size)
{
    FILE* fbin = fopen(fileName, "wb");
    fwrite(objectArray, count, size, fbin);
    fclose(fbin);
}

// 从文件依次恢复数组中的对象
void deserializeArrayFromFile(const char* fileName, void* objectArray,
                              int count, int size)
{
    FILE* fbin = fopen(fileName, "rb");
    fread(objectArray, count, size, fbin);
    fclose(fbin);
}

// 将list中的对象依次写入文件
template <typename T>
void serializeListToFile(const char* fileName, const list<T>& objectList)
{
    FILE* fbin = fopen(fileName, "wb");
    for (auto i = objectList.begin(); i != objectList.end(); ++i)
        fwrite(addressof(*i), 1, sizeof(T), fbin);
    fclose(fbin);
}

// 从文件依次恢复list中的对象
// 要求T有默认构造函数
template <typename T>
void deserializeListFromFile(const char* fileName, list<T>& objectList)
{
    objectList.clear();
    T tObject;
    FILE* fbin = fopen(fileName, "rb");
    while (fread(addressof(tObject), 1, sizeof(T), fbin) == sizeof(T))
        objectList.push_back(tObject);
    fclose(fbin);
}

#endif
