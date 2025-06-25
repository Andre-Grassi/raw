#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include <string.h>

// filepath: /home/andre/Programming/Materias/redes/raw/raw_sockets/objetos
std::string get_file_by_index(const std::string &dir_path, int index)
{
    DIR *dir = opendir(dir_path.c_str());
    if (!dir)
    {
        perror("opendir");
        return "";
    }
    std::vector<std::string> files;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        // Ignora "." e ".."
        if (entry->d_name[0] == '.')
            continue;
        files.push_back(entry->d_name);
    }
    closedir(dir);

    if (index < 0 || index >= (int)files.size())
    {
        std::cerr << "Invalid index\n";
        return "";
    }
    return dir_path + "/" + files[index];
}

// filepath: /home/andre/Programming/Materias/redes/raw/raw_sockets/server.cpp
std::string find_file_with_prefix(const std::string &dir_path, const std::string &prefix)
{
    DIR *dir = opendir(dir_path.c_str());
    if (!dir)
        return "";

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
    return "";
}

int main()
{
    std::string dir_path = "objetos";

    // Digite o índice do arquivo que você deseja obter
    int index;
    std::cout << "Enter file prefix: ";
    std::cin >> index;

    std::string file_path = find_file_with_prefix(dir_path, std::to_string(index + 1));
    if (file_path.empty())
    {
        std::cerr << "Failed to get file by index\n";
        return 1;
    }
    std::cout << "File at index " << index << ": " << file_path << "\n";
    return 0;
}