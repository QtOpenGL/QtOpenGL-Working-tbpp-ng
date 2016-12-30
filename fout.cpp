// 输出文件

#ifndef FOUT_CPP
#define FOUT_CPP

#include <fstream>

using namespace std;

// fout用于输出大量序列化的数据，然后用其他程序分析
// fout2用于输出少量调试数据
// 少量且实时的调试数据输出到cout
ofstream fout("out.txt");
ofstream fout2("out2.txt");

#endif
