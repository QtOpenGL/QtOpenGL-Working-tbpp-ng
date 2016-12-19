all: CXXFLAGS += -std=c++11 -Ofast -frename-registers
all: main.exe

allopt: CXXFLAGS += -std=c++11 -Ofast -fassociative-math -flto -fno-signed-zeros -fno-trapping-math -freciprocal-math -frename-registers -march=native
allopt: main.exe

debug: CXXFLAGS += -std=c++11 -Ddebug -Og
debug: main.exe

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
