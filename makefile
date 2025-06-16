LIB = -lm

# Own modules created for the game (name without extension)
MODULES = network

CFLAGS = -g -Wall -Wextra

all: game

game: game.o object_files
	g++ -o game game.o $(MODULES:=.o) $(LIB)

game.o: game.cpp
	g++ -c $(CFLAGS) game.cpp

object_files: $(MODULES:=.cpp) $(MODULES:=.hpp)
	g++ -c $(CFLAGS) $(MODULES:=.cpp)

clean:
	rm -f *.o game