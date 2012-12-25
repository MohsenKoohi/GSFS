//in header:

#define max_iml_lru_levels (4)		//intlog(max_inodes=2^30,225)
struct IML_LRU_spec{
	unsigned int	levels,
			indecis_num,
			start_bn_of_level[max_iml_lru_levels],
			end_bn_of_level[max_iml_lru_levels],
			len_of_level[max_iml_lru_levels];
};

#define iml_igt_len_bits	(1)
#define iml_igt_len		(1<<iml_igt_len_bits)
#define iml_igt_mask		(3)
#define iml_lru_integrity_len	(1<<(Block_Size_Bits-gsfs_hashlen_shift-3+iml_igt_len_bits))
#define iml_lru_integrity_max	(1<<(Block_Size_Bits-gsfs_hashlen_shift))

enum{
	IGT_NOT_TESTED=0,
	IGT_OK=1,
	IGT_CORRUPTED=2,
	IGT_CHANGED=3,
};

#define iml_lru_l0_pages	1//(256)
#define iml_lru_l1_pages	1//(128)
#define iml_lru_l2_pages	1//(64)
#define iml_lru_l3_pages	(1)

struct IML_LRU{
	struct IML_LRU_spec	*spec;
	
	struct block_device	*bdev;
	
	short			top_level,
				levels_num;
	
	char			root_hash[gsfs_hashlen];
	
	struct rw_semaphore	*iml_lru_rwsem;
	struct buffer_head      ***iml_lru_bh;
	unsigned long 		**iml_lru_time;
	char 			***iml_lru_integrity;
	sector_t 		**iml_lru_bh_number;
	short 			*iml_lru_count;
	void 			(*update_fn)(void*);
	void			* update_data;
};

//iml_lru.c
void set_integrity_array_index(char *array, unsigned short index, unsigned char val);
unsigned char get_integrity_array_val_of_index(char *array, unsigned short index);
int initialize_iml_lru(struct IML_LRU *il, struct IML_LRU_spec *spec, struct block_device *bdev, 
		       char* root_hash, void (*update_fn)(void*), void* data);
int initialize_iml_lru_pages(struct IML_LRU_spec *spec, struct block_device* bdev, char* root_hash);
void exit_iml_lru(struct IML_LRU *il);
void sync_iml_lru(struct IML_LRU *il);
int iml_lru_set_hash(struct IML_LRU* il, unsigned int index, char* dest_hash);
int iml_lru_get_hash(struct IML_LRU* il, unsigned int index, char* dest_hash);

//in super for create:
/*
gsd->ihp_spec.levels=0;
	gsd->ihp_spec.indecis_num=gsd->total_inodes;
	
	while(l>1){
		unsigned int m=0;
		
		if(l%fl_cn)
			m=1;
		m+=(l/fl_cn);
		gsd->ihp_spec.start_bn_of_level[i]=k;
		gsd->ihp_spec.len_of_level[i]=m;
		k+=(m-1);
		gsd->ihp_spec.end_bn_of_level[i]=k;
		gsd->ihp_spec.levels++;
		k++;
		i++;
		l=m;
		
		printk("<0>" "l:%u, start:%lu;, end:%lu, len:%u, k:%lu, i:%lu\n",l, \
			gsd->ihp_spec.start_bn_of_level[i-1], gsd->ihp_spec.end_bn_of_level[i-1],\
			gsd->ihp_spec.len_of_level[i-1], k ,i-1);
		
	}
	k--;
//end create
*/
#include "gsfs.h"

#define down_write(p)
#define down_read(p)
#define up_write(p)
#define up_read(p)
#define lock_buffer(p)
#define unlock_buffer(p)

struct buffer_head* iml_lru_get_bh_and_its_integ_array(struct IML_LRU *il, unsigned char level, 
						       unsigned int index, char ** integ_array);

int levels_pages []={	iml_lru_l0_pages, 
			iml_lru_l1_pages, 
			iml_lru_l2_pages, 
			iml_lru_l3_pages };

#define print_integ_array(dest,integ_array) printhexstring(integ_array, dest, iml_lru_integrity_len)

#ifdef gsfs_test
	#define iml_lru_test
#endif

#ifdef iml_lru_test
	#define gwil(p)	p
#else
	#define gwil(p)
#endif

void print_il_buffers(struct IML_LRU* il){
	char 	repp[1000],
		*rep=repp;
	int 	i,
		j;
	
	for(i=0;i<il->levels_num;i++){
		sprintf(rep, "* level%d: count:%d :",i, il->iml_lru_count[i]);
		rep+=strlen(rep);

		for(j=0; j<il->iml_lru_count[i]; j++){
			sprintf(rep, "#%d: bh_num:%lu", j, il->iml_lru_bh_number[i][j]);
			rep+=strlen(rep);
		}
	}
	
	printk("<0>" "print_il_buffers: %s\n",repp);
	
	return;
}

void set_integrity_array_index(char *array, unsigned short index, unsigned char val){
	unsigned short	byte_offset,
			bit_offset,
			val1=val,
			k;
			
	if( unlikely(index >= iml_lru_integrity_max) )
		return;
	
	byte_offset=index>>(3-iml_igt_len_bits);
	bit_offset=(index & iml_igt_mask)<<iml_igt_len_bits;
	
	k=iml_igt_mask<<bit_offset;
	array[byte_offset]&=~k;
	
	val=(val & iml_igt_mask)<<bit_offset;
	array[byte_offset]|=val;
	
	//gwil(printk("<0>" "set_integrity_array_index for  index:%d, val1:%d, byte_offset:%d, bit_offset:%d, val:%d\n",
	//		index, val1, byte_offset, bit_offset, val));
	
	return;
}

unsigned char get_integrity_array_val_of_index(char *array, unsigned short index){
	unsigned short	byte_offset,
			bit_offset,
			val;
			
	if(index>iml_lru_integrity_max)
		return -1;
	
	byte_offset=index>>(3-iml_igt_len_bits);
	bit_offset=(index & iml_igt_mask)<<iml_igt_len_bits;
	
	val=array[byte_offset]>>bit_offset;
	val&=iml_igt_mask;
	
	return val;
}

//we assume that page root is verified and its integ_array index is set to IGT_OK
//returns one of IGTs
unsigned char iml_lru_verify_integrity(char* page, char* integ_array, unsigned short index){
	int 	i,
		j,
		prev_fault,
		verified[hash_levels],
		level_div[]={fl_hg, sl_hg, tl_hg},
		level_off[]={fl_off, sl_off, tl_off, root_off};
	unsigned char	ret=0;
	
	#ifdef iml_lru_test
	char 	repp[1000],
		*rep=repp;
		
		sprintf(rep,"iml_lru_verify_integrity for index:%d and integ_array: ", index);
		rep+=strlen(rep);
		print_integ_array(rep, integ_array);
		rep+=strlen(rep);
	#endif
	
	j=index;
	prev_fault=0;
	
	for(i=0;i<=hash_levels;i++){
		int		start,
				pstart;
		char		phash[gsfs_hashlen];
		
		if(!prev_fault)
			ret=get_integrity_array_val_of_index(integ_array, j+level_off[i]);
		
		gwil(sprintf(rep, "# iteration i:%d, prev_fault:%d  ret:%d ", i, prev_fault, ret));
		gwil(rep+=strlen(rep));
		
		if(ret==IGT_OK || (i==0 && ret==IGT_CHANGED) || ret==IGT_CORRUPTED){
			int k;
			
			for(j=0;j<i;j++)
				for(k=0;k<level_div[j];k++)
					set_integrity_array_index(integ_array, level_off[j]+verified[j]+k, ret);
			
			gwil(sprintf(rep, "new integ_array: "));
			gwil(rep+=strlen(rep));
			gwil(print_integ_array(rep, integ_array));
			
			gwil(printk("<0>" "%s * \n", repp));
			
			return ret;
		}
		
		if(i==hash_levels || ret==IGT_CHANGED)
			break;
		
		j/=level_div[i];
		start=(level_off[i]+j*level_div[i])<<gsfs_hashlen_shift;
				
		skein512(gsfs_hashlen_bits, page+start, level_div[i]<<(gsfs_hashlen_shift+3), phash);
		
		pstart=((j+level_off[i+1])<<gsfs_hashlen_shift);
		
		prev_fault=0;
		if(strncmp(page+pstart, phash, gsfs_hashlen)){
			prev_fault=1;
			ret=IGT_CORRUPTED;
		}
		
		verified[i]=j*level_div[i];
		
		gwil(sprintf(rep, " new_j:%d, start/16:%d, pstart/16:%d, prev_fault:%d, verified[i]:%d "\
					,j, start/16, pstart/16, prev_fault, verified[i]));
		gwil(rep+=strlen(rep));
	}
	
	printk("<1>" "Verifying hash reached final return\n");
	
	gwil(sprintf(rep, "* Verifying hash reached final return\n"));
	gwil(printk("<0>" "%s * \n", repp));
	
	return -1;
}

int iml_lru_update_hash(struct IML_LRU *il,unsigned char level,unsigned int index, unsigned short offset, char* hash){
	struct buffer_head *bh;
	char * integ_array=0;
	int ret=-1;
	
	#ifdef iml_lru_test
	char	repp[1500],
		*rep=repp;
		
		sprintf(rep, "iml_lru_update_hash for level:%d, index:%d, offset:%d, new hash:", level, index, offset);
		rep+=strlen(rep);
		printhexstring(hash, rep, 16);
		rep+=strlen(rep);
	#endif
	
	bh=iml_lru_get_bh_and_its_integ_array(il, level, index, &integ_array);

	if(likely(bh && integ_array)){
		char *page;
		int k;
		
		lock_buffer(bh);
		
		page=(char*)bh->b_data;
		
		k=iml_lru_verify_integrity(page, integ_array, offset);
		
		gwil(sprintf(rep, "ret of verify_integrity: %d * ",k));
		rep+=strlen(rep);
		
		if(k==IGT_OK || k==IGT_CHANGED){
			memcpy(page+(offset<<gsfs_hashlen_shift), hash, gsfs_hashlen);
			
			set_integrity_array_index(integ_array, offset, IGT_CHANGED);
			
			set_buffer_uptodate(bh);
			mark_buffer_dirty(bh);
			
			//il->update_fn(il->update_data);
			
			ret=0;
		}
		
		unlock_buffer(bh);
	}
	
	gwil(printk("<0>" "%s * ret:%d *\n",repp, ret));
	
	return ret;
}

//becareful that index here is index in il and isn't index of a page for level
void synchronize_bh(struct IML_LRU* il, unsigned char level,unsigned int index, char get_level_sem){
	struct buffer_head *bh;
	unsigned char root_hash[gsfs_hashlen];
	unsigned short	changes[fl_cn/fl_hg],
			num=0,
			k=0;
	unsigned int	parent_index=0,
			offset_in_parent=0,
			i=0;
	char *ia;
	
	#ifdef iml_lru_test
	char	repp[1500],
		*rep=repp;
		
		sprintf(rep, "synchronize_bh for level:%d, index:%d, get_level_sem:%d and integ_array: ", level, index, get_level_sem);
		rep+=strlen(rep);
		print_integ_array(rep, il->iml_lru_integrity[level][index]);
		rep+=strlen(rep);
		sprintf(rep, " * ");
		rep+=strlen(rep);
	#endif
	
	//if(get_level_sem)
	//	down_write(&il->iml_lru_rwsem[level]);
	
	ia=il->iml_lru_integrity[level][index];
	bh=il->iml_lru_bh[level][index];
	
	lock_buffer(bh);
	
	for(i=0; i<fl_cn/fl_hg; i++){
		unsigned char	update_exist=0,
				failed_exist=0,
				j;
		
		k=i*fl_hg;
		for(j=0; j<fl_hg; j++, k++){
			int ret;
			
			ret=get_integrity_array_val_of_index(ia, k);
			
			if(ret==IGT_CHANGED)
				update_exist=1;
			
			if(ret==IGT_CORRUPTED){
				failed_exist=1;
				break;
			}
		}
		
		k=i*fl_hg;
		if(update_exist && !failed_exist){
			changes[num++]=k;
			
			for(j=0; j<fl_hg; j++, k++)
				set_integrity_array_index(ia, k, IGT_OK);
				
			gwil(sprintf(rep, "i: %d, up_ex:%d #", i, update_exist));
			gwil(rep+=strlen(rep));
		}
	}
	
	gwil(sprintf(rep, "num=%d * ",num));
	gwil(rep+=strlen(rep));
	
	k=0;
	if(num){
		char* page=(char*)bh->b_data;
		
		k=update_hash_block_to_root(page, changes, num);
		memcpy(root_hash, page+hash_root_offset, gsfs_hashlen);
		
		gwil(sprintf(rep, "new root_hash: "));
		gwil(rep+=strlen(rep));
		gwil(printhexstring(root_hash, rep, gsfs_hashlen));
		gwil(rep+=strlen(rep));
		
		if(k==0){
			int ind=il->iml_lru_bh_number[level][index];
			
			k=1;
			parent_index=ind/fl_cn;
			offset_in_parent=ind%fl_cn;
			
			gwil(sprintf(rep, "k=%d, level:%d, ind:%d,  parent_index:%d, offset_in_parent:%d * ",\
					k, level, ind, parent_index, offset_in_parent));
			gwil(rep+=strlen(rep));
		
		}
		
		set_buffer_uptodate(bh);
		mark_buffer_dirty(bh);
		unlock_buffer(bh);
		//write_one_bh_dev(bh);
	}
	else
		unlock_buffer(bh);
	
	//if(get_level_sem)
	//	up_write(&il->iml_lru_rwsem[level]);
	
	if(k==1){
		if(level!=il->top_level)
			iml_lru_update_hash(il, level+1, parent_index, offset_in_parent, root_hash);
		else
			memcpy(il->root_hash, root_hash, gsfs_hashlen);
	}
back:	
	gwil(sprintf(rep, "new integ_array : "));
	gwil(rep+=strlen(rep));
	gwil(print_integ_array(rep, il->iml_lru_integrity[level][index]));
	gwil(rep+=strlen(rep));
	gwil(sprintf(rep, " * "));
	gwil(rep+=strlen(rep));
	
	gwil(printk("<0>" "%s\n",repp));
	
	return;
}

//this function only read bh from disk and adjust its root hash with root_hash
//if it is integrated it is added to iml-lru
int iml_lru_read_bh(struct IML_LRU* il, unsigned char level, unsigned int page_index, char* root_hash){
	int	min,
		k,
		ret=-1;
	char	*pp;
	struct buffer_head* bh;
	#ifdef iml_lru_test
	char 	repp[1000],
		*rep=repp;
	#endif
	
	gwil(sprintf(rep, "iml_lru_read_bh for level:%d, page_index:%d, il->top_level:%d, il->spec->len_of_level[level]:%d * ",\
			 level, page_index, il->top_level, il->spec->len_of_level[level]));
	
	gwil(rep+=strlen(rep));
	
	if(level > il->top_level)
		goto back;
	
	if(page_index>= il->spec->len_of_level[level])
		goto back;	
	
	bh=read_one_bh_dev(il->bdev, il->spec->start_bn_of_level[level]+page_index);
	
	gwil(sprintf(rep, "read bh for page:%d and bh:%lx * ",il->spec->start_bn_of_level[level]+page_index,(unsigned long)bh));
	gwil(rep+=strlen(rep));
	
	if(!bh)
		goto back;
		
	lock_buffer(bh);	
	pp=(char*)bh->b_data;
	
	gwil(sprintf(rep, "input root hash: "));
	gwil(rep+=strlen(rep));
	gwil(printhexstring(root_hash, rep, 16));
	gwil(rep+=strlen(rep));
	gwil(sprintf(rep, "and page root hash: "));
	gwil(rep+=strlen(rep));
	gwil(printhexstring(&pp[hash_root_offset], rep, 16));
	gwil(rep+=strlen(rep));
	
	k=strncmp(root_hash, &pp[hash_root_offset], gsfs_hashlen);
	
	gwil(sprintf(rep, "ret of strncmp :%d * ",k));
	gwil(rep+=strlen(rep));
	
	unlock_buffer(bh);
	
	if(k){
		if(level!=il->top_level){
			char *ia=0;
			int page_offset=page_index/fl_cn;
			
			bh=iml_lru_get_bh_and_its_integ_array(il, level+1, page_offset, &ia);
			if(ia && bh){
				int ind=page_index%fl_cn;
				
				lock_buffer(bh);
				set_integrity_array_index(ia, ind, IGT_CORRUPTED);
				unlock_buffer(bh);
				
				gwil(sprintf(rep, "setting index in parent(%d) integ_array to IGT_CORRUPTED * ",ind));
				gwil(rep+=strlen(rep));
			}
		}
		goto back;
	}
	
	get_bh(bh);
		
	//adding new bh to iml-lru
	gwil(sprintf(rep, "add new bh to iml-lru * "));
	gwil(rep+=strlen(rep));
	
	down_write(&il->iml_lru_rwsem[level]);
	
	if(il->iml_lru_count[level]<levels_pages[level]){
		//we can add new bh to iml-lru
		min=il->iml_lru_count[level];
		il->iml_lru_count[level]++;
	}
	else{
		//we should use the least recently used place for this new bh
		int i;
		
		min=0;
		for(i=1;i<levels_pages[level];i++)
			if(il->iml_lru_time[level][i]<il->iml_lru_time[level][min])
				min=i;
		
		if(buffer_dirty(il->iml_lru_bh[level][min]))
			synchronize_bh(il, level, min, 0);
		else
			wait_on_buffer(il->iml_lru_bh[level][min]);
		
		brelse(il->iml_lru_bh[level][min]);
	}
	gwil(sprintf(rep, "place of bh (min):%d * ",min));
	gwil(rep+=strlen(rep));
	
	il->iml_lru_bh[level][min]=bh;
	il->iml_lru_time[level][min]=jiffies_64;	
	il->iml_lru_bh_number[level][min]=page_index;
	memset(il->iml_lru_integrity[level][min], 0, iml_lru_integrity_len);
	set_integrity_array_index(il->iml_lru_integrity[level][min], root_off, IGT_OK);
	
	up_write(&il->iml_lru_rwsem[level]);
	
	ret=0;
	
	gwil(sprintf(rep, " integ_array: "));
	gwil(rep+=strlen(rep));
	gwil(print_integ_array(rep, il->iml_lru_integrity[level][min]));
	gwil(rep+=strlen(rep));

back:
	gwil(printk("<0>" "%s * ret:%d *\n",repp,ret));
	
	return ret;
	
}

int initialize_iml_lru(struct IML_LRU *il, struct IML_LRU_spec *spec, struct block_device *bdev,
		       char *root_hash, void (*update_fn)(void*), void* data){
	int 	i,
		ret=0;
	
	il->spec=spec;
	il->bdev=bdev;	
	il->top_level=il->spec->levels-1;
	il->levels_num=il->spec->levels;
	
	il->iml_lru_rwsem=kzalloc(sizeof(struct rw_semaphore)*il->levels_num, GFP_KERNEL);
	il->iml_lru_bh=kzalloc(sizeof(struct buffer_head**)*il->levels_num, GFP_KERNEL);
	il->iml_lru_time=kzalloc(sizeof(unsigned long*)*il->levels_num, GFP_KERNEL);
	il->iml_lru_integrity=kzalloc(sizeof(char**)*il->levels_num, GFP_KERNEL);
	il->iml_lru_bh_number=kzalloc(sizeof(sector_t *)*il->levels_num, GFP_KERNEL);
	il->iml_lru_count=kzalloc(sizeof(short)*il->levels_num, GFP_KERNEL);
	
	for(i=0; i<il->levels_num; i++){
		int j;
		
		init_rwsem(&il->iml_lru_rwsem[i]);
		
		il->iml_lru_bh[i]=kzalloc(sizeof(struct buffer_head*)*levels_pages[i], GFP_KERNEL);
		
		il->iml_lru_time[i]=kzalloc(sizeof(unsigned long)*levels_pages[i], GFP_KERNEL);
		
		il->iml_lru_integrity[i]=kzalloc(sizeof(char *)*levels_pages[i], GFP_KERNEL);
		for(j=0; j<levels_pages[i]; j++)
			il->iml_lru_integrity[i][j]=kzalloc(sizeof(char)*iml_lru_integrity_len, GFP_KERNEL);
		
		il->iml_lru_bh_number[i]=kzalloc(sizeof(sector_t)*levels_pages[i], GFP_KERNEL);
				
		il->iml_lru_count[i]=0;
	}
	
	if(iml_lru_read_bh(il, il->top_level, 0, root_hash)){
		exit_iml_lru(il);
		ret=-1;
	}
	
	il->update_fn=update_fn;
	il->update_data=data;
	
	gwil(printk("<0>" "initialize_iml_lru with ret:%d\n",ret));
	
	return ret;
}

int initialize_iml_lru_pages(struct IML_LRU_spec *spec, struct block_device* bdev, char* root_hash){
	struct buffer_head *bh;
	unsigned short 	i,
			len=fl_cn/fl_hg,
			changes[fl_cn/fl_hg];
	char 		prev_root_hash[gsfs_hashlen];
		
	#ifdef iml_lru_test
	char 	repp[1000],
		*rep=repp;
		
	sprintf(rep, "initialize_iml_lru_pages * ");
	rep+=strlen(rep);
	#endif
	
	memset(prev_root_hash, 0, gsfs_hashlen);
	
	for(i=0; i<len; i++)
		changes[i]=i*fl_hg;
	
	gwil(sprintf(rep, "len: %d * ",len));
	gwil(rep+=strlen(rep));
	
	for(i=0; i<spec->levels; i++){
		int j=0;
		char* page;
		
		bh=read_one_bh_dev(bdev, spec->start_bn_of_level[i]);
		get_bh(bh);
		lock_buffer(bh);
		
		page=(char*)bh->b_data;
		
		for(j=0; j<fl_cn; j++)
			memcpy(page+(j<<gsfs_hashlen_shift), prev_root_hash, gsfs_hashlen);
		
		update_hash_block_to_root(page, changes, len);
		
		gwil(sprintf(rep, "* iteration #%d: start_bn_of_level: %d, end_bn_of_level:%d prev_root_hash:",\
				  i,spec->start_bn_of_level[i], spec->end_bn_of_level[i]));
		gwil(rep+=strlen(rep));
		gwil(printhexstring(prev_root_hash, rep, 16));
		gwil(rep+=strlen(rep));
		
		memcpy(prev_root_hash, page+hash_root_offset, gsfs_hashlen);
		
		for(j=(spec->start_bn_of_level[i]+1); j<=spec->end_bn_of_level[i]; j++){
			struct buffer_head* bh2;
			char* page2;
			
			bh2=read_one_bh_dev(bdev, j);
			get_bh(bh2);
			lock_buffer(bh2);
			
			page2=(char*)bh2->b_data;
			memcpy(page2, page, Block_Size);
			
			mark_buffer_dirty(bh2);
			set_buffer_uptodate(bh2);
			unlock_buffer(bh2);
			write_one_bh_dev(bh2);
			put_bh(bh2);
		}
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		write_one_bh_dev(bh);
		put_bh(bh);
	}
	
	memcpy(root_hash, prev_root_hash, gsfs_hashlen);
	
	gwil(sprintf(rep, "* new root_hash: "));
	gwil(rep+=strlen(rep));
	gwil(printhexstring(prev_root_hash, rep, 16));
	gwil(rep+=strlen(rep));
		
	gwil(printk("<0>" "%s\n",repp));
	
	return 0;
}

void exit_iml_lru(struct IML_LRU* il){
	int i;
	
	gwil(printk("<0>" "exit_iml_lru\n"));
	
	for(i=0; i<il->levels_num; i++){
		int j;
		
		down_write(&il->iml_lru_rwsem[i]);
		
		kfree(il->iml_lru_bh[i]);
		
		kfree(il->iml_lru_time[i]);
		
		for(j=0; j<levels_pages[i]; j++)
			kfree(il->iml_lru_integrity[i][j]);
		kfree(il->iml_lru_integrity[i]);
		
		for(j=0; j<il->iml_lru_count[i]; j++)
			brelse(il->iml_lru_bh[i][j]);
		kfree(il->iml_lru_bh_number[i]);
				
		up_write(&il->iml_lru_rwsem[i]);
	}

	kfree(il->iml_lru_rwsem);
	kfree(il->iml_lru_bh);
	kfree(il->iml_lru_time);
	kfree(il->iml_lru_integrity);
	kfree(il->iml_lru_bh_number);
	kfree(il->iml_lru_count);
		
	memset(il, 0, sizeof(struct IML_LRU));
	
	return;
}

int iml_lru_get_root_hash_of_page_from_upper_level(struct IML_LRU* il, unsigned char level, 
						   unsigned int index, char *root_hash){
	int 	par_index,
		par_offset,
		ret=-1,
		k;
	struct buffer_head *bh;
	char 	*integ_array=0,
		*pp;
		
	#ifdef iml_lru_test
	char 	repp[1000],
		*rep=repp;
		
	sprintf(rep, "iml_lru_get_root_hash_of_page_from_upper_level for level: %d, index: %d, il->top_level:%d *",\
		level, index, il->top_level);
	rep+=strlen(rep);
	#endif
	
	if(unlikely( !root_hash || (level>(il->top_level-1)) ))
		goto back;
	
	par_index=index/fl_cn;
	par_offset=index%fl_cn;
	
	bh=iml_lru_get_bh_and_its_integ_array(il, level+1, par_index, &integ_array);
	gwil(sprintf(rep, "par_index: %d, par_offset:%d bh: %lx, integ_array:%lx * ", par_index, par_offset, (unsigned long)bh, (unsigned long)integ_array));
	gwil(rep+=strlen(rep));
	
	if(!bh || !integ_array)
		goto back;
	
	gwil(print_integ_array(rep, integ_array));
	gwil(rep+=strlen(rep));
	
	k=get_integrity_array_val_of_index(integ_array, root_off);
	gwil(sprintf(rep, "ret of get_integrity_array_val_of_index(k) for root_off: %d * ",k));
	gwil(rep+=strlen(rep));
	
	if(k!=IGT_OK)
		goto back;
	
	lock_buffer(bh);
	pp=(char*)bh->b_data;
	
	k=get_integrity_array_val_of_index(integ_array, par_offset);
	gwil(sprintf(rep, "ret of get_integrity_array_val_of_index(k) for index: %d * ",k));
	gwil(rep+=strlen(rep));
		
	if(k==IGT_NOT_TESTED){
		k=iml_lru_verify_integrity(pp, integ_array, par_offset);
		
		gwil(sprintf(rep, "ret of iml_lru_verify_integrity(k): %d * ",k));
		gwil(rep+=strlen(rep));
	}
	
	if(likely( k==IGT_OK || k==IGT_CHANGED )){
		memcpy(root_hash, pp+(par_offset<<gsfs_hashlen_shift), gsfs_hashlen);
		ret=0;
		
		gwil(sprintf(rep, "returened integrated: root_hash from upper_level is : "));
		gwil(rep+=strlen(rep));
		gwil(printhexstring(root_hash, rep, 16));
		gwil(rep+=strlen(rep));
	}
	else
		ret=-1;
	
	unlock_buffer(bh);
	
back:
	gwil(printk("<0>" "%s * ret:%d\n", repp, ret));
	
	return ret;
}

struct buffer_head* iml_lru_get_bh_and_its_integ_array(struct IML_LRU *il, unsigned char level, 
						       unsigned int index, char ** integ_array){
	struct buffer_head *bh=0;
	int 	i,
		search_again=0;
	
	#ifdef iml_lru_test
	char	repp[1000],
		*rep=repp;
	
	sprintf(rep, "iml_lru_get_bh_and_its_integ_array for level: %d, index: %d * ",level , index);
	rep+=strlen(rep);
	#endif
	
	if(level>il->top_level)
		goto bad_back;
	
	if(index>=il->spec->len_of_level[level])
		goto bad_back;
search:
	down_read(&il->iml_lru_rwsem[level]);
	
	gwil(sprintf(rep, "searching * "));
	gwil(rep+=strlen(rep));
	
	for(i=0; i<il->iml_lru_count[level]; i++)
		if(il->iml_lru_bh_number[level][i]==index){
			int k;
			
			gwil(sprintf(rep, "i=%d * integ_array: ",i));
			gwil(rep+=strlen(rep));
			gwil(print_integ_array(rep, il->iml_lru_integrity[level][i]));
			gwil(rep+=strlen(rep));
			
			k=get_integrity_array_val_of_index(il->iml_lru_integrity[level][i], root_off);
			
			gwil(sprintf(rep, " * k:%d * ",k));
			gwil(rep+=strlen(rep));
			
			if(k==IGT_OK){
				bh=il->iml_lru_bh[level][i];
				if(integ_array)
					*integ_array=il->iml_lru_integrity[level][i];
			}
			else{ 
				bh=0;
				if(integ_array)
					*integ_array=0;
			}
			il->iml_lru_time[level][i]=jiffies_64;
			
			up_read(&il->iml_lru_rwsem[level]);
			
			goto back;
		}
		
	up_read(&il->iml_lru_rwsem[level]);
	
	gwil(sprintf(rep, "no result in search * "));
	gwil(rep+=strlen(rep));
	
	if(!search_again){
		char root_hash[gsfs_hashlen];
		
		search_again=1;
			
		memset(root_hash, 0, gsfs_hashlen);
		i=iml_lru_get_root_hash_of_page_from_upper_level(il, level, index, root_hash);
		
		gwil(sprintf(rep, "iml_lru_get_root_hash_of_page_from_upper_level : %d* ",i));
		gwil(rep+=strlen(rep));
		
		if(!i){
			i=iml_lru_read_bh(il, level, index, root_hash);
			
			gwil(sprintf(rep, "iml_lru_read_bh: %d* ",i));
			gwil(rep+=strlen(rep));
					
			if(!i)
				goto search;
		}
	}

bad_back:
	if(integ_array)
		*integ_array=0;
	bh=0;

back:
	gwil(printk("<0>" "%s * bh:%lx * \n",repp,(unsigned long)bh));
	
	return bh;
}

void sync_iml_lru(struct IML_LRU *il){
	short i=0;
	int j;
	
	for(i=0; i<il->levels_num; i++){
		
		down_write(&il->iml_lru_rwsem[i]);
		
		for(j=0; j<il->iml_lru_count[i]; j++){
			if(buffer_dirty(il->iml_lru_bh[i][j]))
				synchronize_bh(il, i, j, 0);
				//write_one_bh_dev(il->iml_lru_bh[i][j]);
		}
		
		up_write(&il->iml_lru_rwsem[i]);
	}
	
	return;
}

int iml_lru_get_hash(struct IML_LRU* il, unsigned int index, char* dest_hash){
	struct buffer_head *bh;
	char * integ_array=0;
	int	ret=-1,
		page_offset=index%fl_cn,
		page_index=index/fl_cn;
	
	if(index>=il->spec->indecis_num)
		return -1;
			
	bh=iml_lru_get_bh_and_its_integ_array(il, 0, page_index, &integ_array);
	
	if(likely(bh && integ_array)){
		char *page;
		int k;
		
		lock_buffer(bh);
		
		page=(char*)bh->b_data;
		
		k=iml_lru_verify_integrity(page, integ_array, page_offset);
		
		if(k==IGT_OK || k==IGT_CHANGED){
			memcpy(dest_hash, page+(page_offset<<gsfs_hashlen_shift), gsfs_hashlen);
			
			ret=0;
		}
		
		unlock_buffer(bh);
	}
	
	return ret;
}

inline int iml_lru_set_hash(struct IML_LRU* il, unsigned int index, char* src_hash){
	int ret;
	int 	page_index=index/fl_cn,
		page_offset=index%fl_cn;

	if(index>=il->spec->indecis_num)
		return -1;
	
	//print_il_buffers(il);
	
	ret=iml_lru_update_hash(il, 0, page_index, page_offset, src_hash);
	
	//print_il_buffers(il);
	
	return ret;
}
