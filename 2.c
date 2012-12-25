
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <sys/syscall.h>
struct aa{
union{
		unsigned int reg_inode_security_block;
		struct {
			unsigned int	data_hash_block,
					metadata_hash_block;
		} dir_inode_security_block;
	} aas;
};

struct linux_dirent {
		unsigned long  d_ino;     /* Inode number */
		unsigned long  d_off;     /* Offset to next linux_dirent */
               unsigned short d_reclen;  /* Length of this linux_dirent */
               char           d_name[];  /* Filename (null-terminated) */
                                   /* length is actually (d_reclen - 2 -
                                      offsetof(struct linux_dirent, d_name) */
               /*
               char           pad;       // Zero padding byte
               char           d_type;    // File type (only since Linux 2.6.4;
                                         // offset is (d_reclen - 1))
               */

        };

	//#define ww4
	#ifdef ww4
		#define ww(mes) mes
	#else
		#define ww(mes)
	#endif

void main(){
	ww(int i=213;)
	int fd=open(".",O_RDONLY|O_DIRECTORY);
	void* buf=malloc(1024);
	int n;
	do{
		n=syscall(SYS_getdents,fd,buf,1024);
		printf("%d \n",n);
		int pos=0;
		while(pos<n){
			struct linux_dirent* d=(struct linux_dirent*)(buf+pos);
			printf("%d %30s %10d %10d %d \n",d->d_ino,d->d_name,pos,d->d_off,buf);
			pos+=d->d_reclen;
		}
	}while(n);
	return;
}
