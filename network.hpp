#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>

enum message_type
{
    ACK,
    NACK,
    OK_ACK,
    UNKNOWN,
    DATA_SIZE,
    TXT_ACK_NAME,
    VID_ACK_NAME,
    IMG_ACK_NAME,
    END,
    RIGHT,
    UP,
    DOWN,
    LEFT,
    UNKOWN2,
    ERROR
};

class Message
{
public:
    uint8_t const start_delimiter = 0b01111110;
    uint8_t size : 7;
    uint8_t sequence : 5;
    uint8_t type : 4;
    uint8_t checksum;
    uint8_t *data;

    Message(uint8_t size, uint8_t sequence, uint8_t type, uint8_t *data);

    // Parity word checksum
    void calculate_checksum();
};

class Network
{
public:
    struct socket
    {
        int socket_fd;
        char *interface_name;
    };

    socket my_socket;
    socket other_socket;

    Network(char *my_interface_name, char *other_interface_name);

    uint send_message(Message *message);
    Message *receive_message();
};