#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>

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

    Network net = Network("vethad51af4", "vethad51af4");

    Map map = Map(is_player);

    map.print();

    bool end = false;
    bool can_move = true;
    bool is_sending_treasure = false;
    uint8_t sequence = 0;
    while (!end)
    {
        if (is_player && can_move)
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
                int32_t sent_bytes = net.send_message(&message);
                if (sent_bytes == -1)
                    perror("Error sending message");
                else
                    sequence++;

                can_move = false; // Prevent further moves until ACK is received
            }
        }
        else if (is_player && !can_move)
        {
            // Wait for ACK from server
            Message *received_message = net.receive_message();
            if (received_message)
            {
                switch (received_message->type)
                {
                case ACK:
                    printf("ACK received. You can move again.\n");
                    can_move = true; // Allow player to move again
                    break;
                case NACK:
                    printf("NACK received. Invalid message. Try again\n");
                    // TODO voltar o movimento do jogador
                    can_move = true; // Allow player to try again
                    break;
                }

                delete received_message;
            }
        }
        else if (!is_player && !is_sending_treasure)
        {
            // Server logic: wait for player move
            Message *received_message = net.receive_message();
            if (received_message)
            {
                switch (received_message->type)
                {
                case UP:
                case DOWN:
                case LEFT:
                case RIGHT:
                {
                    printf("Player moved. Sending ACK.\n");
                    Message ack_message = Message(0, sequence, ACK, NULL);
                    int32_t sent_bytes = net.send_message(&ack_message);
                    if (sent_bytes == -1)
                        perror("Error sending ACK");
                    else
                        sequence++;
                    break;
                }
                default:
                {
                    printf("Received unknown message type. Sending NACK.\n");
                    Message nack_message = Message(0, sequence, NACK, NULL);
                    net.send_message(&nack_message);
                    break;
                }
                }

                delete received_message;
            }
        }
    }
    return 0;
}