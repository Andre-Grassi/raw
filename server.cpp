#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <cmath>
#include <string.h>
#include <dirent.h>

// prefix não pode ter o path do diretório
std::string find_file_with_prefix(const std::string &dir_path, const std::string &prefix)
{
    DIR *dir = opendir(dir_path.c_str());
    if (!dir)
        throw std::runtime_error("Could not open directory: " + dir_path);

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_name[0] == '.')
            continue; // Ignora "." e ".."
        if (strncmp(entry->d_name, prefix.c_str(), prefix.size()) == 0)
        {
            closedir(dir);
            return dir_path + "/" + entry->d_name;
        }
    }
    closedir(dir);
    throw std::runtime_error("No file found with prefix: " + prefix);
}

int main(int argc, char *argv[])
{
    srand((time(NULL)));

    Network net = Network("enp3s0");

    Map map = Map(false); // Server mode

    bool end = false;
    bool can_move = true;
    bool is_sending_treasure = false;
    while (!end)
    {
        map.print();
        uint8_t treasure_index;
        if (!is_sending_treasure)
        {
            // Server logic: wait for player move

            Message *received_message;
            net.receive_message(received_message, false);

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
                    Message ack_message = Message(0, net.my_sequence, ACK, NULL);
                    net.send_message(&ack_message);
                }

                break;
            }
            default:
            {
                printf("Received unknown message type. Sending NACK.\n");
                Message nack_message = Message(0, net.my_sequence, NACK, NULL);
                net.send_message(&nack_message);
                break;
            }
            }
            received_message = nullptr;

            delete received_message;
        }
        if (is_sending_treasure)
        {
            // Server logic: send treasure to player
            printf("Sending treasure to player...\n");

            // Obtém o nome do tesouro baseado no índice do tesouro encontrado
            std::string prefix = std::to_string(treasure_index + 1);

            // Pega o nome do arquivo do tesouro com base no prefixo, já que
            // a terminação é variável (pode ser .txt, .mp4 e .jpg)
            std::string name = find_file_with_prefix(TREASURE_DIR, prefix);

            // Pega o sufixo do arquivo

            Treasure *treasure = new Treasure(name, false);

            // Size + 1 to include null terminator
            Message ack_treasure = Message(treasure->filename.size() + 1, net.my_sequence, TXT_ACK_NAME, treasure->filename_data);
            net.send_message(&ack_treasure);

            // Envia mensagem com o tamanho do arquivo
            printf("ACK received. Sending treasure file size: %ld.\n", treasure->size);
            Message size_message = Message((uint8_t)8, net.my_sequence, DATA_SIZE, (uint8_t *)&treasure->size);
            Message *return_message = net.send_message(&size_message);

            if (return_message && return_message->type == TOO_BIG)
            {
                is_sending_treasure = false; // Reset sending state
                // Envia ack
                Message ack_too_big = Message(0, net.my_sequence, ACK, NULL);
                net.send_message(&ack_too_big);

                // Checa se todos os tesouros foram encontrados
                bool all_treasures_found = true;
                size_t i = 0;
                while (i < NUM_TREASURES && all_treasures_found)
                {
                    if (!map.treasures[i].found)
                    {
                        all_treasures_found = false;
                    }
                    i++;
                }

                if (all_treasures_found)
                {
                    puts("All treasures found! Ending game.");
                    end = true;
                }

                delete treasure;
                continue;
            }

            // Envia o arquivo do tesouro
            puts("ACK received. Sending treasure file data...");

            uint8_t *data_chunk = new uint8_t[MAX_DATA_SIZE];
            // Copia treasure->data para buffer com o dobro do seu tamanho
            uint64_t buffer_size = treasure->size * 2;
            uint8_t *buffer = new uint8_t[buffer_size];
            size_t j = 0;
            long int bytes_proibidos = 0;
            long int bytes_extras = 0;
            for (size_t i = 0; i < treasure->size; i++)
            {
                if (treasure->data[i] == 0x88 || treasure->data[i] == 0x81)
                {
                    if (j % 127 == 126)
                        bytes_extras++;

                    buffer[j] = treasure->data[i];
                    j++;
                    buffer[j] = 0b11111111;
                    j++;
                }
                else
                {
                    buffer[j] = treasure->data[i];
                    j++;
                }
            }

            buffer_size = j;
            uint32_t num_messages = std::ceil((double)(buffer_size + bytes_extras) / MAX_DATA_SIZE);
            size_t inicio = 0;

            for (int i = 0; i < num_messages; i++)
            {
                uint8_t chunk_size = std::min((size_t)MAX_DATA_SIZE, (size_t)(buffer_size - inicio));

                // Vê se o último byte é proibido
                if (chunk_size == 127 &&
                    (buffer[inicio + chunk_size - 1] == 0x81 ||
                     buffer[inicio + chunk_size - 1] == 0x88))
                {
                    // Deixa o byte proibido para a próxima mensagem
                    chunk_size--;
                }

                memcpy(data_chunk, buffer + inicio, chunk_size);

                Message treasure_message = Message(chunk_size, net.my_sequence, DATA, data_chunk);
                net.send_message(&treasure_message);

                // Atualiza o próximo início de mensagem
                inicio += chunk_size;
            }

            // Envia mensagem de fim de transmissão
            Message end_message = Message(0, net.my_sequence, END, NULL);
            net.send_message(&end_message);

            delete[] data_chunk;
            delete treasure;

            is_sending_treasure = false; // Reset sending state

            // Checa se todos os tesouros foram encontrados
            bool all_treasures_found = true;
            size_t i = 0;
            while (i < NUM_TREASURES && all_treasures_found)
            {
                if (!map.treasures[i].found)
                {
                    all_treasures_found = false;
                }
                i++;
            }

            if (all_treasures_found)
            {
                puts("All treasures found! Ending game.");
                end = true;
            }
        }
    }
    return 0;
}
