#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    srand(0);

    Network net = Network();

    Map map = Map(true); // Player mode

    map.print();

    bool end = false;
    bool found_treasure = false;
    uint8_t num_found_treasures = 0;
    Treasure *treasure = nullptr;
    while (!end)
    {
        Message *returned_message = nullptr;
        if (!found_treasure)
        {
            printf("Enter your move (w/a/s/d): ");

            // Lê do teclado apenas WASD
            char input;
            bool is_valid_move = false;
            message_type move;
            do
            {
                std::cin >> input;

                is_valid_move = false;

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
                {
                    printf("Invalid input. Use only 'w', 'a', 's', or 'd'.\n");
                    continue;
                }

                if (!is_valid_move)
                    puts("Can't move in that direction. Try another one.");
            } while (!is_valid_move);

#ifndef VERBOSE
            // Limpa a tela com chamada ao sistema
            system("clear");
#endif

            map.print();
            Message message = Message(0, net.my_sequence, move, NULL);
            returned_message = net.send_message(&message);
            switch (returned_message->type)
            {
            case ACK:
                printf("ACK received. You can move again.\n");
                break;
            case TXT_ACK_NAME:
            case VID_ACK_NAME:
            case IMG_ACK_NAME:
            case NON_REGULAR_ACK:
                printf("Received a treasure message.\n");
                puts("Found a treasure!");
                found_treasure = true;
                num_found_treasures++;
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
            case NON_REGULAR_ACK:
                printf("Received a non-regular treasure message.\n");
                found_treasure = false;

                // Checa se achou todos os tesouros
                if (num_found_treasures == NUM_TREASURES)
                {
                    puts("All treasures found! Ending game.");
                    end = true;
                }

                delete treasure;
                treasure = nullptr;

                break;
            case DATA_SIZE:
            {
                if (treasure)
                {
                    // Recebe o tamanho do tesouro
                    uint64_t size = *((uint64_t *)returned_message->data);

                    // Calcula o espaço livre disponível
                    struct statvfs st;
                    statvfs(TREASURE_DIR, &st);

#ifdef VERBOSE
                    printf("Free space available: %lu bytes\n", st.f_bsize * st.f_bavail);
#endif
                    uint64_t free_space = st.f_bsize * st.f_bavail + SIZE_TOLERANCE;

                    treasure->size = size;

                    printf("Treasure size: %lu bytes\n", size);

                    if (size > free_space)
                    {
                        printf("Treasure size (%lu bytes) exceeds available space (%lu bytes). Sending TOO_BIG message.\n", size, free_space);
                        Message too_big_message = Message(0, net.my_sequence, TOO_BIG, NULL);
                        net.send_message(&too_big_message);
                        found_treasure = false;
                        // Checa se achou todos os tesouros
                        if (num_found_treasures == NUM_TREASURES)
                        {
                            puts("All treasures found! Ending game.");
                            end = true;
                        }

                        delete treasure;
                        treasure = nullptr;

                        continue;
                    }

                    treasure->data = new uint8_t[size];
                }
                break;
            }
            case TOO_BIG:
                printf("Received TOO_BIG message. Treasure cannot be received.\n");
                found_treasure = false;

                // Checa se achou todos os tesouros
                if (num_found_treasures == NUM_TREASURES)
                {
                    puts("All treasures found! Ending game.");
                    end = true;
                }

                delete treasure;
                treasure = nullptr;

                break;
            case DATA:
            {
#ifdef VERBOSE
                puts("Writing data...");
#endif
                uint8_t chunk_size = returned_message->size;
                uint8_t *buffer = new uint8_t[chunk_size];
                size_t j = 0;
                for (size_t i = 0; i < chunk_size; i++)
                {
                    buffer[j] = returned_message->data[i];

                    // Se for byte proibido, pula o byte de stuffing
                    if (returned_message->data[i] == FORBIDDEN_BYTE_1 || returned_message->data[i] == FORBIDDEN_BYTE_2)
                        i++;

                    j++;
                }

#ifdef VERBOSE
                size_t bytes_written = fwrite(buffer, 1, j, treasure->file);
                printf("Wrote %zu bytes to treasure file.\n", bytes_written);
#else
                fwrite(buffer, 1, j, treasure->file);
#endif

                delete[] buffer;
                break;
            }
            case END:
            {
                fclose(treasure->file);
                printf("Treasure %s received successfully!\n", treasure->filename.c_str());
                found_treasure = false;

                // Obtém o sufixo do nome do tesouro e o abre conforme o tipo
                std::string suffix = treasure->filename.substr(treasure->filename.find_last_of('.'));
                if (suffix == ".txt")
                {
                    std::string command = "less \"" + treasure->filename + "\" 2>/dev/null";
                    system(command.c_str());
                }
                else if (suffix == ".mp4")
                {
                    std::string command = "celluloid \"" + treasure->filename + "\" 2>/dev/null";
                    system(command.c_str());
                }
                else if (suffix == ".jpg")
                {
                    std::string command = "eog \"" + treasure->filename + "\" 2>/dev/null";
                    system(command.c_str());
                }
                else
                    printf("Unknown treasure type.\n");

                // Checa se achou todos os tesouros
                if (num_found_treasures == NUM_TREASURES)
                {
                    puts("All treasures found! Ending game.");
                    end = true;
                }

                delete treasure;
                treasure = nullptr;

                break;
            }
            }

            // Envia ACK
            Message ack_message = Message(0, net.my_sequence, ACK, NULL);
            net.send_message(&ack_message);
        }

        delete returned_message;
    }
    return 0;
}