#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define kernel_module (0)

#include "rsa.c"
#include "message.h"

#define KEY_SIZE 1024
#define EXPONENT 65537

void prepare_dest(char* dest){
	int i=0;
	int len=strlen(dest);
	
	while(i<len && dest[i]>=(char)32 && dest[i]<=(char)126){
		i++;
		//printf("%d ",(int)dest[i]);
	}
	//printf("%d ",(int)dest[i]);
	//printf("\n");
	if(i<strlen(dest))
		dest[i]=0;
	
	return;
}

void login(int fid,char* dest){
	char priv[30];
	int ptlen=64;
	unsigned int r;
	char pt[ptlen];
	char dpt[128];
	int retd;
	struct message mes;
	
	prepare_dest(dest);
	
	sprintf(priv,"%s/rsa_priv_%d.txt",dest,getuid());
	
	rsa_context* rsa=read_rsa_private_key_from_file(priv);
	if(rsa->len==0 || rsa->N.n==0){
		retd=1;
		goto back;
	}
	printf("\n  . Loging in to GSFS ...");
	fflush(stdout);
	
	retd=2;
	int ret=rsa_check_privkey(rsa);
	if(ret)
		goto back;
	
	mes.pid=getpid();
	mes.type=LOGIN;
	mes.data=rsa;
	mes.datalen=-1;
	
	srand(time(0));
	for(int i=0;i<ptlen/4;i++){
		r=rand();
		memcpy(pt+4*i,&r,4);
	}
	
	mes.data2=pt;
	mes.data2len=ptlen;
	
	retd=2;
	ret=rsa_1024_encrypt(rsa, RSA_PRIVATE, ptlen, pt, dpt);
	if(ret)
		goto back;
	
	mes.data3=dpt;
	mes.data3len=128;
	
	retd=3;
	ret=write(fid,(char*)&mes,sizeof(mes));

back:
	if(ret)
		printf(" failed. Your private key is not valid and/or for you %d.",retd);
	else
		printf(" ok");
	printf("\n\n");
	
	rsa_free(rsa);
	free(rsa);
	
	return;
}

void logout(int fid){
	printf("\n  . Loging out from GSFS ...");
	fflush(stdout);
	
	struct message mes;
	mes.pid=getpid();
	mes.type=LOGOUT;
	mes.data=0;
	mes.datalen=-1;
	if(write(fid,(char*)&mes,sizeof(mes)))
		printf(" failed");
	else
		printf(" ok");
	printf("\n\n");
	
	return;
}

void generate(int fid,char* dest){
	char 	pub[30],
		priv[30];
		
	prepare_dest(dest);

	sprintf(pub,"%s/rsa_pub_%d.txt",dest,getuid());
	sprintf(priv,"%s/rsa_priv_%d.txt",dest,getuid());
	printf("\t\tYour Public Key File: %s\n\t\tYour Private Key File: %s\n",pub,priv);
	
	if(generate_rsa_1024(KEY_SIZE,EXPONENT,priv,pub))
		return;
	
	rsa_context* rsa=read_rsa_public_key_from_file(pub);
	
	//printf("rsa->len:%d, rsa->N.n=%d\n",rsa->len, rsa->N.n);
	
	if(rsa->len==0 || rsa->N.n==0)
		return;
	
	printf("\n  . Adding new user to GSFS ...");
	fflush(stdout);
	
	struct message mes;
	
	mes.pid=getpid();
	mes.type=NEWUSER;
	mes.data=rsa;
	mes.datalen=-1;
	
	if(write(fid,(char*)&mes,sizeof(mes)))
		printf(" failed\n\n");
	else
		printf(" ok\n\n");
		
	rsa_free(rsa);
	free(rsa);
	
	return;
}

void makesec(int fid, char* dest){
	
	prepare_dest(dest);
	
	printf("\n  . Making secure directory \"%s\"...",dest);
	fflush(stdout);
	
	struct message mes;
	
	mes.pid=getpid(); 
	mes.type=MAKESEC;
	mes.data=dest;
	mes.datalen=strlen(dest);
	
	if(write(fid,(char*)&mes,sizeof(mes)))
		printf(" failed");
	else
		printf(" ok");
	
	printf("\n\n");
	
	return;	
}

unsigned int* char_array_to_int_array(char* src, int* count){
	int res[1000];
	int 	i=0,
		resc=0;
	int len=strlen(src);
	
	while(i<len){
		int c=0;
		char p[100];
		while(i<len && src[i]!=' ')
			p[c++]=src[i++];
		i++;
		p[c]=0;
		
		res[resc++]=atoi(p);
		//printf("**%d %s\n",res[resc-1],p);
	}
	
	*count=resc;
	
	unsigned int* res2=malloc(sizeof(unsigned int)*resc);
	
	for(i=0;i<resc;i++)
		res2[i]=res[i];
	
	return res2;
}

void add_user_access(int fid, char* dest, char* dest2){
	
	prepare_dest(dest);
	prepare_dest(dest2);
	
	printf("\n  . Adding users to secure directory \"%s\"...",dest);
	fflush(stdout);
	
	unsigned int	users[100],
			writeability[100],
			*res,
			count=0,
			uc=0,
			i=0;
	
	res=char_array_to_int_array(dest2,&count);
	
	if(count==0)
		return;
	
	for(i=0;i<count-1;){
		users[uc]=res[i++];
		writeability[uc]=res[i++];
		uc++;
	}
	
	free(res);
	
	struct message mes;
	
	mes.pid=getpid(); 
	mes.type=ADDUSERS;
	mes.data=dest;
	mes.datalen=strlen(dest);
	mes.data2=users;
	mes.data2len=uc;
	mes.data3=writeability;
	mes.data3len=uc;
	
	if( write(fid, (char*)&mes, sizeof(mes)) ){
		printf(" failed\n\n");
		return;
	}
	
	printf(" ok : \n\n");
		
	for(i=0;i<uc;i++){
		switch(writeability[i]){
			case 1:
				printf("\tUnable to add user with uid: %u because we haven't its public key.(%u)\n",users[i],writeability[i]);
				break;
			
			case 2:
				printf("\tUser with uid: %u was added previously.(%u)\n",users[i],writeability[i]);
				break;
			
			case 3:
				printf("\tNo place to add user with uid: %u.(%u)\n",users[i],writeability[i]);
				break;
			
			case 0:
				printf("\tUser with uid: %u is added successfully.(%u)\n",users[i],writeability[i]);
				break;
			
			default:
				printf("\tUser with uid: %u received no valid response.(%u)\n",users[i],writeability[i]);
				break;
		}
	}
	
	printf("\n");
	
	return;
}

void revoke_user_access(int fid, char* dest, char* dest2){
	
	prepare_dest(dest);
	prepare_dest(dest2);
	
	printf("\n  . Revoking users access to a secure directory \"%s\"...",dest);
	fflush(stdout);
	
	unsigned int	users[100],
			*res,
			count=0,
			uc=0,
			i=0;
	int		ret[100];
	
	memset(ret, -1, 100*sizeof(int));
	
	res=char_array_to_int_array(dest2,&count);
	
	if(count==0)
		return;
	
	for(i=0;i<count;)
		users[uc++]=res[i++];
			
	free(res);
	
	struct message mes;
	
	mes.pid=getpid(); 
	mes.type=REVOKEUSERS;
	mes.data=dest;
	mes.datalen=strlen(dest);
	mes.data2=users;
	mes.data2len=uc;
	mes.data3=ret;
	mes.data3len=uc;
	
	if( write(fid, (char*)&mes, sizeof(mes)) ){
		printf(" failed\n\n");
		return;
	}
	
	printf(" ok : \n\n");
		
	for(i=0;i<uc;i++){
		switch(ret[i]){
			case 0:
				printf("\tUser with uid: %u is revoked successfully. (%d)\n",users[i],ret[i]);
				break;
			
			case -1:
				printf("\tUser with uid: %u has not access to this inode. (%d)\n",users[i],ret[i]);
				break;
			
			default:
				printf("\tUser with uid: %u received no valid response. (%d)\n",users[i],ret[i]);
				break;
		}
	}
	
	printf("\n");
	
	return;
}

int main(int argc,char** argv){
	
	rsa_1024_init();
	
	int fid=open(argv[1],O_RDWR);
	
	if(fid<=0){
		printf("\n\n\t\tWrong argument: %s\n\n\n",argv[1]);
		sleep(1);
		return -1;
	}
	
	int fields_num=7;
	char* fields []={	"Login",
				"Logout",
				"Generate RSA-1024 keys for new user",
				"Make a directory secure (more special)",
				"Add users to a secure directory (more public)",
				"Revoke users access to a secure directory",
				"Exit"
	};
	
	char	c[20];
	char	dest[200],
		dest2[200];
	c[0]='0';
	
	if(argc>2){
		for(int i=0;i<strlen(argv[2]);i++)
			switch(argv[2][i]-'0'){
				case 1:
					generate(fid,argv[3]);
					break;
				
				case 2:
					login(fid,argv[3]);
					break;
				
				case 3:
					makesec(fid,argv[3]);
					break;
					
				case 4:
					add_user_access(fid, argv[3], argv[4]);
					break;
					
				case 5:
					revoke_user_access(fid, argv[3], argv[4]);
					break;
			}
	
		close(fid);
		
		return 0;
	}
		
	while(c[0]){		
		printf("Input the number:\n");
		for(int i=0;i<fields_num;i++)
			printf("\t%d) %s\n",i+1,fields[i]);
		fflush(stdout);
		read(0,c,20);		
		switch(c[0]){
			case '7':
			case 'q':
				//exit
				c[0]=0;
				break;
			
			case '6':
				//revoke user
				
				printf("\tInput secure directory address to add users access: ");
				fflush(stdout);
				
				read(0,dest,200);
				
				printf("\tInput uids to revoke: ");
				fflush(stdout);
				
				read(0,dest2,200);
								
				revoke_user_access(fid,dest,dest2);
								
				break;
				
			case '5':
				//add user
				
				printf("\tInput secure directory address to add users access: ");
				fflush(stdout);
				
				read(0,dest,200);
				
				printf("\tInput uids in form of \"user_id  writeacess \": ");
				fflush(stdout);
				
				read(0,dest2,200);
								
				add_user_access(fid,dest,dest2);
				
				break;
				
			case '4':
				//make secure
				
				printf("\tInput directory address to make it secure: ");
				fflush(stdout);
				
				read(0,dest,200);
				
				makesec(fid,dest);
				
				break;
				
			case '3':
				//generate rsa
				
				printf("\tInput destination: ");				
				fflush(stdout);
				
				read(0,dest,40);
				
				generate(fid,dest);
				
				break;
				
			case '2':
				//logout
				
				logout(fid);
				
				break;
				
			case '1':
				//login
				
				printf("\tInput private key location: ");
				fflush(stdout);
				
				read(0,dest,40);				
				
				login(fid,dest);

				break;
		}
	}
	
	close(fid);
	
	return 0;
}
