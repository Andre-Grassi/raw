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
    Network *net = new Network("enp2s0");

    Message *received_message = net->receive_message();
    // Envia nack
    net->send_message(new Message(0, net->my_sequence, NACK, NULL));

    received_message = net->receive_message();
    net->send_message(new Message(0, net->my_sequence, ACK, NULL));
}