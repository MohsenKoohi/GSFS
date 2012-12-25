#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <string.h>
void main(){
	int size=40*1024*1024;
	char* ad=malloc(size);
	char* ap=ad;
	for(int i=0;i<size/4096;i++){
		char a[20];
		sprintf(a,"*%d*",i);
		int al=strlen(a);
		for(int j=0;j<4096/al;j++){
			sprintf(ad,a);
			ad+=al;
		}
		for(int j=0;j<4096%al;j++)
			sprintf(ad++,"*");
	}
	ap[size]=0;
	printf(ap);
}
