// 包装与随机数有关的函数

#ifndef NEWRANDOM_CPP
#define NEWRANDOM_CPP

#include <cstdlib>
#include <ctime>

using namespace std;

class NewRandom
{
   private:
    unsigned int hash(unsigned int x)
    {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

   public:
    double get()
    {
        return double(rand()) / RAND_MAX;
    }

    NewRandom()
    {
        srand(hash(time(nullptr)));
    }
};

NewRandom newRandom;

#endif
