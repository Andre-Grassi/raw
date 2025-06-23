#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>

#define METADATA_SIZE 4
#define MAX_DATA_SIZE 127
#define TIMEOUT_MS 1000
#define VERBOSE

#define BROKEN_MESSAGE nullptr
#define TIMED_OUT_MSG nullptr

enum message_type
{
    ACK,
    NACK,
    OK_ACK,
    TOO_BIG,
    DATA_SIZE,
    DATA,
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

enum error_type
{
    NO_ERROR,
    BROKEN,
    TIMED_OUT,
    OLD
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
    uint8_t my_sequence;
    uint8_t other_sequence;
    Network(char *my_interface_name);

    int32_t send_message(Message *message);
    error_type receive_message(Message *&returned_message, bool is_waiting_response = false);
};

#endif