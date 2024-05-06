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
