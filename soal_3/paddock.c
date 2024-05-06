#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include "actions.h"

#define PORT 8080
#define LOGFILE "race.log"

FILE *log_file;

void log_message(const char *source, const char *command, const char *info) {
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    char time_str[20];  // Buffer to hold the timestamp string

    // Format the time in the buffer
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", timeinfo);

    fprintf(log_file, "[%s] [%s]: [%s] [%s]\n", source, time_str, command, info);
    fflush(log_file);
}

void process_driver_message(int sock, const char* command, const char* info) {
    const char *response;
    if (strcmp(command, "Gap") == 0) {
        float gap = atof(info);
        response = check_gap(gap);
    } else if (strcmp(command, "Fuel") == 0) {
        float fuel = atof(info);
        response = check_fuel(fuel);
    } else if (strcmp(command, "Tire") == 0) {
        int tire = atoi(info);
        response = check_tire(tire);
    } else if (strcmp(command, "Tire Change") == 0) {
        response = check_tire_change(info);
    } else {
        response = "Invalid command";
    }

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    char time_str[20];  // Buffer to hold the timestamp string
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", timeinfo);

    char buffer[2048] = {0};
    sprintf(buffer, "[%s] [%s]: [%s] [%s]\n", "Paddock", time_str, command, response);  // Replace "time" with the current time
    send(sock, buffer, strlen(buffer), 0);
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    log_file = fopen(LOGFILE, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    pid_t sid = setsid();
    if (sid < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Change the current working directory
    if ((chdir("/")) < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char command[1024] = {0};
        char info[1024] = {0};

        // Baca perintah dari socket
        int bytes_read = read(new_socket, command, sizeof(command));
        if (bytes_read < 0) {
            perror("Read command failed");
            close(new_socket);
            continue;
        }

        // Hapus kurung siku dari perintah
        sscanf(command, "[%[^]]]", command);

        // Baca info dari socket
        bytes_read = read(new_socket, info, sizeof(info));
        if (bytes_read < 0) {
            perror("Read info failed");
            close(new_socket);
            continue;
        }

        // Hapus kurung siku dari info
        sscanf(info, "[%[^]]]", info);

        log_message("Driver", command, info);
        process_driver_message(new_socket, command, info);
    }

    return 0;
}
