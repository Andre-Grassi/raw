LIB = -lm

# Own modules created for the game (name without extension)
MODULES = network game

CFLAGS = -g -Wall -Wextra

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

send_listen: object_files
	g++ -o send send.cpp $(MODULES:=.o) $(LIB)
	g++ -o listen listen.cpp $(MODULES:=.o) $(LIB)

clean:
	rm -f *.o player server send listen objetos/* 