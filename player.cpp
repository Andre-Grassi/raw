#include "network.hpp"
#include "game.hpp"
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>     // Para pathconf (opcional, mas comum com statvfs)

int main(int argc, char *argv[])
{
    srand(0);

    Network net = Network("enp0s31f6");

    Map map = Map(true); // Player mode

    map.print();

    bool end = false;
    bool found_treasure = false;
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
            {
                printf("Invalid input. Use only 'w', 'a', 's', or 'd'.\n");
                continue;
            }
            if (is_valid_move)
            {
                Message message = Message(0, net.my_sequence, move, NULL);
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
                    
                    //Para verificar quanto espaço livre uma máquina tem (lembre-se também de ter uma certa tolerância), use a função statvfs (utilizando como caminho, onde se pretende salvar os arquivos que forem recebidos). O espaço livre em bytes é dado por st.f_bsize * st.f_bavail, onde st é a estrutura statvfs.
                    //Para descobrir que o arquivo é regular e o tamanho do arquivo, utilize a função stat. O tamanho do arquivo em bytes pode ser encontrado em st.st_size onde st é a estrutura stat.
                    struct statvfs st;
                    statvfs(TREASURE_DIR, &st);
                    printf("Free space available: %lu bytes\n", st.f_bsize * st.f_bavail);
                    uint64_t free_space = st.f_bsize * st.f_bavail + SIZE_TOLERANCE;


                    treasure->size = size;

                    printf("File size %llu\n", size);

                    if (size > free_space)
                    {
                        printf("Treasure size (%llu bytes) exceeds available space (%lu bytes). Sending TOO_BIG message.\n", size, free_space);
                        Message too_big_message = Message(0, net.my_sequence, TOO_BIG, NULL);
                        net.send_message(&too_big_message);
                        found_treasure = false;
                        continue;
                        break;
                    }
                    
                    treasure->data = new uint8_t[size];
                    #ifdef VERBOSE
                    printf("Treasure size: %llu bytes\n", size);
                    #endif
                }
                break;
            }
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
                    if (returned_message->data[i] == 0x88 || returned_message->data[i] == 0x81)
                    {
                        i++; // Skip the next byte
                    }
                    j++;
                }

                size_t writed = fwrite(buffer, 1, j, treasure->file);
                delete[] buffer;
#ifdef VERBOSE
                printf("Wrote %zu bytes to treasure file.\n", writed);
#endif
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
            Message ack_message = Message(0, net.my_sequence, ACK, NULL);
            net.send_message(&ack_message);
        }

        delete returned_message;
    }
    return 0;
}