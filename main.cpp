#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{
    srand(0);

    int opt;
    std::string mode;

    while ((opt = getopt(argc, argv, "m:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            mode = optarg;
            if (mode != "player" && mode != "server")
            {
                fprintf(stderr, "Invalid mode. Use 'player' or 'server'.\n");
                return 1;
            }
            break;
        default:
            fprintf(stderr, "Usage: %s -m <mode>\n", argv[0]);
            return 1;
        }
    }

    if (mode.empty())
    {
        fprintf(stderr, "Mode not specified. Use 'player' or 'server'.\n");
        return 1;
    }

    bool is_player;
    if (mode == "player")
        is_player = true;
    else
        is_player = false;

    /*
        Network net = Network("vethad51af4", "vethad51af4");
    */
    Map map = Map(is_player);

    map.print();

    return 0;
}