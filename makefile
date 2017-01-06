all: CXXFLAGS += -std=c++0x -O3 -frename-registers -Wall
all: main.exe

allopt: CXXFLAGS += -std=c++0x -Ofast -fassociative-math -flto -fno-signed-zeros -fno-trapping-math -freciprocal-math -frename-registers -march=native -Wall
allopt: main.exe

debug: CXXFLAGS += -std=c++0x -Ddebug -Og -Wall
debug: main.exe

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
