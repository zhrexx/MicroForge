
all: xam.lua xam.c
	luac -o xam.luac xam.lua 
	xxd -i xam.luac > xam.h 
	gcc xam.c -o xam -static -L./deps -llua -lm

clean: xam.luac xam xam.h
	rm -f xam.luac xam xam.h
