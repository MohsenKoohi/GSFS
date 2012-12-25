//#include <stdio.h>
//#include <linux/string.h>
#include "gsfs.h"

/*
void printarr(unsigned char* a){
	for(int i=15;i>=0;i--)
		printf("%02x ",(unsigned char)a[i]);
	printf("\n");
}
*/

int slow_gmull(unsigned long x[2],unsigned long  y[2],unsigned long  z[2]){
	unsigned long v[2],
				i,
				j,
				k1,
				k,
				l,
				ys;
				
	v[0]=x[0];
	v[1]=x[1];
	z[0]=0;
	z[1]=0;
	
	#define vlow v[0]
	#define vhigh v[1]
	#define ylow y[0]
	#define yhigh y[1]
	#define zlow z[0]
	#define zhigh z[1]
	
	for (i=0;i<2;i++){
		ys=y[i];
		for(j=0;j<64;j++){
			k1=ys&1;
			ys>>=1;
			if(k1){
				zlow^=vlow;
				zhigh^=vhigh;
			}
			k=vhigh&(1UL<<63);
			l=vlow&(1UL<<63);
			vlow<<=1;
			vhigh<<=1;
			if(l)
				vhigh|=1UL;
			if(k)
				vlow^=(0x87UL);
		}
	}
	
	return 0;
}

#define gmull(x,y,z)	 slow_gmull((unsigned long*)(x),(unsigned long*)(y),(unsigned long*)(z))
#define xor(x, y) 		(x)[0]^=(y)[0];	(x)[1]^=(y)[1];

int ghash(unsigned char h[16], unsigned char *a,unsigned long alen,unsigned char* c,unsigned long clen,unsigned char res[16]){
	unsigned long 	i,
			j,
			k,
			len2,
			len;
	unsigned char	tc;
	unsigned char	t[2][16],
			*arr,
			temp[16];
	
	memset(t,0,32);
	tc=0;
	
	for(k=0;k<2;k++){
		if(k==0){
			arr=a;
			len=alen;
		}
		else{
			arr=c;
			len=clen;
		}
			
		len2=len-(len&0xf);
		for(i=0;i<len2;i+=16,tc=1-tc){
			xor((unsigned long*)t[tc],(unsigned long*)(arr+i));
			gmull(t[tc],h,t[1-tc]);
		}
		
		if(len&0xf){
			memset(temp,0,16);
			j=0;
			for(;i<len;i++,j++)
				temp[j]=arr[i];
			xor((unsigned long*)t[tc],(unsigned long*)(temp));
			gmull(t[tc],h,t[1-tc]);
			tc=1-tc;
		}
	}
	
	memcpy(temp,&alen,8);
	memcpy(temp+8,&clen,8);
	xor((unsigned long*)t[tc],(unsigned long*)(temp));
	gmull(t[tc],h,res);
	
	return 0;
}

/*
int main(int argc , char ** argv){
	unsigned long a[2],b[2],z[2];
	a[0]=0;
	a[1]=0;
	b[0]=0;
	b[1]=0;
	unsigned char * aa=(unsigned char*)a;
	unsigned char * bb=(unsigned char*)b;
	aa[0]=3;
	aa[8]=120;
	bb[0]=3;
	bb[8]=100;
	bb[12]=8;
	slow_gmull(a,b,z);
	
	printf("z: ");printarr((unsigned char*)z);
	char res[16];
	char cc[4096];
	memset(cc,0,4096);
	ghash(bb,aa,16,cc,4092,res);
	printf("res: ");printarr((unsigned char*)res);
	return 0;
}
*/

int inc_IV(char *IV){
	struct iv{
		unsigned long iv1;
		unsigned int iv2;
	};
	
	struct iv*	iv=(struct iv*)IV;
	
	if(unlikely(IV==0 || (iv->iv1==-1 && iv->iv2==-1)))
		return -1;
	
	iv->iv1++;
	
	if(iv->iv1==0){
		
		iv->iv2++;
		
		return -1;
	}
	
	return 0;
}

int get_gctr_page(char* apd, char* key, char* IV){
	int 		i;
	char 		ICB[gsfs_aes_keylen];
	struct crypto_cipher *cc;
	
	if(unlikely(!apd || !key || !IV))
		return -1;
	
	memcpy(ICB, IV, 12);
	ICB[12]=0;
	ICB[13]=0;
	ICB[14]=0;
	ICB[15]=1;
	
	cc=crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if(unlikely(!cc))
		return -1;
	
	crypto_cipher_setkey(cc, key, gsfs_aes_keylen);
			
	for(i=0; i<Block_Size/gsfs_aes_keylen; i++){
		ICB[15]++;
		if(ICB[15]==0)
			ICB[14]++;
		
		crypto_cipher_encrypt_one(cc, apd, ICB);
		
		apd+=gsfs_aes_keylen;
	}
	
	crypto_free_cipher(cc);
	
	memset(ICB, 0, gsfs_aes_keylen);
	
	return 0;
}

inline int get_j0(char* j0, char* IV){
	unsigned int	*j0int;
	
	if(unlikely(!j0 || !IV))
		return -1;
	
	memcpy(j0, IV, 12);
	j0int=(unsigned int*)j0;
	j0int[3]=0;
	j0[15]=1;
	
	return 0;
}

int get_gctr_page_and_j0(char* apd, char* j0, char* key, char* IV){
	unsigned int	*j0int;
	int 		i;
	char 		ICB[gsfs_aes_keylen];
	struct crypto_cipher *cc;
	
	if(unlikely(!apd || !j0 || !key || !IV))
		return -1;
	
	memcpy(j0, IV, 12);
	j0int=(unsigned int*)j0;
	j0int[3]=0;
	j0[15]=1;
	
	//printkey(j0);
	memcpy(ICB, j0, gsfs_aes_keylen);
	ICB[15]=1;
	
	cc=crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if(unlikely(!cc))
		return -1;
	
	crypto_cipher_setkey(cc, key, gsfs_aes_keylen);
			
	for(i=0; i<Block_Size/gsfs_aes_keylen; i++){
		ICB[15]++;
		if(ICB[15]==0)
			ICB[14]++;
		
		crypto_cipher_encrypt_one(cc, apd, ICB);
		
		apd+=gsfs_aes_keylen;
	}
	
	crypto_free_cipher(cc);
	
	memset(ICB, 0, gsfs_aes_keylen);
	
	return 0;
}

int get_AT(char* AT, char* apd, char* key, char* j0){
	char	H[gsfs_aes_keylen],
		J[gsfs_aes_keylen];
	struct crypto_cipher *cc;
	int	ret;
	
	if(unlikely(!AT || !apd || !key || !j0))
		return -1;
	
	cc=crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if(unlikely(!cc))
		return -1;
	
	crypto_cipher_setkey(cc, key, gsfs_aes_keylen);
	
	memset(H, 0, gsfs_aes_keylen);
	crypto_cipher_encrypt_one(cc, H, H);
	//printkey(H);
	
	crypto_cipher_encrypt_one(cc, J, j0);
	//printkey(J);
	
	crypto_free_cipher(cc);
	
	ret=ghash(H, 0, 0, apd, Block_Size, AT);
		
	xor((unsigned long*)AT, (unsigned long*)J);
	//printkey(AT);
	
	memset(J, 0, gsfs_aes_keylen);
	memset(H, 0, gsfs_aes_keylen);
	
	return 0;
}