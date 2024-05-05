#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>
#include <string.h>

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
