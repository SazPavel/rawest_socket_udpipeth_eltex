#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    char buf[16];
    char buf2[] = "priv\n";
    int sock;
    struct sockaddr_in addr, client;
    socklen_t size = sizeof(client);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3333);
    addr.sin_addr.s_addr = inet_addr("10.0.2.4");
    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(-1);
    }
    recvfrom(sock, buf, 16, 0, (struct sockaddr*)&client, &size);
    printf("SERVER: %s\n", buf);
    sendto(sock, buf2, sizeof(buf2), 0, (struct sockaddr *)&client, size);
    close(sock);
    exit(0);
}
