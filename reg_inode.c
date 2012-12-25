#include "gsfs.h"

#define l1_bhbof(hbn)	(hbn&255)
#define l2_bhbof(hbn)	((hbn>>8)&255)
#define l3_bhbof(hbn)	((hbn>>16)&1)

#ifdef gsfs_test
	//#define sri_test
#endif
#ifdef sri_test
	#define gsri(w)	w
#else
	#define gsri(w)
#endif

/*
int update_sec_reg_inode_hash_blocks_fe(struct inode* in,unsigned int start_bn,unsigned int len){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct GSFS_sb			*gsb=in->i_sb->s_fs_info;
	unsigned int 	end_bn,
			start_hbn,
			end_hbn,
			hbn_len,
			*blocks,
			i,
			j,
			l1_bh_num=0,
			l2_bh_num=0;
	
	struct buffer_head	*l1_bh=0,
				*l2_bh=0,
				*bhs[2];
	int 	ret=0;
	
	char zerohash[gsfs_bnh_hash_len];
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"update_sec_reg_inode_hash_blocks for in:%ld, start_bn:%u, len:%u * ", in->i_ino, start_bn, len);
	rep+=strlen(rep);
	#endif
	
	end_bn=start_bn+len-1;
	
	if(end_bn<l0_vias_len)
		return 0;
	
	if(start_bn<l0_vias_len)
		start_bn=l0_vias_len;
	
	start_hbn=start_bn/vias_per_block;
	end_hbn=end_bn/vias_per_block;
	
	if(start_bn!=l0_vias_len){
		if(start_hbn==end_hbn)
			return 0;
		
		if(start_bn%vias_per_block!=0)
			start_hbn++;
	}
	
	gsri(sprintf(rep, "start_hbn: %u, end_hbn: %u * ", start_hbn, end_hbn));
	gsri(rep+=strlen(rep));
	
	hbn_len=end_hbn-start_hbn+1;
	if(hbn_len==0)		//I think it is impossible but...
		return 0;
	
	blocks=kzalloc(hbn_len * sizeof(unsigned int), GFP_KERNEL);
	if(BAT_get_some_blocks(gsb, hbn_len, blocks)==-1)
		return -1;
	
	for(i=0;i<hbn_len;i++){
		struct buffer_head *bh;
		
		bh=__bread(in->i_sb->s_bdev, blocks[i], Block_Size);
		memset(bh->b_data, 0, Block_Size);
		
		if(i==0)
			get_bnh_page_hash(zerohash, bh->b_data);
			
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
	}
	
	gsri(sprintf(rep, "zerohash: "));
	gsri(rep+=strlen(rep));
	gsri(printhexstring(zerohash, rep, 12));
	gsri(rep+=strlen(rep));
		
	j=0;
	for(i=start_hbn; i<=end_hbn; i++){
		struct bnh	*bnh,
				*bnh2;
		unsigned int 	l1=l1_bhbof(i),
				l2=l2_bhbof(i),
				l3=l3_bhbof(i);
				
		#ifdef sri_test
			if(strlen(repp)>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
			}
		#endif
		
				
		gsri(sprintf(rep, "## loop: i: %u, l1: %u, l2:%u, l3:%u * ", i,l1,l2,l3));
		gsri(rep+=strlen(rep));
	
		if(l1<l1_bnhs_len && l2==0 && l3==0){
			//blocks are in the inode for first l1_bnhs_len=8 bnhs
			
			ind->reg_inode_security.l1_bnhs[l1].blocknumber=blocks[j++];
			memcpy(ind->reg_inode_security.l1_bnhs[l1].hash, zerohash, gsfs_bnh_hash_len);
			
			gsri(sprintf(rep, "l1<l1_bnhs_len, allocated_block is: %u * ", blocks[j-1]));
			gsri(rep+=strlen(rep));
			
			continue;
		}
		
		if(l2<l2_bnhs_len && l3==0){
			//blocks are in the inode for first l2_bnhs_len=6 bnhs
			
			gsri(sprintf(rep, "l2<l2_bnhs_len * "));
			gsri(rep+=strlen(rep));
						
			if(ind->reg_inode_security.l2_bnhs[l2].blocknumber==0){
				unsigned int bn=0;
				if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
					ret=-1;
					break;
				}
			
				ind->reg_inode_security.l2_bnhs[l2].blocknumber=bn;
				
				if(l1_bh){
					mark_buffer_dirty(l1_bh);
					set_buffer_uptodate(l1_bh);
					brelse(l1_bh);
				}
				
				l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
				memset(l1_bh->b_data, 0, Block_Size);
				l1_bh_num=bn;
				
				gsri(sprintf(rep, "allocated bn to l2_bnhs[l2]: %u * ",bn));
				gsri(rep+=strlen(rep));			
			}
			
			if(l1_bh && l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				mark_buffer_dirty(l1_bh);
				set_buffer_uptodate(l1_bh);
				brelse(l1_bh);
			}
			
			if(!l1_bh || l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				char gh[gsfs_bnh_hash_len];
				
				l1_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l2_bnhs[l2].blocknumber, Block_Size);
				l1_bh_num=ind->reg_inode_security.l2_bnhs[l2].blocknumber;
				
				gsri(sprintf(rep, "openning l2_bnhs[l2] with bn: %u * ",ind->reg_inode_security.l2_bnhs[l2].blocknumber));
				gsri(rep+=strlen(rep));
				
				get_bnh_page_hash(gh, l1_bh->b_data);
				if(strncmp(gh, ind->reg_inode_security.l2_bnhs[l2].hash , gsfs_bnh_hash_len)){
					printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l2:%d\n"
						,in->i_ino, ind->reg_inode_security.l2_bnhs[l2].blocknumber, l2);
					ret=-1;
					
					gsri(sprintf(rep, "badhash * "));
					gsri(rep+=strlen(rep));
					
					break;
				}				
			}
			
			bnh=(struct bnh*)l1_bh->b_data;
			bnh+=l1;
			bnh->blocknumber=blocks[j++];
			memcpy(bnh->hash, zerohash, gsfs_bnh_hash_len);
			
			get_bnh_page_hash(ind->reg_inode_security.l2_bnhs[l2].hash, l1_bh->b_data);
			
			gsri(sprintf(rep, "adding blocks %i and new hash: ",blocks[j-1]));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(ind->reg_inode_security.l2_bnhs[l2].hash, rep, 12));
			gsri(rep+=strlen(rep));
			
			continue;
		}
		
		//we should use l3 to l1 to update bhb
		
		gsri(sprintf(rep, "l3 * "));
		gsri(rep+=strlen(rep));
		
		if(ind->reg_inode_security.l3_bnhs[l3].blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			ind->reg_inode_security.l3_bnhs[l3].blocknumber=bn;
			
			if(l2_bh){
				mark_buffer_dirty(l2_bh);
				set_buffer_uptodate(l2_bh);
				brelse(l2_bh);
			}
			
			l2_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			memset(l2_bh->b_data, 0, Block_Size);
			l2_bh_num=bn;
		}
			
		if(l2_bh && l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			mark_buffer_dirty(l2_bh);
			set_buffer_uptodate(l2_bh);
			brelse(l2_bh);
		}
		
		if(!l1_bh || l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l2_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l3_bnhs[l3].blocknumber, Block_Size);
			l2_bh_num=ind->reg_inode_security.l3_bnhs[l3].blocknumber;
			
			get_bnh_page_hash(gh, l2_bh->b_data);
			if(strncmp(gh, ind->reg_inode_security.l3_bnhs[l3].hash , gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l3:%d\n"
					,in->i_ino, ind->reg_inode_security.l3_bnhs[l3].blocknumber, l3);
				ret=-1;
				break;
			}	
		}
		
		bnh=(struct bnh*)l2_bh->b_data;
		bnh+=l2;
		
		if(bnh->blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			bnh->blocknumber=bn;
			
			if(l1_bh){
				mark_buffer_dirty(l1_bh);
				set_buffer_uptodate(l1_bh);
				brelse(l1_bh);
			}
			
			l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			memset(l1_bh->b_data, 0, Block_Size);
			l1_bh_num=bn;
		}
		
		if(l1_bh && l1_bh_num!=bnh->blocknumber){
			mark_buffer_dirty(l1_bh);
			set_buffer_uptodate(l1_bh);
			brelse(l1_bh);
		}
		
		if(!l1_bh || l1_bh_num!=bnh->blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l1_bh=__bread(in->i_sb->s_bdev, bnh->blocknumber, Block_Size);
			l1_bh_num=bnh->blocknumber;
			
			get_bnh_page_hash(gh, l1_bh->b_data);
			if(strncmp(gh, bnh->hash, gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of not on inode l2:%d\n"
					,in->i_ino, bnh->blocknumber, l2);
				ret=-1;
				break;
			}
		}
		
		bnh2=(struct bnh*)l1_bh->b_data;
		bnh2+=l1;		
		
		bnh2->blocknumber=blocks[j++];
		
		memcpy(bnh2->hash, zerohash, gsfs_bnh_hash_len);
				
		get_bnh_page_hash(bnh->hash, l1_bh->b_data);
		
		get_bnh_page_hash(ind->reg_inode_security.l3_bnhs[l3].hash, l2_bh->b_data);
	}
	
	bhs[0]=l1_bh;
	bhs[1]=l2_bh;
	for(i=0;i<2;i++)
		if(bhs[i]){
			mark_buffer_dirty(bhs[i]);
			set_buffer_uptodate(bhs[i]);
			brelse(bhs[i]);
		}
		
	gsri(sprintf(rep, "ret =  %d * ",ret));
	gsri(rep+=strlen(rep));
	
	#ifdef sri_test
		printk("<0>" "%s\n", repp);
		kfree(repp);
	#endif
	
	return ret;
}
*/

/*
int update_sec_reg_inode_hash_blocks_se(struct inode* in,unsigned int start_bn,unsigned int len){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct GSFS_sb			*gsb=in->i_sb->s_fs_info;
	unsigned int 	end_bn,
			start_hbn,
			end_hbn,
			hbn_len,
			*blocks,
			i,
			j,
			l1_bh_num=0,
			l2_bh_num=0;
	
	int	pl2=-1,
		pl3=-1;
		
	struct bnh 	*l2_active_bnh=0,
			*l3_active_bnh=0;
	
	struct buffer_head	*l1_bh=0,
				*l2_bh=0,
				*bhs[2];
	int 	ret=0;
	
	char zerohash[gsfs_bnh_hash_len];
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"update_sec_reg_inode_hash_blocks for in:%ld, start_bn:%u, len:%u * ", in->i_ino, start_bn, len);
	rep+=strlen(rep);
	#endif
	
	end_bn=start_bn+len-1;
	
	if(end_bn<l0_vias_len)
		return 0;
	
	if(start_bn<l0_vias_len)
		start_bn=l0_vias_len;
	
	start_hbn=start_bn/vias_per_block;
	end_hbn=end_bn/vias_per_block;
	
	if(start_bn!=l0_vias_len){
		if(start_hbn==end_hbn)
			return 0;
		
		if(start_bn%vias_per_block!=0)
			start_hbn++;
	}
	
	gsri(sprintf(rep, "start_hbn: %u, end_hbn: %u * ", start_hbn, end_hbn));
	gsri(rep+=strlen(rep));
	
	hbn_len=end_hbn-start_hbn+1;
	if(hbn_len==0)		//I think it is impossible but...
		return 0;
	
	blocks=kzalloc(hbn_len * sizeof(unsigned int), GFP_KERNEL);
	if(BAT_get_some_blocks(gsb, hbn_len, blocks)==-1)
		return -1;
	
	for(i=0;i<hbn_len;i++){
		struct buffer_head *bh;
		
		bh=__bread(in->i_sb->s_bdev, blocks[i], Block_Size);
		memset(bh->b_data, 0, Block_Size);
		
		if(i==0)
			get_bnh_page_hash(zerohash, bh->b_data);
			
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
	}
	
	gsri(sprintf(rep, "zerohash: "));
	gsri(rep+=strlen(rep));
	gsri(printhexstring(zerohash, rep, 12));
	gsri(rep+=strlen(rep));
		
	j=0;
	for(i=start_hbn; i<=end_hbn; i++){
		struct bnh	*bnh,
				*bnh2;
		unsigned int 	l1=l1_bhbof(i),
				l2=l2_bhbof(i),
				l3=l3_bhbof(i);
		
		if( (l2!=pl2 && l2_active_bnh) || (l3!=pl3 && l3_active_bnh) ){
			//previous l1_bh will be changed therefore we should update its parent before that
			get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
			
			gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u ,pl2: %u to: ", l2_active_bnh->blocknumber, pl2));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l2_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		if( (l3!=pl3 && l3_active_bnh) ){
			//previous l2_bh will be changed therefore we should update its parent before that
			get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
			
			gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u ,pl3: %u to: ", l3_active_bnh->blocknumber, pl3));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l3_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		l2_active_bnh=0;
		l3_active_bnh=0;
		
		#ifdef sri_test
			if(strlen(repp)>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
			}
		#endif
		
		gsri(sprintf(rep, "## loop: i: %u, l1: %u, l2:%u, l3:%u * ", i,l1,l2,l3));
		gsri(rep+=strlen(rep));
	
		if(l1<l1_bnhs_len && l2==0 && l3==0){
			//blocks are in the inode for first l1_bnhs_len=8 bnhs
			
			ind->reg_inode_security.l1_bnhs[l1].blocknumber=blocks[j++];
			memcpy(ind->reg_inode_security.l1_bnhs[l1].hash, zerohash, gsfs_bnh_hash_len);
			
			gsri(sprintf(rep, "l1<l1_bnhs_len, allocated_block is: %u * ", blocks[j-1]));
			gsri(rep+=strlen(rep));
			
			pl2=l2;
			pl3=l3;
			
			continue;
		}
		
		if(l2<l2_bnhs_len && l3==0){
			//blocks are in the inode for first l2_bnhs_len=6 bnhs
			
			gsri(sprintf(rep, "l2<l2_bnhs_len * "));
			gsri(rep+=strlen(rep));
						
			if(ind->reg_inode_security.l2_bnhs[l2].blocknumber==0){
				unsigned int bn=0;
				if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
					ret=-1;
					break;
				}
			
				ind->reg_inode_security.l2_bnhs[l2].blocknumber=bn;
				
				if(l1_bh){
					mark_buffer_dirty(l1_bh);
					set_buffer_uptodate(l1_bh);
					brelse(l1_bh);
				}
				
				l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
				memset(l1_bh->b_data, 0, Block_Size);
				l1_bh_num=bn;
				
				gsri(sprintf(rep, "allocated bn to l2_bnhs[l2]: %u * ",bn));
				gsri(rep+=strlen(rep));			
			}
			
			if(l1_bh && l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				mark_buffer_dirty(l1_bh);
				set_buffer_uptodate(l1_bh);
				brelse(l1_bh);
			}
			
			if(!l1_bh || l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				char gh[gsfs_bnh_hash_len];
				
				l1_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l2_bnhs[l2].blocknumber, Block_Size);
				l1_bh_num=ind->reg_inode_security.l2_bnhs[l2].blocknumber;
				
				gsri(sprintf(rep, "openning l2_bnhs[l2] with bn: %u * ",ind->reg_inode_security.l2_bnhs[l2].blocknumber));
				gsri(rep+=strlen(rep));
				
				get_bnh_page_hash(gh, l1_bh->b_data);
				if(strncmp(gh, ind->reg_inode_security.l2_bnhs[l2].hash , gsfs_bnh_hash_len)){
					printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l2:%d\n"
						,in->i_ino, ind->reg_inode_security.l2_bnhs[l2].blocknumber, l2);
					ret=-1;
					
					gsri(sprintf(rep, "badhash * "));
					gsri(rep+=strlen(rep));
					
					break;
				}				
			}
			
			bnh=(struct bnh*)l1_bh->b_data;
			bnh+=l1;
			bnh->blocknumber=blocks[j++];
			memcpy(bnh->hash, zerohash, gsfs_bnh_hash_len);
			
			//get_bnh_page_hash(ind->reg_inode_security.l2_bnhs[l2].hash, l1_bh->b_data);
			l2_active_bnh=&ind->reg_inode_security.l2_bnhs[l2];
			
			gsri(sprintf(rep, "adding blocks %i * ",blocks[j-1]));
			gsri(rep+=strlen(rep));
			
			pl2=l2;
			pl3=l3;
			
			continue;
		}
		
		//we should use l3 to l1 to update bhb
		
		gsri(sprintf(rep, "l3 * "));
		gsri(rep+=strlen(rep));
		
		if(ind->reg_inode_security.l3_bnhs[l3].blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			ind->reg_inode_security.l3_bnhs[l3].blocknumber=bn;
			
			if(l2_bh){
				mark_buffer_dirty(l2_bh);
				set_buffer_uptodate(l2_bh);
				brelse(l2_bh);
			}
			
			l2_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			memset(l2_bh->b_data, 0, Block_Size);
			l2_bh_num=bn;
		}
			
		if(l2_bh && l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			mark_buffer_dirty(l2_bh);
			set_buffer_uptodate(l2_bh);
			brelse(l2_bh);
		}
		
		if(!l1_bh || l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l2_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l3_bnhs[l3].blocknumber, Block_Size);
			l2_bh_num=ind->reg_inode_security.l3_bnhs[l3].blocknumber;
			
			get_bnh_page_hash(gh, l2_bh->b_data);
			if(strncmp(gh, ind->reg_inode_security.l3_bnhs[l3].hash , gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l3:%d\n"
					,in->i_ino, ind->reg_inode_security.l3_bnhs[l3].blocknumber, l3);
				ret=-1;
				break;
			}	
		}
		
		bnh=(struct bnh*)l2_bh->b_data;
		bnh+=l2;
				
		if(bnh->blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			bnh->blocknumber=bn;
			
			if(l1_bh){
				mark_buffer_dirty(l1_bh);
				set_buffer_uptodate(l1_bh);
				brelse(l1_bh);
			}
			
			l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			memset(l1_bh->b_data, 0, Block_Size);
			l1_bh_num=bn;
		}
		
		if(l1_bh && l1_bh_num!=bnh->blocknumber){
			mark_buffer_dirty(l1_bh);
			set_buffer_uptodate(l1_bh);
			brelse(l1_bh);
		}
		
		if(!l1_bh || l1_bh_num!=bnh->blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l1_bh=__bread(in->i_sb->s_bdev, bnh->blocknumber, Block_Size);
			l1_bh_num=bnh->blocknumber;
			
			get_bnh_page_hash(gh, l1_bh->b_data);
			if(strncmp(gh, bnh->hash, gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of not on inode l2:%d\n"
					,in->i_ino, bnh->blocknumber, l2);
				ret=-1;
				break;
			}
		}
		
		bnh2=(struct bnh*)l1_bh->b_data;
		bnh2+=l1;
		
		bnh2->blocknumber=blocks[j++];
		
		memcpy(bnh2->hash, zerohash, gsfs_bnh_hash_len);
				
		//get_bnh_page_hash(bnh->hash, l1_bh->b_data);
		l2_active_bnh=bnh;
		
		//get_bnh_page_hash(ind->reg_inode_security.l3_bnhs[l3].hash, l2_bh->b_data);
		l3_active_bnh=&ind->reg_inode_security.l3_bnhs[l3];
		
		pl2=l2;
		pl3=l3;
	}
	
	if( l2_active_bnh){
		get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
		
		gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u ,pl2: %u to: ", l2_active_bnh->blocknumber, pl2));
		gsri(rep+=strlen(rep));
		gsri(printhexstring(l2_active_bnh->hash, rep, 12));
		gsri(rep+=strlen(rep));
	}
	
	if(l3_active_bnh){
		get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
		
		gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u ,pl3: %u to: ", l3_active_bnh->blocknumber, pl3));
		gsri(rep+=strlen(rep));
		gsri(printhexstring(l3_active_bnh->hash, rep, 12));
		gsri(rep+=strlen(rep));
	}
	
	bhs[0]=l1_bh;
	bhs[1]=l2_bh;
	for(i=0;i<2;i++)
		if(bhs[i]){
			mark_buffer_dirty(bhs[i]);
			set_buffer_uptodate(bhs[i]);
			brelse(bhs[i]);
		}
		
	gsri(sprintf(rep, "ret =  %d * ",ret));
	gsri(rep+=strlen(rep));
	
	#ifdef sri_test
		printk("<0>" "%s\n", repp);
		kfree(repp);
	#endif
	
	return ret;
}
*/

int update_sec_reg_inode_hash_blocks(struct inode* in,unsigned int start_bn,unsigned int len){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	sri_struct			*sri=inf->sri;
	struct GSFS_sb			*gsb=in->i_sb->s_fs_info;
	
	unsigned int 	end_bn,
			start_hbn,
			end_hbn,
			hbn_len,
			*blocks,
			i,
			j;
	
	#define pl1	sri->active_l1
	#define pl2	sri->active_l2
	#define pl3	sri->active_l3
	
	#define l0_bh_num	sri->l0_bh_num
	#define l1_bh_num	sri->l1_bh_num
	#define l2_bh_num	sri->l2_bh_num
	
	#define l0_bh	sri->l0_bh
	#define l1_bh	sri->l1_bh
	#define l2_bh	sri->l2_bh
	
	#define	l3_active_bnh	sri->l3_active_bnh
	#define	l2_active_bnh	sri->l2_active_bnh
	#define	l1_active_bnh	sri->l1_active_bnh
	
	int 	ret=0;
	
	char zerohash[gsfs_bnh_hash_len];
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"update_sec_reg_inode_hash_blocks for in:%ld, start_bn:%u, len:%u * ", in->i_ino, start_bn, len);
	rep+=strlen(rep);
	#endif
	//printk("<0>" "%s\n",repp);
	
	end_bn=start_bn+len-1;
	
	if(end_bn<l0_vias_len)
		return 0;
	
	if(start_bn<l0_vias_len)
		start_bn=l0_vias_len;
	
	start_hbn=start_bn/vias_per_block;
	end_hbn=end_bn/vias_per_block;
	
	if(start_bn!=l0_vias_len && start_bn%vias_per_block!=0){
		if(start_hbn==end_hbn)
			return 0;
		
		start_hbn++;
	}
	
	gsri(sprintf(rep, "start_hbn: %u, end_hbn: %u * ", start_hbn, end_hbn));
	gsri(rep+=strlen(rep));
	
	hbn_len=end_hbn-start_hbn+1;
	if(hbn_len==0)		//I think it is impossible but...
		return 0;
	
	blocks=kzalloc(hbn_len * sizeof(unsigned int), GFP_KERNEL);
	if(BAT_get_some_blocks(gsb, hbn_len, blocks)==-1)
		return -1;
	
	for(i=0;i<hbn_len;i++){
		struct buffer_head *bh;
		
		//printk("<0>" "update_sec_reg_inode_hash_blocks blocks[i]:%u\n",blocks[i]);
		
		bh=__bread(in->i_sb->s_bdev, blocks[i], Block_Size);
		memset(bh->b_data, 0, Block_Size);
		
		if(i==0)
			get_bnh_page_hash(zerohash, bh->b_data);
			
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
	}
	
	gsri(sprintf(rep, "zerohash: "));
	gsri(rep+=strlen(rep));
	gsri(printhexstring(zerohash, rep, 12));
	gsri(rep+=strlen(rep));
	
	//printk("<0>" "update_sec_reg_inode_hash_blocks for in: %lu,  before lock\n", in->i_ino);
	down_write(&sri->sri_via_rwsem);
	
	j=0;
	for(i=start_hbn; i<=end_hbn; i++){
		struct bnh	*bnh,
				*bnh2;
		unsigned int 	l1=l1_bhbof(i),
				l2=l2_bhbof(i),
				l3=l3_bhbof(i);
		
		if( (l2!=pl2  || l3!=pl3 )&& l2_active_bnh ){
			//previous l1_bh will be changed therefore we should update its parent before that
			if(l0_bh){
				if(l0_bh_num && (sri->l0_flags & sri_flag_bh_changed)){
					get_bnh_page_hash(l1_active_bnh->hash, l0_bh->b_data);
					
					if(sri->l1_flags & sri_flag_bnh_on_bh)
						sri->l1_flags |= sri_flag_bh_changed;
					
					mark_buffer_dirty(l0_bh);
					set_buffer_uptodate(l0_bh);
					
					sri->l0_flags &= ~sri_flag_bh_changed;
				}
				brelse(l0_bh);
				
				l0_bh=0;
				l0_bh_num=0;
				pl1=-1;
				l1_active_bnh=0;
				sri->l0_flags=0;	//clear all flags
			}
			
			if(l1_bh_num && (sri->l1_flags & sri_flag_bh_changed)){
				get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
				
				if(sri->l2_flags & sri_flag_bnh_on_bh)
					sri->l2_flags |= sri_flag_bh_changed;
				
				gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u ,pl2: %u to: ", l2_active_bnh->blocknumber, pl2));
				gsri(rep+=strlen(rep));
				gsri(printhexstring(l2_active_bnh->hash, rep, 12));
				gsri(rep+=strlen(rep));
			}
		}
		
		if((l3!=pl3) && l3_active_bnh){
			//previous l2_bh will be changed therefore we should update its parent before that
			if(l2_bh_num && (sri->l2_flags & sri_flag_bh_changed))
				get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
			
			gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u ,pl3: %u to: ", l3_active_bnh->blocknumber, pl3));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l3_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		l2_active_bnh=0;
		l3_active_bnh=0;
		
		#ifdef sri_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
			}
		#endif
		
		gsri(sprintf(rep, "## loop: i: %u, l1: %u, l2:%u, l3:%u * ", i,l1,l2,l3));
		gsri(rep+=strlen(rep));
	
		if(l1<l1_bnhs_len && l2==0 && l3==0){
			//blocks are in the inode for first l1_bnhs_len=6 bnhs
			
			ind->reg_inode_security.l1_bnhs[l1].blocknumber=blocks[j++];
			memcpy(ind->reg_inode_security.l1_bnhs[l1].hash, zerohash, gsfs_bnh_hash_len);
			
			gsri(sprintf(rep, "l1<l1_bnhs_len, allocated_block is: %u * ", blocks[j-1]));
			gsri(rep+=strlen(rep));
			
			pl2=l2;
			pl3=l3;
			
			continue;
		}
		
		if(l2<l2_bnhs_len && l3==0){
			//blocks are in the inode for first l2_bnhs_len=6 bnhs
			
			gsri(sprintf(rep, "l2<l2_bnhs_len * "));
			gsri(rep+=strlen(rep));
						
			if(ind->reg_inode_security.l2_bnhs[l2].blocknumber==0){
				unsigned int bn=0;
				if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
					ret=-1;
					break;
				}
			
				ind->reg_inode_security.l2_bnhs[l2].blocknumber=bn;
				
				if(l1_bh){
					if(sri->l1_flags & sri_flag_bh_changed){
						mark_buffer_dirty(l1_bh);
						set_buffer_uptodate(l1_bh);
					}
					
					brelse(l1_bh);
				}
				
				l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
				
				memset(l1_bh->b_data, 0, Block_Size);
				
				sri->l2_flags |= sri_flag_bh_changed;
				
				l1_bh_num=bn;
				
				gsri(sprintf(rep, "allocated bn to l2_bnhs[l2]: %u * ",bn));
				gsri(rep+=strlen(rep));			
			}
			
			if(l1_bh && l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				if(sri->l1_flags & sri_flag_bh_changed){
					mark_buffer_dirty(l1_bh);
					set_buffer_uptodate(l1_bh);
				}
				
				brelse(l1_bh);
			}
			
			if(!l1_bh || l1_bh_num!=ind->reg_inode_security.l2_bnhs[l2].blocknumber){
				char gh[gsfs_bnh_hash_len];
				
				l1_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l2_bnhs[l2].blocknumber, Block_Size);
				
				sri->l1_flags &=~sri_flag_bh_changed;
				
				gsri(sprintf(rep, "openning l2_bnhs[l2] with bn: %u * ",ind->reg_inode_security.l2_bnhs[l2].blocknumber));
				gsri(rep+=strlen(rep));
				
				get_bnh_page_hash(gh, l1_bh->b_data);
				if(strncmp(gh, ind->reg_inode_security.l2_bnhs[l2].hash , gsfs_bnh_hash_len)){
					printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l2:%d\n"
						,in->i_ino, ind->reg_inode_security.l2_bnhs[l2].blocknumber, l2);
					ret=-1;
					
					gsri(sprintf(rep, "badhash * "));
					gsri(rep+=strlen(rep));
					
					l1_bh_num=0;
					
					break;
				}
				
				l1_bh_num=ind->reg_inode_security.l2_bnhs[l2].blocknumber;
			}
			
			bnh=(struct bnh*)l1_bh->b_data;
			bnh+=l1;
			
			sri->l1_flags |= sri_flag_bnh_on_bh;
			
			bnh->blocknumber=blocks[j++];
			memcpy(bnh->hash, zerohash, gsfs_bnh_hash_len);
			
			sri->l1_flags |= sri_flag_bh_changed;
			
			//get_bnh_page_hash(ind->reg_inode_security.l2_bnhs[l2].hash, l1_bh->b_data);
			l2_active_bnh=&ind->reg_inode_security.l2_bnhs[l2];
			sri->l2_flags &= ~sri_flag_bnh_on_bh;
			
			gsri(sprintf(rep, "adding blocks %i * ",blocks[j-1]));
			gsri(rep+=strlen(rep));
			
			pl2=l2;
			pl3=l3;
			
			continue;
		}
		
		//we should use l3 to l1 to update bhb
		
		gsri(sprintf(rep, "l3 * "));
		gsri(rep+=strlen(rep));
		
		if(ind->reg_inode_security.l3_bnhs[l3].blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			ind->reg_inode_security.l3_bnhs[l3].blocknumber=bn;
			
			if(l2_bh){
				if(sri->l2_flags & sri_flag_bh_changed){
					mark_buffer_dirty(l2_bh);
					set_buffer_uptodate(l2_bh);
				}
				
				brelse(l2_bh);
			}
			
			l2_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			memset(l2_bh->b_data, 0, Block_Size);
			
			sri->l2_flags |= sri_flag_bh_changed;
			
			l2_bh_num=bn;
		}
			
		if(l2_bh && l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			if(sri->l2_flags & sri_flag_bh_changed){
				mark_buffer_dirty(l2_bh);
				set_buffer_uptodate(l2_bh);
			}
			
			brelse(l2_bh);
		}
		
		if(!l2_bh || l2_bh_num!=ind->reg_inode_security.l3_bnhs[l3].blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l2_bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l3_bnhs[l3].blocknumber, Block_Size);
			
			sri->l2_flags &= ~sri_flag_bh_changed;
			
			get_bnh_page_hash(gh, l2_bh->b_data);
			if(strncmp(gh, ind->reg_inode_security.l3_bnhs[l3].hash , gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l3:%d\n"
					,in->i_ino, ind->reg_inode_security.l3_bnhs[l3].blocknumber, l3);
				ret=-1;
				l2_bh_num=0;
				break;
			}
			
			l2_bh_num=ind->reg_inode_security.l3_bnhs[l3].blocknumber;
		}
		
		bnh=(struct bnh*)l2_bh->b_data;
		bnh+=l2;
				
		if(bnh->blocknumber==0){
			unsigned int bn=0;
			if(BAT_get_some_blocks(gsb, 1, &bn)==-1){
				ret=-1;
				break;
			}
		
			bnh->blocknumber=bn;
			
			if(l1_bh){
				if(sri->l1_flags & sri_flag_bh_changed){
					mark_buffer_dirty(l1_bh);
					set_buffer_uptodate(l1_bh);
				}
				
				brelse(l1_bh);
			}
			
			l1_bh=__bread(in->i_sb->s_bdev, bn, Block_Size);
			
			memset(l1_bh->b_data, 0, Block_Size);
			
			sri->l1_flags |= sri_flag_bh_changed;
			
			l1_bh_num=bn;
		}
		
		if(l1_bh && l1_bh_num!=bnh->blocknumber){
			if(sri->l1_flags & sri_flag_bh_changed){
				mark_buffer_dirty(l1_bh);
				set_buffer_uptodate(l1_bh);
			}
			
			brelse(l1_bh);
		}
		
		if(!l1_bh || l1_bh_num!=bnh->blocknumber){
			char gh[gsfs_bnh_hash_len];
			
			l1_bh=__bread(in->i_sb->s_bdev, bnh->blocknumber, Block_Size);
			
			sri->l1_flags &= ~sri_flag_bh_changed;
			
			get_bnh_page_hash(gh, l1_bh->b_data);
			if(strncmp(gh, bnh->hash, gsfs_bnh_hash_len)){
				printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of not on inode l2:%d\n"
					,in->i_ino, bnh->blocknumber, l2);
				ret=-1;
				l1_bh_num=0;
				break;
			}
			
			l1_bh_num=bnh->blocknumber;
		}
		
		bnh2=(struct bnh*)l1_bh->b_data;
		bnh2+=l1;
		
		bnh2->blocknumber=blocks[j++];
		
		memcpy(bnh2->hash, zerohash, gsfs_bnh_hash_len);
		
		sri->l1_flags |= sri_flag_bh_changed;
				
		//get_bnh_page_hash(bnh->hash, l1_bh->b_data);
		l2_active_bnh=bnh;
		sri->l2_flags |= sri_flag_bnh_on_bh;
		
		//get_bnh_page_hash(ind->reg_inode_security.l3_bnhs[l3].hash, l2_bh->b_data);
		l3_active_bnh=&ind->reg_inode_security.l3_bnhs[l3];
		
		pl2=l2;
		pl3=l3;
	}
	
	/*
	sri->l1_bh_num=l1_bh_num;
	sri->l2_bh_num=l2_bh_num;
	
	sri->active_l2=pl2;
	sri->active_l3=pl3;
		
	sri->l2_active_bnh=l2_active_bnh;
	sri->l3_active_bnh=l3_active_bnh;
	
	sri->l1_bh=l1_bh;
	sri->l2_bh=l2_bh;
	*/
	
	up_write(&sri->sri_via_rwsem);
	//there is no need to mark_inode_dirty/_sync because our caller(truncate) will do it 
	
	gsri(sprintf(rep, "ret =  %d * ",ret));
	gsri(rep+=strlen(rep));
	
	#ifdef sri_test
		printk("<0>" "%s\n", repp);
		kfree(repp);
	#endif
	
	return ret;
}

void delete_sec_reg_inode_hash_blocks(struct inode* in){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct GSFS_sb			*gsb=in->i_sb->s_fs_info;
	int 	i;
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"delete_sec_reg_inode_hash_blocks for in:%ld * ", in->i_ino);
	rep+=strlen(rep);
	#endif
	
	for(i=0; i<l1_bnhs_len; i++)
		if(ind->reg_inode_security.l1_bnhs[i].blocknumber){
			gsri(sprintf(rep, "returning l1_bnhs_len[%d]=%u * ",i,ind->reg_inode_security.l1_bnhs[i].blocknumber));
			gsri(rep+=strlen(rep));
			
			BAT_clear_one_block(gsb, ind->reg_inode_security.l1_bnhs[i].blocknumber);
			ind->reg_inode_security.l1_bnhs[i].blocknumber=0;
		}
	
	for(i=0; i<l2_bnhs_len; i++){
		struct buffer_head *bh;
		char gh[gsfs_bnh_hash_len];
		struct bnh *bnh;
		int j;
		
		#ifdef sri_test
			if(strlen(repp)>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
				repp[0]=0;
			}
		#endif
			
		if(ind->reg_inode_security.l2_bnhs[i].blocknumber==0)
			continue;
		
		bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l2_bnhs[i].blocknumber, Block_Size);
		get_bnh_page_hash(gh, bh->b_data);
		
		gsri(sprintf(rep, "opening l2_bnhs_len[%d]=%u * ",i,ind->reg_inode_security.l2_bnhs[i].blocknumber));
		gsri(rep+=strlen(rep));
		
		if(strncmp(gh, ind->reg_inode_security.l2_bnhs[i].hash, gsfs_bnh_hash_len)){
			printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l2\n"
					,in->i_ino, ind->reg_inode_security.l2_bnhs[i].blocknumber);
			
			gsri(sprintf(rep, "badhash * "));
			gsri(rep+=strlen(rep));
			
			brelse(bh);
			
			continue;
		}
				
		
		bnh=(struct bnh*)bh->b_data;
		for(j=0; j<Block_Size/sizeof(struct bnh); j++){
			//printk("<0>" "%d %d\n",bnh[j].blocknumber,ind->reg_inode_security.l2_bnhs[i].blocknumber);
			if(bnh[j].blocknumber){
				BAT_clear_one_block(gsb, bnh[j].blocknumber);
				
				gsri(sprintf(rep, "# returning blocknumber[%d]=%u ",j,bnh[j].blocknumber));
				gsri(rep+=strlen(rep));
			}
		}
				
		memset(bh->b_data, 0, Block_Size);
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		BAT_clear_one_block(gsb,ind->reg_inode_security.l2_bnhs[i].blocknumber);
		ind->reg_inode_security.l2_bnhs[i].blocknumber=0;
	}
	
	for(i=0; i<l3_bnhs_len; i++){
		struct buffer_head *bh;
		char gh[gsfs_bnh_hash_len];
		struct bnh *bnh;
		int j;
		
		if(ind->reg_inode_security.l3_bnhs[i].blocknumber==0)
			continue;
		
		gsri(sprintf(rep, "opening l3_bnhs_len[%d]=%u * ",i,ind->reg_inode_security.l3_bnhs[i].blocknumber));
		gsri(rep+=strlen(rep));
				
		bh=__bread(in->i_sb->s_bdev, ind->reg_inode_security.l3_bnhs[i].blocknumber, Block_Size);
		
		get_bnh_page_hash(gh, bh->b_data);
		if(strncmp(gh, ind->reg_inode_security.l3_bnhs[i].hash, gsfs_bnh_hash_len)){
			printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of on inode l3\n"
					,in->i_ino, ind->reg_inode_security.l3_bnhs[i].blocknumber);
			brelse(bh);
			continue;
		}
		
		bnh=(struct bnh*)bh->b_data;
		for(j=0; j<Block_Size/sizeof(struct bnh); j++)
			if(bnh[j].blocknumber){
				struct buffer_head *bh2;
				char gh2[gsfs_bnh_hash_len];
				struct bnh *bnh2;
				int k;
				
				#ifdef sri_test
					if(strlen(repp)>800){
						printk("<0>" "%s\n",repp);
						rep=repp;
						repp[0]=0;
					}
				#endif
				
				bh2=__bread(in->i_sb->s_bdev, bnh[j].blocknumber, Block_Size);
				
				get_bnh_page_hash(gh2, bh2->b_data);
				if(strncmp(gh2, bnh[j].hash, gsfs_bnh_hash_len)){
					printk("<0>" "Inode with ino:%ld is not integrated for its hash block number:%u of not on inode l2\n"
							,in->i_ino, bnh[j].blocknumber);
					brelse(bh2);
					continue;
				}
				
				bnh2=(struct bnh*)bh2->b_data;
				for(k=0; k<Block_Size/sizeof(struct bnh); k++)
					if(bnh2[k].blocknumber)
						BAT_clear_one_block(gsb, bnh2[k].blocknumber);
				
				
				memset(bh2->b_data, 0, Block_Size);
				
				mark_buffer_dirty(bh2);
				set_buffer_uptodate(bh2);
				brelse(bh2);
				
				BAT_clear_one_block(gsb, bnh[j].blocknumber);
			}
		
		memset(bh->b_data, 0, Block_Size);
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		BAT_clear_one_block(gsb,ind->reg_inode_security.l3_bnhs[i].blocknumber);
		ind->reg_inode_security.l3_bnhs[i].blocknumber=0;
	}
	
	#ifdef sri_test
		printk("<0>" "%s\n",repp);
		kfree(repp);
	#endif
	
	return;
}

#define set_via (1)
#define get_via (0)

/*
int set_get_blocks_via_first_edition(struct inode *in, unsigned int *blocks, struct ver_IV_AT **vias,
		       int *res, int len, int type){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct super_block		*sb=in->i_sb;
	
	unsigned int 	i,
			l0,
			l1,
			l2,
			l3,
			l0_bh_num=0,
			l1_bh_num=0,
			l2_bh_num=0;
	
	struct buffer_head	*l0_bh=0,
				*l1_bh=0,
				*l2_bh=0,
				*bhs[3];
	
	struct bnh	*l3_active_bnh,
			*l2_active_bnh,
			*l1_active_bnh;
			
	struct ver_IV_AT	*l0_active_via;
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"set_get_block_vias for in: %lu, set: %d", in->i_ino, type);
	rep+=strlen(rep);
	#endif
	
	
	if(unlikely(blocks==0) || unlikely(vias==0) || unlikely(res==0))
		return -1;
				
	for(i=0; i<len; i++){
		#ifdef sri_test
			if(strlen(repp)>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		gsri(sprintf(rep,"# i:%d, blocks[i]:%u *",i,blocks[i]));
		gsri(rep+=strlen(rep));
		
		if(vias[i]==0 || blocks[i]>=in->i_blocks){
			gsri(sprintf(rep,"bad vias or blocks *"));
			gsri(rep+=strlen(rep));
			
			res[i]=-1;
			continue;
		}
		
		l0=l0_offset(blocks[i]);
		l1=l1_offset(blocks[i]);
		l2=l2_offset(blocks[i]);
		l3=l3_offset(blocks[i]);
		
		l3_active_bnh=0;
		l2_active_bnh=0;
		l1_active_bnh=0;
		l0_active_via=0;
		
		gsri(sprintf(rep,"l0: %d, l1: %d, l2: %d, l3: %d * ",l0,l1,l2,l3));
		gsri(rep+=strlen(rep));
			
		
		if(l0<l0_vias_len && l1==0 && l2==0 && l3==0){
			gsri(sprintf(rep,"l0<l0_vias_len *"));
			gsri(rep+=strlen(rep));
			
			if(type==get_via)
				memcpy(vias[i], &ind->reg_inode_security.l0_vias[l0], via_len);
			else
				memcpy(&ind->reg_inode_security.l0_vias[l0], vias[i], via_len);
			
			res[i]=0;
			
			continue;
		}
		
		if(l3>0 || l2>=l2_bnhs_len)
			l3_active_bnh=&ind->reg_inode_security.l3_bnhs[l3];
		
		if(l3_active_bnh){
			gsri(sprintf(rep,"l3_active_bnh with blocknumber: %u *",l3_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l2_bh_num != l3_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l2_bh){
					if(type == set_via){
						mark_buffer_dirty(l2_bh);
						set_buffer_uptodate(l2_bh);
					}
					brelse(l2_bh);
				}
				
				l2_bh=__bread(sb->s_bdev, l3_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l2_bh->b_data);
				if(strncmp(gh, l3_active_bnh->hash, gsfs_bnh_hash_len)){
					l2_bh_num=0;
					res[i]=-2;
					
					gsri(sprintf(rep, "badhash for l3 blocknumber:%u* ", l3_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
										
					continue;
				}
				
				l2_bh_num=l3_active_bnh->blocknumber;
			}
			
			l2_active_bnh=(struct bnh*)l2_bh->b_data;
			l2_active_bnh+=l2;
		}
		else{
			if(l3==0 && l2<l2_bnhs_len && l1>=l1_bnhs_len)
				l2_active_bnh=&ind->reg_inode_security.l2_bnhs[l2];
		}
		
		if(l2_active_bnh){
			gsri(sprintf(rep,"l2_active_bnh with blocknumber: %u *",l2_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l1_bh_num != l2_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l1_bh){
					if(type == set_via){
						mark_buffer_dirty(l1_bh);
						set_buffer_uptodate(l1_bh);
					}
					brelse(l1_bh);
				}
				
				l1_bh=__bread(sb->s_bdev, l2_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l1_bh->b_data);
				if(strncmp(gh, l2_active_bnh->hash, gsfs_bnh_hash_len)){
					res[i]=-2;
					l1_bh_num=0;
					
					gsri(sprintf(rep, "badhash for l2 blocknumber:%u* ", l2_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
										
					continue;
				}
				
				l1_bh_num=l2_active_bnh->blocknumber;
			}
			
			l1_active_bnh=(struct bnh*)l1_bh->b_data;
			l1_active_bnh+=l1;
		}
		else{
			if(l3==0 && l2==0 && l1<l1_bnhs_len && l0>=l0_vias_len)
				l1_active_bnh=&ind->reg_inode_security.l1_bnhs[l1];
		}
		
		if(l1_active_bnh){
			gsri(sprintf(rep,"l1_active_bnh with blocknumber: %u *",l1_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l0_bh_num != l1_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l0_bh){
					if(type == set_via){
						mark_buffer_dirty(l0_bh);
						set_buffer_uptodate(l0_bh);
					}
					brelse(l0_bh);
				}
				
				l0_bh=__bread(sb->s_bdev, l1_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l0_bh->b_data);
				if(strncmp(gh, l1_active_bnh->hash, gsfs_bnh_hash_len)){
					res[i]=-2;
					l0_bh_num=0;
					
					gsri(sprintf(rep, "badhash for l1 blocknumber:%u* ", l1_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					continue;
				}
				
				l0_bh_num=l1_active_bnh->blocknumber;
			}
			
			l0_active_via=(struct ver_IV_AT*)l0_bh->b_data;
			l0_active_via+=l0;
		}
		
		if(!l0_active_via){
			printk("<0>" "Error: no l0_active_via\n");
			
			gsri(sprintf(rep,"no l0_active_via *"));
			gsri(rep+=strlen(rep));
			
			continue;
		}
				
		if(type==get_via){
			//get_via
			memcpy(vias[i], l0_active_via, via_len);
			res[i]=0;
		}
		else{
			//set_via
			memcpy(l0_active_via, vias[i], via_len);
			res[i]=0;
			
			if(l1_active_bnh){
				get_bnh_page_hash(l1_active_bnh->hash, l0_bh->b_data);
				if(l2_active_bnh){
					get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
					if(l3_active_bnh)
						get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
				}
			}
		}
	}
	
	bhs[0]=l0_bh;
	bhs[1]=l1_bh;
	bhs[2]=l2_bh;
	
	for(i=0;i<3;i++)
		if(bhs[i]){
			if(type == set_via){
				mark_buffer_dirty(bhs[i]);
				set_buffer_uptodate(bhs[i]);
			}
			brelse(bhs[i]);
		}
		
	gsri(printk("<0>" "%s\n",repp));
	gsri(kfree(repp));
	
	return 0;
}
*/

/*
//some things should be updated int this second edition 
//be careful that in this edition it is better that blocks to be sorted ascendently
int set_get_blocks_via_second_edition(struct inode *in, unsigned int *blocks, struct ver_IV_AT **vias,
		       int *res, int len, int type){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct super_block		*sb=in->i_sb;
	
	int 		i,
			l0,
			l1,
			l2,
			l3,
			pl0=-1,
			pl1=-1,
			pl2=-1,
			pl3=-1;
	unsigned int	l0_bh_num=0,
			l1_bh_num=0,
			l2_bh_num=0;
	
	struct buffer_head	*l0_bh=0,
				*l1_bh=0,
				*l2_bh=0,
				*bhs[3];
	
	struct bnh	*l3_active_bnh=0,
			*l2_active_bnh=0,
			*l1_active_bnh=0;
			
	struct ver_IV_AT	*l0_active_via;
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"set_get_block_vias for in: %lu, set: %d", in->i_ino, type);
	rep+=strlen(rep);
	#endif
	
	if(unlikely(blocks==0) || unlikely(vias==0) || unlikely(res==0))
		return -1;
				
	for(i=0; i<len; i++){
		#ifdef sri_test
			if(strlen(repp)>800){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		gsri(sprintf(rep,"# i:%d, blocks[i]:%u *",i,blocks[i]));
		gsri(rep+=strlen(rep));
		
		if(vias[i]==0 || blocks[i]>=in->i_blocks){
			gsri(sprintf(rep,"bad vias or blocks *"));
			gsri(rep+=strlen(rep));
			
			res[i]=-1;
			continue;
		}
		
		l0=l0_offset(blocks[i]);
		l1=l1_offset(blocks[i]);
		l2=l2_offset(blocks[i]);
		l3=l3_offset(blocks[i]);
		
		if(l0<l0_vias_len && l1==0 && l2==0 && l3==0){
			gsri(sprintf(rep,"l0<l0_vias_len *"));
			gsri(rep+=strlen(rep));
			
			if(type==get_via)
				memcpy(vias[i], &ind->reg_inode_security.l0_vias[l0], via_len);
			else
				memcpy(&ind->reg_inode_security.l0_vias[l0], vias[i], via_len);
			
			res[i]=0;
			
			continue;
		}
		
		if(type == set_via){
			if(	(l1!=pl1 && l1_active_bnh) || (l2!=pl2 && l2_active_bnh) || 
				(l3!=pl3 && l3_active_bnh) ){
				//previous l0_bh will be changed therefore we should update its parent before that
				get_bnh_page_hash(l1_active_bnh->hash, l0_bh->b_data);
				
				gsri(sprintf(rep, "updating l1_active_bnh hash for block number: %u ,pl1: %u to: ", l1_active_bnh->blocknumber, pl1));
				gsri(rep+=strlen(rep));
				gsri(printhexstring(l1_active_bnh->hash, rep, 12));
				gsri(rep+=strlen(rep));
			
			}
			
			if( (l2!=pl2 && l2_active_bnh) || (l3!=pl3 && l3_active_bnh) ){
				//previous l1_bh will be changed therefore we should update its parent before that
				get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
				
				gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u ,pl2: %u to: ", l2_active_bnh->blocknumber, pl2));
				gsri(rep+=strlen(rep));
				gsri(printhexstring(l2_active_bnh->hash, rep, 12));
				gsri(rep+=strlen(rep));
			}
			
			if( (l3!=pl3 && l3_active_bnh) ){
				//previous l2_bh will be changed therefore we should update its parent before that
				get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
				
				gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u ,pl3: %u to: ", l3_active_bnh->blocknumber, pl3));
				gsri(rep+=strlen(rep));
				gsri(printhexstring(l3_active_bnh->hash, rep, 12));
				gsri(rep+=strlen(rep));
			}
		}
		
		l3_active_bnh=0;
		l2_active_bnh=0;
		l1_active_bnh=0;
		l0_active_via=0;
		
		gsri(sprintf(rep,"l0: %d, l1: %d, l2: %d, l3: %d * ",l0,l1,l2,l3));
		gsri(rep+=strlen(rep));
			
		if(l3>0 || l2>=l2_bnhs_len)
			l3_active_bnh=&ind->reg_inode_security.l3_bnhs[l3];
		
		if(l3_active_bnh){
			gsri(sprintf(rep,"l3_active_bnh with blocknumber: %u *",l3_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l2_bh_num != l3_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l2_bh){
					if(type == set_via){
						mark_buffer_dirty(l2_bh);
						set_buffer_uptodate(l2_bh);
					}
					brelse(l2_bh);
				}
				
				l2_bh=__bread(sb->s_bdev, l3_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l2_bh->b_data);
				if(strncmp(gh, l3_active_bnh->hash, gsfs_bnh_hash_len)){
					l2_bh_num=0;
					res[i]=-2;
					
					gsri(sprintf(rep, "badhash for l3 blocknumber:%u* ", l3_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					continue;
				}
				
				l2_bh_num=l3_active_bnh->blocknumber;
			}
			
			l2_active_bnh=(struct bnh*)l2_bh->b_data;
			l2_active_bnh+=l2;
		}
		else{
			if(l3==0 && l2<l2_bnhs_len && l1>=l1_bnhs_len)
				l2_active_bnh=&ind->reg_inode_security.l2_bnhs[l2];
		}
		
		if(l2_active_bnh){
			gsri(sprintf(rep,"l2_active_bnh with blocknumber: %u *",l2_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l1_bh_num != l2_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l1_bh){
					if(type == set_via){
						mark_buffer_dirty(l1_bh);
						set_buffer_uptodate(l1_bh);
					}
					brelse(l1_bh);
				}
				
				l1_bh=__bread(sb->s_bdev, l2_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l1_bh->b_data);
				if(strncmp(gh, l2_active_bnh->hash, gsfs_bnh_hash_len)){
					res[i]=-2;
					l1_bh_num=0;
					
					gsri(sprintf(rep, "badhash for l2 blocknumber:%u* ", l2_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					continue;
				}
				
				l1_bh_num=l2_active_bnh->blocknumber;
			}
			
			l1_active_bnh=(struct bnh*)l1_bh->b_data;
			l1_active_bnh+=l1;
		}
		else{
			if(l3==0 && l2==0 && l1<l1_bnhs_len && l0>=l0_vias_len)
				l1_active_bnh=&ind->reg_inode_security.l1_bnhs[l1];
		}
		
		if(l1_active_bnh){
			gsri(sprintf(rep,"l1_active_bnh with blocknumber: %u *",l1_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l0_bh_num != l1_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l0_bh){
					if(type == set_via){
						mark_buffer_dirty(l0_bh);
						set_buffer_uptodate(l0_bh);
					}
					brelse(l0_bh);
				}
				
				l0_bh=__bread(sb->s_bdev, l1_active_bnh->blocknumber, Block_Size);
				
				get_bnh_page_hash(gh, l0_bh->b_data);
				if(strncmp(gh, l1_active_bnh->hash, gsfs_bnh_hash_len)){
					res[i]=-2;
					l0_bh_num=0;
					
					gsri(sprintf(rep, "badhash for l1 blocknumber:%u* ", l1_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					continue;
				}
				
				l0_bh_num=l1_active_bnh->blocknumber;
			}
			
			l0_active_via=(struct ver_IV_AT*)l0_bh->b_data;
			l0_active_via+=l0;
		}
		
		if(!l0_active_via){
			printk("<0>" "Error: no l0_active_via\n");
			
			gsri(sprintf(rep,"no l0_active_via *"));
			gsri(rep+=strlen(rep));
			
			continue;
		}
				
		if(type==get_via){
			//get_via
			memcpy(vias[i], l0_active_via, via_len);
			//res[i]=0;
		}
		else{
			//set_via
			memcpy(l0_active_via, vias[i], via_len);
			//res[i]=0;
			
		}
		res[i]=0;
		
		pl0=l0;
		pl1=l1;
		pl2=l2;
		pl3=l3;
	}
	
	if(type == set_via){
		if(l1_active_bnh){
			get_bnh_page_hash(l1_active_bnh->hash, l0_bh->b_data);
			
			gsri(sprintf(rep, "updating l1_active_bnh hash for block number: %u ,pl1: %u to: ", l1_active_bnh->blocknumber, pl1));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l1_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		if(l2_active_bnh){
			get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
			
			gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u ,pl2: %u to: ", l2_active_bnh->blocknumber, pl2));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l2_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		if(l3_active_bnh){
			get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
			
			gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u ,pl3: %u to: ", l3_active_bnh->blocknumber, pl3));
			gsri(rep+=strlen(rep));
			gsri(printhexstring(l3_active_bnh->hash, rep, 12));
			gsri(rep+=strlen(rep));
		}
		
		mark_inode_dirty(in);
		inf->igflags|=igflag_inode_metadata_changed;
	}
	
	bhs[0]=l0_bh;
	bhs[1]=l1_bh;
	bhs[2]=l2_bh;
	
	for(i=0;i<3;i++)
		if(bhs[i]){
			if(type == set_via){
				mark_buffer_dirty(bhs[i]);
				set_buffer_uptodate(bhs[i]);
			}
			brelse(bhs[i]);
		}
		
	gsri(printk("<0>" "%s\n",repp));
	gsri(kfree(repp));
	
	return 0;
}
*/

int set_get_blocks_via(struct inode *in, unsigned int *blocks, struct ver_IV_AT **vias,
		       int *res, int len, int type){
	struct GSFS_inode 		*inf=(struct GSFS_inode*)in->i_private;
	sri_struct			*sri=inf->sri;
	struct GSFS_inode_disk_inf 	*ind=&(inf->disk_info);
	struct super_block		*sb=in->i_sb;
	
	int 		i,
			l0,
			l1,
			l2,
			l3,
			dirty_inode=0;
			
			
	#define pl1	sri->active_l1
	#define pl2	sri->active_l2
	#define pl3	sri->active_l3
	
	#define l0_bh_num	sri->l0_bh_num
	#define l1_bh_num	sri->l1_bh_num
	#define l2_bh_num	sri->l2_bh_num
	
	#define l0_bh	sri->l0_bh
	#define l1_bh	sri->l1_bh
	#define l2_bh	sri->l2_bh
	
	#define	l3_active_bnh	sri->l3_active_bnh
	#define	l2_active_bnh	sri->l2_active_bnh
	#define	l1_active_bnh	sri->l1_active_bnh
			
	struct ver_IV_AT	*l0_active_via;
	
	#ifdef sri_test
	char 	*rep,
		*repp;
	rep=kzalloc(1000,GFP_KERNEL);
	repp=rep;
	
	sprintf(rep,"set_get_block_vias for in: %lu, set: %d", in->i_ino, type);
	rep+=strlen(rep);
	#endif
	
	if(unlikely(blocks==0) || unlikely(vias==0) || unlikely(res==0))
		return -1;
	
	down_write(&sri->sri_via_rwsem);
				
	for(i=0; i<len; i++){
		#ifdef sri_test
			if(rep-repp>500){
				printk("<0>" "%s\n",repp);
				rep=repp;
				rep[0]=0;
			}
		#endif
		
		gsri(sprintf(rep,"# i:%d, blocks[i]:%u *",i,blocks[i]));
		gsri(rep+=strlen(rep));
		
		if(vias[i]==0 || blocks[i]>=in->i_blocks){
			gsri(sprintf(rep,"bad vias or blocks *"));
			gsri(rep+=strlen(rep));
			
			res[i]=-1;
			continue;
		}
		
		l0=l0_offset(blocks[i]);
		l1=l1_offset(blocks[i]);
		l2=l2_offset(blocks[i]);
		l3=l3_offset(blocks[i]);
		
		if(l0<l0_vias_len && l1==0 && l2==0 && l3==0){
			gsri(sprintf(rep,"l0<l0_vias_len *"));
			gsri(rep+=strlen(rep));
			
			if(type==get_via)
				memcpy(vias[i], &ind->reg_inode_security.l0_vias[l0], via_len);
			else{
				memcpy(&ind->reg_inode_security.l0_vias[l0], vias[i], via_len);
				dirty_inode=1;
			}
			
			res[i]=0;
			
			continue;
		}
		
		//if(type == set_via){
			if( (l1!=pl1 || l2!=pl2 || l3!=pl3) && l1_active_bnh ){
				//previous l0_bh will be changed therefore we should update its parent before that
				if(l0_bh_num && (sri->l0_flags & sri_flag_bh_changed)){
					get_bnh_page_hash(l1_active_bnh->hash, l0_bh->b_data);
					
					if(sri->l1_flags & sri_flag_bnh_on_bh)
						sri->l1_flags |= sri_flag_bh_changed;
					
					dirty_inode=1;
				
					gsri(sprintf(rep, "updating l1_active_bnh hash for block number: %u, l0_bh_num: %u, pl1: %u to: ", l1_active_bnh->blocknumber, l0_bh_num, pl1));
					gsri(rep+=strlen(rep));
					gsri(printhexstring(l1_active_bnh->hash, rep, 12));
					gsri(rep+=strlen(rep));
				}
			}
			
			if( (l2!=pl2 || l3!=pl3) && l2_active_bnh ){
				//previous l1_bh will be changed therefore we should update its parent before that
				if(l1_bh_num && (sri->l1_flags & sri_flag_bh_changed)){
					get_bnh_page_hash(l2_active_bnh->hash, l1_bh->b_data);
					
					if(sri->l2_flags & sri_flag_bnh_on_bh)
						sri->l2_flags |= sri_flag_bh_changed;
				
					dirty_inode=1;
					
					gsri(sprintf(rep, "updating l2_active_bnh hash for block number: %u, l1_bh_num: %u, pl2: %u to: ", l2_active_bnh->blocknumber, l1_bh_num, pl2));
					gsri(rep+=strlen(rep));
					gsri(printhexstring(l2_active_bnh->hash, rep, 12));
					gsri(rep+=strlen(rep));
				}
			}
			
			if( (l3!=pl3 && l3_active_bnh) ){
				//previous l2_bh will be changed therefore we should update its parent before that
				if(l2_bh_num && (sri->l2_flags & sri_flag_bh_changed)){
					get_bnh_page_hash(l3_active_bnh->hash, l2_bh->b_data);
					
					//l3_active_bnh is always on inode 
					
					dirty_inode=1;
				
					gsri(sprintf(rep, "updating l3_active_bnh hash for block number: %u, l2_bh_num: %u, pl3: %u to: ", l3_active_bnh->blocknumber, l2_bh_num, pl3));
					gsri(rep+=strlen(rep));
					gsri(printhexstring(l3_active_bnh->hash, rep, 12));
					gsri(rep+=strlen(rep));
				}
			}
		//}
		
		l3_active_bnh=0;
		l2_active_bnh=0;
		l1_active_bnh=0;
		l0_active_via=0;
		
		gsri(sprintf(rep,"l0: %d, l1: %d, l2: %d, l3: %d * ",l0,l1,l2,l3));
		gsri(rep+=strlen(rep));
		
		//printk("<0>" "%s\n", repp);rep=repp;rep[0]=0;
		if(l3>0 || l2>=l2_bnhs_len)
			l3_active_bnh=&ind->reg_inode_security.l3_bnhs[l3];
		
		if(l3_active_bnh){
			gsri(sprintf(rep,"l3_active_bnh with blocknumber: %u *",l3_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l2_bh_num != l3_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				
				if(l2_bh){
					if(sri->l2_flags & sri_flag_bh_changed){
						mark_buffer_dirty(l2_bh);
						set_buffer_uptodate(l2_bh);
					}
					brelse(l2_bh);
				}
				
				if(likely(l3_active_bnh->blocknumber)){
					l2_bh=__bread(sb->s_bdev, l3_active_bnh->blocknumber, Block_Size);
					
					sri->l2_flags&=~sri_flag_bh_changed;
					
					get_bnh_page_hash(gh, l2_bh->b_data);
					if(strncmp(gh, l3_active_bnh->hash, gsfs_bnh_hash_len)){
						l2_bh_num=0;
						res[i]=-2;
						
						gsri(sprintf(rep, "badhash for l3 blocknumber:%u* ", l3_active_bnh->blocknumber));
						gsri(rep+=strlen(rep));
						
						continue;
					}
					
					l2_bh_num=l3_active_bnh->blocknumber;
				}
				else{
					gsri(sprintf(rep, "bad l3_active_bnh->blocknumber: %u* ", l3_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					res[i]=-1;
					
					continue;
				}
			}
			
			l2_active_bnh=(struct bnh*)l2_bh->b_data;
			l2_active_bnh+=l2;
			
			sri->l2_flags|=sri_flag_bnh_on_bh;
		}
		else{
			if( l3==0 && ( (l2<l2_bnhs_len && l2>0) || (l2==0 && l1>=l1_bnhs_len) ) ){
				
				l2_active_bnh=&ind->reg_inode_security.l2_bnhs[l2];
				
				sri->l2_flags&=~sri_flag_bnh_on_bh;
				
				gsri(sprintf(rep," * l2<l2_bnhs_len, block: %u",ind->reg_inode_security.l2_bnhs[l2].blocknumber);)
				gsri(rep+=strlen(rep);)
			}
		}
		
		if(l2_active_bnh){
			gsri(sprintf(rep,"l2_active_bnh with blocknumber: %u *",l2_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
		
			if(l1_bh_num != l2_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				if(l1_bh){
					if(sri->l1_flags & sri_flag_bh_changed){
						mark_buffer_dirty(l1_bh);
						set_buffer_uptodate(l1_bh);
					}
					brelse(l1_bh);
				}
				
				if(likely(l2_active_bnh->blocknumber)){
					l1_bh=__bread(sb->s_bdev, l2_active_bnh->blocknumber, Block_Size);
					
					sri->l1_flags&=~sri_flag_bh_changed;
					
					get_bnh_page_hash(gh, l1_bh->b_data);
					if(strncmp(gh, l2_active_bnh->hash, gsfs_bnh_hash_len)){
						res[i]=-2;
						l1_bh_num=0;
						
						gsri(sprintf(rep, "badhash for l2 blocknumber:%u* ", l2_active_bnh->blocknumber));
						gsri(rep+=strlen(rep));
						
						continue;
					}
					
					l1_bh_num=l2_active_bnh->blocknumber;
				}
				else{
					gsri(sprintf(rep, "bad l2_active_bnh->blocknumber: %u* ", l2_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					res[i]=-1;
					
					continue;
				}
			}
			
			l1_active_bnh=(struct bnh*)l1_bh->b_data;
			l1_active_bnh+=l1;
			
			sri->l1_flags|=sri_flag_bnh_on_bh;
		}
		else{
			if( l3==0 && l2==0 && ( (l1<l1_bnhs_len && l1>0) || (l1==0 && l0>=l0_vias_len) ) ){
				l1_active_bnh=&ind->reg_inode_security.l1_bnhs[l1];
				
				sri->l1_flags&=~sri_flag_bnh_on_bh;
				
				gsri(sprintf(rep," * l1<l1_bnhs_len, block: %u",ind->reg_inode_security.l1_bnhs[l1].blocknumber);)
				gsri(rep+=strlen(rep);)
			}
		}
		
		if(l1_active_bnh){
			gsri(sprintf(rep,"l1_active_bnh with blocknumber: %u *",l1_active_bnh->blocknumber));
			gsri(rep+=strlen(rep));
									
			if(l0_bh_num != l1_active_bnh->blocknumber){
				char gh[gsfs_bnh_hash_len];
				
				if(l0_bh){
					if(sri->l0_flags & sri_flag_bh_changed){
						mark_buffer_dirty(l0_bh);
						set_buffer_uptodate(l0_bh);
					}
					brelse(l0_bh);
				}
				
				if(likely(l1_active_bnh->blocknumber)){
					l0_bh=__bread(sb->s_bdev, l1_active_bnh->blocknumber, Block_Size);
					
					sri->l0_flags &= ~sri_flag_bh_changed;
					
					get_bnh_page_hash(gh, l0_bh->b_data);
					if(strncmp(gh, l1_active_bnh->hash, gsfs_bnh_hash_len)){
						res[i]=-2;
						l0_bh_num=0;
						
						gsri(sprintf(rep, "badhash for l1 blocknumber:%u* ", l1_active_bnh->blocknumber));
						gsri(rep+=strlen(rep));
						
						continue;
					}
					
					l0_bh_num=l1_active_bnh->blocknumber;
				}
				else{
					gsri(sprintf(rep, "bad l1_active_bnh->blocknumber: %u* ", l1_active_bnh->blocknumber));
					gsri(rep+=strlen(rep));
					
					res[i]=-1;
					
					continue;
				}
			}
			
			l0_active_via=(struct ver_IV_AT*)l0_bh->b_data;
			l0_active_via+=l0;
			
			//l0_via is always on bnh
		}
				
		if(!l0_active_via){
			printk("<0>" "Error: no l0_active_via\n");
			
			gsri(sprintf(rep,"no l0_active_via *"));
			gsri(rep+=strlen(rep));
			
			res[i]=-1;
			
			continue;
		}
				
		if(type==get_via)
			memcpy(vias[i], l0_active_via, via_len);
		else{	//set_via
			memcpy(l0_active_via, vias[i], via_len);
			
			sri->l0_flags |= sri_flag_bh_changed;
			dirty_inode=1;
		} 
		
		res[i]=0;
		
		pl1=l1;
		pl2=l2;
		pl3=l3;
	}
	
	/*
	sri->active_l1=pl1;
	sri->active_l2=pl2;
	sri->active_l3=pl3;
	
	sri->l0_bh_num=l0_bh_num;
	sri->l1_bh_num=l1_bh_num;
	sri->l2_bh_num=l2_bh_num;

	sri->l0_bh=l0_bh;
	sri->l1_bh=l1_bh;
	sri->l2_bh=l2_bh;
	
	sri->l1_active_bnh=l1_active_bnh;
	sri->l2_active_bnh=l2_active_bnh;
	sri->l3_active_bnh=l3_active_bnh;
	*/
	
	up_write(&sri->sri_via_rwsem);
	
	if(dirty_inode){
		mark_inode_dirty_sync(in);
		inf->igflags|=igflag_inode_metadata_changed;
	}
	
	gsri(printk("<0>" "%s\n",repp));
	gsri(kfree(repp));
	
	return 0;
}

inline int get_blocks_via(struct inode *in, unsigned int *blocks, struct ver_IV_AT **vias,
			  int *res, int len){
	return set_get_blocks_via(in, blocks, vias, res, len, get_via);
}

inline int set_blocks_via(struct inode *in, unsigned int *blocks, struct ver_IV_AT **vias,
			  int *res, int len){
	return set_get_blocks_via(in, blocks, vias, res, len, set_via);
}
