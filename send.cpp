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

    teste meu_pacote;
    meu_pacote.marcador_inicio = 0b01111110;
    meu_pacote.tamanho = tamanho_buffer; // Tamanho do buffer em 7 bits
    meu_pacote.sequencia = 0b00000;      // Sequência de 5 bits
    meu_pacote.tipo = 0b1111;            // Tipo de mensagem de 4 bits
    meu_pacote.checksum = 0b00000000;    // Checksum de 8 bits, pode ser calculado depois
    meu_pacote.data = buffer_data;

    Network net = Network("vethad51af4", "vethad51af4");

    Message msg = Message(meu_pacote.tamanho, meu_pacote.sequencia, meu_pacote.tipo, buffer_data);

    printf("Enviando msg: %d\n", net.send_message(&msg));

    return 0;
}