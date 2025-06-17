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
};

int main()
{
    uint8_t buffer_data[100] = {55};
    // Coloca cada campo da struct teste dentro de um buffer
    teste meu_pacote = {0b01111110, 0b1111111, 0b00000, 0b1111, 0b00000000};

    uint32_t pacote_em_32bits = 0;

    pacote_em_32bits |= (uint32_t)meu_pacote.marcador_inicio << 24;
    pacote_em_32bits |= (uint32_t)meu_pacote.tamanho << 17;
    pacote_em_32bits |= (uint32_t)meu_pacote.sequencia << 12;
    pacote_em_32bits |= (uint32_t)meu_pacote.tipo << 8;
    pacote_em_32bits |= (uint32_t)meu_pacote.checksum;

    // Imprime os bits do pacote em 32 bits
    printf("Pacote em 32 bits: ");
    for (int i = 31; i >= 0; --i)
    {
        printf("%d", (pacote_em_32bits >> i) & 1);
    }
    printf("\n");

    return 0;
}