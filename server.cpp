#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <cmath>
#include <string.h>

int main(int argc, char *argv[])
{
    srand(0);

    Network net = Network("enp3s0");

    Map map = Map(false); // Server mode

    map.print();

    bool end = false;
    bool can_move = true;
    bool is_sending_treasure = false;
    uint8_t sequence = 0;
    while (!end)
    {
        uint8_t treasure_index;
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
                    map.move_player((message_type)received_message->type);

                    // Testa se player encontrou um tesouro
                    for (int i = 0; i < NUM_TREASURES; ++i)
                    {
                        if (map.player_position == map.treasures[i].position && !(map.treasures[i].found))
                        {
                            map.treasures[i].found = true; // Mark treasure as found
                            is_sending_treasure = true;
                            treasure_index = i;
                            printf("Player found a treasure at (%d, %d)!\n", map.treasures[i].position.x, map.treasures[i].position.y);
                            break;
                        }
                    }

                    if (!is_sending_treasure)
                    {
                        printf("Player moved. Sending ACK.\n");
                        map.print();
                        Message ack_message = Message(0, sequence, ACK, NULL);
                        int32_t sent_bytes = net.send_message(&ack_message);
                        if (sent_bytes == -1)
                            perror("Error sending ACK");
                        else
                            sequence++;
                        break;
                    }

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
        if (is_sending_treasure)
        {
            // Server logic: send treasure to player
            printf("Sending treasure to player...\n");

            std::string name = TREASURE_DIR + std::to_string(treasure_index + 1) + ".txt";
            Treasure *treasure = new Treasure(name, false);

            Message ack_treasure = Message(treasure->filename.size(), sequence, TXT_ACK_NAME, treasure->filename_data);
            int32_t sent_bytes = net.send_message(&ack_treasure);
            if (sent_bytes == -1)
                perror("Error sending ack treasure");
            else
                sequence++;

            // Espera ACK do jogador
            Message *received_message = net.receive_message();
            if (received_message)
            {
                if (received_message->type == ACK)
                {
                    // Envia mensagem com o tamanho do arquivo
                    printf("ACK received. Sending treasure file size: %ld.\n", treasure->size);
                    Message size_message = Message((uint8_t)8, sequence, DATA_SIZE, (uint8_t *)&treasure->size);
                    int32_t sent_size_bytes = net.send_message(&size_message);

                    // Espera ACK do jogador
                    received_message = net.receive_message();
                    if (received_message && received_message->type == ACK)
                    {
                        sequence++;

                        // Envia o arquivo do tesouro
                        puts("ACK received. Sending treasure file data...");

                        uint32_t num_messages = std::ceil((double)treasure->size / MAX_DATA_SIZE);
                        uint8_t *data_chunk = new uint8_t[MAX_DATA_SIZE];
                        for (int i = 0; i < num_messages; i++)
                        {
                            uint8_t chunk_size = std::min((size_t)MAX_DATA_SIZE, (size_t)(treasure->size - (i * MAX_DATA_SIZE)));
                            memcpy(data_chunk, treasure->data + (i * MAX_DATA_SIZE), chunk_size);

                            Message treasure_message = Message(chunk_size, sequence, DATA, data_chunk);
                            int32_t sent_file_bytes = net.send_message(&treasure_message);
                            if (sent_file_bytes == -1)
                                perror("Error sending treasure file");
                            else
                                sequence++;

                            // Espera ACK do jogador
                            received_message = net.receive_message();
                            if (received_message && received_message->type == ACK)
                            {
                                printf("ACK received for chunk %d.\n", i + 1);
                            }
                            else
                            {
                                printf("NACK received or no response for chunk %d. Retrying...\n", i + 1);
                                i--; // Reenviar o mesmo chunk
                            }
                        }

                        // Envia mensagem de fim de transmissão
                        received_message = nullptr;
                        do
                        {
                            int32_t sent_end_bytes = -1;
                            do
                            {
                                Message end_message = Message(0, sequence, END, NULL);
                                sent_end_bytes = net.send_message(&end_message);

                                if (sent_end_bytes == -1)
                                    perror("Error sending end message");

                                else
                                    printf("End message sent.\n");

                            } while (sent_end_bytes == -1);
                            sequence++;

                            // Espera ACK do jogador
                            received_message = net.receive_message();
                            if (!received_message)
                                sequence--; // No response, retry
                        } while (!received_message || received_message->type != ACK);
                        printf("ACK received for end message. Treasure transfer complete.\n");
                        delete[] data_chunk;
                        delete treasure;
                        is_sending_treasure = false; // Reset sending state
                    }
                }
            }
        }
    }
    return 0;
}