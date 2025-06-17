#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>

#include "network.hpp"

Message::Message(uint8_t size, uint8_t sequence, uint8_t type, uint8_t *data)
{
    this->size = size;
    this->sequence = sequence;
    this->type = type;
    this->data = data;
    this->checksum = 0;
}

// Parity word checksum
void Message::calculate_checksum()
{
    checksum = 0;
    checksum ^= start_delimiter;
    checksum ^= size;
    checksum ^= sequence;
    checksum ^= type;

    for (uint8_t i = 0; i < size; ++i)
    {
        checksum ^= data[i];
    }
}

int cria_raw_socket(char *nome_interface_rede)
{
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1)
    {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }

    int ifindex = if_nametoindex(nome_interface_rede);

    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        fprintf(stderr, "Erro ao fazer setsockopt: "
                        "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }

    return soquete;
}

Network::Network(char *my_interface_name, char *other_interface_name)
{
    my_socket.socket_fd = cria_raw_socket(my_interface_name);
    my_socket.interface_name = my_interface_name;

    other_socket.socket_fd = cria_raw_socket(other_interface_name);
    other_socket.interface_name = other_interface_name;
}

uint Network::send_message(Message *message)
{
    // Empacota os metadados em uma variável de 32 bits
    uint32_t metadata_package = 0;

    metadata_package |= (uint32_t)message->start_delimiter << 24;
    metadata_package |= (uint32_t)message->size << 17;
    metadata_package |= (uint32_t)message->sequence << 12;
    metadata_package |= (uint32_t)message->type << 8;
    metadata_package |= (uint32_t)message->checksum;

    return 1;
}