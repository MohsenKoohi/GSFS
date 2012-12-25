#include "gsfs.h"

void initialize_lru(struct GSFS_sb *gsb){
	int i;
	
	for(i=0;i<LRU_COUNT;i++){
		init_rwsem(&gsb->lru_rwsem[i]);
		gsb->lru_count[i]=0;
	}
	
	return;
}

struct buffer_head* get_lru_bh(struct GSFS_sb* gsb, unsigned char lru, sector_t num){
	int 	i,	
		min;
		
	down_read(&gsb->lru_rwsem[lru]);
	
	for(i=0;i<gsb->lru_count[lru];i++)
		if(gsb->lru_bh_number[lru][i]==num){
			gsb->lru_time[lru][i]=jiffies_64;
		
			up_read(&gsb->lru_rwsem[lru]);
		
			return gsb->lru_bh[lru][i];
		}
	
	up_read(&gsb->lru_rwsem[lru]);
	
	down_write(&gsb->lru_rwsem[lru]);
	
	if(gsb->lru_count[lru]<LRU_BH_MAX){
		
		gsb->lru_bh[lru][gsb->lru_count[lru]]=read_one_bh_dev(gsb->sb->s_bdev,num);
		
		get_bh(gsb->lru_bh[lru][gsb->lru_count[lru]]);
		
		gsb->lru_time[lru][gsb->lru_count[lru]]=jiffies_64;
		gsb->lru_bh_number[lru][gsb->lru_count[lru]]=num;
	
		up_write(&gsb->lru_rwsem[lru]);

		return gsb->lru_bh[lru][gsb->lru_count[lru]++];
	}
	
	min=0;
	for(i=1;i<LRU_BH_MAX;i++)
		if(gsb->lru_time[lru][i]<gsb->lru_time[lru][min])
			min=i;
	
	wait_on_buffer(gsb->lru_bh[lru][min]);
	
	if(buffer_dirty(gsb->lru_bh[lru][min]))
		write_one_bh_dev(gsb->lru_bh[lru][min]);	
	
	brelse(gsb->lru_bh[lru][min]);
	
	gsb->lru_bh[lru][min]=read_one_bh_dev(gsb->sb->s_bdev, num);
	
	get_bh(gsb->lru_bh[lru][min]);
	
	gsb->lru_time[lru][min]=jiffies_64;	
	gsb->lru_bh_number[lru][min]=num;
	
	up_write(&gsb->lru_rwsem[lru]);
	
	return gsb->lru_bh[lru][min];
}

void exit_lru(struct GSFS_sb *gsb){
	short i=0;
	int j;
	
	for(i=0;i<LRU_COUNT;i++){
	
		down_write(&gsb->lru_rwsem[i]);
		
		for(j=0;j<gsb->lru_count[i];j++){
		
			if(buffer_dirty(gsb->lru_bh[i][j]))
				write_one_bh_dev(gsb->lru_bh[i][j]);
			
			brelse(gsb->lru_bh[i][j]);
		}
		
		up_write(&gsb->lru_rwsem[i]);
	}
	
	return;
}

void sync_lru(struct GSFS_sb *gsb){
	short i=0;
	int j;
	
	for(i=0;i<LRU_COUNT;i++){
	
		down_write(&gsb->lru_rwsem[i]);
		
		for(j=0;j<gsb->lru_count[i];j++){
			if(buffer_dirty(gsb->lru_bh[i][j]))
				write_one_bh_dev(gsb->lru_bh[i][j]);			
		}
		
		up_write(&gsb->lru_rwsem[i]);
	}
	
	return;
}

void set_BAT(struct GSFS_sb* gsb, sector_t from, sector_t to){
	sector_t 	batbn,
			bat_s=bat_bn(from),
			bat_e=bat_bn(to);
	unsigned int	s_of,
			e_of,
			s_of_bit,
			e_of_bit,
			s_of_byte,
			e_of_byte,
			i,
			k;			
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;		
	struct buffer_head* bh;
	char* p;
	
	if(from>gsd->total_blocks || to>gsd->total_blocks)
		return;
	//printk("<0>" "%d %d %d %d\n",bat_s,bat_e,from,to);
	for (batbn=bat_s; batbn<=bat_e; batbn++){
		bh=get_lru_bh(gsb,BAT_LRU,batbn+gsd->bat_start);
		if(!bh)
			continue;
		lock_buffer(bh);
		//printk("<0>" "bat %d : %d\n",batbn,bh->b_count);
		p=(char*)bh->b_data;
		s_of= (batbn == bat_s)? bat_offset(from) : 0;
		e_of= (batbn == bat_e)? bat_offset(to) : Blocks_per_BAT_Block;
		//printk("<0>" "s_of  %d %d\n",s_of,e_of);
		//printk("<0>" "bat_offset  %d %d %x\n",bat_offset(from), bat_offset(to),Blocks_per_BAT_Block-1);
		s_of_bit= s_of & 7;
		e_of_bit= e_of & 7;
		s_of_byte= s_of >> 3;
		e_of_byte= e_of >> 3;
		
		if(s_of_byte == e_of_byte){
			k=0;
			for(i=s_of_bit ; i<=e_of_bit ; i++)
				k |= 1<<i;
			p[s_of_byte]|=k;
			//printk("<0>" "s_of_bit %d %d\n",s_of_bit,e_of_bit);
			//printk("<0>" "s_of_byte %d %d %d\n",s_of_byte,p[s_of_byte],k);
		}
		else{	
			k=0;
			for(i=s_of_bit ; i<=7 ; i++)
				k|= 1<<i;
			p[s_of_byte]|=k;
			k=0;
			for(i=0 ; i<=e_of_bit ; i++)
				k|= 1<<i;
			p[e_of_byte]|=k;
			for(i=s_of_byte+1; i<e_of_byte; i++)
				p[i]=(char)0xff;
		}
		//int mm=atomic_read(&bh->b_count);
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);		
		//printk("<0>" "%d : %d %d  \n",batbn,bh->b_count,*(int*)bh->b_data);
	}
	//printk("<0>" "Bye set_block_used\n");	
	gsd->free_blocks-=(to-from+1);
	gsb->sb->s_dirt=1;
	gsb->sgflags|=sgflag_BAT;
	return;
}

void clear_BAT(struct GSFS_sb* gsb, sector_t from, sector_t to){
	sector_t 	batbn,
			bat_s=bat_bn(from),
			bat_e=bat_bn(to);
	unsigned int	s_of,
			e_of,
			s_of_bit,
			e_of_bit,
			s_of_byte,
			e_of_byte,
			i,
			k;			
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;		
	struct buffer_head* bh;
	char* p;
	
	if(from>gsd->total_blocks || to>gsd->total_blocks)
		return;
	//printk("<0>" "%d %d\n",bat_s,bat_e);
	for (batbn=bat_s; batbn<=bat_e; batbn++){						
		bh=get_lru_bh(gsb,BAT_LRU,batbn+gsd->bat_start);
		if(!bh)
			continue;
		lock_buffer(bh);
		//printk("<0>" "%d : %d\n",batbn,bh->b_count);
		p=(char*)bh->b_data;
		s_of= (batbn == bat_s)? bat_offset(from) : 0;
		e_of= (batbn == bat_e)? bat_offset(to) : Blocks_per_BAT_Block;
		//printk("<0>" "s_of  %d %d\n",s_of,e_of);
		//printk("<0>" "bat_offset  %d %d %x\n",bat_offset(from), bat_offset(to),Blocks_per_BAT_Block-1);
		s_of_bit= s_of & 7;
		e_of_bit= e_of & 7;
		s_of_byte= s_of >> 3;
		e_of_byte= e_of >> 3;
		
		if(s_of_byte == e_of_byte){
			k=0;
			for(i=s_of_bit ; i<=e_of_bit ; i++)
				k |= 1<<i;
			p[s_of_byte]&=~k;
			//printk("<0>" "s_of_bit %d %d\n",s_of_bit,e_of_bit);
			//printk("<0>" "s_of_byte %d %d %d\n",s_of_byte,p[s_of_byte],k);
		}
		else{	
			k=0;
			for(i=s_of_bit ; i<=7 ; i++)
				k|= 1<<i;
			p[s_of_byte]&=~k;
			k=0;
			for(i=0 ; i<=e_of_bit ; i++)
				k|= 1<<i;
			p[e_of_byte]&=~k;
			for(i=s_of_byte+1; i<e_of_byte; i++)
				p[i]=0x00;
		}
		//int mm=atomic_read(&bh->b_count);
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);		
		//printk("<0>" "%d : %d %d %d \n",batbn,bh->b_count,mm,m);
	}
	//printk("<0>" "Bye set_block_used\n");
	gsd->free_blocks+=(to-from+1);
	gsb->sb->s_dirt=1;
	gsb->sgflags|=sgflag_BAT;
	return;
}

void set_IAT(struct GSFS_sb* gsb, unsigned int ino, sector_t inblock){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct buffer_head* bh=get_lru_bh(gsb,IAT_LRU,gsd->iat_start+iat_bn(ino));
	unsigned int* ii;
	if(bh){
		lock_buffer(bh);
		ii=(unsigned int*)bh->b_data;
		ii[iat_offset(ino)]=(unsigned int)inblock ;
		//printk("<0>" "iat %d %d sa\n",iat_offset(ino),0xffffffff & inblock );
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		gsd->free_inodes++;
		gsb->sb->s_dirt=1;
	}
	gsb->sgflags|=sgflag_IAT;
	return;
}

void clear_IAT(struct GSFS_sb* gsb, unsigned int ino){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct buffer_head* bh=get_lru_bh(gsb,IAT_LRU,gsd->iat_start+iat_bn(ino));
	unsigned int* ii;
	if(bh){
		lock_buffer(bh);
		ii=(unsigned int*)bh->b_data;
		ii[iat_offset(ino)]=0;
		//printk("<0>" "iat %d %d sa\n",iat_offset(ino),0xffffffff & inblock );
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		gsd->free_inodes++;
		gsb->sb->s_dirt=1;	
	}
	gsb->sgflags|=sgflag_IAT;
	return;
}

char clear_one_bit_in_cont_pages(struct GSFS_sb* gsb, char type, unsigned int index, unsigned int start, 
				 unsigned int end, unsigned int* free){
	struct buffer_head* bh;
	char prev=-1;
	
	if(bat_bn(index)>(end-start))
		return -1;
	
	bh=get_lru_bh(gsb, type, start+bat_bn(index));
	
	if(bh){
		char* pp;
		int	index_offset=bat_offset(index),
			page_offset=index_offset>>3,
			bit=index_offset&7;
					
		lock_buffer(bh);
		pp=(char*)bh->b_data;
		
		prev=pp[page_offset]&(1<<bit);
		if(prev){
			pp[page_offset] &= ~(1<<bit);
			
			mark_buffer_dirty(bh);
			set_buffer_uptodate(bh);
									
			(*free)++;
			gsb->sb->s_dirt=1;
		}
		
		unlock_buffer(bh);
	}
	
	return prev;
}

int BAT_clear_one_block(struct GSFS_sb* gsb, unsigned int bn){
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	char k;
	
	k=clear_one_bit_in_cont_pages(gsb, BAT_LRU, bn, gsd->bat_start, gsd->bat_end, &gsd->free_blocks);
	if(k!=0)
		gsb->sgflags|=sgflag_BAT;
	
	return k;
}

int SAT_clear_one_index(struct GSFS_sb* gsb, unsigned int index){
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	char k;
	
	k=clear_one_bit_in_cont_pages(gsb, SAT_LRU, index, gsd->sat_start, gsd->sat_end, &gsd->free_sec_indecis);
	if(k!=0)
		gsb->sgflags|=sgflag_SAT;
	
	return k;
}

char test_one_bit_in_cont_pages(struct GSFS_sb* gsb, char type, unsigned int index, unsigned int start, 
				 unsigned int end){
	struct buffer_head* bh;
	char prev=0;
	
	if(bat_bn(index)>(end-start))
		return -1;
	
	bh=get_lru_bh(gsb, type, start+bat_bn(index));
	
	if(bh){
		char* pp;
		int	index_offset=bat_offset(index),
			page_offset=index_offset>>3,
			bit=index_offset&7;
					
		lock_buffer(bh);
		pp=(char*)bh->b_data;
		
		prev=pp[page_offset]&(1<<bit);
		
		unlock_buffer(bh);
	}
	
	return prev;
}

int test_one_SAT_index(struct GSFS_sb* gsb, unsigned int index){
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	char k;
	
	k=test_one_bit_in_cont_pages(gsb, SAT_LRU, index, gsd->sat_start, gsd->sat_end);
	
	return k;
}

int get_some_bits_from_cont_pages(struct GSFS_sb* gsb, char type, unsigned int start, unsigned int end,
				unsigned int *last, unsigned int *free, unsigned int total,
				unsigned int some, unsigned int* res){
	unsigned int 	i,
			pg,
			pg_start,
			off,
			offlen,
			first_pg=0,
			bat_len;
	char* p;
	char  b;
	short  bit;
	struct buffer_head* bh;
	
	if(*free<some)
		return -1;
	
	bat_len=end-start+1;
	off=(bat_offset(*last))>>3;
	
	pg=bat_bn(*last);
	pg_start=pg;
	offlen=Block_Size;
	
	if(pg==(bat_len-1) && total!=Blocks_per_BAT_Block)
		offlen=bat_offset(total)>>3;
	
	bh=get_lru_bh(gsb, type, start+pg);
	lock_buffer(bh);
	p=(char*)bh->b_data;
	
	for(i=0;i<some;i++){
		while(off<offlen && p[off]==(char)0xff)
			off++;
		
		if(off<offlen && p[off]!=(char)0xff){
			b=p[off];
			bit=0;
			
			while(bit<8 && ((b>>bit)&0x01)!=0)
				bit++;
			
			res[i]=(pg<<Blocks_per_BAT_Block_Bits)+(off<<3)+bit;
			//printk("<0>" "bat_get_some blocks %d %d %d %d %d\n",i,off,(int)p[off],bit,res[i]);
			p[off] |= 1<<bit;
			
			mark_buffer_dirty(bh);
			set_buffer_uptodate(bh);
	
			continue;
		}
		
		if(pg==pg_start && first_pg){ //there is no more free block
			unlock_buffer(bh);
			some=i;
			*free=0;
			goto ret;
			
		}
		
		first_pg=1;
		
		//there is no bit in current page therefore we should open a new page
		pg++;	
		if(pg==bat_len)
			pg=0;
		
		offlen=Block_Size;
		
		if(pg==(bat_len-1))
			offlen=bat_offset(total)>>3;
		off=0;
	
		unlock_buffer(bh);
		
		bh=get_lru_bh(gsb, type, start+pg);
		lock_buffer(bh);
		p=(char*)bh->b_data;
		i--;
	}
	
	unlock_buffer(bh);
	
	(*free)-=some;
	*last=res[some-1];
ret:
	gsb->sb->s_dirt=1;
		
	return some;
}

inline int BAT_get_some_blocks(struct GSFS_sb* gsb,unsigned int some,unsigned int* res){
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	int k;
	
	k=get_some_bits_from_cont_pages(gsb, BAT_LRU, gsd->bat_start, gsd->bat_end, &gsd->last_block, 
					   &gsd->free_blocks, gsd->total_blocks, some, res);
	gsb->sgflags|=sgflag_BAT;
	return k;
}

inline unsigned int SAT_get_one_index(struct GSFS_sb* gsb){
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	int res;
	int l;
	
	l=get_some_bits_from_cont_pages(gsb, SAT_LRU, gsd->sat_start, gsd->sat_end, &gsd->last_sec_index, 
					   &gsd->free_sec_indecis, gsd->total_sec_indecis, 1, &res);
	gsb->sgflags|=sgflag_SAT;
	if(l==1)
		return res;
	else
		return -1;
}

unsigned int IAT_get_one_inode(struct GSFS_sb* gsb, unsigned int block){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct buffer_head* bh;
	unsigned int 	ino,
			lastip,
			iat_len;
			
	unsigned int * 	pp;
			
	if(gsd->free_inodes==0)
		return 0;
	
	iat_len=gsd->iat_end-gsd->iat_start;
	ino=gsd->last_inode+1;
	lastip=Blocks_per_IAT_Block-1;
	if(iat_bn(ino)==iat_len)
		lastip=iat_offset(gsd->total_inodes-1);
	bh=get_lru_bh(gsb,IAT_LRU,gsd->iat_start+iat_bn(ino));
	lock_buffer(bh);
	pp=(unsigned int*)bh->b_data;
	while(ino!=gsd->last_inode && pp[iat_offset(ino)]){
		if(iat_offset(ino)==lastip){
			if(ino==gsd->total_inodes-1)
				ino=0;
			else 
				ino++;
			lastip=Blocks_per_IAT_Block-1;
			if(iat_bn(ino)==iat_len)
				lastip=iat_offset(gsd->total_inodes-1);
			unlock_buffer(bh);
			bh=get_lru_bh(gsb,IAT_LRU,gsd->iat_start+iat_bn(ino));
			lock_buffer(bh);
			pp=(unsigned int*)bh->b_data;			
		}
		else 
			ino++;
	}
	if(pp[iat_offset(ino)])
		return 0;
	gsd->last_inode=ino;
	pp[iat_offset(ino)]=block;
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	gsb->sb->s_dirt=1;
	gsb->sgflags|=sgflag_IAT;
	return ino;
}

int set_get_IHP(struct GSFS_sb* gsb, unsigned int index, char* hash, char get){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct buffer_head* bh;
	char* page;
	int ret=-1;
	
	if(index>=gsb->gsb_disk.total_sec_indecis)
		return -1;
	
	bh=get_lru_bh(gsb, IHP_LRU, gsd->ihp_start+ihp_bn(index));
	
	if(bh){
		unsigned int offset=(ihp_offset(index)<<gsfs_hashlen_shift);
	
		lock_buffer(bh);
		
		page=(char*)bh->b_data;
		
		if(get)
			memcpy(hash, page+offset, gsfs_hashlen);
		else{
			memcpy(page+offset, hash, gsfs_hashlen);
			
			mark_buffer_dirty(bh);
			set_buffer_uptodate(bh);
			
			gsb->sb->s_dirt=1;
			gsb->sgflags|=sgflag_IHP;
		}
		
		unlock_buffer(bh);
		
		ret=0;
	}
		
	return ret;
}

inline int set_IHP(struct GSFS_sb* gsb, unsigned int index, char* hash){
	return set_get_IHP(gsb, index, hash, 0);
}

inline int get_IHP(struct GSFS_sb* gsb, unsigned int index, char* hash){
	return set_get_IHP(gsb, index, hash, 1);
}

inline int copy_mpi_to_disk(mpi_on_disk* dest,mpi* src){
	int ret;
	
	//dest->s=src->s;
	//dest->n=src->n;
	ret=copy_from_user(&dest->s, &src->s, sizeof(int));
	
	if(!ret)
		ret=copy_from_user(&dest->n, &src->n, sizeof(int));
	
	if(!ret)
		ret=copy_from_user(dest->p, src->p, src->n*sizeof(t_int));
	
	/*
	printk("<0>" "struct=%lx p:%lx %d\n",dest, dest->p,ret);
	printk("<0>" "struct=%lx p:%lx %d\n",src, src->p,ret);
	if(src->n>=2){
		printkey((char*)dest->p);
		printkey((char*)src->p);
	}
	*/
	return ret;
}

int add_new_public_key(struct GSFS_sb* gsb,rsa_context* rsa,uid_t uid){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct buffer_head *bh,*bh2;
	unsigned short 	i;
	unsigned int res;
	struct pb_page_entry* pen;
	struct on_disk_user_info *uinf;
	
	if(gsd->pb_num>=total_users)
		return -1;
	
	down_write(&gsb->rsa_keys_rwsem);
	
	bh=__bread(gsb->sb->s_bdev, gsd->pb_page, Block_Size);

	i=0;
	
	pen=(struct pb_page_entry*)bh->b_data;
	
	while(i<gsd->pb_num && i<total_users){
		if(pen->block!=0 && pen->uid==uid)
			goto bad_ret;
		
		if(pen->block==0)
			break;
		i++;
		pen++;
	}
	
	if(BAT_get_some_blocks(gsb,1,&res)!=1)
		goto bad_ret;
	
	//printk("<0>" "add_new_public_key: res:%d\n",res);
	pen=(struct pb_page_entry*)bh->b_data;
	
	pen+=i;
	pen->uid=uid;
	pen->block=res;
	
	gsd->pb_num++;
	
	set_buffer_uptodate(bh);
	mark_buffer_dirty(bh);
	brelse(bh);
	
	bh2=__bread(gsb->sb->s_bdev,res,Block_Size);
	lock_buffer(bh2);
	
	uinf=(struct on_disk_user_info*)bh2->b_data;
	memset(uinf,1,sizeof(struct on_disk_user_info));
	
	uinf->uid=uid;
	
	copy_mpi_to_disk(&uinf->user_pbk.N,&rsa->N);
	copy_mpi_to_disk(&uinf->user_pbk.E,&rsa->E);
	uinf->user_pbk.len=rsa->len;
	
	mark_buffer_dirty(bh2);
	set_buffer_uptodate(bh2);
	unlock_buffer(bh2);
	
	brelse(bh2);
	gsb->sb->s_dirt=1;
	gsb->sgflags|=sgflag_pb_page;
	
	up_write(&gsb->rsa_keys_rwsem);
	
	return 0;

bad_ret:
	up_write(&gsb->rsa_keys_rwsem);
	
	return -1;
}

inline void copy_mpi_from_disk(mpi* dest,mpi_on_disk* src){
	
	dest->s=src->s;
	dest->n=src->n;
	
	dest->p=kzalloc(src->n*sizeof(t_int),GFP_KERNEL);
	
	memcpy(dest->p, src->p, src->n*sizeof(t_int));
	
	/*
	printk("<0>" "copy_mpi_from_disk n:%d \n",src->n);
	if(src->n>=2){
	printkey(dest->p);
	printkey(src->p);
	}
	*/
	
	return;
}

rsa_context* get_public_key_from_disk(struct on_disk_public_key* odkey){
	rsa_context* rsa;
	
	rsa=kzalloc(sizeof(rsa_context),GFP_KERNEL);
	
	rsa_init(rsa);
	
	copy_mpi_from_disk(&rsa->N,&odkey->N);
	copy_mpi_from_disk(&rsa->E,&odkey->E);
	
	rsa->len=odkey->len;
	
	return rsa;
}

struct rsa_key* get_rsa_key(struct GSFS_sb* gsb,uid_t uid, char private){
	struct GSFS_sb_ondisk * gsd=&gsb->gsb_disk;
	struct rsa_key 	*key=0,
			*ret=0;
	struct pb_page_entry* pen;
	unsigned int 	block,
			i,
			j;
	struct buffer_head* bh;
	struct on_disk_user_info* od_uinf;
	
	//printk("<0>" "Get rsa key\n");
	
	down_read(&gsb->rsa_keys_rwsem);
	
	key=gsb->first_rsa_key;
	
	while(key){
		
		spin_lock(&key->lock);
		
		//printk("<0>" "grk: l1 uid:%d private:%d\n",key->uid,key->is_private);
		
		if(key->uid==uid){
			
			if(private && key->is_private)
				ret=key;
			else{
				if(!private)
					ret=key;
				else
					ret=0;
			}
			spin_unlock(&key->lock);
			
			up_read(&gsb->rsa_keys_rwsem);
			
			return ret;
		}
		
		spin_unlock(&key->lock);
		
		key=key->next;

	}
	
	//all of our on disk keys are public therefore :
	if(private){
		up_read(&gsb->rsa_keys_rwsem);
		
		return 0;
	}
	
	block=0;
	i=0;
	j=0;
	
	bh=__bread(gsb->sb->s_bdev, gsd->pb_page, Block_Size);
	pen=(struct pb_page_entry*)bh->b_data;
		
	while(i<gsd->pb_num && j<total_users){
		//printk("<0>" "grk: l2 block:%d uid:%d\n", pen->block, pen->uid);	
		if(pen->block!=0 && pen->uid==uid){			
			block=pen->block;
			break;
		}
		if(pen->block!=0)
			i++;
		pen++;
		j++;
	}
	brelse(bh);
		
	up_read(&gsb->rsa_keys_rwsem);
	
	if(i==gsd->pb_num || block==0)
		return 0;
	
	down_write(&gsb->rsa_keys_rwsem);
	
	key=0;
	bh=__bread(gsb->sb->s_bdev,block,Block_Size);
		
	lock_buffer(bh);
	
	od_uinf=(struct on_disk_user_info*)bh->b_data;
	
	//printk("<0>" "grk: l4 uid:%d od_uinf_uid:%d\n",uid ,od_uinf->uid);
	
	if(od_uinf->uid==uid){
		
		key=kzalloc(sizeof(struct rsa_key),GFP_KERNEL);
		
		if(gsb->first_rsa_key==0)
			gsb->first_rsa_key=key;
		if(gsb->last_rsa_key!=0)
			gsb->last_rsa_key->next=key;
		gsb->last_rsa_key=key;
	
		key->uid=uid;
		key->is_private=0;
		key->key=get_public_key_from_disk(&od_uinf->user_pbk);
	
		spin_lock_init(&key->lock);
	}
	unlock_buffer(bh);
		
	brelse(bh);	
	
	up_write(&gsb->rsa_keys_rwsem);
	
	return key;
}

//becareful that first we test
//if private key (user_rsa) is the private key of our public key for uid
int GSFS_add_new_private_key(struct super_block* sb,rsa_context* user_rsa,uid_t uid, char* pt, int ptlen, char* ct, int ctlen){
	struct GSFS_sb * gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct rsa_key	*key;
	int 	ret=0,
		olen;
	char	dpt[128];
	
		
	key=get_rsa_key(gsb, uid, 0);
	
	if(!key || !key->key || key->is_private)
		return -1;
	
	spin_lock(&key->lock);
	
	ret=rsa_1024_decrypt(key->key, RSA_PUBLIC, &olen, ct, dpt, 128);
	//printkey(dpt);
	//printkey(pt);
	//printk("<0>" "%d %d\n",ret,key->key->N.n);
	
	if(ret || olen!=ptlen )
		goto back;
	
	ret=strncmp(dpt,pt,ptlen);
	if(ret)
		goto back;
	
	rsa_init(key->key);
	
	mpi_copy_fu(&key->key->N,&user_rsa->N);
	mpi_copy_fu(&key->key->E,&user_rsa->E);
	mpi_copy_fu(&key->key->D,&user_rsa->D);
	mpi_copy_fu(&key->key->P,&user_rsa->P);
	mpi_copy_fu(&key->key->Q,&user_rsa->Q);
	mpi_copy_fu(&key->key->DP,&user_rsa->DP);
	mpi_copy_fu(&key->key->DQ,&user_rsa->DQ);
	mpi_copy_fu(&key->key->QP,&user_rsa->QP);
	mpi_copy_fu(&key->key->RN,&user_rsa->RN);
	mpi_copy_fu(&key->key->RP,&user_rsa->RP);
	mpi_copy_fu(&key->key->RQ,&user_rsa->RQ);
	
	key->key->len=user_rsa->len;
	
	key->is_private=1;
	
	ret=0;
	
	/*
	char a[20],b[200],c[200];
	int clen;
	ret=rsa_1024_encrypt(key->key, RSA_PRIVATE, 20,a,b);
	printk("<0>" "%d\n",ret);
	ret=rsa_1024_decrypt(key->key, RSA_PUBLIC, &clen, b, c, 128);
	printk("<0>" "%d %d %d\n",ret,strncmp(a,c,20),clen);
	*/
	
back:	spin_unlock(&key->lock);

	return ret;
};

int GSFS_remove_private_key(struct super_block* sb, uid_t uid){
	struct GSFS_sb * gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct rsa_key	*key,
			*prev;
	int 	ret=-1;
	
	down_write(&gsb->rsa_keys_rwsem);

	key=gsb->first_rsa_key;
	prev=0;
	while(key){		
		spin_unlock_wait(&key->lock);
		
		if(key->uid==uid && key->is_private){
			if(prev==0)
				gsb->first_rsa_key=key->next;
			else
				prev->next=key->next;
			if(gsb->last_rsa_key==key)
				gsb->last_rsa_key=prev;
			rsa_free(key->key);
			kfree(key);
			ret=0;
			break;
		}
		if(key->uid==uid){
			ret=-1;
			break;
		}
		prev=key;
		key=key->next;
	}
	
	up_write(&gsb->rsa_keys_rwsem);
	
	return -1;
}

void free_all_rsa_keys(struct GSFS_sb* gsb){
	struct rsa_key* rkey;
	
	down_write(&gsb->rsa_keys_rwsem);
	
	rkey=gsb->first_rsa_key;
	
	while(rkey){
		struct rsa_key* temp;
		
		spin_unlock_wait(&rkey->lock);
		
		if(rkey->key){
			if(rkey->is_private)
				rsa_free(rkey->key);
			else
				mpi_free(&rkey->key->N,&rkey->key->E,0);
		}
		
		kfree(rkey->key);
		temp=rkey->next;
		kfree(rkey);
		rkey=temp;
	}
	
	up_write(&gsb->rsa_keys_rwsem);
	
	gsb->first_rsa_key=0;
	gsb->last_rsa_key=0;
	
	return;
}
