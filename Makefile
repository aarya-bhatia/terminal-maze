all: main

main: .obj/main.o
	g++ -lncurses -lm $^ -o $@

.obj/main.o: main.cpp
	mkdir -p .obj/
	g++ -c -Wall -g -std=c++11 -MMD -MP $< -o $@

clean:
	rm -rf .obj/ main

-include .obj/main.d