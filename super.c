#include "gsfs.h"

MODULE_LICENSE("GPL");

//char* name="aa";
//module_param(name,charp,S_IRUGO);

char super_time_hash[46];
gt(struct inode* rootin=0;)

struct page * process_virt_to_page(struct mm_struct* mm, unsigned long address) {
	//printk("<0>" "process virt to page");
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;
	
	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
        	return 0;
	
	pud = pud_offset(pgd, address);
	if (pud_none(*pud) || pud_bad(*pud))
        	return 0;
	
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
        	return 0;
	
	ptep = pte_offset_kernel(pmd, address);
	if (!ptep)
	        return 0;
	
	//printk("<0>" "%lx %lx %lx %lx",pgd,pud,pmd,ptep);
	
	return pte_page(*ptep);
}

char* get_tm(char* p){
	struct timeval tv;
	struct tm tm;
	
	do_gettimeofday(&tv);
	
	time_to_tm(tv.tv_sec, 3.5*3600, &tm);
	
	sprintf(p, "%ld/%d/%d, %d:%d:%d", 1900+tm.tm_year,  tm.tm_mon,  tm.tm_mday,
				          tm.tm_hour,  tm.tm_min,  tm.tm_sec);
	
	return p;
}

struct buffer_head* read_one_bh_dev(struct block_device* bd, sector_t num){
	struct buffer_head * bh;
	
	bh=__getblk(bd, num, bd->bd_block_size);
	
	if(bh){
		//printk("<0>" "read bh: b_count:%d\n",atomic_read(&bh->b_count));
		lock_buffer(bh);
		bh->b_end_io=end_buffer_read_sync;
		submit_bh(READ,bh);
		wait_on_buffer(bh);	
		set_buffer_uptodate(bh);
		clear_buffer_dirty(bh);
		//printk("<0>" "read bh: b_count:%d\n",atomic_read(&bh->b_count));
	}
	return bh;
}

int write_one_bh_dev(struct buffer_head*bh){
	//printk("<0>" "buffer count1 %d %d\n",atomic_read(&bh->b_count),bh->b_blocknr);
	lock_buffer(bh);
	get_bh(bh);
	bh->b_end_io=end_buffer_write_sync;
	if(submit_bh(WRITE,bh)){
		unlock_buffer(bh);
		return -1;
	}
	wait_on_buffer(bh);
	set_buffer_uptodate(bh);
	clear_buffer_dirty(bh);
	//printk("<0>" "buffer count2 %d\n",atomic_read(&bh->b_count));
	return 0;
}

#ifdef gsfs_test
	#define test_write_super
#endif
#ifdef test_write_super
	#define gwts(p) p
#else
	#define gwts(p)
#endif

void GSFS_write_super(struct super_block* sb){
	struct buffer_head*bh;
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct GSFS_sb_ondisk	*gsd=&gsb->gsb_disk;
	struct rsa_key* root_key;
	char * time_hash;
	
	#ifdef test_write_super
	char	repp[1000],
		*rep=repp;
	
	memset(rep, 1000, 0);
	#endif
	
	if(!sb->s_dirt)
		return;
	
	gwts(sprintf(rep,"GSFS_write_super with sb->s_dirt:%d and gsb->sgflags:%x* ",sb->s_dirt, gsb->sgflags));
	gwts(rep+=strlen(rep));
	
	//lock_kernel();
	
	if(!gsb->sgflags){
		gwts(sprintf(rep,"Some changes to super block without setting sgflags *"));
		gwts(rep+=strlen(rep));
		
		gws(printk("<0>" "Some changes to super block without setting sgflags.\n"));
		printk("<1>" "Some changes to super block without setting sgflags.\n");
	}
	
	sync_lru(gsb);
	
	if(gsb->sgflags & sgflag_pb_page){
		int ret;
		
		ret=get_pb_page_hash(gsb,gsd->public_keys_hash);
		if(ret)
			printk("<1>" "Errors in get_pb_page_hash\n");
		
		gwts(sprintf(rep,"sgflag_pb_page => get_pb_page_hash with this result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->public_keys_hash, rep, 16));
		gwts(rep+=strlen(rep));	
	}
	
	if(gsb->sgflags & sgflag_IAT){
		int ret;
		
		ret=get_IAT_hash(gsb,gsd->IAT_hash);
		
		gwts(sprintf(rep,"sgflag_IAT => get_IAT_hash with this result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->IAT_hash, rep, 16));
		gwts(rep+=strlen(rep));	
	}
	
	if(gsb->sgflags & sgflag_BAT){
		int ret;
		
		ret=get_BAT_hash(gsb,gsd->BAT_hash);
		
		gwts(sprintf(rep,"sgflag_BAT => get_BAT_hash with this result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->BAT_hash, rep, 16));
		gwts(rep+=strlen(rep));	
	}
	
	if(gsb->sgflags & sgflag_SAT){
		int ret;
		
		ret=get_SAT_hash(gsb,gsd->SAT_hash);
		
		gwts(sprintf(rep,"sgflag_SAT => get_SAT_hash with this result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->SAT_hash, rep, 16));
		gwts(rep+=strlen(rep));	
	}
	
	if(gsb->sgflags & sgflag_IHP){
		int ret;
		
		ret=get_IHP_hash(gsb,gsd->IHP_hash);
		
		gwts(sprintf(rep,"sgflag_IHP => get_IHP_hash with this result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->IHP_hash, rep, 16));
		gwts(rep+=strlen(rep));	
	}
		
	if(gsb->sgflags){
		int ret;
		
		ret=get_sb_hash(gsb,gsd->sb_hash);
		
		gwts(sprintf(rep,"sgflags => get_sb_hash with result %d and this hash ",ret));
		gwts(rep+=strlen(rep));	
		gwts(printhexstring(gsd->sb_hash, rep, 16));
		gwts(rep+=strlen(rep));	
		
		root_key=get_rsa_key(gsb,0,1);
		
		if(root_key && root_key->key){
			int ret;
			
			spin_lock(&root_key->lock);
			
			time_hash=kzalloc(30+gsfs_hashlen,GFP_KERNEL);
			
			get_tm(time_hash);
			memcpy(time_hash+30,gsd->sb_hash,gsfs_hashlen);
			
			ret=rsa_1024_encrypt(root_key->key, RSA_PRIVATE, 30+gsfs_hashlen, time_hash,
						gsd->root_signed_super_block_hash);
			
			if(ret){
				gw(printk("<0>" "Some errors in signing hash of super block: %d\n",ret));
				printk("<1>" "Some errors in signing hash of super block: %d\n",ret);
				
				gwts(sprintf(rep,"Some errors in signing hash of super block * "));
				gwts(rep+=strlen(rep));
			}
			else{
				memcpy(super_time_hash, time_hash, 46);
				
				gw(printk("<0>" "We have written for time %s this hash:\n",time_hash));
				gw(printkey(time_hash+30));
				
				gwts(sprintf(rep,"Written hash for %s is: ",time_hash));
				gwts(rep+=strlen(rep));	
				gwts(printhexstring(time_hash+30, rep, 16));
				gwts(rep+=strlen(rep));	
	
			}
			
			spin_unlock(&root_key->lock);
			
			kfree(time_hash);
			
			sb->s_dirt=0;
			gsb->sgflags=0;	
		}
		else{
			gsb->sgflags=sgflag_sb_ondisk;
			
			gwts(sprintf(rep,"Sign key for super block is not available* "));
			gwts(rep+=strlen(rep));
		}
		
	}
	
	bh=__getblk(sb->s_bdev,0,Block_Size);
	
	//lock_buffer(bh);
	memcpy(bh->b_data,&gsb->gsb_disk,min((unsigned long)Block_Size,sizeof(struct GSFS_sb_ondisk)));
	//unlock_buffer(bh);
	
	//write_one_bh_dev(bh);
	
	mark_buffer_dirty(bh);
	
	brelse(bh);
	
	//unlock_kernel();
	
	gws(printk("<0>" "write super ends:%d\n",atomic_read(&bh->b_count)));
	
	gwts(printk("<0>" "%s\n",repp));
	
	return;
}

void GSFS_put_super(struct super_block* sb){
	char 	repp[300],
		*rep=repp;
	
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	
	if(!gsb){
		gt(printk("<0>" "put super with no gsb\n"));
		return;
	}
	gt(printsemkeys(rep,gsb));
	
	//lock_kernel();
		
	exit_lru(gsb);
	
	free_all_rsa_keys(gsb);
	free_incom_inodes(gsb);
	
	memset(gsb, 0, sizeof(struct GSFS_sb));
	kfree(sb->s_fs_info);
	sb->s_fs_info=0;
	
	gws(printk("<0>" "put super"));
	gws(if(!sb->s_root) 
		printk("<0>" "put super: no s_root\n"));
	gws(if(sb->s_root && sb->s_root->d_inode)
		printk("<0>" "put super %d\n",atomic_read(&sb->s_root->d_inode->i_count)));
	
	//unlock_kernel();
	
	if(super_time_hash[0]){
		sprintf(rep,"Written hash for %s is: ",super_time_hash);
		rep+=strlen(rep);	
		printhexstring(super_time_hash+30, rep, 16);
	}
	else
		sprintf(rep,"No new changes in super block hash.");
	
	printk("<0>" "%s\n",repp);
	
	gt(if(rootin)
		printk("<0>" "rootin:%u\n",atomic_read(&rootin->i_count)));
	
	return;
}

void post_fix_update_inode(struct inode* in, int wait){
	struct GSFS_inode		*in_info=(struct GSFS_inode*)in->i_private;
	int 	res_num;
	
	if(!in_info)
		return;
	
	down_write(&in_info->inode_rwsem);
	
	res_num=avl_tree_get_size(in_info->children);
	printk("<0>" "inode: %ld children_num:%d\n",in->i_ino, res_num);	
	if(res_num){
		atn** res;
		int i;
		
		res=kzalloc(sizeof(atn *)*res_num,GFP_KERNEL);
		avl_tree_get_all_nodes(in_info->children, res, res_num);
		
		for(i=0;i<res_num;i++)
			if(res[i]->data->inode)
				post_fix_update_inode(res[i]->data->inode, wait);
	}
	
	//write_inode_to_disk(in, wait, 0, 0, 0, 0, 0);
	
	up_write(&in_info->inode_rwsem);
	
	return;
}

int GSFS_sync_fs(struct super_block* sb, int wait){
	
	GSFS_write_super(sb);
	
	return 0;
}

int GSFS_statfs (struct dentry * den, struct kstatfs* buf){
	struct GSFS_sb* gsb=(struct GSFS_sb*)den->d_sb->s_fs_info;
	
	//container_of(buf,struct kstatfs,f_type);
	buf->f_type=0;
	buf->f_bsize=Block_Size;
	buf->f_blocks=gsb->gsb_disk.total_blocks;
	buf->f_bfree=gsb->gsb_disk.free_blocks;
	buf->f_bavail=gsb->gsb_disk.free_blocks;
	buf->f_files=gsb->gsb_disk.total_inodes;
	buf->f_ffree=gsb->gsb_disk.free_inodes;	
	
	return 0;
}

int GSFS_create_disk(struct super_block* sb){
	sector_t 	k,
			j;
	struct block_device* bdev=sb->s_bdev;
	sector_t i;
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct GSFS_sb_ondisk* gsd=&gsb->gsb_disk;
	struct buffer_head ** bha,
			   ** temp;
	struct inode* in;
	
	gsd->disk_size=bdev->bd_part->nr_sects<<9;
	gsd->total_blocks=gsd->disk_size>>Block_Size_Bits;
	
	if(gsd->total_blocks> Max_Block_Number){
		printk("<0>" "Disk size is larger than maximum disk size can be processed by current version of GSFS.\n");
		goto fault;
	}
	
	gsd->total_inodes=gsd->total_blocks>>3;
	gsd->total_sec_indecis=gsd->total_inodes;
	
	k=0;
	k++;					//one block for superblock
	
	gsd->bat_start=k;
	k+=bat_bn(gsd->total_blocks-1);		//bat blocks
	gsd->bat_end=k;
	k++;
	
	gsd->iat_start=k;
	k+=iat_bn(gsd->total_inodes-1);		//iat blocks
	gsd->iat_end=k;
	k++;
	
	gsd->pb_page=k;				//public keys indecis page
	k++;
	
	gsd->sat_start=k;			//secure indecis allocation table
	k+=sat_bn(gsd->total_inodes-1);
	gsd->sat_end=k;
	k++;
	
	gsd->ihp_start=k;			//inodes hash pages
	k+=ihp_bn(gsd->total_sec_indecis);
	gsd->ihp_end=k;
	
	j=gsd->disk_size-((k+1)<<Block_Size_Bits);
	j=j-j/128;
	j&=~(Block_Size-1);
	gsd->max_file_size=min(j,Max_File_Size);
	//printk("<0>" "file_size:%lu\n",gsd->max_file_size);
		
	gsd->last_block=k;
	gsd->last_sec_index=0;
	gsd->last_inode=0;
	
	gsd->free_blocks=gsd->total_blocks;
	gsd->free_inodes=gsd->total_inodes-1;
	gsd->free_sec_indecis=gsd->total_inodes;
	
	gsd->root_inode_has_secure_child=0;
	
	bha=kmalloc((k+1)*sizeof(struct buffer_head*),GFP_KERNEL);
	temp=bha;
	
	for(i=0;i<=k;i++){
		*bha=__getblk(bdev,i,Block_Size);
		//printk("<0>" "%d %lx\n",i,(void*)(*bha));
		memset((*bha)->b_data,0,Block_Size);
		if(i==0)
			memcpy(bha[0]->b_data,gsd,min((unsigned long)Block_Size,sizeof(struct GSFS_sb_ondisk)));
		set_buffer_uptodate(*bha);
		mark_buffer_dirty(*bha);
		(*bha)->b_end_io=end_buffer_write_sync;
		lock_buffer(*bha);
		submit_bh(WRITE,*bha);
		bha++;
	}
	
	bha=temp;
	for(i=0;i<=k;i++){
		wait_on_buffer(*temp);
		//brelse(*temp);
		//printk("<0>" " %d\n",(*temp)->b_count);
		temp++;
	}
	kfree(bha);
		
	set_BAT(gsb,0,k);
	set_IAT(gsb,0,-1);
		
	in=GSFS_get_new_locked_inode_and_add_its_link(gsb,0,0,igflag_dir);
	if(!in)
		goto fault;
	
	in->i_nlink=1;
	add_some_blocks_to_inode(in,1);
	
	unlock_new_inode(in);
	
	write_inode_to_disk(in, 1);
	iput(in);
	
	sb->s_dirt=1;
	gsb->sgflags=sgflag_sb_ondisk | sgflag_pb_page | sgflag_IAT | sgflag_BAT | sgflag_SAT | sgflag_IHP;
	
	return 0;
	
fault:	
	kfree(sb->s_fs_info);
	sb->s_fs_info=0;
	
	return -1;
}

int check_sb_integrity(struct GSFS_sb* gsb){
	char 	pb_hash[gsfs_hashlen],
		iat_hash[gsfs_hashlen],
		bat_hash[gsfs_hashlen],
		sat_hash[gsfs_hashlen],
		ihp_hash[gsfs_hashlen],
		sb_hash[gsfs_hashlen],
		time_hash[gsfs_rsalen];
	struct rsa_key* root_key;
	struct GSFS_sb_ondisk *gsd=&gsb->gsb_disk;
	int	olen,
		ret;
	
	ret=get_pb_page_hash(gsb,pb_hash);
	if(ret || strncmp(pb_hash,gsd->public_keys_hash,gsfs_hashlen)){
		printk("<0>" "written pb_page_hash %d:\n",ret);
		printkey(gsd->public_keys_hash);
		printkey(pb_hash);
		printk("<0>" "Public keys has been changed.\n");
		return -1;
	}
	
	ret=get_IAT_hash(gsb,iat_hash);
	if(ret || strncmp(iat_hash,gsd->IAT_hash,gsfs_hashlen)){
		printk("<0>" "IAT has been changed.\n");
		return -1;
	}
	
	ret=get_BAT_hash(gsb,bat_hash);
	if(ret || strncmp(bat_hash,gsd->BAT_hash,gsfs_hashlen)){
		printk("<0>" "BAT has been changed.\n");
		return -1;
	}
	
	ret=get_SAT_hash(gsb,sat_hash);
	if(ret || strncmp(sat_hash, gsd->SAT_hash, gsfs_hashlen)){
		printk("<0>" "SAT has been changed.\n");
		return -1;
	}
	
	ret=get_IHP_hash(gsb, ihp_hash);
	if(ret || strncmp(ihp_hash, gsd->IHP_hash, gsfs_hashlen)){
		printk("<0>" "IHP has been changed.\n");
		return -1;
	}
	
	ret=get_sb_hash(gsb,sb_hash);
	if(ret || strncmp(sb_hash,gsd->sb_hash,gsfs_hashlen)){
		printk("<0>" "Super Block has been changed.\n");
		return -1;
	}
	
	root_key=get_rsa_key(gsb,0,0);
	if(root_key && root_key->key){
		if(rsa_1024_decrypt(root_key->key, RSA_PUBLIC, &olen, gsd->root_signed_super_block_hash, 
				time_hash, gsfs_rsalen)){
			gw(printk("<0>" "Some errors in verifying hash of super block\n"));
			printk("<1>" "Some errors in verifying hash of super block\n");
		}
		else{
			printk("<0>" "The written RSA-1024 signed hash (%d) at %s is:\n",olen,time_hash);
			printkey(time_hash+30);
		}
		
		if(strncmp(sb_hash,time_hash+30,gsfs_hashlen)){
			printk("<0>" "Super Block has been changed.\n");
			return -1;
		}
	}
	else
		printk("<0>" "Public key of super user is not available.\nWorking without any integrity.\n");
	
	return 0;
}

int GSFS_fill_sb(struct super_block *sb, void* data, int silent){
	char* p=(char*)data;
	struct buffer_head * bh;
	struct GSFS_sb * gsb;
	struct GSFS_sb_ondisk *gsd;
	struct inode* root_inode;
	struct GSFS_inode* root_info;
	
	printk("<0>" "Input Data= %s, disk_name=%s",p,sb->s_bdev->bd_disk->disk_name);
	
	if(GSFS_chardev_init(sb))
		return -1;
	
	gsb=kzalloc(sizeof(struct GSFS_sb),GFP_KERNEL);
	
	gsb->sb=sb;
	gsd=&gsb->gsb_disk;
	sb->s_fs_info=gsb;
	
	set_blocksize(sb->s_bdev,Block_Size);
	sb->s_blocksize=Block_Size;
	sb->s_time_gran=1000*1000*1000;
	sb->s_blocksize_bits=Block_Size_Bits;
	//sb->s_maxbytes=Max_File_Size;
	sb->s_magic=GSFS_MAGIC;
	sb->s_op=&GSFS_super_operations;
	
	initialize_lru(gsb);
	init_rwsem(&gsb->rsa_keys_rwsem);
	init_rwsem(&gsb->incom_inodes_rwsem);
	
	if(data && strlen(data)>=6 && !strncmp(data,"create",6)){
		p+=6;
		if(*p == ',')
			p++;
		data=(void*)p;
		
		if(GSFS_create_disk(sb))	//create disk data structures and initialize gsb
			return -1;
	}
	else{
		bh=read_one_bh_dev(sb->s_bdev,0);
		get_bh(bh);
		memcpy(gsd,bh->b_data,min(sizeof(struct GSFS_sb_ondisk),(unsigned long)Block_Size));
		brelse(bh);
		
		if(gsd->disk_size != (sb->s_bdev->bd_part->nr_sects<<9) ||
		   gsd->total_blocks != (sb->s_bdev->bd_part->nr_sects>>(Block_Size_Bits-9)) ){
			printk("<0>" "Disk Size has been changed.\n");
			return -1;
		}
		
		//verifying integrity of super block fields:
		//IAT BAT pbpage userskey superblock
		if(check_sb_integrity(gsb))
			return -1;
		sb->s_dirt=0;
	}
	
	printk("<0>" "total block number=%d, bat_s=%d, bat_end=%d, iat_start=%d, iat_end=%d, pb_page=%d, sat_start=%d, sat_end:%d, ihp_start:%d, ihp_end:%d\n",
			gsd->total_blocks ,gsd->bat_start,gsd->bat_end, gsd->iat_start, gsd->iat_end, gsd->pb_page,
			gsd->sat_start, gsd->sat_end, gsd->ihp_start, gsd->ihp_end);
	
	sb->s_maxbytes=gsd->max_file_size;
	
	root_inode=GSFS_get_inode(gsb,1);
	if(!root_inode){
		printk("<0>" "Some errors for integrity testing of root_inode\n");
		return -1;
	}
	
	gsb->root_inode=root_inode;
	
	gt(rootin=root_inode);
	gt(printk("<0>" "rootin: %u\n",atomic_read(&rootin->i_count)));
	
	sb->s_root=d_alloc_root(root_inode);
	
	root_info=(struct GSFS_inode*)root_inode->i_private;
	
	//initializing gum_struct of gsb
	sema_init(&gsb->gum_struct.gum_struct_sem, 1);		//unlocked
	sema_init(&gsb->gum_struct.gum_is_ready_sem, 0);	//locked
	
	gsb->gum_struct.gum_is_initialized=1;
	
	return 0;
}
 
int GSFS_get_sb( struct file_system_type *type, int flags, const char* dev_name, 
					 void* data, struct vfsmount *mnt){
	int ret;
	
	ret=get_sb_bdev(type, flags, dev_name, data, GSFS_fill_sb, mnt);
	
	#ifdef gsfs_test
		clear_test_indecis();
	#endif
	
	memset(super_time_hash, 0, 46);
	
	return ret;
}

void GSFS_kill_sb(struct super_block* sb){
	
	GSFS_gum_exit();
	
	GSFS_chardev_exit();

	kill_block_super(sb);
	
	return;
}

struct super_operations GSFS_super_operations={
	.write_super 	=	GSFS_write_super,
	.put_super	=	GSFS_put_super, 
	.sync_fs	=	GSFS_sync_fs,
	.destroy_inode	=	GSFS_destroy_inode,
	.delete_inode	=	GSFS_delete_inode,
	//.clear_inode	=	GSFS_clear_inode,
	.statfs		=	GSFS_statfs,
	.write_inode	=	GSFS_write_inode,	
};

struct file_system_type GSFS_fstype={
	.owner = THIS_MODULE,
	.name = "GSFS",
	.get_sb = GSFS_get_sb,
	.kill_sb = GSFS_kill_sb,
	.fs_flags = FS_REQUIRES_DEV,
};

static int GSFS_init(void){
	char *p;
	
	printk("<0>" "In The Name of God\n");
	/*	
	struct task_struct* p;
	for_each_process(p)
		if(p->pid==7158	){		
			struct page* mp=process_virt_to_page(p->mm,0x602010);
			void* ad=page_address(mp);			
			ad+=0x10UL;
                        int *m=(int*)ad;
			printk("<0>" "%d",*m);
			(*m)=456987;
		}
	*/
	rsa_1024_init();
	
	if(register_filesystem(&GSFS_fstype))
		return -1;
	
	p=kzalloc(32,GFP_KERNEL);
	get_tm(p);
	printk("<0>" "end of initializing in %s (%ld).\n",p,strlen(p));
	kfree(p);

	//printk("<0>" "%ld\n",sizeof(struct GSFS_inode_disk_inf));

	return 0;
}

static void GSFS_exit(void){
	
	unregister_filesystem(&GSFS_fstype);
	
	return;
}

module_init(GSFS_init);

module_exit(GSFS_exit);

#ifdef gsfs_test
	void printsemkeys(char* dest, struct GSFS_sb* gsb){
		int i=0;
		
		sprintf(dest,"inode sems : ");
		dest+=strlen(dest);
		
		for(i=0;i<max_inode_sems;i++)
			if(gsb->inode_sems_test[i]){
				sprintf(dest, "%d ", i);
				dest+=strlen(dest);
			}
			
		sprintf(dest, "* ");
		
		return;
	}
#endif
