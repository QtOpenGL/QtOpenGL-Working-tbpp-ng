// ∂‘œÛ≥ÿ

#ifndef POOL_CPP
#define POOL_CPP

template <class T, size_t MAX_POOL>
class Pool
{
   public:
    T p[MAX_POOL];
    size_t _size;

    Pool()
    {
        _size = 0;
    }

    T& operator[](size_t i)
    {
        return p[i];
    }

    size_t size()
    {
        return _size;
    }

    void push_back(T t)
    {
        p[_size] = t;
        ++_size;
    }

    void erase(size_t i)
    {
        --_size;
        p[i] = p[_size];
    }
};

#endif
