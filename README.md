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
Mendefinisikan sesuai dengan perintah "kalkulator sederhana yang dapat menghitung peroperasian dari angka 1-9. 
```c
char *numbers[] = {"nol", "satu", "dua", "tiga", "empat", "lima", "enam", "tujuh", "delapan", "sembilan"};
```
Mengubah kalimat/kata yang dimasukkan menjadi angka untuk dioperasikan.
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
Pengoperasian kalkulator, setelah kata (angka) yang dimasukkan diubah formatnya menjadi angka.
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
Mengubah angka yang didapat dari operasi kalkulator kembali menjadi kalimat/kata-kata.
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
Tambahan untuk mencatat waktu yang akan dicatat dengan aktivitas lainnya dalam file histori.log.
```c
void getTime(char *timeString) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}
```
Fungsi main yang akan mengeluarkan 4 opsi jika pilihan (-operasi) belum disertakan.
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
Parent process, program akan mengubah input menjadi angka dan melakukan operasi dari angka yang telah diubah, dari fungsi sebelumnya (convertToNumber).
```c
    if (pid > 0) { // Parent process
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        char result[100];
        calculate(argv[1], numbers[0], numbers[1], result);
        write(pipe_parent_to_child[1], result, strlen(result) + 1);

	close(pipe_parent_to_child[1]);
	close(pipe_child_to_parent[0]);

	wait(NULL);

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
Membuat output, dan format pencatatan file histori.log.
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
Melanjutkan, child process yang mengubah hasil angka dari parent process menjadi kalimat.
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
Error di awal pengerjaan:
![WhatsApp Image 2024-05-05 at 15 08 32_dd225a5e](https://github.com/iryandae/Sisop-3-2024-MH-IT22/assets/151121570/a99b4827-408b-4d3c-8b18-139863f83bd1)
![WhatsApp Image 2024-05-05 at 14 58 50_1cdc6e46](https://github.com/iryandae/Sisop-3-2024-MH-IT22/assets/151121570/74cdefb0-f221-4c31-9335-d3f4a5ac327d)
*output berupa angka

## Soal 3
Pada file ``actions.c``

`check_gap(float gap)`: Fungsi ini memeriksa jarak antara mobil pengguna dengan mobil di depannya dalam balapan, yang diwakili oleh parameter `gap`. Rentang output fungsi ini adalah sebagai berikut:
   - Jika `gap` < 3.5, fungsi akan mengembalikan string "Gogogo"
   - Jika `gap` berada dalam rentang 3.5-10, fungsi akan mengembalikan string "Push"
   - Jika `gap` > 10, fungsi akan mengembalikan string "Stay out of trouble"

`check_fuel(float fuel)`: Fungsi ini memeriksa tingkat bahan bakar dalam tangki mobil, yang diwakili oleh parameter `fuel`. Rentang output fungsi ini adalah sebagai berikut:
   - Jika `fuel` >80%, fungsi akan mengembalikan string "Push Push Push"
   - Jika `fuel` berada dalam rentang 50% - 80%, fungsi akan mengembalikan string "You can go"
   - Jika `fuel` kurang dari 50%, fungsi akan mengembalikan string "Conserve Fuel"

`check_tire(int tire)`: Fungsi ini memeriksa keausan ban mobil, yang diwakili oleh parameter `tire`. Rentang output fungsi ini adalah sebagai berikut:
   - Jika `tire` > 80%, fungsi akan mengembalikan string "Go Push Go Push"
   - Jika `tire` berada dalam rentang 50% - 80%, fungsi akan mengembalikan string "Good Tire Wear"
   - Jika `tire` berada dalam rentang 30% - 50%, fungsi akan mengembalikan string "Conserve Your Tire"
   - Jika `tire` kurang dari 30%, fungsi akan mengembalikan string "Box Box Box"

`check_tire_change(const char* tire_type)`: Fungsi ini memeriksa tipe ban saat ini yang digunakan oleh mobil, yang diwakili oleh parameter `tire_type`. Rentang output fungsi ini adalah sebagai berikut:
   - Jika tipe ban adalah "Soft", fungsi akan mengembalikan string "Mediums Ready"
   - Jika tipe ban adalah "Medium", fungsi akan mengembalikan string "Box for Softs"
   - Jika tipe ban tidak dikenali, fungsi akan mengembalikan string "Invalid tire type"
```c
#include <stdio.h>
#include <string.h>
#include "actions.h"

const char* check_gap(float gap) {
    if (gap < 3.5) {
        return "Gogogo";
    } else if (gap <= 10) {
        return "Push";
    } else {
        return "Stay out of trouble";
    }
}

const char* check_fuel(float fuel) {
    if (fuel > 80) {
        return "Push Push Push";
    } else if (fuel >= 50) {
        return "You can go";
    } else {
        return "Conserve Fuel";
    }
}

const char* check_tire(int tire) {
    if (tire > 80) {
        return "Go Push Go Push";
    } else if (tire > 50) {
        return "Good Tire Wear";
    } else if (tire > 30) {
        return "Conserve Your Tire";
    } else {
        return "Box Box Box";
    }
}

const char* check_tire_change(const char* tire_type) {
    if (strcmp(tire_type, "Soft") == 0) {
        return "Mediums Ready";
    } else if (strcmp(tire_type, "Medium") == 0) {
        return "Box for Softs";
    } else {
        return "Invalid tire type";
    }
}
```

Pada bagian file``paddock.c``.
Deklarasikan library yang akan digunakan.
```c
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
```

Definisikan port server yang akan digunakan dan log file yang akan digunakan.
```c
#define PORT 8080
#define LOGFILE "race.log"

FILE *log_file;
```

Catat pesan ke file log dengan format sumber pesan, waktu, perintah, dan informasi terkait.
```c
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
```

Proses pesan yang diterima dari driver(menerima ``command`` dan ``info``), memanggil fungsi yang sesuai dari file "actions.h", dan mengirimkan respons kembali ke driver melalui socket.
```c
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
```

Inisialisasikan server socket, membuat socket, mengikatnya ke alamat dan port tertentu, mendengarkan koneksi, dan kemudian menerima dan memproses pesan dari klien. keluar dan print pesan error jika gagal.
```c
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
```

Fork proses sehingga proses anak akan menerima koneksi dan menjalankan tugasnya, sementara proses induk akan keluar, membiarkan proses anak berjalan sebagai daemon.
```c
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
```

Setelah melakukan fork, proses anak mengatur ulang beberapa pengaturan sistem seperti mask file mode, SID (Session ID), dan direktori kerja saat ini sehingga proses anak berjalan sebagai daemon terpisah.
```c
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
```

Tutup file deskriptor standar.
```c
// Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
```

Server menerima koneksi baru dari klien menggunakan accept(), kemudian membaca perintah dan informasi yang dikirimkan oleh klien melalui socket. Command dan info dibaca dari socket, lalu di-parse untuk menghapus kurung siku, kemudian dicatat ke dalam file log, dan diproses menggunakan fungsi `process_driver_message`.
```c
while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr )&address, (socklen_t)&addrlen)) < 0) {
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
```

Pada bagian ``driver.c``
Deklarasikan library yang akan digunakan.
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
```

Definiskan port yang akan digunakan dan ukuran buffer yang dibutuhkan.
```c
#define PORT 8080
#define BUFFER_SIZE 1024
```

Deklarasikan variabel struct sockaddr_in yang digunakan untuk menyimpan alamat server dan klien, variabel sock yang digunakan untuk menampung file descriptor dari socket yang dibuat, variabel valread untuk menyimpan nilai yang dibaca dari socket, dan array buffer yang digunakan untuk menyimpan data yang diterima dari server.
```c
int main(int argc, char *argv[]) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
```

Membuat socket dengan menggunakan fungsi socket(). Jika pembuatan socket gagal, pesan kesalahan akan dicetak dan program akan keluar dengan nilai `-1`.
```c
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
```

Tentukan alamat server dalam struktur serv_addr.
```c
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
```

Gunakan fungsi connect() untuk menghubungkan ke server. Jika koneksi gagal, pesan kesalahan akan dicetak dan program akan keluar dengan nilai `-1`.
```c
 if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
```
Program membaca argumen baris perintah untuk menentukan command (`-c`) dan info (`-i`) yang akan dikirim ke server. Argumen baris perintah diproses dengan menggunakan loop `for` untuk memeriksa setiap argumen.
```c
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
```

Pesan dengan command dan info yang telah ditentukan diapit oleh kurung siku, kemudian dikirim ke server menggunakan fungsi send(). Program mencetak pesan yang dikirim ke server. Program menerima pesan balasan dari server menggunakan fungsi read() dan mencetaknya di layar. Setelah semua proses selesai, program mengembalikan nilai 0 sebagai penanda bahwa program berjalan dengan sukses.
```c
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

```
Berikut adalah output dari program :

<img width="738" alt="Screenshot 2024-05-11 at 15 28 16" src="https://github.com/iryandae/Sisop-3-2024-MH-IT22/assets/150358232/21b97b68-d5b8-4556-b8e3-f1516bc1b3f5">

dalam gambar ini output belum selesai diproses, namun saat demo tidak ada masalah, program dapat berhasil menampilkan output yang diharapkan.


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
