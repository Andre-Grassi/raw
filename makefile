LIB = -lm

# Own modules created for the game (name without extension)
MODULES = network game

CFLAGS = -g -Wall -Wextra

all: main

main: main.o object_files
	g++ -o main main.o $(MODULES:=.o) $(LIB)

main.o: main.cpp
	g++ -c $(CFLAGS) main.cpp

object_files: $(MODULES:=.cpp) $(MODULES:=.hpp)
	g++ -c $(CFLAGS) $(MODULES:=.cpp)

send_listen: object_files
	g++ -o send send.cpp $(MODULES:=.o) $(LIB)
	g++ -o listen listen.cpp $(MODULES:=.o) $(LIB)

clean:
	rm -f *.o main send listen 