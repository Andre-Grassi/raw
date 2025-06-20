#include "network.hpp"
#include <stdio.h>
#include <string.h>
#include <bitset>

using std::bitset;

struct teste
{
    uint8_t marcador_inicio;
    uint8_t tamanho : 7;
    uint8_t sequencia : 5;
    uint8_t tipo : 4;
    uint8_t checksum;
    uint8_t *data;
};

int main()
{

    uint8_t buffer_data[1] = {0b11111111};
    uint8_t tamanho_buffer = sizeof(buffer_data);

    Network net = Network("vethad51af4", "vethad51af4");

    Message *msg = net.receive_message();

    return 0;
}