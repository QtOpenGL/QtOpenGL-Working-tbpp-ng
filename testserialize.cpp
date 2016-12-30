#include <stdio.h>
#include <iostream>
#include "civil.cpp"
#include "newrandom.cpp"
#include "serialize.cpp"
#include "space.cpp"

using namespace std;

const int MAX_MAIN_LOOP = 100;

int main()
{
    // ≥ı ºªØ–««Ú
    planets.push_back(Planet(50.0, 50.0, 100.0));
    for (int i = 1; i < MAX_PLANET; ++i)
        planets.push_back(Planet(newRandom.get() * 100.0,
                                 newRandom.get() * 100.0,
                                 newRandom.get() * 1.0));
    space.calcCurv();

    auto bytes = serialize(space);
    // fout << hex << setfill('0');
    // for (unsigned char b : bytes) fout << setw(2) << int(b) << ' ';
    // fout << '\n';
    FILE* fbin;
    fbin = fopen("out.bin", "wb");
    fwrite(begin(bytes), 1, end(bytes) - begin(bytes), fbin);
    fclose(fbin);

    Space space2;
    array<unsigned char, sizeof(space2)> bytes2;
    fbin = fopen("out.bin", "rb");
    fread(begin(bytes2), 1, end(bytes2) - begin(bytes2), fbin);
    fclose(fbin);
    deserialize(bytes2, space2);

    fout << "{";
    fout << space.curvtt.serializeList();
    fout << ",";
    fout << space2.curvtt.serializeList();
    fout << "}";

    return 0;
}
