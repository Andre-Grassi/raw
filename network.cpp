#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "network.hpp"
#include <sys/time.h>

// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

Message::Message(uint8_t size, uint8_t sequence, uint8_t type, uint8_t *data)
{
    this->size = size;
    this->sequence = sequence;
    this->type = type;
    this->data = data;
    calculate_checksum();
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

    struct sockaddr_ll endereco;
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }

    struct packet_mreq mr;
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

Network::Network(char *my_interface_name)
{
    my_socket.socket_fd = cria_raw_socket(my_interface_name);
    my_socket.interface_name = my_interface_name;
    my_sequence = 0;
    other_sequence = 0;
}

int32_t send_message_aux(Network *net, Message *message)
{
    int32_t sent_bytes = -1;

    uint32_t metadata_package = 0;

    metadata_package |= (uint32_t)message->start_delimiter;
    metadata_package |= (uint32_t)message->size << 8;
    metadata_package |= (uint32_t)message->sequence << 15;
    metadata_package |= (uint32_t)message->type << 20;
    metadata_package |= (uint32_t)message->checksum << 24;

    uint8_t *final_package = new uint8_t[METADATA_SIZE + message->size + 10];
    std::memset(final_package, 0, METADATA_SIZE + message->size + 10);

    // Copia os 4 bytes do pacote em 32 bits para o pacote final
    for (size_t i = 0; i < METADATA_SIZE; i++)
    {
        final_package[i] = (metadata_package >> (i * 8)) & 0xFF;
    }

    // Copia os dados do buffer para o pacote final
    for (size_t i = 0; i < message->size; i++)
    {
        final_package[i + METADATA_SIZE] = message->data[i];
    }

#ifdef VERBOSE
    puts("Sending message:");
    // Imprime os bits do pacote final
    for (size_t i = 0; i < (uint8_t)(METADATA_SIZE + message->size); i++)
    {
        printf("Byte %zu: ", i);
        for (int j = 7; j >= 0; --j)
        {
            printf("%d", (final_package[i] >> j) & 1);
        }
        printf("\n");
    }
    puts("");
#endif

    sent_bytes = send(net->my_socket.socket_fd, final_package, METADATA_SIZE + message->size + 10, 0);

    return sent_bytes;
}

int32_t Network::send_message(Message *message)
{
    int32_t sent_bytes = -1;
    Message *return_message;
    bool error = false;
    do
    {
        error = false;
        sent_bytes = send_message_aux(this, message);
        if (sent_bytes == -1)
            error = true;

        else if (message->type != ACK && message->type != NACK)
        {
            long long comeco = timestamp();
            struct timeval timeout = {.tv_sec = TIMEOUT_MS / 1000, .tv_usec = (TIMEOUT_MS % 1000) * 1000};
            setsockopt(my_socket.socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
            long long elapsed_time = 0;

            error_type error_t = receive_message(return_message);

            elapsed_time = timestamp() - comeco;

            if (elapsed_time > TIMEOUT_MS || error_t == BROKEN || return_message->type == NACK)
                error = true;
        }
    } while (error);

    my_sequence++;
    return sent_bytes;
}

error_type Network::receive_message(Message *returned_message)
{
    uint8_t *received_package = new uint8_t[METADATA_SIZE + MAX_DATA_SIZE + 10];
    ssize_t received_bytes;
    uint8_t start_delimiter;
    uint32_t metadata_package;

    // Espera até receber um pacote com tamanho mínimo e início válido
    bool error;
    do
    {
        received_bytes = recv(this->my_socket.socket_fd, received_package, METADATA_SIZE + MAX_DATA_SIZE + 10, 0);

        if (received_bytes >= METADATA_SIZE)
        {
            // Extrai os metadados do pacote recebido
            metadata_package = 0;

            for (size_t i = 0; i < METADATA_SIZE; i++)
            {
                metadata_package |= (received_package[i] << (i * 8));
            }

            start_delimiter = metadata_package & 0xFF;
        }

        error = (received_bytes < METADATA_SIZE || start_delimiter != 0b01111110);
    } while (error);

    uint8_t size = (metadata_package >> 8) & 0x7F;      // 7 bits
    uint8_t sequence = (metadata_package >> 15) & 0x1F; // 5 bits

    // Checa se é a sequência esperada
    if (sequence > other_sequence)
    {
        fprintf(stderr, "Sequência inesperada: esperado %d, recebido %d\n", other_sequence, sequence);
        delete[] received_package;
        return error_type::BROKEN; // Retorna nullptr se a sequência não for a esperada
    }
    else if (sequence < other_sequence)
    {
        // Mensagem antiga recebida, ignora
        return error_type::OLD;
    }

    uint8_t type = (metadata_package >> 20) & 0x0F;              // 4 bits
    uint8_t checksum_original = (metadata_package >> 24) & 0xFF; // 8 bits

    uint8_t *data = new uint8_t[size];

    // Copia os dados do pacote recebido para o buffer de dados
    for (size_t i = 0; i < size; i++)
        data[i] = received_package[i + METADATA_SIZE];

    // Imprime os bits do metadata_package
#ifdef VERBOSE
    puts("Received message:");
    printf("Metadata Package: ");
    for (int i = 0; i < 32; i++)
    {
        if (i % 8 == 0)
            printf("\nByte %d: ", i / 8);
        printf("%d", (metadata_package >> i) & 1);
    }
    printf("\n");

    // Imprime bits de start_delimiter
    printf("Start Delimiter: ");
    for (int i = 7; i >= 0; --i)
    {
        printf("%d", (start_delimiter >> i) & 1);
    }

    // Imprime bits de size
    printf("\nSize: ");
    for (int i = 6; i >= 0; --i)
    {
        printf("%d", (size >> i) & 1);
    }

    // Imprime bits de sequence
    printf("\nSequence: ");
    for (int i = 4; i >= 0; --i)
    {
        printf("%d", (sequence >> i) & 1);
    }

    // Imprime bits de type
    printf("\nType: ");
    for (int i = 3; i >= 0; --i)
    {
        printf("%d", (type >> i) & 1);
    }

    // Imprime bits de checksum
    printf("\nChecksum: ");
    for (int i = 7; i >= 0; --i)
    {
        printf("%d", (checksum_original >> i) & 1);
    }
    printf("\n");

    // Imprime os bits do buffer de dados byte a byte
    printf("Data: ");
    for (size_t i = 0; i < size; i++)
    {
        printf("\nByte %zu: ", i);
        for (int j = 7; j >= 0; --j)
        {
            printf("%d", (data[i] >> j) & 1);
        }
    }
    printf("\n");
#endif

    // Cria e retorna a mensagem recebida
    Message *message = new Message(size, sequence, type, data);

    // Valida o checksum recebido com o calculado
    if (checksum_original != message->checksum)
    {
        fprintf(stderr, "Checksum inválido: esperado %d, recebido %d\n", message->checksum, checksum_original);
        delete[] received_package;
        delete message;
        return error_type::BROKEN; // Retorna nullptr se o checksum não bater
    }

    other_sequence++; // Atualiza a sequência do outro lado

    returned_message = message; // Atribui a mensagem recebida ao ponteiro passado

    // Libera o buffer de recebimento
    delete[] received_package;

    return error_type::NO_ERROR; // Retorna a mensagem recebida com sucesso
}