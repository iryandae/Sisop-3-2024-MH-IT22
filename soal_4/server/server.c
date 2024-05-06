// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

void downloadcsv()
{
    system("rm -f ../myanimelist.csv");
    system("wget --content-disposition --no-check-certificate \"https://drive.google.com/uc?export=download&id=10p_kzuOgaFY3WT6FVPJIXFbkej2s9f50\" -P ../");
}

void display_csv_content() {
    FILE *file = fopen("../myanimelist.csv", "r");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}

void handle_client(int client_socket);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    downloadcsv();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
  
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        printf("Client connected.\n");

        handle_client(client_socket);

        printf("Client disconnected.\n");
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket) {
    char buffer[MAX_BUFFER_SIZE] = {0};
    int valread;

    while (1) {
        valread = read(client_socket, buffer, MAX_BUFFER_SIZE);
        if (valread == 0) {
            printf("Client disconnected unexpectedly.\n");
            break;
        } else if (valread < 0) {
            perror("Reading failed");
            break;
        }

        printf("Received command: %s\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("Client requested to exit.\n");
            break;
        }

        if (strcmp(buffer, "tampilkan") == 0) {
            printf("Displaying CSV content:\n");
            display_csv_content();
            write(client_socket, "CSV content displayed.\n", strlen("CSV content displayed.\n"));
        } else {
            write(client_socket, buffer, strlen(buffer));
        }

        memset(buffer, 0, sizeof(buffer));
    }
}
