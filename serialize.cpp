// 序列化
// 将任何对象转换为二进制数组，以及恢复

#ifndef SERIALIZE_CPP
#define SERIALIZE_CPP

#include <array>
#include <memory>

using namespace std;

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

template <typename T>
void deserialize(const array<unsigned char, sizeof(T)>& bytes, T& object)
{
    copy(begin(bytes), end(bytes),
         reinterpret_cast<unsigned char*>(addressof(object)));
}

#endif
