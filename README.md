# Sisop-3-2024-MH-IT22
## Anggota Kelompok
- 5027231003  Chelsea Vania Hariyono
- 5027231024  Furqon Aryadana
- 5027231057  Elgracito Iryanda Endia


## Soal 1
Pada file ```auth.c```
```c
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include <time.h>

int main(){
	
	char *path="/home/user/modul_3/soal_1/new_data/";
	struct dirent *de;
	struct stat buf;
	
	time_t t=time(NULL);
	struct tm *tm_info=localtime(&t);
	char time_str[20];
	strftime(time_str,sizeof(time_str),"%d/%m/%y %H:%M:%S",tm_info);
	
	DIR *dir=opendir(path);
	if(dir==NULL) return 0;
	while((de=readdir(dir))!=NULL){
		char filename[100];
		strcpy(filename,path);
		strcat(filename,de->d_name);
		//condition untuk mengecek nama file yang akan dipindahkan ke shared memory
		if(strstr(de->d_name,"trashcan.csv")!=NULL || strstr(de->d_name,"parkinglot.csv")!=NULL){
			char old_path[200],new_path[200];
			strcpy(old_path,filename);
			
			strcpy(new_path,"/home/user/modul_3/soal_1/microservices/database/");
			strcat(new_path,de->d_name);
			moveFile(old_path,new_path);
			timeRecord(time_str,de->d_name); //fungsi mencatat waktu pemindahan file (untuk keperluan file db.c)
			sharedMemory(new_path); //fungsi shared memory
		} //jika nama file tidak sesuai, akan dihapus
		else{
			removeFile(filename);
		}
	}
	closedir(dir);
	return 0;
}
```
Fungsi ```sharedMemory()``` berisi program untuk menetapkan suatu direktori menjadi sebuah **shared memory**
```c
void sharedMemory(char *filename){
	key_t key=1234;
	int shmid=shmget(key,4096,IPC_CREAT | 0666);
	if(shmid==-1){
		perror("shmget");
		exit(1);
	}
	char *data=shmat(shmid,NULL,0);
	if(data==(char *)(-1)){
		perror("shmat");
		exit(1);
	}
	FILE *file=fopen(filename,"r");
	if(file!=NULL){
		char line[200];
		while(fgets(line,sizeof(line),file)!=NULL){
			strcat(data,line);
		}
		fclose(file);
	}
	if(shmdt(data)==-1){
		perror("shmdt");
		exit(1);
	}
}
```
Fungsi ```recordTime()``` adalah fungsi yang digunakan untuk mencatat waktu dan nama file yang dipindahkan
```c
void timeRecord(char *waktu, char *file){
	FILE *record=fopen("/home/user/modul_3/soal_1/timeRec.log","a");
		
	if(record==NULL){
		perror("unable to open");
		exit(EXIT_FAILURE);
	}
	fprintf(record, "[%s] %s\n", waktu,file);
	fclose(record);
}
void removeFile(char *str){
	if(remove(str)==0) printf("%s deleted\n",str);
}
void moveFile(char *source, char *destination){
	pid_t pid=fork();
	if(pid==0){
		char *moving[]={"mv",source,destination,NULL};
		execv("/bin/mv",moving);
		exit(EXIT_FAILURE);
	}
	else if(pid<0){
		exit(EXIT_FAILURE);
	}
	else if(pid>0){
		wait(NULL);
	}
}
```
Di dalam file ```rate.c``` terdapat fungsi ```findBest``` yang digunakan sebagai program untuk mencari rating terbaik dalam suatu file dan menyimpannya dalam variable-variable
```c
void findBest(char *filename,char *shm){
    FILE *file=fopen(filename,"r");
    if(file==NULL){
        printf("Cannot open file: %s\n",filename);
        return;
    }
    char line[256];
    char bestLine[256];
    float bestRating=0.0;
    char bestName[100];
    while (fgets(line,sizeof(line),file)){
        char name[100];
        float rating;
        sscanf(line,"%[^,],%f",name,&rating);
        if(rating>bestRating) {
            bestRating=rating;
            strcpy(bestName,name);
            strcpy(bestLine,line);
        }
    }
    fclose(file);
    sprintf(shm,"Name: %s\nRating: %.1f\n\n",bestName,bestRating);
}
```
Kemudian variable tersebut akan diprint ke dalam console terminal sesuai dengan format yang sudah diberikan dan akan proses tersebut akan diulang untuk setiap file yang diakses dalam shared memory
```c
int main(){
    key_t key=1234;
    int shmid=shmget(key,1024,IPC_CREAT | 0666);
    char *shm=shmat(shmid,NULL,0);

    DIR *d;
    struct dirent *dir;
    d=opendir("/home/user/modul_3/soal_1/microservices/database/");
    if(d){
        while((dir=readdir(d))!=NULL){
            if(dir->d_type==DT_REG){
                char filepath[512];
                sprintf(filepath,"/home/user/modul_3/soal_1/microservices/database/%s",dir->d_name);
                findBest(filepath,shm);
                if(strstr(dir->d_name,"parkinglot")!=NULL){
                	printf("Type: Parking Lot\nFilename: %s\n\n--------------------------------\n\n%s",dir->d_name,shm);
				}
				else if(strstr(dir->d_name,"trashcan")!=NULL){
                	printf("Type: Trash Can\nFilename: %s\n\n--------------------------------\n\n%s",dir->d_name,shm);
				}
            }
        }
        closedir(d);
    }
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
```
Berikut merupakan contoh hasil output dari file ```rate.c```

![Cuplikan layar 2024-05-10 132405](https://github.com/iryandae/Sisop-3-2024-MH-IT22/assets/121481079/dcd948f1-0b64-430a-83d5-a00057136ca2)

Pada file db.c program akan mengakses catatan waktu yang sudah dibuat oleh auth.c dan memprosesnya berdasarkan format yang diinginkan untuk kemudian disimpan di dalam file ```db.log```
```c
#include <stdio.h>
#include <string.h>

int main(){
	FILE *inputFile=fopen("/home/user/modul_3/soal_1/timeRec.log","r");
    FILE *outputFile=fopen("/home/user/modul_3/soal_1/microservices/database/db.log","w");
    
    if(inputFile==NULL || outputFile==NULL){
    	perror("unable to open");
		return 1;
	}
	char line[300];
	while(fgets(line,sizeof(line),inputFile)){
		char timestamp[20],filename[50];
		sscanf(line,"[%[^]]] %s",timestamp,filename);
		char *type;
		if(strstr(filename,"trashcan")==NULL){
			type="Trash Can";
		}
		else if(strstr(filename,"Parking Lot")==NULL){
			type="Parking Lot";
		}
		fprintf(outputFile,"[%s] [%s] [%s]\n",timestamp,type,filename);
	}
	fclose(inputFile);
	fclose(outputFile);
	
	remove("/home/user/modul_3/soal_1/timeRec.log");
	return 0;
}
```
## Soal 2
Buat folder untuk mempermudah
```shell
mkdir max && cd max
```
Buat configurasi file dudududu.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h> // Library tambahan untuk waktu
```
```c
char *numbers[] = {"nol", "satu", "dua", "tiga", "empat", "lima", "enam", "tujuh", "delapan", "sembilan"};
```
```c
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
```
```c
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
```
```c
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
```
```c
void getTime(char *timeString) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}
```
```c
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
```
```c
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
```
```c
            if (strcmp(result, "ERROR") == 0) {
                sprintf(log_message, "[%s] [%s] ERROR pada %s.\n", timeString, (strcmp(argv[1], "-kali") == 0) ? "KALI" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "TAMBAH" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "KURANG" : "BAGI",
                                                                    (strcmp(argv[1], "-kali") == 0) ? "perkalian" :
                                                                    (strcmp(argv[1], "-tambah") == 0) ? "penjumlahan" :
                                                                    (strcmp(argv[1], "-kurang") == 0) ? "pengurangan" : "pembagian");
            } else if (strcmp(argv[1], "-kurang") == 0 && atoi(result) < 0) {
                sprintf(log_message, "[%s] [KURANG] ERROR pada pengurangan.\n", timeString);
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
    }
```
```c
else { // Child process
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
```

## Soal 3

## Soal 4
```c
//server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAXLINE 1024
#define MAX_ANIME 100

struct Anime{
    char hari[20];
    char genre[20];
    char judul[50];
    char status[20];
};

char *get_current_time(){
    time_t now;
    time(&now);
    return ctime(&now);
}
```
```c
void write_to_log(const char *type, const char *message){
    FILE *fp;
    time_t now;
    struct tm *local;
    char timestamp[20];

    time(&now);
    local = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%d/%m/%y", local);

    fp = fopen("change.log", "a");
    if (fp == NULL){
        printf("Error opening log file.\n");
        return;
    }

    fprintf(fp, "[%s] [%s] %s\n", timestamp, type, message);
    fclose(fp);
}
```
```c
void read_myanimelist(struct Anime myanimelist[], int *count){
    FILE *fp;
    char line[MAXLINE];
    *count = 0;

    fp = fopen("myanimelist.csv", "r");
    if (fp == NULL){
        printf("File not found!\n");
        return;
    }

    while (fgets(line, MAXLINE, fp) != NULL){
        sscanf(line, "%[^,],%[^,],%[^,],%s", myanimelist[*count].hari, myanimelist[*count].genre, myanimelist[*count].judul, myanimelist[*count].status);
        (*count)++;
    }

    fclose(fp);
}
```
```c
void send_all_titles(int sockfd, struct Anime myanimelist[], int count){
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < count; i++){
        sprintf(buffer + strlen(buffer), "%d. %s\n", i + 1, myanimelist[i].judul);
    }

    send(sockfd, buffer, strlen(buffer), 0);
}
```
```c
void send_titles_by_genre(int sockfd, struct Anime myanimelist[], int count, char genre[]){
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));
    int found = 0;
    int num = 0;

    for (int i = 0; i < count; i++){
        if (strcmp(myanimelist[i].genre, genre) == 0){
            sprintf(buffer + strlen(buffer), "%d. %s\n", ++num, myanimelist[i].judul);
            found = 1;
        }
    }

    if (!found){
        sprintf(buffer, "Tidak ada anime dengan genre %s.\n", genre);
    }

    send(sockfd, buffer, strlen(buffer), 0);
}
```
```c
void send_titles_by_day(int sockfd, struct Anime myanimelist[], int count, char day[]){
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));
    int found = 0;
    int num = 0;

    for (int i = 0; i < count; i++){
        if (strcmp(myanimelist[i].hari, day) == 0){
            sprintf(buffer + strlen(buffer), "%d. %s\n", ++num, myanimelist[i].judul);
            found = 1;
        }
    }

    if (!found){
        sprintf(buffer, "Tidak ada anime yang tayang di hari %s.\n", day);
    }

    send(sockfd, buffer, strlen(buffer), 0);
}
```
```c
void send_status(int sockfd, struct Anime myanimelist[], int count, char judul[]){
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));
    int found = 0;

    for (int i = 0; i < count; i++){
        if (strcmp(myanimelist[i].judul, judul) == 0){
            sprintf(buffer, "%s\n", myanimelist[i].status);
            found = 1;
            break;
        }
    }

    if (!found){
        sprintf(buffer, "Judul %s tidak ditemukan.\n", judul);
    }

    send(sockfd, buffer, strlen(buffer), 0);
}
```
```c
void add_anime_to_file(struct Anime myanimelist[], int *count, char input[]){
    char hari[20], genre[20], judul[50], status[20];
    sscanf(input, "%[^,],%[^,],%[^,],%s", hari, genre, judul, status);

    FILE *fp;
    fp = fopen("myanimelist.csv", "a");
    if (fp == NULL){
        printf("File not found!\n");
        return;
    }

    fprintf(fp, "%s,%s,%s,%s\n", hari, genre, judul, status);
    fclose(fp);

    strcpy(myanimelist[*count].hari, hari);
    strcpy(myanimelist[*count].genre, genre);
    strcpy(myanimelist[*count].judul, judul);
    strcpy(myanimelist[*count].status, status);
    (*count)++;

    write_to_log("ADD", judul);
}
```
```c
void edit_anime(struct Anime myanimelist[], int count, char input[]){
    char judul[50], hari[20], genre[20], status[20];
    sscanf(input, "%[^,],%[^,],%[^,],%s", judul, hari, genre, status);

    int found = 0;

    for (int i = 0; i < count; i++){
        if (strcmp(myanimelist[i].judul, judul) == 0){
            strcpy(myanimelist[i].hari, hari);
            strcpy(myanimelist[i].genre, genre);
            strcpy(myanimelist[i].status, status);
            found = 1;
            break;
        }
    }

    if (!found){
        printf("Judul %s tidak ditemukan.\n", judul);
    }
    else{
        FILE *fp;
        fp = fopen("myanimelist.csv", "w");
        if (fp == NULL)
        {
            printf("File not found!\n");
            return;
        }

        for (int i = 0; i < count; i++)
        {
            fprintf(fp, "%s,%s,%s,%s\n", myanimelist[i].hari, myanimelist[i].genre, myanimelist[i].judul, myanimelist[i].status);
        }

        fclose(fp);
        write_to_log("EDIT", judul);
    }
}
```
```c
void delete_anime(struct Anime myanimelist[], int *count, char judul[]){
    int found = 0;

    for (int i = 0; i < *count; i++)
    {
        if (strcmp(myanimelist[i].judul, judul) == 0)
        {
            for (int j = i; j < *count - 1; j++)
            {
                myanimelist[j] = myanimelist[j + 1];
            }
            (*count)--;
            found = 1;
            break;
        }
    }

    if (!found){
        printf("Judul %s tidak ditemukan.\n", judul);
    }

    FILE *fp;
    fp = fopen("myanimelist.csv", "w");
    if (fp == NULL){
        printf("File not found!\n");
        return;
    }

    for (int i = 0; i < *count; i++){
        fprintf(fp, "%s,%s,%s,%s\n", myanimelist[i].hari, myanimelist[i].genre, myanimelist[i].judul, myanimelist[i].status);
    }

    fclose(fp);
    write_to_log("DEL", judul);
}
```
```c
int main(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[MAXLINE] = {0};

    struct Anime myanimelist[MAX_ANIME];
    int count;
    read_myanimelist(myanimelist, &count);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while (1){
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, sizeof(buffer));

        if (valread < 0){
            perror("Error reading from socket");
            continue; // Tetap lanjut ke iterasi berikutnya jika terjadi kesalahan baca
        }

        printf("Received : %s\n\n", buffer);
        if (strcmp(buffer, "exit") == 0){
            printf("Received : exit\n\n");
            break;
        }

        char command[10], temp[100];
        sscanf(buffer, "%s %[^\n]", command, temp);

        if (strcmp(command, "tampilkan") == 0){
            send_all_titles(new_socket, myanimelist, count);
        }
        else if (strcmp(command, "genre") == 0){
            send_titles_by_genre(new_socket, myanimelist, count, temp);
        }
        else if (strcmp(command, "hari") == 0){
            send_titles_by_day(new_socket, myanimelist, count, temp);
        }
        else if (strcmp(command, "status") == 0){
            send_status(new_socket, myanimelist, count, temp);
        }
        else if (strcmp(command, "add") == 0){
            char input[MAXLINE];
            memset(input, 0, sizeof(input));
            strcpy(input, temp);
            add_anime_to_file(myanimelist, &count, input);
            send(new_socket, "Anime berhasil ditambahkan.\n", strlen("Anime berhasil ditambahkan.\n"), 0);
        }
        else if (strcmp(command, "edit") == 0){
            char input[MAXLINE];
            memset(input, 0, sizeof(input));
            strcpy(input, temp);
            edit_anime(myanimelist, count, input);
            send(new_socket, "Anime berhasil diubah.\n", strlen("Anime berhasil diubah.\n"), 0);
        }
        else if (strcmp(command, "delete") == 0){
            delete_anime(myanimelist, &count, temp);
            send(new_socket, "Anime berhasil dihapus.\n", strlen("Anime berhasil dihapus.\n"), 0);
        }
        else{
            send(new_socket, "Invalid Command\n", strlen("Invalid Command\n"), 0);
        }
    }

    close(new_socket);
    return 0;
}
```
```c
//client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAXLINE 1024
```
```c
int main(){
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[MAXLINE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nConnection Failed \n");
        return -1;
    }

    while (1){
        printf("You : ");
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        send(sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "exit") == 0){
            printf("Exiting the client\n");
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, sizeof(buffer) - 1);
        printf("Server : \n%s\n", buffer);
    }

    close(sock);
    return 0;
}
```
