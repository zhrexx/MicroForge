
all: xam.lua xam.c
	./external/luac -o xam.luac xam.lua 
	./external/xxd -i xam.luac > bytecode.h 
	gcc xam.c -o xam -static -L./deps -llua -lm

clean: xam.luac xam bytecode.h
	rm -f xam.luac xam bytecode.h
