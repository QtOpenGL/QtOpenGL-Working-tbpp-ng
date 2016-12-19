all: CXXFLAGS += -std=c++0x -Ofast -frename-registers
all: main.exe

allopt: CXXFLAGS += -std=c++0x -Ofast -fassociative-math -flto -fno-signed-zeros -fno-trapping-math -freciprocal-math -frename-registers -march=native
allopt: main.exe

debug: CXXFLAGS += -std=c++0x -Ddebug -Og
debug: main.exe

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
