#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock == -1)
    {
        perror("Erro ao criar socket");
        return -1;
    }

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(0);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr); // IP do PC 2

    char packet[100] = "Mensagem do PC 1 para o PC 2";
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(dest)) == -1)
    {
        perror("Erro ao enviar pacote");
        return -1;
    }

    printf("Pacote enviado!\n");
    close(sock);
    return 0;
}