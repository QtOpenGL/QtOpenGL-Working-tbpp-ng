#include <iostream>

using namespace std;

void f(initializer_list<double> list)
{
    for (int i = 0; i < list.size(); ++i)
    {
        cout << *(list.begin() + i) << endl;
    }
}

int main()
{
    f({1.0, 2.2, 3.3});
}
