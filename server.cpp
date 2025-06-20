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

    Map map = Map(false); // Server mode

    map.print();

    bool end = false;
    bool can_move = true;
    bool is_sending_treasure = false;
    uint8_t sequence = 0;
    while (!end)
    {
        if (!is_sending_treasure)
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
                received_message = nullptr;

                delete received_message;
            }
        }
    }
    return 0;
}