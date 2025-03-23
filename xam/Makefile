
all: xam.lua xam.c
	luac -o xam.luac xam.lua 
	xxd -i xam.luac > bytecode.h 
	gcc xam.c -o xam -static -L./deps -llua -lm
