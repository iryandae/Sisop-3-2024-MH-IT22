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
		
		if(strstr(de->d_name,"trashcan.csv")!=NULL || strstr(de->d_name,"parkinglot.csv")!=NULL){
			char old_path[200],new_path[200];
			strcpy(old_path,filename);
			
			strcpy(new_path,"/home/user/modul_3/soal_1/microservices/database/");
			strcat(new_path,de->d_name);
			moveFile(old_path,new_path);
			timeRecord(time_str,de->d_name);
			sharedMemory(new_path);
		}
		else{
			removeFile(filename);
		}
	}
	closedir(dir);
	return 0;
}
