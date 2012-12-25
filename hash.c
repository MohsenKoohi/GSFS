#include "gsfs.h"

inline int get_IAT_hash(struct GSFS_sb* gsb, char* hashval){
	return get_hash_of_sequential_pages(gsb->sb, gsb->gsb_disk.iat_start, gsb->gsb_disk.iat_end,hashval);
}

inline int get_BAT_hash(struct GSFS_sb* gsb, char* hashval){
	return get_hash_of_sequential_pages(gsb->sb, gsb->gsb_disk.bat_start, gsb->gsb_disk.bat_end,hashval);
}

inline int get_SAT_hash(struct GSFS_sb* gsb, char* hashval){
	return get_hash_of_sequential_pages(gsb->sb, gsb->gsb_disk.sat_start, gsb->gsb_disk.sat_end, hashval);
}

inline int get_IHP_hash(struct GSFS_sb* gsb, char* hashval){
	return get_hash_of_sequential_pages(gsb->sb, gsb->gsb_disk.ihp_start, gsb->gsb_disk.ihp_end, hashval);
}

int get_pb_page_hash(struct GSFS_sb* gsb,char* hashval){
	unsigned int 	*blocks,
			bnum,
			i,
			ret;
	struct pb_page_entry* pen;
	struct buffer_head*bh;
	
	bnum=0;
	i=0;
	ret=0;
	
	down_read(&gsb->rsa_keys_rwsem);
	
	blocks=kzalloc(sizeof(unsigned int)*(1+gsb->gsb_disk.pb_num),GFP_KERNEL);
	
	blocks[bnum++]=gsb->gsb_disk.pb_page;
	if(blocks[0]>=gsb->gsb_disk.total_blocks){
		ret=-1;
		goto back;
	}
	
	bh=__bread(gsb->sb->s_bdev, gsb->gsb_disk.pb_page, Block_Size);
		
	pen=(struct pb_page_entry*)bh->b_data;
	while(bnum<=gsb->gsb_disk.pb_num && i<total_users){
		if(pen->block!=0){
			blocks[bnum]=pen->block;
			if(blocks[bnum]>=gsb->gsb_disk.total_blocks){
				ret=-1;
				break;
			}
			bnum++;
		}
		pen++;
		i++;
	}	
	
	brelse(bh);
	
	if(!ret)
		ret=get_hash_of_non_sequential_pages(gsb->sb, blocks, bnum, hashval);
	
back:	up_read(&gsb->rsa_keys_rwsem);
	
	kfree(blocks);	

	return ret;
}

inline int get_sb_hash(struct GSFS_sb* gsb,char* hashval){
	//printk("<0>" "sb_hash :%d\n",offsetof(struct GSFS_sb_ondisk, sb_hash));
	return skein512(	gsfs_hashlen_bits, (char*)(&gsb->gsb_disk), 
			(offsetof(struct GSFS_sb_ondisk, sb_hash))<<3, hashval);
}

int get_hash_of_sequential_pages(struct super_block *sb, unsigned int start, unsigned int end, char* hashval){
	struct 	buffer_head* bh;
	char	*hashes,
		*hashes2;
	int	hasheslen,
		i,
		ret;
	
	hasheslen=gsfs_hashlen*(end-start+1);
	hashes=kzalloc(hasheslen,GFP_KERNEL);
	hashes2=hashes;
	
	for(i=start;i<=end;i++){
		bh=__bread(sb->s_bdev,i,Block_Size);
		lock_buffer(bh);
		ret=skein512(gsfs_hashlen_bits, bh->b_data, Block_Size_in_Bits, hashes);
		if(ret){
			printk("<1>" "SeqHash: Some erros is Skein512 hashing for block %u\n",i);
			goto back;
		}
		unlock_buffer(bh);
		brelse(bh);
		hashes+=gsfs_hashlen;
	}
	
	ret=skein512(gsfs_hashlen_bits, hashes2, hasheslen<<3, hashval);
	if(ret)
		printk("<1>" "SeqHash: Some erros is Skein512 hashing for all of hash\n");
	
back:	kfree(hashes2);
	
	return ret;
}

int get_hash_of_non_sequential_pages(struct super_block *sb, unsigned int * blocks, unsigned int num, char* hashval){
	struct 	buffer_head* bh;
	char	*hashes,
		*hashes2;
	int	hasheslen,
		i,
		ret;
	
	hasheslen=gsfs_hashlen*num;
	hashes=kzalloc(hasheslen,GFP_KERNEL);
	hashes2=hashes;
	
	for(i=0;i<num;i++){
		bh=__bread(sb->s_bdev,blocks[i],Block_Size);
		//lock_buffer(bh);
		ret=skein512(gsfs_hashlen_bits, bh->b_data, Block_Size_in_Bits, hashes);
		if(ret){
			printk("<1>" "NonSeqHash: Some erros is Skein512 hashing for block %u\n",blocks[i]);
			goto back;
		}
		//unlock_buffer(bh);
		brelse(bh);
		hashes+=gsfs_hashlen;		
	}
	
	ret=skein512(gsfs_hashlen_bits, hashes2, hasheslen<<3, hashval);
	if(ret)
		printk("<1>" "NonSeqHash: Some erros is Skein512 hashing for all of hash\n");
	
back:	kfree(hashes2);
	
	return ret;
}

inline int get_gdirent_hash(char* dest,struct GSFS_dirent* gd){
	int ret;
	
	ret=skein512(gsfs_hashlen_bits, (char*)gd, gsfs_dirent_len<<3, dest);
	
	return ret;
};

int update_hash_block_to_root(char* page, unsigned short *changes,unsigned short ch_len){
	unsigned short  ch1s[fl_cn],
			ch2s[sl_cn],
			*temp,
			*ch1,
			*ch2;
	int 	i,
		c1len,
		c2len,
		level_div[]={fl_hg, sl_hg, tl_hg},
		level_off[]={fl_off, sl_off, tl_off, root_off};
	
	if(!page || !ch_len || !changes)
		return -1;
	
	ch1=(unsigned short*)ch1s;
	ch2=(unsigned short*)ch2s;
	memcpy(ch1, changes, sizeof(unsigned short)*ch_len);
	c1len=ch_len;

	for(i=0;i<hash_levels;i++){
		int 	j=0,
			level_hash_len_bits,
			data_src_offset,
			hash_dest_offset;
		
		c2len=0;
		ch2[c2len++]=ch1[j++]/level_div[i];
		
		for(;j<c1len;j++){
			int m;
			m=ch1[j]/level_div[i];
			if(m!=ch2[c2len-1])
				ch2[c2len++]=m;
		}
		
		level_hash_len_bits=((level_div[i])<<(gsfs_hashlen_shift+3));
		
		for(j=0;j<c2len;j++){
			data_src_offset=(ch2[j]*level_div[i]+level_off[i])<<gsfs_hashlen_shift;
			hash_dest_offset=(ch2[j]+level_off[i+1])<<gsfs_hashlen_shift;
			skein512(gsfs_hashlen_bits, page+data_src_offset ,level_hash_len_bits, 
				 page+hash_dest_offset);
			//printk("<0>" "%d %d\n",data_src_offset/16,hash_dest_offset/16);
		}
		//printk("<0>" "%d***\n",level_hash_len_bits/8);
		temp=ch1;
		ch1=ch2;
		ch2=temp;
		
		c1len=c2len;
	}
	
	return 0;
}

inline int get_user_block_hash(char* dest, char* page){
	int ret;
	
	if(!dest || !page)
		return -1;
	
	ret=skein512(gsfs_hashlen_bits, page, Block_Size_in_Bits, dest);
	
	return ret;
}

inline int get_inode_metadata_hash_for_parent(struct inode* in, char* dest){
	struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
	int ret;
	
	if(unlikely(!in || !dest))
		return -1;
	
	ret=skein512(gsfs_hashlen_bits, (char*) (&inf->disk_info), 
		     (sizeof(struct GSFS_inode_disk_inf))<<3, dest);
	
	return ret;
}

int verify_hash_integrity(char* page,char* integ_arr,unsigned short index,char* dest,char* verifid_root){
	unsigned int 	max=inode_integrity_array_len_bits;
	int 	i,
		j,
		verified[hash_levels],
		level_div[]={fl_hg, sl_hg, tl_hg},
		level_off[]={fl_off, sl_off, tl_off, root_off};
	
	if(!is_set_one_index(integ_arr, root_off, max)){
		if(strncmp(verifid_root, page+hash_root_offset, gsfs_hashlen))
			return -1;
		set_one_index(integ_arr, root_off, max);
	}
	
	j=index;
		
	for(i=0;i<=hash_levels;i++){
		int	start,
			pstart;
		char	phash[gsfs_hashlen];
		//printk("<0>" "%d\n",j+level_off[i]);
		if(is_set_one_index(integ_arr, j+level_off[i], max)){
			int k;
			
			memcpy(dest, page+(index<<gsfs_hashlen_shift), gsfs_hashlen);
			
			for(j=0;j<i;j++)
				for(k=0;k<level_div[j];k++)
					set_one_index(integ_arr, level_off[j]+verified[j]+k, max);
			
			return 0;
		}
		
		if(i==hash_levels)
			break;
		
		j/=level_div[i];
		start=(level_off[i]+j*level_div[i])<<gsfs_hashlen_shift;
		//printk("<0>" "%d %d\n",start/16, level_div[i]);
		
		skein512(gsfs_hashlen_bits, page+start, level_div[i]<<(gsfs_hashlen_shift+3), phash);
		//printkey(phash);
		
		pstart=((j+level_off[i+1])<<gsfs_hashlen_shift);
		if(strncmp(page+pstart, phash, gsfs_hashlen))
			return -1;
		
		verified[i]=j*level_div[i];
	}
	
	printk("<1>" "Verifying hash reached final return\n");
	gw(printk("<0>" "Verifying hash reached final return\n"));
	
	return -1;
}

inline int get_crust_hash(char* dest, struct crust_state* crust){
	return skein512(gsfs_hashlen_bits, (char*)crust, sizeof(struct crust_state)<<3, dest);
}

inline int get_bnh_page_hash(char* dest, char* bnh_page){
	return skein512(gsfs_bnh_hash_len_bits, bnh_page, Block_Size_in_Bits, dest);
}

