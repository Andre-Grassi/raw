#include "network.hpp"
#include <stdio.h>
#include <string.h>
#include <bitset>
#include "game.hpp"
#include <string>
#include <iostream>
#include <cmath>
#include <dirent.h>

using std::bitset;

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

int main()
{
    int treasure_index = 0; // Index of the treasure to be sent
    Network *net = new Network("enp3s0");
    // Obtém o nome do tesouro baseado no índice do tesouro encontrado
    std::string prefix = std::to_string(treasure_index + 1);

    // Pega o nome do arquivo do tesouro com base no prefixo, já que
    // a terminação é variável (pode ser .txt, .mp4 e .jpg)
    std::string name = find_file_with_prefix(TREASURE_DIR, prefix);

    // Pega o sufixo do arquivo

    Treasure *treasure = new Treasure(name, false);

    // Size + 1 to include null terminator
    Message ack_treasure = Message(treasure->filename.size() + 1, net->my_sequence, TXT_ACK_NAME, treasure->filename_data);
    int32_t sent_bytes = net->send_message(&ack_treasure);
}