#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>

#include "network.hpp"

uint64_t timestamp()
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

    // Copia os dados
    // Isso é necessário para pois ao deletar a mensagem, o ponteiro
    // data poderia ser liberado sem querer
    if (data)
    {
        this->data = new uint8_t[size];
        std::memcpy(this->data, data, size);
    }
    else
        this->data = nullptr;

    calculate_checksum();
}

Message::~Message()
{
    if (data)
    {
        delete[] data;
        data = nullptr;
    }
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
        fprintf(stderr, "Error creating socket: Make sure you are running as root!\n");
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
        fprintf(stderr, "Error binding socket.\n");
        exit(-1);
    }

    struct packet_mreq mr;
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        fprintf(stderr, "Error setting setsockopt: Check if the network interface was specified correctly.\n");
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

Network::~Network()
{
    if (my_socket.socket_fd != -1)
    {
        if (close(my_socket.socket_fd) == -1)
            fprintf(stderr, "Error when closing socket.\n");

        my_socket.socket_fd = -1;
    }
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

Message *Network::send_message(Message *message)
{
    int32_t sent_bytes = -1;
    Message *return_message = nullptr;
    bool error = false;
    do
    {
        error = false;

        sent_bytes = send_message_aux(this, message);

        if (sent_bytes == -1)
            error = true;

        else if (message->type != ACK && message->type != NACK)
        {
            // TODO faltando loop de timeout aqui???
            uint64_t comeco = timestamp();
            struct timeval timeout = {.tv_sec = TIMEOUT_MS / 1000, .tv_usec = (TIMEOUT_MS % 1000) * 1000};
            setsockopt(my_socket.socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

            error_type error_t = receive_message(return_message, true);

            if (error_t == TIMED_OUT || error_t == BROKEN)
                error = true;
            else if (return_message->type == NACK)
            {
                error = true;
                delete return_message;
                return_message = nullptr;
            }
        }
    } while (error);

    my_sequence++;
    return return_message;
}

error_type Network::receive_message(Message *&returned_message, bool is_waiting_response)
{
    uint8_t *received_package = new uint8_t[METADATA_SIZE + MAX_DATA_SIZE + 10];
    ssize_t received_bytes;
    uint8_t start_delimiter;
    uint32_t metadata_package;

    // Espera até receber um pacote com tamanho mínimo e início válido
    bool error;
    do
    {
        uint64_t start = timestamp();
        struct timeval timeout = {.tv_sec = TIMEOUT_MS / 1000, .tv_usec = (TIMEOUT_MS % 1000) * 1000};
        setsockopt(my_socket.socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

        received_bytes = recv(this->my_socket.socket_fd, received_package, METADATA_SIZE + MAX_DATA_SIZE + 10, 0);

        uint64_t elapsed_time = timestamp() - start;

        if (is_waiting_response && elapsed_time > TIMEOUT_MS)
        {
#ifdef VERBOSE
            fprintf(stderr, "Timed out while waiting for answer.\n");
#endif
            return error_type::TIMED_OUT;
        }

#ifdef VERBOSE
        printf("Received bytes: %zd\n", received_bytes);
#endif
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

    uint8_t i1 = sequence;
    uint8_t i2 = other_sequence;
    int8_t distance = (signed)((i1 << 3) - (i2 << 3));

    if (distance > 0) // received sequence > expected sequence
    {
        fprintf(stderr, "Unexpected sequence: expected %d, received %d\n", other_sequence, sequence);
        delete[] received_package;
        if (is_waiting_response)
            return error_type::BROKEN;
        else
        {
            Message *nack_message = new Message(0, my_sequence, NACK, NULL);
            send_message(nack_message);
            delete nack_message;
            return receive_message(returned_message, false); // Chama novamente para receber a mensagem de volta
        }
    }

    else if (distance < 0) // received sequence < expected sequence
    {
#ifdef VERBOSE
        fprintf(stderr, "Old message received, ignoring...");
#endif

        // Mensagem antiga recebida, ignora
        // Envia ACK denovo
        my_sequence--;
        Message *ack_message = new Message(0, my_sequence, ACK, NULL);
        send_message(ack_message);
        delete ack_message;
        return receive_message(returned_message, false); // Chama novamente para receber a próxima mensagem
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
        fprintf(stderr, "Invalid checksum: expected %d, received %d\n", message->checksum, checksum_original);
        if (!is_waiting_response)
        {
            Message *nack_message = new Message(0, my_sequence, NACK, NULL);
            send_message(nack_message);
            delete nack_message;
        }
        delete[] received_package;
        delete message;
        return receive_message(returned_message, false); // Chama novamente para receber a próxima mensagem
    }

    other_sequence++; // Atualiza a sequência do outro lado

    returned_message = message; // Atribui a mensagem recebida ao ponteiro passado

    // Libera o buffer de recebimento
    delete[] received_package;

    return error_type::NO_ERROR; // Retorna a mensagem recebida com sucesso
}