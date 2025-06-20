#include "game.hpp"
#include <cstring>

Coordinate::Coordinate(uint8_t x, uint8_t y)
{
    this->x = x;
    this->y = y;
}

Coordinate::Coordinate()
{
    this->x = 0;
    this->y = 0;
}

bool Coordinate::operator==(const Coordinate &other) const
{
    return (this->x == other.x && this->y == other.y);
}

Map::Map(bool is_player)
{
    // Inicializa grid com EMPTY
    memset(grid, EMPTY, sizeof(grid));

    player_position = Coordinate(0, 0);
    grid[0][0] = PLAYER;

    if (!is_player)
    {
        // Inicializa tesouros em posições aleatórias
        for (int i = 0; i < NUM_TREASURES; ++i)
        {
            int x, y;
            do
            {
                x = rand() % GRID_SIDE;
                y = rand() % GRID_SIDE;
            } while (grid[x][y] != EMPTY); // Garante que a posição esteja vazia

            treasures[i] = Coordinate(x, y);
            grid[x][y] = TREASURE;
        }
    }
}

void Map::print()
{
    for (int y = GRID_SIDE - 1; y >= 0; --y)
    {
        for (int x = 0; x < GRID_SIDE; ++x)
        {
            switch (grid[x][y])
            {
            case EMPTY:
                printf(" . ");
                break;
            case PLAYER:
                printf(" P ");
                break;
            case VISITED:
                printf(" V ");
                break;
            case TREASURE:
                printf(" T ");
                break;
            }
            printf("|");
        }
        printf("\n");
    }
}

bool Map::move_player(message_type movement)
{
    bool valid_move = false;
    Coordinate old_position = player_position;

    switch (movement)
    {
    case UP:
        if (player_position.y < GRID_SIDE - 1)
        {
            player_position.y++;
            valid_move = true;
        }
        break;
    case DOWN:
        if (player_position.y > 0)
        {
            player_position.y--;
            valid_move = true;
        }
        break;
    case LEFT:
        if (player_position.x > 0)
        {
            player_position.x--;
            valid_move = true;
        }
        break;
    case RIGHT:
        if (player_position.x < GRID_SIDE - 1)
        {
            player_position.x++;
            valid_move = true;
        }
        break;
    default:
        return false;
    }

    if (valid_move)
    {
        grid[player_position.x][player_position.y] = PLAYER;
        grid[old_position.x][old_position.y] = VISITED;
        return true;
    }

    return false;
}