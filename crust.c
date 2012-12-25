#include "gsfs.h"
/*	//moved to gsfs.h
#define 	crust_m			(16)		//change m and 2 below parameter concurrently
#define 	crust_m_is_pow_of_2 	(1)
#define 	crust_m_log2		(4)
#define 	crust_d			(4)
#define 	crust_keylenbit		(128)
#define 	crust_keylen		(crust_keylenbit>>3)
#define 	crust_maxver		((1<<16)-1)//((unsigned int)ppow(crust_m,crust_d)-1)

struct crust_state{
	unsigned char	versions	[crust_d][crust_d];
	unsigned char keys		[crust_d][crust_keylen];		
	unsigned char count;
};
*/
#define printf(...) printk("<0>" __VA_ARGS__)

#if (crust_m_is_pow_of_2)
	#define crust_split_version(ver,verarr)	pow2_split_version(ver,verarr)
#else 
	#define crust_split_version(ver,verarr)	non_pow2_split_version(ver,verarr)
#endif

unsigned long ppow(unsigned int m,unsigned int d){
	unsigned long res=1;
	unsigned int i;
	
	for (i=1;i<=d;i++)
		res*=m;
	return res;
}

void pow2_split_version(unsigned int ver,unsigned char* verarr){
	int i;
	unsigned int md1=crust_m-1;
	
	for(i=0;i<crust_d;i++){
		verarr[i]=(ver&md1);
		ver>>=crust_m_log2;
	}
	return;
}

void non_pow2_split_version(unsigned int ver,unsigned char* verarr){
	int i;
	
	for(i=0;i<crust_d;i++){
		verarr[i]=(ver%crust_m);
		ver/=crust_m;
	}
	return;
}

void printhexstring(unsigned char* hs, char* dest, int len ){
	int i;
	char pp[1000];
	char* pps=pp;
	
	memset(pps,0,1000);
	for(i=0;i<len;i++){			
		sprintf(pps,"%02x ",(unsigned char)hs[i]);
		pps+=3;
		if(i%16==15 || i==crust_keylen-1){
			sprintf(pps,"#");
			pps++;
		}
	}
	pp[999]=0;
	sprintf(dest,"%s* ",pp);
	
	return;
}


void printkey(unsigned char* key){
	int i;
	char pp[100];
	char* pps=pp;
	
	for(i=0;i<crust_keylen;i++){			
		sprintf(pps,"%02x ",(unsigned char)key[i]);
		pps+=3;
		if(i%16==15 || i==crust_keylen-1){
			sprintf(pps,"\n");
			pps++;
		}
	}
	*pps=0;
	pp[99]=0;
	printf("%s",pp);
	return;
}

void printver(unsigned char* ver){
	int i;
	char pp[100];
	char* pps=pp;
	for(i=crust_d-1;i>=0;i--){			
		sprintf(pps,"%02u ",(unsigned char)ver[i]);
		pps+=3;
		if(i==0)
			sprintf(pps,"\n");
		else
			sprintf(pps,",");
		pps+=1;
	}
	printf("%s",pp);
	return;
}

int crust_key_derivate(unsigned char* srcver,  unsigned char* srckey, unsigned char* destver, unsigned char* destkey){
	int 	i,
		j,
		k;
	unsigned int	md1=crust_m-1;
	unsigned char key[crust_keylen+1];

	i=crust_d-1;
	while(i>0 && srcver[i]==destver[i])
		i--;
	
	if(i==0 && srcver[0]==destver[i]){
		for(k=0;k<crust_keylen;k++)
			destkey[k]=srckey[k];
		return 0;
	}
	
	if(srcver[i]<destver[i])
		return -1;
	
	j=i-1;
	while(j>=0){
		if(srcver[j]!=md1)
			return -1;
		j--;
	}
	
	for(j=0;j<crust_keylen;j++)
		destkey[j]=srckey[j];
	
	while(i>=0){
		//printf("***%d %d\n",i,srcver[i]-destver[i]);
		for(j=srcver[i]-destver[i];j>0;j--){			
			for(k=0;k<crust_keylen;k++)
				key[k]=destkey[k];
			key[crust_keylen]=i;
			skein512(crust_keylenbit,key,crust_keylenbit+8,destkey);
		}
		i--;		
	}
	//printf("\n");
	return 0;
}

int crust_get_next_state(struct crust_state* newstate, unsigned int newversion,unsigned char* origkey){
	unsigned char	 	ver[crust_d],
				fullver[crust_d];
	int 	i,
		j;		
	unsigned int	md1=crust_m-1;

	newstate->count=0;
	if(newversion >  crust_maxver)
		return -1;
	
	for(i=0;i<crust_d;i++)
		fullver[i]=md1;	
	crust_split_version(newversion,newstate->versions[0]);
	if(!crust_key_derivate(fullver,origkey,newstate->versions[0],newstate->keys[0]))
		newstate->count=1;
	//else
		//printf("Error\n");
	
	for(i=0;i<crust_d;i++)
		ver[i]=newstate->versions[0][i];
	i=0;
	while(i<crust_d && ver[i]==md1)
		i++;
	if(i<crust_d)
		ver[i++]=md1;	
	while(i<crust_d){
		if(ver[i]>0){
			ver[i]--;
			for(j=0;j<crust_d;j++)
				newstate->versions[newstate->count][j]=ver[j];
			if(!crust_key_derivate(fullver,origkey,newstate->versions[newstate->count],newstate->keys[newstate->count]))
				newstate->count++;	
			//else
				//printf("Error\n");
		}
		ver[i]=md1;
		i++;
	}
	/*
	for(i=0;i<newstate->count;i++){
		printf(" *** %d:\n",i);
		printver(newstate->versions[i]);
		printkey(newstate->keys[i]);
	}	
	*/
	return 0;
}

int crust_get_key_of_state(struct crust_state* state,unsigned int version,unsigned char* key){
	unsigned char verarr[crust_d];
	int i;
	
	crust_split_version(version,verarr);
	for(i=0;i<state->count;i++)
		if(!crust_key_derivate(state->versions[i],state->keys[i],verarr,key))
			return 0;
	return -1;
}