#include "gsfs.h"
void GSFS_invalidatepage(struct page *page, unsigned long offset);
#ifdef gsfs_test
	#define development_pagecache
#endif

#ifdef development_pagecache
	#define gwas(mesg) mesg
#else
	#define	gwas(mesg) 
#endif

#define ALLOC_PAGES_MAX_ORDER 5
#define Loop_Pages_WB 10000

#define aes_blocks_per_page 	(Block_Size/gsfs_aes_keylen)

#define BCOUNT_MIN_OCL	0000

unsigned int get_some_pages(unsigned int len,struct page **res){
	unsigned int 	order,
			ret,
			rescount,
			i,
			pow;
	struct page* pages;
	
	rescount=0;
	ret=0;
	order=0;
	pow=1;
	while(len){
		//printk("<0>" "gsp : %u  %u %u %u\n",order,len,pow,ret);
		if((len&1) || (order==ALLOC_PAGES_MAX_ORDER)){
			pages=alloc_pages(GFP_KERNEL,order);
			//printk("<0>" "Alloc page: %lx, count: %d\n",pages,atomic_read(&pages->_count));
			
			if(pages){
				for(i=0;i<pow;i++){
					res[rescount]=pages;
					if(i!=0)
						get_page(pages);
					
					SetPagePrivate(pages);
					set_page_private(pages, (unsigned long)kzalloc(gp_len, GFP_KERNEL));
					
					rescount++;
					pages++;
				}
				ret+=pow;
				yield();
			}
			
		}
		if(order==ALLOC_PAGES_MAX_ORDER)
			len--;
		else{
			order++;
			pow<<=1;
			len>>=1;
		}
	}
	
	return ret;
}

int get_block_key_of_reg_inode(char* block_key, crust_ver_type ver, struct inode *inode, 
				       unsigned int block_num){
	struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	int 		i,
			ret,
			len=gsfs_aes_keylen+sizeof(unsigned int),
			min_index=0;
	unsigned long	min_time;
	char 		temp[len];
	
	//printk("<0>" "get_block_key for inode:%lu, block_num:%u\n", inode->i_ino, block_num);
	
	if(unlikely(!inf->inode_crust_struct || !inf->sri)){
		return -1;
	}
	
	min_time=inf->sri->key_lru_time[0];
	
	for(i=0; i<key_lru_num; i++){
		if(inf->sri->key_lru_time[i]==0){
			//add new key to lru
add_new_key:
			//memset(temp, 0, len);
			//printk("<0>" "add_new_key\n");printkey(temp);
			
			ret=crust_get_key_of_state(&inf->inode_crust_struct->crust_state, ver, temp);
			//printk("<0>" "crust_key:\n");printkey(temp);
			
			if(unlikely(ret))
				return -1;
			
			memcpy(temp+gsfs_aes_keylen, (char*)(&inode->i_ino), sizeof(unsigned int));
			//printk("<0>" "add ino\n");printkey(temp+4);
			
			ret=skein512(gsfs_aes_keylen_bits, temp, len<<3, inf->sri->key_lru_key[i]);
			//printk("<0>" "skein\n");printkey(inf->sri->key_lru_key[i]);
			
			inf->sri->key_lru_ver[i]=ver;
			
			goto create_update;
		}
		
		if(inf->sri->key_lru_ver[i]==ver){
create_update:		
			//printk("<0>" "update time from: %ld to %ld\n",inf->sri->key_lru_time[i],jiffies_64);
			
			inf->sri->key_lru_time[i]=jiffies_64;
			
			memcpy(temp, inf->sri->key_lru_key[i], gsfs_aes_keylen);
			memcpy(temp+gsfs_aes_keylen, (char*)(&block_num), sizeof(unsigned int));
			
			//printk("<0>" "before skein\n");printkey(temp);printkey(temp+4);
			
			ret=skein512(gsfs_aes_keylen_bits, temp, len<<3, block_key);
			
			memset(temp, 0, len);
			
			return 0;
		}
		
		if(inf->sri->key_lru_time[i]<min_time){
			min_time=inf->sri->key_lru_time[i];
			min_index=i;
		}
	}
	
	i=min_index;
	goto add_new_key;
}

typedef struct{
	struct ver_IV_AT	*via;
	unsigned int		block;
} vb_struct;

typedef struct{
	struct inode		*inode;
	vb_struct		**vbs;
	unsigned int 		count;
	char			must_be_sorted;
}set_vias_struct;

int set_vias_cmp(const void *v1, const void *v2){
	vb_struct	*vb1=*((vb_struct**)v1),
			*vb2=*((vb_struct**)v2);
	
	return vb1->block-vb2->block;
}

void set_vias_swp(void *v1, void *v2, int size){
	vb_struct	**vb1=(vb_struct**)v1,
			**vb2=(vb_struct**)v2,
			*temp_vb;
	
	temp_vb=*vb1;
	*vb1=*vb2;
	*vb2=temp_vb;
	
	return;
}

int set_vias(void *data){
	int 			i,
				*res;
	unsigned int		*blocks;
	struct ver_IV_AT	**vias;
	set_vias_struct		*svs=(set_vias_struct*)data;
	
	#ifdef development_pagecache
		char 	repp[1000],
			*rep=repp;
	#endif
	
	if(unlikely(!svs || !svs->inode || !svs->vbs))
		return -1;
	
	gwas(sprintf(rep, "set_vias for inode: %lu, count:%u, must_be_sorted:%d",svs->inode->i_ino, svs->count, svs->must_be_sorted));
	gwas(rep+=strlen(rep));
	
	gwas(sprintf(rep, " *  block numbers:"));
	gwas(rep+=strlen(rep));
	gwas(for(i=0;i<svs->count;i++){sprintf(rep," #%u",svs->vbs[i]->block);rep+=strlen(rep);if(rep-repp>800){printk("<0>" "%s\n",repp);rep=repp;rep[0]=0;}});
	gwas(rep+=strlen(rep));
	
	if(svs->must_be_sorted){
		
		sort(svs->vbs, svs->count, sizeof(vb_struct*), set_vias_cmp, set_vias_swp);
		
		gwas(sprintf(rep, " * sorted to :"));
		gwas(rep+=strlen(rep));
		gwas(for(i=0;i<svs->count;i++){sprintf(rep," #%u",svs->vbs[i]->block);rep+=strlen(rep);if(rep-repp>800){printk("<0>" "%s\n",repp);rep=repp;rep[0]=0;}});
		gwas(rep+=strlen(rep));
	}
	
	vias=kzalloc(sizeof(struct ver_IV_AT*)*svs->count, GFP_KERNEL);
	blocks=kzalloc(sizeof(unsigned int)*svs->count, GFP_KERNEL);
		
	for(i=0; i<svs->count; i++){
		vias[i]=svs->vbs[i]->via;
		blocks[i]=svs->vbs[i]->block;
		
		memset(svs->vbs[i], 0, sizeof(vb_struct));
		kfree(svs->vbs[i]);
	}
	
	res=kzalloc(sizeof(int)*svs->count, GFP_KERNEL);
	
	set_blocks_via(svs->inode, blocks, vias, res, svs->count);
	
	gwas(sprintf(rep, " * results of set_blocks_via :"));
	gwas(rep+=strlen(rep));
	gwas(for(i=0;i<svs->count;i++){sprintf(rep," # res[%d]: %d", i, res[i]);rep+=strlen(rep);if(rep-repp>800){printk("<0>" "%s\n",repp);rep=repp;rep[0]=0;}});
	gwas(rep+=strlen(rep));
	
	//svs->inode->i_sb->s_dirt=1;
	
	memset(vias, 0, sizeof(struct ver_IV_AT*)*svs->count);
	kfree(vias);
	kfree(blocks);
	kfree(res);
	
	kfree(svs->vbs);
	kfree(svs);
	
	gwas(printk("<0>" "%s\n",repp));
	
	return 0;
}

#ifdef development_pagecache
	#define capv_test
#endif

#ifdef capv_test
	#define capv(p)		p
#else
	#define capv(p)
#endif

int complete_aux_pages_and_vias_for_write(struct inode* inode, struct page **blockpages, unsigned int bcount,
					  struct page **res_pages, unsigned int *res_count){
	int			i,
				j,
				via_count=0,
				ret=0;
	struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	struct page		*page;
	struct GSFS_page	*gp;
	vb_struct		**vbs;
	unsigned int		last_block;
	char			//block_key[gsfs_aes_keylen],
				j0[gsfs_aes_keylen],
				must_be_sorted=0;
	unsigned long		*pd,
				*apd;
	
	struct page	**results,
			**IVs,
			**keys;
	unsigned int	pages_count,
			real_count;
			
	char		*IVs_charp=0,
			*keys_charp=0;
				
	#ifdef capv_test
		char	repp[1000],
			*rep=repp;
		
		sprintf(rep,"complete_aux_pages_and_vias_for_write for inode:%lu, count:%u", inode->i_ino, bcount);
		rep+=strlen(rep);
	#endif
	
	vbs=kzalloc(sizeof(vb_struct*)*bcount, GFP_KERNEL);
	
	last_block=0;
	*res_count=0;
	
	pages_count=bcount/aes_blocks_per_page;
	if(bcount%aes_blocks_per_page)
		pages_count++;
	
	results=kzalloc(sizeof(struct page*)*bcount, GFP_KERNEL);
	IVs=kzalloc(sizeof(struct page*)*pages_count, GFP_KERNEL);
	keys=kzalloc(sizeof(struct page*)*pages_count, GFP_KERNEL);
	
	for(i=0;i<pages_count;i++){
		IVs[i]=alloc_page(GFP_KERNEL);
		keys[i]=alloc_page(GFP_KERNEL);
	}
	
	real_count=0;
	
	for(i=0; i<bcount; i++){
		#ifdef capv_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		if(i%aes_blocks_per_page==0){
			int c=i/aes_blocks_per_page;
			
			IVs_charp=page_address(IVs[c]);
			keys_charp=page_address(keys[c]);
		}
		
		page=blockpages[i];
		
		if(unlikely(!page)){
			continue;
		}
		
		capv(sprintf(rep,"## i:%d, page: %lx, index: %lu ", i, (unsigned long)page, page->index));
		capv(rep+=strlen(rep));
		
		gp=(struct GSFS_page*)page_private(page);
		if(unlikely(!gp || !gp->sip)){
epwb:
			end_page_writeback(page);
			
			capv(sprintf(rep,"* epwb "));
			capv(rep+=strlen(rep));
			
			continue;
		}
		
		gp->sip->spflags &= ~spflag_aux_page_is_ready;
		
		if(unlikely(!gp->sip->via || !gp->sip->aux_page)){
			
			goto epwb;
		}		
		
		capv(sprintf(rep," * spflag_key_is_ready:%d, ver: %u, max_ver:%u ", gp->sip->spflags & spflag_key_is_ready, gp->sip->via->ver, inf->inode_crust_struct->max_ver));
		capv(rep+=strlen(rep));
		
		capv(sprintf(rep," * via : "));
		capv(rep+=strlen(rep));
		capv(printhexstring((char*) gp->sip->via, rep, 32));
		capv(rep+=strlen(rep));
				
		if( !(gp->sip->spflags & spflag_key_is_ready)    ||
		      gp->sip->via->ver !=inf->inode_crust_struct->max_ver ){
			
			gp->sip->via->ver=inf->inode_crust_struct->max_ver;
			
			ret=get_block_key_of_reg_inode(gp->sip->key, gp->sip->via->ver, inode, page->index);
			if(unlikely(ret)){
				goto epwb;
			}
			
			gp->sip->spflags |= spflag_key_is_ready;
			
			capv(sprintf(rep," * get_block_key_of_reg_inode : "));
			capv(rep+=strlen(rep));
			capv(printhexstring(gp->sip->key, rep, 16));
			capv(rep+=strlen(rep));
		}
		
		ret=inc_IV(gp->sip->via->IV);
		if(unlikely(ret)){
			printk("<0>" "Unable to write page: %lu of inode: %lu, IV is full.\n", page->index, inode->i_ino);
			goto epwb;
		}
		
		capv(sprintf(rep," * new IV: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(gp->sip->via->IV, rep, 12));
		capv(rep+=strlen(rep));
		
		results[real_count]=gp->sip->aux_page;
		real_count++;
		
		memcpy(IVs_charp, gp->sip->via->IV, gsfs_IV_len);
		memcpy(keys_charp, gp->sip->key, gsfs_aes_keylen);
		IVs_charp+=gsfs_aes_keylen;
		keys_charp+=gsfs_aes_keylen;
	}
	
	capv(sprintf(rep," **##** real_count: %u **##** ", real_count));
	capv(rep+=strlen(rep));
	
	if(likely(real_count)){
		int response=-1;
		OCL_kernel_struct* oks=0;
		
		oks=gum_get_gctr_pages(inode->i_sb, IVs, keys, results, real_count);
		if(unlikely(oks)){
			oks->waiters_num++;
			up(&oks->wanu_sem);
		}
		
		capv(sprintf(rep," **##** oks: %lx **##** ", (unsigned long)oks));
		capv(rep+=strlen(rep));
			
		if(likely(oks)){
			down(&oks->sem);
			response=oks->ret;
			
			capv(sprintf(rep," **##** ret of gum: %d **##** ", oks->ret));
			capv(rep+=strlen(rep));
			
			if(atomic_inc_return(&oks->waiters_returned)==oks->waiters_num){
				capv(sprintf(rep," **##** freeing oks **##** "));
				capv(rep+=strlen(rep));
				
				memset(oks, 0, sizeof(OCL_kernel_struct));
				kfree(oks);
			}
		}
		
		if(response)
			//there was no gum or there was some problem in our gum, 
			//therefore we should do its work ourselves
			for(i=0; i<real_count; i++){
				page=((struct GSFS_page*)page_private(results[i]))->origin_page;
				gp=(struct GSFS_page*)page_private(page);
				apd=(unsigned long*)page_address(gp->sip->aux_page);
		
				ret=get_gctr_page((char*)apd, gp->sip->key, gp->sip->via->IV);
				if(unlikely(ret)){
					end_page_writeback(page);
					continue;
				}
			}
			
		//here all of our aux_pages are ready
	}
	
	for(i=0; i<real_count; i++){
		#ifdef capv_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		page=((struct GSFS_page*)page_private(results[i]))->origin_page;
		gp=(struct GSFS_page*)page_private(page);
		
		capv(sprintf(rep,"#SR# i:%d, page: %lx, index: %lu ", i, (unsigned long)page, page->index));
		capv(rep+=strlen(rep));
		
		pd=(unsigned long*)page_address(page);
		apd=(unsigned long*)page_address(gp->sip->aux_page);
		
		ret=get_j0(j0, gp->sip->via->IV);
		if(unlikely(ret)){
			end_page_writeback(page);
			continue;
		}
		
		for(j=0; j<Block_Size/sizeof(unsigned long); j++)
			apd[j]^=pd[j];
		
		capv(sprintf(rep,"16-byte of apd: "));
		capv(rep+=strlen(rep));
		capv(printhexstring((char*)apd, rep, 16));
		capv(rep+=strlen(rep));
		capv(sprintf(rep,"j0: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(j0, rep, 16));
		capv(rep+=strlen(rep));
		
		get_AT(gp->sip->via->AT, (char*)apd, gp->sip->key, j0);
		
		capv(sprintf(rep," * new AT: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(gp->sip->via->AT, rep, 16));
		capv(rep+=strlen(rep));
		
		if(page->index<last_block)
			must_be_sorted=1;
		last_block=page->index;
		
		vbs[via_count]=kzalloc(sizeof(vb_struct), GFP_KERNEL);
		vbs[via_count]->via=gp->sip->via;
		vbs[via_count]->block=page->index;
		via_count++;
		
		capv(sprintf(rep," * via_count: %u i:%d ",via_count,i));
		capv(rep+=strlen(rep));
				
		gp->sip->spflags |= spflag_aux_page_is_ready;
		
		res_pages[(*res_count)++]=gp->sip->aux_page;
	}
	
	capv(sprintf(rep," * i:%d ",i));
	capv(rep+=strlen(rep));
	
	if(via_count){
		struct task_struct	*kt;
		set_vias_struct		*svs;
		
		svs=kzalloc(sizeof(set_vias_struct), GFP_KERNEL);
		svs->vbs=vbs;
		svs->count=via_count;
		svs->inode=inode;
		svs->must_be_sorted=must_be_sorted;
		
		//kt=kthread_run(set_vias, svs, "gsfs-set vias thread");
		
		//we have something less here
		//if you want to run set_vias as an independent thread
		//you should 
		//	1)set s_dirt of superblock yourself
		//	2)add a flag to spflag to stop release page until via is written
		
		set_vias(svs);
		if (0&&IS_ERR(kt)){
			printk("<0>" "Failed to create kernel thread\n");
			
			for(i=0; i<via_count; i++){
				memset(vbs[i], 0, sizeof(vb_struct));
				kfree(vbs[i]);
			}
			
			kfree(svs);
			
			goto no_count;
		}
	}
	else{
no_count:
		kfree(vbs);
	}
	
	for(i=0;i<pages_count;i++){
		memset(page_address(IVs[i]), 0, Block_Size);
		__free_page(IVs[i]);
	
		memset(page_address(keys[i]), 0, Block_Size);
		__free_page(keys[i]);
	}
	
	memset(results, 0, sizeof(struct page*)*bcount);
	kfree(results);
	memset(IVs, 0, sizeof(struct page*)*pages_count);
	kfree(IVs);
	memset(keys, 0, sizeof(struct page*)*pages_count);
	kfree(keys);
	
	//memset(block_key, 0, gsfs_aes_keylen);
	memset(j0, 0, gsfs_aes_keylen);
	
	capv(printk("<0>" "%s\n",repp));
	
	return 0;
}

//all of the pages have their via, you should unlock blockpages
int complete_aux_pages_and_authenticate_for_read(struct inode* inode, struct page **blockpages, 
						 unsigned int bcount, OCL_kernel_struct *oks){
	//struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	int			i,
				j,
				ret;
	struct page		*page;
	char			j0[gsfs_aes_keylen],
				AT[gsfs_aes_keylen];
	struct GSFS_page	*gp;
	unsigned long		*pd,
				*apd;

	#ifdef capv_test
		char	repp[1000],
			*rep=repp;
		
		sprintf(rep,"complete_aux_pages_and_authenticate_for_read for inode: %lu, count: %u", inode->i_ino, bcount);
		rep+=strlen(rep);
	#endif
	
	for(i=0; i<bcount; i++){
		#ifdef capv_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		page=blockpages[i];
		
		if(unlikely(!page)){
			continue;
		}
		
		capv(sprintf(rep,"#FR# i:%d, page: %lx, index: %lu ", i, (unsigned long)page, page->index));
		capv(rep+=strlen(rep));
		
		gp=(struct GSFS_page*)page_private(page);
		if(unlikely(!gp || !gp->sip || !gp->sip->via || !gp->sip->aux_page)){
upr:	
			unlock_page(page);
			if(likely(gp && gp->sip)){
				gp->sip->spflags &= ~spflag_page_is_ready_for_read;
				gp->sip->spflags |= spflag_page_is_refused_in_first_round_of_read;
			}
			
			continue;
		}
		
		gp->sip->spflags &= ~spflag_page_is_authenticated;
		gp->sip->spflags &= ~spflag_page_is_refused_in_first_round_of_read;
		
		capv(sprintf(rep," * ver: %u, aux_page: %lx, via ", gp->sip->via->ver, (unsigned long)gp->sip->aux_page));
		capv(rep+=strlen(rep));
		capv(printhexstring((char*) gp->sip->via, rep, 32));
		capv(rep+=strlen(rep));
				
		capv(sprintf(rep," * IV: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(gp->sip->via->IV, rep, 12));
		capv(rep+=strlen(rep));
		
		pd=(unsigned long*)page_address(page);
		ret=get_j0(j0, gp->sip->via->IV);
		
		if(unlikely(ret)){
			goto upr;
		}
		
		capv(sprintf(rep," * j0: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(j0, rep, 16));
		capv(rep+=strlen(rep));
		
		get_AT(AT, (char*)pd, gp->sip->key, j0);
		
		capv(sprintf(rep," * AT: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(AT, rep, 16));
		capv(rep+=strlen(rep));
		capv(sprintf(rep," * via AT: "));
		capv(rep+=strlen(rep));
		capv(printhexstring(gp->sip->via->AT, rep, 16));
		capv(rep+=strlen(rep));
		capv(sprintf(rep,"16-byte of pd: "));
		capv(rep+=strlen(rep));
		capv(printhexstring((char*)pd, rep, 16));
		capv(rep+=strlen(rep));
		
		if(strncmp(AT, gp->sip->via->AT, gsfs_aes_keylen)){
			printk("<0>" "Page %lu of inode %lu is not integrated.\n", page->index, inode->i_ino);
			goto upr;
		}
		
		gp->sip->spflags |= spflag_page_is_authenticated;
	}
	
	//waiting for aux_pages completeion with ocl
	if(oks){
		int response;
		
		down(&oks->sem);
		response=oks->ret;
		
		capv(sprintf(rep," **##** ret of gum: %d **##** ", oks->ret));
		capv(rep+=strlen(rep));
		
		if(response==0)
			for(i=0; i<bcount; i++){
				struct GSFS_page* gp=(struct GSFS_page*)page_private(blockpages[i]);
				if(likely(gp && gp->sip))
					gp->sip->spflags |= spflag_aux_page_is_ready;
			}	
		
		if(atomic_inc_return(&oks->waiters_returned)==oks->waiters_num){
			
			capv(sprintf(rep," **##** freeing oks **##** "));
			capv(rep+=strlen(rep));
			
			for(i=0;i<oks->IVs_pages_count;i++){
				memset(page_address(oks->IVs[i]), 0, Block_Size);
				__free_page(oks->IVs[i]);
			
				memset(page_address(oks->keys[i]), 0, Block_Size);
				__free_page(oks->keys[i]);
			}
			
			memset(oks->results, 0, sizeof(struct page*)*oks->results_count);
			kfree(oks->results);
			memset(oks->IVs, 0, sizeof(struct page*)*oks->IVs_pages_count);
			kfree(oks->IVs);
			memset(oks->keys, 0, sizeof(struct page*)*oks->IVs_pages_count);
			kfree(oks->keys);
				
			memset(oks, 0, sizeof(OCL_kernel_struct));
			kfree(oks);
		}
	}
	
	for(i=0; i<bcount; i++){
		#ifdef capv_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		page=blockpages[i];
		
		if(unlikely(!page)){
			continue;
		}
		
		capv(sprintf(rep,"#SR# i:%d, page: %lx, index: %lu ", i, (unsigned long)page, page->index));
		capv(rep+=strlen(rep));
		
		gp=(struct GSFS_page*)page_private(page);
		if(unlikely(
			!gp || !gp->sip || 			
			(gp->sip->spflags & spflag_page_is_refused_in_first_round_of_read) ||
			!(gp->sip->spflags & spflag_page_is_authenticated) 
			) ){
			continue;
		}

		pd=(unsigned long*)page_address(page);
		apd=(unsigned long*)page_address(gp->sip->aux_page);
		
		if(!(gp->sip->spflags & spflag_aux_page_is_ready)){
			
			capv(sprintf(rep,"We should calculate this aux_page ourselves "));
			capv(rep+=strlen(rep));
		
			ret=get_gctr_page((char*)apd, gp->sip->key, gp->sip->via->IV);
			if(unlikely(ret)){
				unlock_page(page);
				continue;
			}
		}
		
		for(j=0; j<Block_Size/sizeof(unsigned long); j++)
			pd[j]^=apd[j];
		
		gp->sip->spflags |= spflag_page_is_ready_for_read;
		unlock_page(page);
	}
	
	memset(j0, 0, gsfs_aes_keylen);
	
	capv(printk("<0>" "%s\n",repp));
	
	return 0;
}

static void GSFS_sec_readpages_bio_end(struct bio *bio, int err){
	int		i,
			utd = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct bio_vec	*bv=bio->bi_io_vec;
	struct page	*p,
			**pages=0;
	unsigned int	count=0;
	OCL_kernel_struct *oks=0;
		
	gwas(printk("<0>" "GSFS_sec_readpages_bio_end started and vcnt: %u,err: %d\n",bio->bi_vcnt,err));
	
	if(likely(bio->bi_vcnt))
		pages=kzalloc(sizeof(struct page*)*bio->bi_vcnt, GFP_KERNEL);
	
	for(i=0;i<bio->bi_vcnt;i++){
		p=bv->bv_page;
		//gwas(printk("<0>" "GSFS_sec_readpages_bio_end started and page:%lx\n",(unsigned long)p));
	
		if(likely(utd)){
			SetPageUptodate(p);
			pages[count++]=p;
		}
		else{
			ClearPageUptodate(p);
			SetPageError(p);
			unlock_page(p);
		}
		
		bv++;
	}
	
	oks=(OCL_kernel_struct*)bio->bi_private;	
	
	if(likely(count) && likely(pages[0]->mapping && pages[0]->mapping->host))
		complete_aux_pages_and_authenticate_for_read(pages[0]->mapping->host, pages, count, oks);
	
	gwas(printk("<0>" "GSFS_sec_readpages_bio_end ended. pid:%u\n",current->pid));
	
	if(likely(pages)){
		memset(pages, 0, sizeof(struct page*)*bio->bi_vcnt);
		kfree(pages);
	}
	
	bio_put(bio);
	
	return;
}

static void GSFS_readpages_bio_end(struct bio *bio, int err){
	int i;
	int utd = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct bio_vec * bv=bio->bi_io_vec;
	struct page* p;
	
	gwas(printk("<0>" "GSFS_readpages_bio_end started and vcnt: %u,err: %d\n",bio->bi_vcnt,err));
	
	for(i=0;i<bio->bi_vcnt;i++){
		p=bv->bv_page;
		if(utd)
			SetPageUptodate(p);
		else{
			ClearPageUptodate(p);
			SetPageError(p);
		}
		//printk("<0>" "mybioend %d %lx %d\n",i,(unsigned long)p,utd);
		unlock_page(p);
		bv++;
	}
	
	bio_put(bio);
	
	gwas(printk("<0>" "GSFS_readpages_bio_end ended. pid:%u\n",current->pid));
	
	return;
}

int pagecmp(const void *p1, const void *p2){
	struct page	*page1=*((struct page**)p1),
			*page2=*((struct page**)p2);
			
	return (((struct GSFS_page*)page_private(page1))->disk_bnr)-(((struct GSFS_page*)page_private(page2))->disk_bnr);
}

void pageswp(void *p1, void *p2, int size){
	struct page **page1=(struct page**)p1,
		    **page2=(struct page**)p2,
		    *pp;
	//printk("<0>" "pageswp page1=%lx,page2=%lx,%d",(unsigned long)*page1,(unsigned long)*page2,size);
	//return;
	pp=*page1;
	*page1=*page2;
	*page2=pp;
	return;
}

int GSFS_sort_and_submit_bio_for_sec_pages(struct inode* inode,struct page** blockpages,unsigned int bcount,
				       int mode, bio_end_io_t* end_bio_func ){
	//struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	sector_t 	current_block,
			prev_block;	
	unsigned int	l,
			newbio,
			biolen=0,
			prev_biolen=0,
			biocount=0;		 
	int 		last_add_res=0,
			ret=0;
	struct bio	*bio=0;
	struct page	**pages=0;
	//struct GSFS_page	*gp;
	
	OCL_kernel_struct* oks=0;
	
	#ifdef development_pagecache
		char	repp[1000],
			*rep=repp;
		
		sprintf(rep,"GSFS_sort_and_submit_bio_for_sec_pages for inode:%lu, write=%d, bcount=%d",inode->i_ino, mode==WRITE,bcount);
		rep+=strlen(rep);
	#endif
	
	if(unlikely(!blockpages || !bcount))
		return -1;
	
	sort(blockpages, bcount, sizeof(struct page*), pagecmp, pageswp);
	
	if(mode==WRITE){
		unsigned int count=0;
		
		pages=kzalloc(sizeof(struct page*)*bcount, GFP_KERNEL);
		
		complete_aux_pages_and_vias_for_write(inode, blockpages, bcount, pages, &count);
		
		blockpages=pages;
		bcount=count;
		
		gwas(sprintf(rep, " * new count: %u", count));
		gwas(rep+=strlen(rep));
	}
	else{
	
		int i;
		struct page	**results=0,
				**IVs=0,
				**keys=0;
		unsigned int	pages_count=0,
				real_count=0;
				
		char		*IVs_charp=0,
				*keys_charp=0;
		struct GSFS_page *gp;
		struct page	*page;
		
		if(bcount>=BCOUNT_MIN_OCL){
			pages_count=bcount/aes_blocks_per_page;
			if(bcount%aes_blocks_per_page)
				pages_count++;
			
			results=kzalloc(sizeof(struct page*)*bcount, GFP_KERNEL);
			IVs=kzalloc(sizeof(struct page*)*pages_count, GFP_KERNEL);
			keys=kzalloc(sizeof(struct page*)*pages_count, GFP_KERNEL);
			
			for(i=0;i<pages_count;i++){
				IVs[i]=alloc_page(GFP_KERNEL);
				keys[i]=alloc_page(GFP_KERNEL);
			}
			
			real_count=0;
		}
		
		for(i=0; i<bcount; i++){
			if(bcount>=BCOUNT_MIN_OCL && i%aes_blocks_per_page==0){
				int c=i/aes_blocks_per_page;
				
				IVs_charp=page_address(IVs[c]);
				keys_charp=page_address(keys[c]);
			}
			
			page=blockpages[i];
			
			if(unlikely(!page)){
				continue;
			}
			
			gp=(struct GSFS_page*)page_private(page);
			if(unlikely(!gp || !gp->sip)){
	
				continue;
			}
			
			if( !(gp->sip->spflags & spflag_key_is_ready) ){
			
				ret=get_block_key_of_reg_inode(gp->sip->key, gp->sip->via->ver, inode, page->index);
				if(unlikely(ret)){
					continue;
				}
				
				gp->sip->spflags |= spflag_key_is_ready;
			}
			
			gp->sip->spflags &= ~spflag_aux_page_is_ready;
			
			if(unlikely(!gp->sip->via || !gp->sip->aux_page)){
				continue;
			}		
			
			if(bcount>=BCOUNT_MIN_OCL){
				results[real_count]=gp->sip->aux_page;
				real_count++;
				
				memcpy(IVs_charp, gp->sip->via->IV, gsfs_IV_len);
				memcpy(keys_charp, gp->sip->key, gsfs_aes_keylen);
				IVs_charp+=gsfs_aes_keylen;
				keys_charp+=gsfs_aes_keylen;
			}
		}
		
		if(likely(real_count)){	
			oks=gum_get_gctr_pages(inode->i_sb, IVs, keys, results, real_count);
			if(unlikely(!oks)){
				//freeing allocated IVs, keys, results
				
				for(i=0;i<pages_count;i++){
					memset(page_address(IVs[i]), 0, Block_Size);
					__free_page(IVs[i]);
				
					memset(page_address(keys[i]), 0, Block_Size);
					__free_page(keys[i]);
				}
				
				memset(results, 0, sizeof(struct page*)*bcount);
				kfree(results);
				memset(IVs, 0, sizeof(struct page*)*pages_count);
				kfree(IVs);
				memset(keys, 0, sizeof(struct page*)*pages_count);
				kfree(keys);
				
			}				
			//gt(printk("<0>" "oks in read: %lx \n",(unsigned long)oks));
		}
	}

	l=1;
	newbio=1;
	
	prev_block=((struct GSFS_page*)page_private(blockpages[0]))->disk_bnr;
	
	//printk("<0>" "1GSFS_sort_and_submit_bio_for_sec_pages for inode:%lu, write=%d, bcount=%d\n",inode->i_ino, mode==WRITE,bcount);
	do{
		#ifdef development_pagecache
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		//allocating new bio and adding prev_block to this new bio
		if(newbio){
			if(prev_biolen)
				biolen=min(bcount-l+1, prev_biolen);
			else
				biolen=bcount-l+1;
			
			bio=bio_alloc(GFP_KERNEL,biolen);
			
			while(!bio){
				if(!biolen){
bad_ret:					
					ret=-1;
					break;
				}
				
				gwas(sprintf(rep, " * Can't allocate bio %d",biolen));
				gwas(rep+=strlen(rep));
				
				biolen/=2;
				bio=bio_alloc(GFP_KERNEL,biolen);
				
			}
			
			if(unlikely(!bio))
				goto bad_ret;
			
			//adding oks to new bio
			if(mode==READ){
				bio->bi_private=oks;
				if(likely(oks))
					oks->waiters_num++;
			}
			
			prev_biolen=biolen;
			
			gwas(sprintf(rep," * new biolen=%d",biolen));
			gwas(rep+=strlen(rep));
			
			bio->bi_bdev=inode->i_sb->s_bdev;
			bio->bi_sector=Block_to_Sector(prev_block);
			bio->bi_end_io=end_bio_func;
			
			newbio=0;
			last_add_res=bio_add_page(bio,blockpages[l-1],Block_Size,0);
			
			gwas(sprintf(rep, " * add prev page: %d, with address:%lx, lar:%u prev_block: %lu ",l,(unsigned long)blockpages[l-1], last_add_res,prev_block));
			gwas(rep+=strlen(rep));
			
			biocount=1;
		}
		
		//adding new page to bio if it is possible		
		if(likely(l!=bcount)){
			current_block=((struct GSFS_page*)page_private(blockpages[l]))->disk_bnr;
			
			gwas(sprintf(rep," # l:%d current_block:%lu, pre_bl: %lu ", l, current_block, prev_block));
			gwas(rep+=strlen(rep));
			
			if(current_block==(prev_block+1) && biocount<biolen ){
				prev_block=current_block;
				
				last_add_res=bio_add_page(bio,blockpages[l],Block_Size,0);
				
				gwas(sprintf(rep," * added_page: %lx, last_add_res: %d",(unsigned long)blockpages[l],last_add_res));
				gwas(rep+=strlen(rep));
				
				if(last_add_res==Block_Size){
					biocount++;
					l++;
					
					continue;
				}
			}
			
			prev_block=current_block;
		}
		
		//reading disk for current blocks		
		submit_bio(mode,bio);
		
		gwas(sprintf(rep," * biocount:%d l:%d bio_bi_vcnt:%d",biocount,l,bio->bi_vcnt));
		gwas(rep+=strlen(rep));
		
		newbio=1;
		l++;
		
	}while(l<=bcount);
	
	if(mode==READ && likely(oks))
		up(&oks->wanu_sem);
	
	gwas(printk("<0>" "%s\n",repp));
	
	if(pages)
		kfree(pages);
	
	return ret;
}

int GSFS_sort_and_submit_bio_for_pages(struct inode* inode,struct page** blockpages,unsigned int bcount,
				       int mode, bio_end_io_t* end_bio_func ){
	sector_t current_block,
		 prev_block;	
	unsigned int 	l,
			newbio,
			biolen=0,
			prev_biolen=0,
			biocount=0;		 
	int 	last_add_res=0,
		ret=0;
	struct bio* bio=0;
	
	if(!blockpages || !bcount)
		return -1;
	
	sort(blockpages, bcount, sizeof(struct page*), pagecmp, pageswp);
	
	gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages for write=%d: after sorting,number of bcount=%d\n",mode==WRITE,bcount));

	l=1;
	newbio=1;

// 	if(unlikely(!page_private(blockpages[0]))){
// 		printk("<0>" "nothing to do\n");
// 		return -1;
// 	}

	prev_block=((struct GSFS_page*)page_private(blockpages[0]))->disk_bnr;

	do{
		//allocating new bio and adding prev_block to this new bio
		if(newbio){
			if(prev_biolen)
				biolen=min(bcount-l+1, prev_biolen);
			else
				biolen=bcount-l+1;
			
			bio=bio_alloc(GFP_KERNEL,biolen);
			
			while(!bio){
				gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages0: Can't allocate bio %d",biolen));
				
				if(!biolen){
bad_ret:				gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages1: Something bad no bio "));
					ret=-1;
					break;
				}
				
				biolen/=2;
				bio=bio_alloc(GFP_KERNEL,biolen);
				
			}
			
			if(unlikely(!bio))
				goto bad_ret;
			
			prev_biolen=biolen;
			
			gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages2: biolen=%d",biolen));
			
			bio->bi_bdev=inode->i_sb->s_bdev;
			bio->bi_sector=Block_to_Sector(prev_block);
			bio->bi_end_io=end_bio_func;
			
			newbio=0;
			last_add_res=bio_add_page(bio,blockpages[l-1],Block_Size,0);
			
			gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages3: %d lar:%u prev_block: %lu\n ",l,last_add_res,prev_block));
			
			biocount=1;
		}
		
		//adding new page to bio if it is possible		
		if(l!=bcount){
			current_block=((struct GSFS_page*)page_private(blockpages[l]))->disk_bnr;
			
			if(current_block==(prev_block+1) && biocount<biolen ){
				prev_block=current_block;
				
				last_add_res=bio_add_page(bio,blockpages[l],Block_Size,0);
				
				if(last_add_res==Block_Size){
					biocount++;
					l++;
					continue;
				}
			}
			
			prev_block=current_block;
		}
		
		//reading disk for current blocks		
		submit_bio(mode,bio);
		
		gwas(printk("<0>" "GSFS_sort_and_submit_bio_for_pages4 biocount:%d l:%d bio_bi_vcnt:%d\n",biocount,l,bio->bi_vcnt));
		
		newbio=1;
		l++;
		
	}while(l<=bcount);
	
	return ret;
}

//we have some new pages with valid index and not in lru  and Radix-Tree
//input them in lru and radixtree whithout get_page
int GSFS_readpages(struct file *filp, struct address_space *mapping, struct list_head *pages, unsigned nr_pages){
	struct page	*p;
	struct inode	*inode=mapping->host;
	struct GSFS_inode *inf=(struct GSFS_inode*)inode->i_private;
	struct page	**blockpages;
	unsigned	bcount=0,
			l;
	int		ret=0;
	
	gwas(printk("<0>" "Readpages started for inode:%lu\n",inode->i_ino));
	
	blockpages=kzalloc(nr_pages*sizeof(struct page*),GFP_KERNEL);
	l=0;
	
	while(l<nr_pages){
		struct GSFS_page *gp;
		
		p=list_entry(pages->prev,struct page,lru);
		if(unlikely(!p)){
			ret=-1;
			goto retb;
		}			
		
		list_del(&p->lru);
		
		l++;
		
		gp=(struct GSFS_page*)page_private(p);
		
		if(unlikely(!gp || !gp->disk_bnr))
			continue;
		
		blockpages[bcount]=p;
		bcount++;
		
		gwas(printk("<0>" "Readpages add_to_page_cache_lru page: %lx, index:%lu, priv:%lx\n",(unsigned long)p, p->index, page_private(p)));
		
		if(likely(!(gp->flags & pflag_page_is_added_to_pagecache))){
			add_to_page_cache_lru(p, mapping, p->index, GFP_KERNEL);
			
			gp->flags|=pflag_page_is_added_to_pagecache;
		}
		else
			lock_page(p);
	}
	
	if(unlikely(!bcount)){
		ret=-1;
		goto retb;
	}
	
	if(inf->igflags & igflag_secure)
		ret=GSFS_sort_and_submit_bio_for_sec_pages(inode, blockpages, bcount, READ, GSFS_sec_readpages_bio_end);
	else
		ret=GSFS_sort_and_submit_bio_for_pages(inode, blockpages, bcount, READ, GSFS_readpages_bio_end);
retb:	
	kfree(blockpages);
	
	gwas(printk("<0>" "Readpages ended. pid:%u\n",current->pid));
	
	return ret;
}

int GSFS_readpage(struct file *filp, struct page *page){
	LIST_HEAD(lh);
	
	gwas(printk("<0>" "Readpage for inode %lu\n",filp->f_dentry->d_inode->i_ino));
	
	if(!page || !filp)
		return -1;
	list_add(&page->lru,&lh);
	return GSFS_readpages(filp,filp->f_mapping,&lh,1);
}

void inline GSFS_put_data_page_of_inode(struct inode* in, struct page* p){
	//printk("<0>" "PDPI inode %lx, _count %d\n",in?in->i_ino:-1,atomic_read(&p->_count));
	if(atomic_read(&p->_count)>1)
		put_page(p);
	return;
	/*
	struct buffer_head* bh=(struct buffer_head *)p->private;
	if(PageDirty(p)){
		write_one_bh_dev(bh);
		//printk("<0>" "DDDDDDDD");
		page_clear_dirty(p);
	}
	brelse(bh);
	return;
	*/
}

//returns the page related to pn if exists. returned page may be locked.
//returns data page with sip, via and aux_page
struct page* GSFS_get_data_page_of_inode(struct inode* in, unsigned int pn, char withread){
	int			i=0;
				
	struct GSFS_inode	*inf=(struct GSFS_inode*)in->i_private;	
	struct page		*page;
	LIST_HEAD(lh);
	
	#ifdef development_pagecache
		char	repp[1000],
			*rep=repp;
		
		sprintf(rep, "GSFS_get_data_page_of_inode for in: %lu, pn: %u, withread: %d, is_sec: %d ", in->i_ino, pn, withread, inf->igflags & igflag_secure);
		rep+=strlen(rep);
	#endif
	
	if(withread)
		page=find_get_page(in->i_mapping,pn);
	else
		page=find_lock_page(in->i_mapping,pn);
	
	gwas(sprintf(rep, "* find_get/lock_page: %lx", (unsigned long)page));
	gwas(rep+=strlen(rep));
	
	//if(page)
	//printk("<0>" "* find_get/lock_page: %lx, pr: %lx, count: %d\n", (unsigned long)page, page_private(page), atomic_read(&page->_count));
		
	if(!page || !page_private(page)){
		int first_status=0;
		
		if(!page){
			i=get_some_pages(1,&page);
			first_status=0;
		}else{
			//!page_private
			struct GSFS_page* gp;
			
			SetPagePrivate(page);
			set_page_private(page, (unsigned long)kzalloc(gp_len, GFP_KERNEL));
			
			gwas(sprintf(rep,"adding private to page: %lx * ",(unsigned long)page_private(page)));
			gwas(rep+=strlen(rep));
			
			gp=(struct GSFS_page*)page_private(page);
			gp->flags |= pflag_page_is_added_to_pagecache;
			i=1;
			first_status=1;
		}
		
		gwas(sprintf(rep, "* get_some_pages: i:%d, page:%lx", i, (unsigned long)page));
		gwas(rep+=strlen(rep));	
				
		if(likely(i && page)){
			struct GSFS_page	*gp;
			
			gp=(struct GSFS_page*)page_private(page);
			
			if(inf->igflags & igflag_secure){
				struct ver_IV_AT	*via;
				int			res=0;
				
				via=kzalloc(via_len, GFP_KERNEL);
				i=get_blocks_via(in, &pn, &via, &res, 1);
				
				gwas(sprintf(rep, "* via_res: %d, via: ", res));
				gwas(rep+=strlen(rep));
				gwas(printhexstring((char*)via, rep, 32));
				gwas(rep+=strlen(rep));
				
				if(likely(res==0)){
					struct page		*aux_page;
					
					aux_page=alloc_page(GFP_KERNEL);
					
					gwas(sprintf(rep, "* aux_page: %lx", (unsigned long)aux_page));
					gwas(rep+=strlen(rep));
					
					if(likely(aux_page)){
						
						SetPagePrivate(aux_page);
						
						gp->sip=kzalloc(sip_len, GFP_KERNEL);
						
						if(!withread)
							gp->sip->spflags |= spflag_page_is_ready_for_read;
							
						gp->sip->aux_page=aux_page;
						set_page_private(aux_page, (unsigned long)kzalloc(gp_len, GFP_KERNEL));
						((struct GSFS_page*)page_private(aux_page))->origin_page=page;
						
						gp->sip->via=via;
					}
					else{
			bad_ret:
						kfree(via);
						kfree(gp);
						
						set_page_private(page, 0);
						ClearPagePrivate(page);
						
						__free_page(page);
						
						page=0;
						
						goto ret;
					
					}
				}
				else
					goto bad_ret;
			}
			
			page->index=pn;
			
			gp->disk_bnr=get_dp_bn_of_in(in, pn);
			if(inf->igflags & igflag_secure)
				((struct GSFS_page*)page_private(gp->sip->aux_page))->disk_bnr=gp->disk_bnr;
			
			if(withread){
				list_add(&page->lru,&lh);
				
				i=GSFS_readpages(0,in->i_mapping,&lh,1);
			}
			else{	
				if(first_status==0)
					add_to_page_cache_lru(page, in->i_mapping, pn, GFP_KERNEL);
			}
			
			gwas(sprintf(rep, "* disk_bnr: %u ", gp->disk_bnr));
			gwas(rep+=strlen(rep));
		}
	}
ret:
	//gwas(printk("<0>" "%s\n",repp));
	
	return page;
}

inline struct page* GSFS_get_data_page_of_inode_with_read(struct inode* in, unsigned int pn){
	return GSFS_get_data_page_of_inode(in, pn, 1);
}

//returns an empty page with sip, aux_page and via
inline struct page* GSFS_get_locked_data_page_of_inode_without_read(struct inode* in, unsigned int pn){
	return GSFS_get_data_page_of_inode(in, pn, 0);
}

/*
struct page* GSFS_get_locked_data_page_of_inode_without_read(struct inode* inode,unsigned int pn){
	struct page		*page;
	struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	
	#ifdef development_pagecache
	char 	repp[1000],
		*rep;
	rep=repp;
	
	sprintf(rep,"GSFS_get_locked_data_page_of_inode_without_read in: %lu, sec_inode: %d, pn: %u", inode->i_ino, inf->igflags&igflag_secure, pn);
	rep+=strlen(rep);
	#endif
	
	page=find_lock_page(inode->i_mapping,pn);
	
	gwas(sprintf(rep," * find_lock_page: %lu",(unsigned long)page));
	gwas(rep+=strlen(rep));
	
	if(!page){
		if(get_some_pages(1,&page)){
			struct GSFS_page	*gp;
			
			gwas(sprintf(rep," * allocated page: %lu",(unsigned long)page));
			gwas(rep+=strlen(rep));
			
			gp=(struct GSFS_page*)page_private(page);
			
			gp->disk_bnr=get_dp_bn_of_in(inode, pn);
			
			gwas(sprintf(rep," * disk_bnr: %u", gp->disk_bnr));
			gwas(rep+=strlen(rep));
			
			if(inf->igflags & igflag_secure){
				gp->sip=kzalloc(sip_len, GFP_KERNEL);
				//gp->sip->via=kzalloc(via_len,GFP_KERNEL);
				
				gp->sip->aux_page=alloc_page(GFP_KERNEL);
				set_page_private(gp->sip->aux_page, (unsigned long)page);
				SetPagePrivate(gp->sip->aux_page);
				
				gwas(sprintf(rep," * aux_page: %lu, gp->sip->via: %lu ",(unsigned long)gp->sip->aux_page, (unsigned long)gp->sip->via));
				gwas(rep+=strlen(rep));
				//gwas(printhexstring((char*)gp->sip->via, rep, 32));
				//gwas(rep+=strlen(rep));
			}
			
			page->index=pn;
			
			add_to_page_cache_lru(page, inode->i_mapping, pn, GFP_KERNEL);
		}
		else{
			gwas(sprintf(rep," * cant allocate new page ???!!!"));
			gwas(rep+=strlen(rep));
		}
	}
	
	gwas(printk("<0>" "%s*\n", repp));
	
	return page;
}
*/

//returns data pages with sip, via, aux_page
int GSFS_get_data_pages_of_inode(struct inode* in, unsigned int pn[],unsigned int num,struct page** res, int odirect){
	/*
	printk("<0>" "num:%d\n",num);
	int i=0,k=0;
	for(i=0;i<num;i++){
		res[i]=GSFS_get_data_page_of_inode(in, pn[i]);
		if(res[i])
			k++;
		printk("<0>" "i:%d , res:%lx\n", i, (unsigned long)res[i]);
	}
	return k;
	*/
	
	int 	i,
		j,
		ret=0,
		l;
	LIST_HEAD(lh);
	struct page		*page,
				**pages;
	struct ver_IV_AT	**vias;
	unsigned int		*blocks,
				*js;
	struct GSFS_inode 	*inf=(struct GSFS_inode*)in->i_private;
	
	#ifdef development_pagecache
	char 	repp[1000],
		*rep;
	rep=repp;
	
	sprintf(rep,"GSFS_get_data_pages_of_inode in: %lu, sec_inode: %d, num: %u * ", in->i_ino, inf->igflags&igflag_secure, num);
	rep+=strlen(rep);
	#endif
	
	if(S_ISDIR(in->i_mode))	
		return 0;
	
	pages=kzalloc(sizeof(struct page*)*num, GFP_KERNEL);
	vias=kzalloc(sizeof(struct ver_IV_AT*)*num, GFP_KERNEL);
	blocks=kzalloc(sizeof(unsigned int)*num, GFP_KERNEL);
	js=kzalloc(sizeof(unsigned int)*num, GFP_KERNEL);
	
	l=0;
	
	for(j=0;j<num;j++){
		#ifdef development_pagecache
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
			}
		#endif 
		
		page=find_get_page(in->i_mapping,pn[j]);
		
		gwas(sprintf(rep,"# j: %u, pn[j]: %u, find_get_page: %lx * ", j, pn[j], (unsigned long)page));
		gwas(rep+=strlen(rep));
		
		if(page && odirect){
			//printk("deleteing page:%lx locked:%d\n",page,PageLocked(page));
			lock_page(page);
			remove_from_page_cache(page);
			//unlock_page(page);
			GSFS_invalidatepage(page,0);
			page=0;
		}
		
		if(!page || !page_private(page)){
			struct GSFS_page *gp=0;
			
			if(!page){
				i=get_some_pages(1,&page);
				
				gwas(sprintf(rep,"adding page: %lx * ",(unsigned long)page));
				gwas(rep+=strlen(rep));
				
				if(!i)
					continue;
				
				gp=(struct GSFS_page*)page_private(page);
			}
			else{
				//!page_private(page)
				SetPagePrivate(page);
				set_page_private(page, (unsigned long)kzalloc(gp_len, GFP_KERNEL));
				
				gwas(sprintf(rep,"adding private to page: %lx * ",(unsigned long)page_private(page)));
				gwas(rep+=strlen(rep));
				
				gp=(struct GSFS_page*)page_private(page);
				gp->flags |= pflag_page_is_added_to_pagecache;
			}
			
			gp->disk_bnr=get_dp_bn_of_in(in, pn[j]);
			
			gwas(sprintf(rep,"disk_bnr: %u * ", gp->disk_bnr));
			gwas(rep+=strlen(rep));
			
			if(inf->igflags&igflag_secure){
				pages[l]=page;
				vias[l]=kzalloc(via_len, GFP_KERNEL);
				blocks[l]=pn[j];
				js[l]=j;
				
				gwas(sprintf(rep,"allocating vias[l] and setting blocks[l] * "));
				gwas(rep+=strlen(rep));
			}
			else
				list_add(&page->lru,&lh);
			
			page->index=pn[j];
			l++;
			
			if(inf->igflags&igflag_secure)
				continue;
		}
		
		res[j]=page;
		ret++;
	}
	
	gwas(sprintf(rep,"* l: %u *", l));
	gwas(rep+=strlen(rep));
	
	if(l){
		if(inf->igflags & igflag_secure){
			int	*vias_res;
			
			vias_res=kzalloc(l*sizeof(int), GFP_KERNEL);
			
			get_blocks_via(in, blocks, vias, vias_res, l);
			
			for(i=0; i<l; i++){
				struct page		*page,
							*aux_page;
				struct GSFS_page	*gp;
				
				page=pages[i];
				gp=(struct GSFS_page*)page_private(page);
				
				#ifdef development_pagecache
					if(rep-repp>500){
						printk("<0>" "%s\n",repp);
						rep=repp;
					}
				#endif 
				
				gwas(sprintf(rep,"$$i: %u, vias_res[i]: %d, block: %u", i, vias_res[i], blocks[i]));
				gwas(rep+=strlen(rep));
				
				if(vias_res[i]==0){
					
					aux_page=alloc_page(GFP_KERNEL);
					
					if(!aux_page){
						gwas(sprintf(rep,"* no aux_page "));
						gwas(rep+=strlen(rep));
				
						goto free_count;
					}
									
					gp->sip=kzalloc(sip_len, GFP_KERNEL);
					
					gp->sip->via=vias[i];
					gp->sip->aux_page=aux_page;
					
					SetPagePrivate(aux_page);
					set_page_private(aux_page, (unsigned long)kzalloc(gp_len, GFP_KERNEL));
					((struct GSFS_page*)page_private(aux_page))->origin_page=page;
					((struct GSFS_page*)page_private(aux_page))->disk_bnr=gp->disk_bnr;
					
					list_add(&page->lru,&lh);
					ret++;
					res[js[i]]=page;
					
					gwas(sprintf(rep,"* adding page with vias: "));
					gwas(rep+=strlen(rep));
					gwas(printhexstring((char*)vias[i], rep, 32));
					gwas(rep+=strlen(rep));
				}
				else{
		free_count:		gwas(sprintf(rep,"* kfree vias[i] for bad res"));
					gwas(rep+=strlen(rep));
				
					kfree(vias[i]);
					
					kfree(gp);
					
					ClearPagePrivate(page);
					set_page_private(page, 0);
					
					__free_page(page);
				}
				
			}
			
			kfree(vias_res);
		}
		
		i=GSFS_readpages(0,in->i_mapping,&lh,l);
		
		gwas(sprintf(rep,"* ret of gsfs_readpages: %d *", i));
		gwas(rep+=strlen(rep));
				
	}
	
	kfree(vias);
	kfree(blocks);
	kfree(pages);
	kfree(js);
	
	gwas(sprintf(rep,"ret: %d ", ret));
	gwas(rep+=strlen(rep));
	
	gwas(printk("<0>" "%s\n",repp));
	
	return ret;
}

int GSFS_set_page_dirty(struct page *page){
	struct inode* inode=page->mapping->host;
	
	SetPageDirty(page);
	
	radix_tree_tag_set(&inode->i_mapping->page_tree, page->index, PAGECACHE_TAG_DIRTY);
	//gwas(printk("<0>" "Set Page Dirty for page %lu of inode %lu and isdirty:%u, _count:%d\n",page->index,inode->i_ino,PageDirty(page),atomic_read(&page->_count)));
	
	return 0;
}

void GSFS_free_page_private(struct page* page){
	struct GSFS_page	*gp;
	
	#ifdef development_pagecache
		char	repp[1000],
			*rep;
		rep=repp;
		
		sprintf(rep, "GSFS_free_page_private for page %lu of inode %lu _count:%d LRU:%d, dirty:%d, WB:%d * ",page->index,0,atomic_read(&page->_count),PageLRU(page),PageDirty(page),PageWriteback(page));
		rep+=strlen(rep);
	#endif 
	
	gp=(struct GSFS_page*)page_private(page);
	
	gwas(sprintf(rep, " gp: %lx * ",(unsigned long)gp));
	gwas(rep+=strlen(rep));
	
	if(likely(gp)){
		gwas(sprintf(rep, " sip: %lx * ",(unsigned long)gp->sip));
		gwas(rep+=strlen(rep));
		
		if(gp->sip){
			gwas(sprintf(rep, " aux_page: %lx * ",(unsigned long)gp->sip->aux_page));
			gwas(rep+=strlen(rep));
			
			if(gp->sip->aux_page){
				
				gwas(sprintf(rep, " aux_page->private: %lx * ",(unsigned long)page_private(gp->sip->aux_page)));
				gwas(rep+=strlen(rep));
				
				if(page_private(gp->sip->aux_page))
					kfree((void*)page_private(gp->sip->aux_page));
				
				set_page_private(gp->sip->aux_page, 0);
				ClearPagePrivate(gp->sip->aux_page);
				
				memset(page_address(gp->sip->aux_page), 0, Block_Size);
				
				put_page( gp->sip->aux_page);
				//__free_page(gp->sip->aux_page);
			}
			
			gwas(sprintf(rep, " via : %lx * ",(unsigned long)gp->sip->via));
			gwas(rep+=strlen(rep));
			
			if(gp->sip->via){
				memset(gp->sip->via, 0, via_len);
				kfree(gp->sip->via);
			}
			
			memset(gp->sip, 0, sip_len);
			kfree(gp->sip);
			
			memset(page_address(page), 0, Block_Size);
		}
		
		kfree(gp);
	}
	
	ClearPagePrivate(page);
	set_page_private(page, 0);
	
	gwas(printk("<0>" "%s\n", repp));
	
	return;
}

//can page be released? non zero for Yes
//int GSFS_releasepage(struct page *page, gfp_t gfp){
void GSFS_invalidatepage(struct page *page, unsigned long offset){
	
	if(offset)
		return;
	
	//printk("<0>" "invalidatepage for page:%lx index:%lu  _count:%d LRU:%d, dirty:%d, WB:%d \n ",(unsigned long)page,page->index,atomic_read(&page->_count),PageLRU(page),PageDirty(page),PageWriteback(page));
	
	GSFS_free_page_private(page);
	
	ClearPageReferenced(page);
	ClearPageUptodate(page);
	
	//remove_from_page_cache(page);
	//ClearPageMappedToDisk(page);
	//page_cache_release(page); 
	
	unlock_page(page);
	put_page( page);

	//printk("<0>" "invalidatepage for page:%lx index:%lu  _count:%d LRU:%d, dirty:%d, WB:%d \n ",(unsigned long)page,page->index,atomic_read(&page->_count),PageLRU(page),PageDirty(page),PageWriteback(page));
	
	return;
}

int GSFS_releasepage(struct page *page, gfp_t gfp){
		
	//gwas(printk("<0>" "invalidate page for page %lu of inode %lu and offset:%lu\n",page->index,page->mapping->host->i_ino,offset));
	//(printk("<0>" "releasepage for page %lx of inode %lu count:%d locked:%d\n ",page,page->mapping->host->i_ino,atomic_read(&page->_count),PageLocked(page)));
	
	if(PageWriteback(page))
		wait_on_page_writeback(page);
	
	GSFS_free_page_private(page);
	
	ClearPageReferenced(page);
	ClearPageUptodate(page);
	
	return 1;
}

int GSFS_launder_page(struct page *page){
		
	gwas(printk("<0>" "Launderpage for page %lu of inode %lu\n",page->index,page->mapping->host->i_ino));
	
	return 0;
}

void GSFS_sync_page(struct page *page){
		
	gwas(printk("<0>" "sync_page for page %lu of inode %lu\n",page->index,page->mapping->host->i_ino));
	return;
}

static void GSFS_writepages_bio_end(struct bio *bio, int err){
	int i;
	int utd = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct bio_vec * bv=bio->bi_io_vec;
	struct page* p;
	//gwas(printk("<0>" "GSFS_writepages_bio_end started and vcnt: %u,err: %d\n",bio->bi_vcnt,err));
	
	for(i=0;i<bio->bi_vcnt;i++){
		p=bv->bv_page;
		
		if(!utd){
			SetPageError(p);
			printk("<1>" "wp_bio_end error for index %lu of inode%lu",p->index,p->mapping->host->i_ino);
		}
		
		//gwas(printk("<0>" "wp_bio_end i:%d &page:%lx _count%d inode:%lu index:%lu\n",i,(unsigned long)p,atomic_read(&p->_count),p->mapping->host->i_ino,p->index));
		
		end_page_writeback(p);
		//put_page(p);
		bv++;
	}
	bio_put(bio);
	
	//gwas(printk("<0>" "GSFS_writepages_bio_end ended. pid:%u\n",current->pid));
	
	return;
}

static void GSFS_sec_writepages_bio_end(struct bio *bio, int err){
	int			i,
				utd = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct bio_vec		*bv=bio->bi_io_vec;
	struct page		*p,
				*op;
	
	#ifdef development_pagecache
		struct GSFS_page	*gp;
		char	repp[1000],
			*rep=repp;
			
		sprintf(rep, "GSFS_sec_writepages_bio_end and vcnt: %u, err: %d",bio->bi_vcnt,err);
		rep+=strlen(rep);
	#endif
	
	for(i=0;i<bio->bi_vcnt;i++){
		#ifdef development_pagecache
			if(rep-repp>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		
		p=bv->bv_page;
		
		gwas(sprintf(rep,"# i:%d, aux_page:%lx, _count: %d,", i, (unsigned long)p, atomic_read(&p->_count)));
		gwas(rep+=strlen(rep));
		//gwas(sprintf(rep,"16-byte of apd: "));
		//gwas(rep+=strlen(rep));
		//gwas(printhexstring((char*)page_address(p), rep, 16));
		//gwas(rep+=strlen(rep));
		
		
		op=((struct GSFS_page*)page_private(p))->origin_page;
		
		gwas(sprintf(rep," ,original_page:%lx", (unsigned long)op));
		gwas(rep+=strlen(rep));
		
		if(likely(op)){
			gwas(gp=(struct GSFS_page*)page_private(op));
			
			gwas(sprintf(rep," ,_count: %d, inode:%lu, index:%lu, gp: %lx",atomic_read(&op->_count),op->mapping->host->i_ino, op->index, (unsigned long)gp));
			gwas(rep+=strlen(rep));
			
			gwas(if(gp){sprintf(rep, " ,block_num: %u",gp->disk_bnr);rep+=strlen(rep);});
			
			end_page_writeback(op);
		}
		
		if(unlikely(!utd)){
			SetPageError(p);
			printk("<1>" "wp_bio_end error for index %lu of inode%lu",p->index,p->mapping->host->i_ino);
		}
		
		bv++;
	}
	
	bio_put(bio);
	
	gwas(printk("<0>" "%s\n",repp));
	
	return;
}

int GSFS_writepages(struct address_space *mapping, struct writeback_control *wbc){
	struct inode		*inode=mapping->host;
	struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	unsigned long 	ott;
	pgoff_t 	current_index,
			start_index,
			end_index,
			nr_to_write=wbc->nr_to_write,
			done_index=0;
	unsigned int	cycled,
			nr_pages,
			i,
			done,
			sync=(wbc->sync_mode==WB_SYNC_ALL),
			ppcount,
			range_whole;
	struct  page	**pages,
			*page,
			**pp;
	//sector_t	*blocks;	
	
	ott=0;
	if(wbc->older_than_this)
		ott=*wbc->older_than_this;
	
	gwas(printk("<0>" "writepages for inode:%lu, mode==all:%d, bdi:%lx, &older_than_this:%lu, nr_to_write:%lx, nonblocking:%u, range_s:%llx, range_e:%llx, range_cyc:%u, map->wb_ind:%lu, wbc->no_up:%d\n"
		,inode->i_ino,wbc->sync_mode==WB_SYNC_ALL,(unsigned long)wbc->bdi,ott,wbc->nr_to_write,wbc->nonblocking,wbc->range_start,wbc->range_end,wbc->range_cyclic,mapping->writeback_index,wbc->no_nrwrite_index_update));	
	
	cycled=1;
	range_whole=0;
	
	if(wbc->range_cyclic){
		start_index=mapping->writeback_index;
		end_index=-1;
		if(start_index)
			cycled=0;
	}
	else{
		start_index=wbc->range_start>>Block_Size_Bits;
		end_index=wbc->range_end>>Block_Size_Bits;
		if (wbc->range_start == 0 && wbc->range_end == LLONG_MAX)
			range_whole = 1;
	}
	
	current_index=start_index;
	
	pages=kmalloc(Loop_Pages_WB*sizeof(struct page*),GFP_KERNEL);
	pp=kmalloc(Loop_Pages_WB*sizeof(struct page*),GFP_KERNEL);
	//blocks=kmalloc(Loop_Pages_WB*sizeof(sector_t),GFP_KERNEL);
	
	done=0;
retry:	ppcount=0;
	while(!done && (current_index<=end_index)){
		
		nr_pages=find_get_pages_tag(mapping, &current_index, PAGECACHE_TAG_DIRTY, Loop_Pages_WB,pages);
		
		if(nr_pages==0)
			break;
		
		for(i=0;i<nr_pages;i++){
			page=pages[i];
			
			if(!page || page->index>end_index){
				done=1;
				break;
			}
			
			done_index=page->index;
			lock_page(page);
			
			if( (page->mapping!=mapping) || !PageDirty(page) || (!sync && PageWriteback(page)) ){
				unlock_page(page);
				continue;
			}
			if(sync && PageWriteback(page))
				wait_on_page_writeback(page);
			if(!clear_page_dirty_for_io(page)){
				unlock_page(page);
				continue;			
			}
			
			pp[ppcount]=page;
			
			/*
			blocks[ppcount]=get_dp_bn_of_in(inode,page->index);			
			if(!blocks[ppcount]){
				//printk("<0>" "Somethings bad page with out any block for inode %lu, index%lu\n",inode->i_ino,page->index);
				unlock_page(page);
				radix_tree_tag_clear(&mapping->page_tree, page->index, PAGECACHE_TAG_DIRTY);	
				continue;
			}
			page->private=(unsigned long)(blocks+ppcount);
			*/
			
			set_page_writeback(page);
			unlock_page(page);			
			ppcount++;
			
			if(ppcount==Loop_Pages_WB){
				//printk("<0>" "WP writing some pages %d\n",ppcount);
				
				if(inf->igflags & igflag_secure)
					GSFS_sort_and_submit_bio_for_sec_pages(inode, pp, ppcount, WRITE, GSFS_sec_writepages_bio_end);
				else
					GSFS_sort_and_submit_bio_for_pages(inode, pp, ppcount, WRITE, GSFS_writepages_bio_end);
	
				ppcount=0;
			}			
			if(nr_to_write>0){
				nr_to_write--;
				if(nr_to_write==0 && !sync){
					done=1;
					break;
				}
			}
		}
		
		cond_resched();
	}
	if(ppcount){
		//printk("<0>" "WP writing some pages %d\n",ppcount);
		if(inf->igflags & igflag_secure)
			GSFS_sort_and_submit_bio_for_sec_pages(inode, pp, ppcount, WRITE, GSFS_sec_writepages_bio_end);
		else
			GSFS_sort_and_submit_bio_for_pages(inode, pp, ppcount, WRITE, GSFS_writepages_bio_end);
	}
	
	if(!cycled && !done){
		cycled=1;
		current_index=0;
		end_index=mapping->writeback_index-1;
		goto retry;
	}
	
	if (wbc && !wbc->no_nrwrite_index_update) {
		if (wbc->range_cyclic || (range_whole && nr_to_write > 0))
			mapping->writeback_index = done_index;
		wbc->nr_to_write = nr_to_write;
	}
	
	kfree(pages);
	kfree(pp);
	//kfree(blocks);
	
	gwas(printk("<0>" "writepages ended for inode:%lu nr_to_write:%lx, done_index:%lu\n",inode->i_ino,nr_to_write,done_index));
	
	return 0;
}

int GSFS_writepage(struct page *page, struct writeback_control *wbc){
	
	gwas(printk("<0>" "writepage for page %lu of inode %lu\n",page->index,page->mapping->host->i_ino));
	GSFS_writepages(page->mapping,wbc);
	return 0;
}

struct address_space_operations GSFS_address_space_operations={
	.readpages	=	GSFS_readpages,
	.readpage	=	GSFS_readpage,
	.releasepage	=	GSFS_releasepage,
	.invalidatepage	=	GSFS_invalidatepage,
	.set_page_dirty	=	GSFS_set_page_dirty,
	//.launder_page	=	GSFS_launder_page,
	.sync_page	=	GSFS_sync_page,
	.writepage	=	GSFS_writepage,
	.writepages	=	GSFS_writepages,
};