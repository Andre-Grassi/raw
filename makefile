LIB = -lm

# Own modules created for the game (name without extension)
MODULES = network game

CFLAGS = -g -Wall -Wextra

ifeq ($(VERBOSE),1)
    CFLAGS += -DVERBOSE
endif

all: main

main: player.o server.o object_files
	g++ -o player player.o $(MODULES:=.o) $(LIB)
	g++ -o server server.o $(MODULES:=.o) $(LIB)

player.o: player.cpp
	g++ -c $(CFLAGS) player.cpp

server.o: server.cpp
	g++ -c $(CFLAGS) server.cpp

object_files: $(MODULES:=.cpp) $(MODULES:=.hpp)
	g++ -c $(CFLAGS) $(MODULES:=.cpp)

clean:
	rm -f *.o player server objetos/* 