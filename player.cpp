#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
    srand(0);

    Network net = Network("enp2s0");

    Map map = Map(true); // Player mode

    map.print();

    bool end = false;
    bool found_treasure = false;
    uint8_t sequence = 0;
    Treasure *treasure = nullptr;
    while (!end)
    {
        Message *returned_message = nullptr;
        if (!found_treasure)
        {
            printf("Enter your move (w/a/s/d): ");

            // Lê do teclado apenas WASD
            char input;
            std::cin >> input;

            message_type move;

            bool is_valid_move = false;

            if (input == 'w' || input == 'a' || input == 's' || input == 'd')
            {
                switch (input)
                {
                case 'w': // Cima
                    move = UP;
                    break;
                case 's': // Baixo
                    move = DOWN;
                    break;
                case 'a': // Esquerda
                    move = LEFT;
                    break;
                case 'd': // Direita
                    move = RIGHT;
                    break;
                }

                // Process the move
                is_valid_move = map.move_player(move);
            }
            else
                printf("Invalid input. Use only 'w', 'a', 's', or 'd'.\n");

            if (is_valid_move)
            {
                Message message = Message(0, sequence, move, NULL);
                returned_message = net.send_message(&message);
            }
            else
                puts("Can't move in that direction. Try another one.");

            switch (returned_message->type)
            {
            case ACK:
                printf("ACK received. You can move again.\n");
                break;
            case TXT_ACK_NAME:
            case VID_ACK_NAME:
            case IMG_ACK_NAME:
                printf("Received a treasure message.\n");
                puts("Found a treasure!");
                found_treasure = true;
            }
        }

        if (found_treasure)
        {
            if (!returned_message)
                net.receive_message(returned_message, false);

            // Obtém informações do tesouro
            switch (returned_message->type)
            {
            case TXT_ACK_NAME:
            case VID_ACK_NAME:
            case IMG_ACK_NAME:
            {
                std::string filename = std::string((char *)returned_message->data);
                printf("Treasure filename: %s\n", filename.c_str());
                treasure = new Treasure(filename, true);
                break;
            }
            case DATA_SIZE:
            {
                if (treasure)
                {
                    // Recebe o tamanho do tesouro
                    uint64_t size = *((uint64_t *)returned_message->data);
                    treasure->size = size;
                    treasure->data = new uint8_t[size];
                    printf("Treasure size: %llu bytes\n", size);
                }
                break;
            }
            case DATA:
            {
                puts("Writing data...");
                uint8_t chunk_size = returned_message->size;
                size_t writed = fwrite(returned_message->data, 1, chunk_size, treasure->file);
                printf("Wrote %zu bytes to treasure file.\n", writed);
                break;
            }
            case END:
            {
                fclose(treasure->file);
                printf("Treasure %s received successfully!\n", treasure->filename.c_str());
                found_treasure = false;
                break;
            }
            }

            // Send ACK
            Message ack_message = Message(0, sequence, ACK, NULL);
            net.send_message(&ack_message);
        }

        delete returned_message;
    }
    return 0;
}