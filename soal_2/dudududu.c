#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h> // Library tambahan untuk waktu

char *numbers[] = {"nol", "satu", "dua", "tiga", "empat", "lima", "enam", "tujuh", "delapan", "sembilan"};

void convertToNumber(char *input, int *result) {
    char *token = strtok(input, " ");
    int i = 0;
    while (token != NULL) {
        for (int j = 0; j < 10; j++) {
            if (strcmp(token, numbers[j]) == 0) {
                result[i++] = j;
                break;
            }
        }
        token = strtok(NULL, " ");
    }
}

void calculate(char *operation, int a, int b, char *result) {
    int res;
    if (strcmp(operation, "-kali") == 0) {
        res = a * b;
    } else if (strcmp(operation, "-tambah") == 0) {
        res = a + b;
    } else if (strcmp(operation, "-kurang") == 0) {
        res = a - b;
    } else if (strcmp(operation, "-bagi") == 0) {
        if (b == 0) {
            strcpy(result, "ERROR");
            return;
        }
        res = a / b;
    }
    sprintf(result, "%d", res);
}

void convertToWords(int number, char *result) {
    if (number < 0) {
        strcpy(result, "ERROR");
        return;
    }

    if (number <= 9) {
        strcpy(result, numbers[number]);
    } else if (number <= 19) {
        switch (number) {
            case 10: strcpy(result, "sepuluh"); break;
            case 11: strcpy(result, "sebelas"); break;
            default:
                sprintf(result, "%s belas", numbers[number % 10]);
        }
    } else if (number <= 99) {
        sprintf(result, "%s puluh %s", numbers[number / 10], (number % 10 == 0) ? "" : numbers[number % 10]);
    } else {
        strcpy(result, "ERROR");
    }
}

void getTime(char *timeString) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

int main(int argc, char *argv[]) {
    if (argc != 2 || (strcmp(argv[1], "-kali") != 0 && strcmp(argv[1], "-tambah") != 0 && strcmp(argv[1], "-kurang") != 0 && strcmp(argv[1], "-bagi") != 0)) {
        printf("Usage: %s [operation]\n", argv[0]);
        printf("Operations:\n");
        printf("-kali\t: multiplication\n");
        printf("-tambah\t: addition\n");
        printf("-kurang\t: subtraction\n");
        printf("-bagi\t: division\n");
        return 1;
    }

    char input[100];
    printf("Masukkan dua angka (dalam kata, pisahkan dengan spasi): ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';

    int numbers[2];
    convertToNumber(input, numbers);

    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        perror("Pipe failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (pid > 0) { // Parent process
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        char result[100];
        calculate(argv[1], numbers[0], numbers[1], result);
        write(pipe_parent_to_child[1], result, strlen(result) + 1);

        char sentence[100];
        read(pipe_child_to_parent[0], sentence, sizeof(sentence));
        printf("Hasil %s dari %d dan %d adalah %s.\n", (strcmp(argv[1], "-kali") == 0) ? "perkalian" :
                                                        (strcmp(argv[1], "-tambah") == 0) ? "penjumlahan" :
                                                        (strcmp(argv[1], "-kurang") == 0) ? "pengurangan" : "pembagian",
                                                        numbers[0], numbers[1], sentence);

        FILE *fp = fopen("histori.log", "a");
        if (fp != NULL) {
            char log_message[200];
            char timeString[20];
            getTime(timeString);
            if (strcmp(result, "ERROR") == 0) {
                sprintf(log_message, "[%s] [%s] ERROR pada %s.\n", timeString, (strcmp(argv[1], "-kali") == 0) ? "KALI" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "TAMBAH" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "KURANG" : "BAGI",
                                                                    (strcmp(argv[1], "-kali") == 0) ? "perkalian" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "penjumlahan" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "pengurangan" : "pembagian");
            } else {
                sprintf(log_message, "[%s] [%s] Hasil %s dari %d dan %d adalah %s.\n", timeString, (strcmp(argv[1], "-kali") == 0) ? "KALI" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "TAMBAH" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "KURANG" : "BAGI",
                                                                    (strcmp(argv[1], "-kali") == 0) ? "perkalian" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "penjumlahan" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "pengurangan" : "pembagian",
                                                                    numbers[0], numbers[1], sentence);
            }
            fputs(log_message, fp);
            fclose(fp);
        }

        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);
    } else { // Child process
        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);

        char result[100];
        read(pipe_parent_to_child[0], result, sizeof(result));

        char sentence[100];
        convertToWords(atoi(result), sentence);

        write(pipe_child_to_parent[1], sentence, strlen(sentence) + 1);

        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        exit(0);
    }

    return 0;
}
