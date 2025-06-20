#ifndef GAME_H
#define GAME_H

#include "network.hpp"

#define GRID_SIDE 8
#define NUM_TREASURES 8
#define TREASURE_DIR "objetos/"

class Coordinate
{
public:
    uint8_t x;
    uint8_t y;

    Coordinate();
    Coordinate(uint8_t x, uint8_t y);
};

enum cell_type
{
    EMPTY,
    PLAYER,
    VISITED,
    TREASURE,
};

class Map
{
public:
    cell_type grid[GRID_SIDE][GRID_SIDE];
    Coordinate treasures[NUM_TREASURES];
    Coordinate player_position;

    Map(bool is_player);

    void print();
    bool move_player(message_type movement);
};

#endif