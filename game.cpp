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
    size_t tamanho_buffer = sizeof(buffer_data) / sizeof(buffer_data[0]);
    // Coloca cada campo da struct teste dentro de um buffer
    teste meu_pacote = {0b01111110, 0b1111111, 0b00000, 0b1111, 0b00000000, buffer_data};

    uint32_t pacote_em_32bits = 0;

    pacote_em_32bits |= (uint32_t)meu_pacote.marcador_inicio;
    pacote_em_32bits |= (uint32_t)meu_pacote.tamanho << 8;
    pacote_em_32bits |= (uint32_t)meu_pacote.sequencia << 15;
    pacote_em_32bits |= (uint32_t)meu_pacote.tipo << 20;
    pacote_em_32bits |= (uint32_t)meu_pacote.checksum << 24;

    uint8_t *pacote_final = new uint8_t[4 + tamanho_buffer];
    // Copia os 4 bytes do pacote em 32 bits para o pacote final
    for (int i = 0; i < 4; ++i)
    {
        pacote_final[i] = (pacote_em_32bits >> (i * 8)) & 0xFF;
    }
    // Copia os dados do buffer para o pacote final
    for (size_t i = 0; i < tamanho_buffer; ++i)
    {
        pacote_final[4 + i] = buffer_data[i];
    }

    // Imprime os bits do pacote final
    for (size_t i = 0; i < 4 + tamanho_buffer; ++i)
    {
        printf("Byte %zu: ", i);
        for (int j = 7; j >= 0; --j)
        {
            printf("%d", (pacote_final[i] >> j) & 1);
        }
        printf("\n");
    }

    return 0;
}