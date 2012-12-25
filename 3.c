#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <string.h>
void main(){
	int size=64*1024*1024;
	char abc[27],
	     p[35];
	for(int i=0;i<26;i++)
		abc[i]='A'+i;
	abc[26]=0;
	for(int i=0;i<size/4096;i++){
		for(int j=0;j<4096/32;j++){
			sprintf(p,"*%d*%i*%s***\n",i,j,abc);
			p[31]='\n';
			p[32]=0;
			printf("%s",p);
		}
	}
	return;
}
