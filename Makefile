
# this file is clearly amazing
# i guessed at the sfml dependencies
# --rain

CC = g++
OBJS = $(patsubst %.cpp,build/%.o,$(wildcard *.cpp))
CFLAGS = -std=c++11 -Iinclude
LDFLAGS = -lGL -lsfml-system -lsfml-graphics -lsfml-window

IMGUIOBJS = $(patsubst include/imgui/%.cpp,build/imgui_%.o,$(wildcard include/imgui/*.cpp))
JSONOBJS = build/json.o
TINYFDOBJS = build/tinyfd.o

.PHONY: mkdir all clean imgui json tinyfd polyedit


all: mkdir imgui json tinyfd polyedit

mkdir:
	mkdir -p build

clean:
	rm -r build


polyedit: build/polyedit

build/polyedit: $(OBJS) $(IMGUIOBJS) $(JSONOBJS) $(TINYFDOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<


imgui: $(IMGUIOBJS)

build/imgui_%.o: include/imgui/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<


json: $(JSONOBJS)

build/json.o: include/jsoncpp.cpp
	$(CC) $(CFLAGS) -c -o $@ $<


tinyfd: $(TINYFDOBJS)

build/tinyfd.o: tinyfiledialogs.c
	$(CC) -c -o $@ $<
