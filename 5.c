#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <string.h>
struct users{
	int k[20];
};

void main(){
	
	struct users u;
	struct users* up=&u;
	struct users** uu,**uu2;
	
	*uu=up;
	printf("up:%lx\n",up);
	printf("uu:%lx *uu:%lx\n",uu,*uu);
	
	uu2=malloc(8);
	printf("uu2:%lx *uu2:%lx\n",uu2,*uu2);
	*uu2=*uu;
	printf("uu2:%lx *uu2:%lx\n",uu2,*uu2);
	//root=add_event_to_atn(root,5,6,uu,8,0);

	struct users* d;
	memcpy(&d, uu2, 8);
	printf("d:%lx &d:%lx\n",d,&d);
	return;
}
