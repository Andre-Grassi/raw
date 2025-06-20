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
    bool can_move = true;
    bool is_sending_treasure = false;
    uint8_t sequence = 0;
    while (!end)
    {
        if (can_move)
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
                {
                    perror("Error sending message");
                    // TODO reenviar mensagem
                }
                else
                {
                    sequence++;
                    map.print();
                }
                can_move = false; // Prevent further moves until ACK is received
            }
            else
                puts("Can't move in that direction. Try another one.");
        }
        else if (!can_move)
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
    }
    return 0;
}