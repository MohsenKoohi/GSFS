#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <string.h>
int min(int a,int b){
	if(a<b)
		return a;
	return b;
}
char* getmealpha(char* array,int size){

	char* ap=array;
	char abc[27],
	     p[35];
	for(int i=0;i<26;i++)
		abc[i]='A'+i;
	abc[26]=0;
	int len=0;
	int i=0;
	while(size-len>0){
		int llen=min(4096,size-len);
		//printf("%d\n",size-len);	
		for(int j=0;j<=
			llen/32;j++){
			sprintf(p,"*%d*M%i*%s****\n",i,j,abc);
			p[31]='\n';
			p[32]=0;
			sprintf(ap,"%s",p);
			ap+=32;
			//printf("1\n");
		}
		len+=llen;
		i++;
	}
	array[size]=0;
	return array;
}

void main(){
	int size=4096;
	char *array=malloc(size+32);
	getmealpha(array,size);
	int fd=open("/media/fs1/1",O_CREAT|O_RDWR);
	
	int s=lseek(fd,64,SEEK_SET);
	printf("%d %d %lx\n",fd,s,array);
	s=write(fd,array,54);
	printf("%d\n",s);
	
	s=lseek(fd,9432,SEEK_SET);
	printf("%d %d %lx\n",fd,s,array);
	s=write(fd,array,480);
	printf("%d\n",s);
		
	close(fd);
	
	return;
}
