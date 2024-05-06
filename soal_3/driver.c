#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char *command = NULL;
    char *info = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            command = argv[i + 1];
            i++;  // Skip the next argument
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            info = argv[i + 1];
            i++;  // Skip the next argument
        }
    }

    // Membuat pesan dengan perintah dan info diapit oleh kurung siku
    char command_message[2048] = {0};
    sprintf(command_message, "[%s]", command);
    send(sock , command_message , strlen(command_message) , 0 );

    char info_message[2048] = {0};
    sprintf(info_message, "[%s]", info);
    send(sock , info_message , strlen(info_message) , 0 );

    printf("Command sent: %s %s\n", command_message, info_message);

    valread = read(sock , buffer, BUFFER_SIZE);
    printf("Received message: %s\n", buffer);

    return 0;
}
