#include "gsfs.h"

#ifdef gsfs_test
	#define test_indecis_num 30
	char test_indecis[test_indecis_num][2];
	
	void clear_test_indecis(void){
		int i;
		int j;
		
		for (i=0;i<test_indecis_num;i++)
			for(j=0;j<2;j++)
				test_indecis[i][j]=0;
		
		return;
	}
#endif

int set_and_get_one_index(unsigned char *array, unsigned int max){
	unsigned int	len,
			index,
			i,
			j;
	int ret;

	len=(max>>3);
	if(max&7)
		len++;
	ret=-1;
	index=0;
	for(i=0;i<len && ret==-1 ;i++,index+=8){
		if(array[i]==0xff)
			continue;
		for(j=0;j<8;j++)
			if(!(array[i] & (1<<j))){			
				ret=(index+j);
				array[i]|=(1<<j);
				goto func_ret;
			}		
	}
	
func_ret:
	if(ret>=max)
		ret=-1;
	return ret;
}

inline int bit_set_reset(unsigned char* array, unsigned int index, unsigned int max, unsigned char set){
	unsigned int 	ar_ind,
			ar_off,
			ret=0;
	
	if(!array || index>=max || set>1)
		return -1;
	
	ar_ind= (index>>3);
	ar_off= (index&7);
	
	ret=(array[ar_ind]&(1<<ar_off));
	
	if(set)
		array[ar_ind]|=(1<<ar_off);
	else
		array[ar_ind]&=(~(1<<ar_off));
	
	return ret;
}

//returns previous value
inline int set_one_index(unsigned char* array, unsigned int index, unsigned int max){
	
	return bit_set_reset(array, index, max, 1);
}

//returns previous value
inline int clear_one_index(unsigned char* array, unsigned int index, unsigned int max){
	
	return bit_set_reset(array, index, max, 0);
}

inline int is_set_one_index(unsigned char* array, unsigned int index, unsigned int max){
	unsigned int 	ar_ind,
			ar_off;
	
	if(!array || index>=max)
		return -1;
	
	ar_ind= (index>>3);
	ar_off= (index&7);
	
	return (array[ar_ind]&(1<<ar_off));
}

//length of res should be equal to max
inline int get_all_set_indecis(unsigned char* array, unsigned int* res, unsigned int max){
	unsigned int	max_byte,
			i,
			index,
			res_count;
	
	max_byte=max>>3;
	if(max&7)
		max_byte++;
	
	index=0;
	res_count=0;
	
	for(i=0; i<max_byte; i++, index+=8){
		int j;
		
		if(!array[i])
			continue;
		
		for(j=0; j<8; j++)
			if(array[i] & (1<<j))
				res[res_count++]=index+j;
	}
	
	return res_count;
}

struct users* get_users(struct users* u){
	if(u){
		spin_lock(&u->lock);
		
		u->count++;
	
		spin_unlock(&u->lock);
	}
	
	return u;
}

void put_users(struct users*u){
	int f=0;
	if (u){
		spin_lock(&u->lock);
	
		if(u->count){		
			u->count--;
			if(u->count==0){
				kfree(u->users);
				kfree(u->writability);
				u->users=0;
				u->writability=0;
				f=1;
			}
		}
		
		spin_unlock(&u->lock);
		
		if(f)
			kfree(u);
	}
	
	return;
}

crust_struct* get_crust_struct(crust_struct *cs){
	if(cs){
		spin_lock(&cs->lock);
		
		cs->count++;
	
		spin_unlock(&cs->lock);
	}
	
	return cs;
}

void put_crust_struct(crust_struct *cs){
	int f=0;
	if (cs){
	//	printk("<0>" "put_crust_struct for user_block:%u, count:%d\n",cs->user_block, cs->count);
		spin_lock(&cs->lock);
	
		if(cs->count){		
			cs->count--;
			if(cs->count==0)
				f=1;
		}
		
		spin_unlock(&cs->lock);
		
		if(f){
			if(cs->owner_key){
				memset(cs->owner_key, 0, gsfs_aes_keylen);
				kfree(cs->owner_key);
			}
			
			memset(cs, 0, sizeof(crust_struct));
			kfree(cs);
		}
	}

	return;
}

#ifdef gsfs_test
	//#define test_user_check_access
#endif
#ifdef test_user_check_access
	#define gwtu(p) (p)
#else
	#define gwtu(p) 
#endif

inline int user_check_access(struct users* users, uid_t uid, int mask){
	int i;
	int ret=-1;
	
	#ifdef test_user_check_access
	char repp[1000],
		*rep=repp;
	#endif
	
	if(unlikely(!users || !users->users || !users->writability || !users->count)){
		gwtu(printk("<0>" "user_check_access with these params: %lx %lx %lx %lx \n",	\
			(unsigned long)users, (unsigned long)users->users,			\
			(unsigned long)users->writability, (unsigned long)users->count));
			
		return -1;
	}

	gwtu(sprintf(rep,"user_check_access for users: %lx* ",(unsigned long)users));
	gwtu(rep+=strlen(rep));
	
	spin_lock(&users->lock);
	
	gwtu(sprintf(rep,"count:%d and num: %d* ",users->count, users->users_num));
	gwtu(rep+=strlen(rep));

	for(i=0; i<users->users_num; i++){
		gwtu(sprintf(rep,"user_id:%u  writability:%u # ",users->users[i],users->writability[i]));
		gwtu(rep+=strlen(rep));	
		
		if(users->users[i] == uid){
			if(mask & MAY_WRITE)
				ret=!users->writability[i];
			else
				ret=0;
			
			break;
		}
	}		
		
	spin_unlock(&users->lock);
	
	gwtu(printk("<0>" "%s and ret:%d\n",repp,ret));
	
	return ret;
}

inline void set_inode_ibytes_iblocks(struct inode* inode){
	
	inode->i_bytes=inode->i_size&(Block_Size-1);
	
	inode->i_blocks=1+(inode->i_size>>Block_Size_Bits);
	
	if(inode->i_bytes==0){
		if(inode->i_size!=0)
			inode->i_bytes=Block_Size;
	
		inode->i_blocks--;
	}
	
	return;
}

//you should get down_write of inode for pin before calling
void add_child_to_parent(struct inode* pin, struct inode* in){
	struct GSFS_inode	*pinf=(struct GSFS_inode*)pin->i_private,
				*inf=(struct GSFS_inode*)in->i_private;
	struct child* child;
	atn* atn;
	
	atn=avl_tree_search( pinf->children, inf->disk_info.index_in_parent);
	
	if(atn)
		child=atn->data;	
	else{
		child=kzalloc(sizeof(struct child),GFP_KERNEL);
		child->index=inf->disk_info.index_in_parent;
		pinf->children=avl_tree_insert(pinf->children, child);
	}
	
	child->inode=in;	
	
	return;
}

//you should get down_write of inode for pin before calling
void remove_child_from_parent(struct inode* pin, struct inode* in){
	struct GSFS_inode	*pinf=(struct GSFS_inode*)pin->i_private,
				*inf=(struct GSFS_inode*)in->i_private;
	atn* atn;
	
	atn=avl_tree_search( pinf->children, inf->disk_info.index_in_parent);
	
	if(atn)
		atn->data->inode=0;

	return;
}

inline void GSFS_inode_init(struct inode* inode){
	struct GSFS_inode* inode_info=(struct GSFS_inode*)inode->i_private;
	
	init_rwsem(&inode_info->inode_rwsem);

	inode_info->add_event_to_parent=general_add_event_to_parent;
	
	inode->i_op=&GSFS_inode_operations;
	
	if(S_ISDIR(inode->i_mode))
		inode->i_fop=&GSFS_dir_fops;
	else{
		inode->i_fop=&GSFS_file_fops;
		inode->i_data.a_ops=&GSFS_address_space_operations;
		
		if(inode_info->igflags & igflag_secure){
			inode_info->sri=kzalloc(sri_len, GFP_KERNEL);
		
			//inode_info->sri->active_l1=-1;
			//inode_info->sri->active_l2=-1;
			//inode_info->sri->active_l3=-1;
			
			init_rwsem(&inode_info->sri->sri_via_rwsem);
		}
	}
	
	return;	
}

//you should not lock cs before calling 
int read_owner_key_for_crust_struct(crust_struct *cs, struct super_block* sb, char *user_block_hash){
	//struct GSFS_inode	*inf=(struct GSFS_inode*)in->i_private,
	//			*pinf=0;
	//struct inode		*pin=0;
	char	*key;
		//*tkey;
	int 	ret=-1;
		//len;
	
	//if(!(inf->igflags & igflag_secure))
	//	return -1;
	
	spin_lock(&cs->lock);
	
	if(cs->owner_key){
		ret=0;
		goto rret;
	}
	
	key=kmalloc(gsfs_aes_keylen, GFP_KERNEL);
	
	/*
	if(inf->disk_info.parent_ino!=0){
		//for non_root inodes we can test if their parent has child's owner key
		pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);
		pinf=(struct GSFS_inode*)pin->i_private;
		
		down_write(&pinf->inode_rwsem);
		
		ret=get_event(pinf, inf->disk_info.index_in_parent, Non_FL_Owner_Key_Set_Event, key, &len);
		if(!ret)
			goto good_no_add_event_ret;
	}
	
	if(inf->igflags & igflag_active_parent_link){
		//surely root and its children cant have igflag_active_parent_link
		unsigned short 	gdlen=gsfs_dirent_len,
				gd_per_page=Block_Size/gdlen,
				gdpage=inf->disk_info.index_in_parent/gd_per_page,
				gdoffset=inf->disk_info.index_in_parent%gd_per_page;
		struct 	buffer_head* bh;
		struct GSFS_dirent *gda;
		
		if(!(pinf->igflags&igflag_present_owner_key))
			if(read_owner_key_for_sec_inode(pin))	
				goto bad_ret;
		
		bh=__bread(in->i_sb->s_bdev,get_dp_bn_of_in(pin,gdpage),Block_Size);
		
		gda=(struct GSFS_dirent*)bh->b_data;
					
		ret=decrypt_owner_key(key, gda[gdoffset].gd_child_security_fields.gd_owner_key_link, pinf->owner_key);
					
		brelse(bh);	
		
		if(ret)
			goto bad_ret;
		else
			goto good_add_event_ret;
	}
	*/
	
	//if(inf->igflags & igflag_active_user_block){
	
	if(key){
		struct buffer_head* bh;
		char 	ubhash[gsfs_hashlen];
		struct inode_user_page* iup;
		struct rsa_key *rkey;
	
		bh=__bread(sb->s_bdev, cs->user_block, Block_Size);
		if(!bh)
			goto bad_ret;
		
		lock_buffer(bh);
		iup=(struct inode_user_page*)bh->b_data;
		
		if(iup->owner_key.uid != current->loginuid)
			goto badbh_ret;
		
		ret=get_user_block_hash(ubhash, bh->b_data);
		if(ret || strncmp(ubhash, user_block_hash, gsfs_hashlen))			
			goto badbh_ret;
		
		rkey=get_rsa_key((struct GSFS_sb*)sb->s_fs_info, current->loginuid, 1);
		if(!rkey || !rkey->key)
			goto badbh_ret;
		
		spin_lock(&rkey->lock);
		
		ret=rsa_decrypt_owner_key_from_user_block(key, iup->owner_key.rsa_encrypted_key, rkey->key);
		
		spin_unlock(&rkey->lock);	
badbh_ret:
		unlock_buffer(bh);
		
		brelse(bh);
		/*
		if(ret)
			goto bad_ret;	
		else
			goto good_add_event_ret;
		*/
	}
	
/*
	goto bad_ret;
	
good_add_event_ret:

	tkey=kmalloc(gsfs_aes_keylen, GFP_KERNEL);
	memcpy(tkey, key, gsfs_aes_keylen);
	
	add_event_to_inode(pin, inf->disk_info.index_in_parent, Non_FL_Owner_Key_Set_Event, tkey, 
			   gsfs_aes_keylen, event_flag_from_disk);
	
good_no_add_event_ret:

	inf->owner_key=key;
	inf->igflags|=igflag_present_owner_key;
	ret=0;
	
	//printk("<0>" "owner_key of inode %lu is \n",in->i_ino);
	//printkey(key);

put_pin_ret:	

	if(pin){
		iput(pin);
		if(pinf)
			up_write(&pinf->inode_rwsem);
	}
	return ret;
*/
bad_ret:
	//ret=-1;

	if(!ret)
		cs->owner_key=key;
	else{
		memset(key, gsfs_hashlen,0);
		kfree(key);
	}
rret:
	spin_unlock(&cs->lock);
	
	return ret;
}

#ifdef gsfs_test
	#define test_write_inode_to_disk
#endif
#ifdef test_write_inode_to_disk
	#define gwtw(p)	p
#else
	#define gwtw(p)
#endif 
//cit
int update_reg_inode_from_events(struct inode* in){
	struct GSFS_inode 	*inf=(struct GSFS_inode*)in->i_private;
	sri_struct 		*sri=inf->sri;
	//struct GSFS_inode_disk_inf *ind;
	
	#ifdef test_write_inode_to_disk
		char 	*rep,
			repp[1000];
		rep=repp;
		sprintf(rep, "update_reg_inode_from_events for in: %lu * ",in->i_ino);
		rep+=strlen(rep);
	#endif
	
	if(!sri){
		printk("<0>" "sec-reg-inode with no sri for writing to disk\n");
		return -1;
	}
	
	if(sri->l1_active_bnh && sri->l0_bh_num && (sri->l0_flags & sri_flag_bh_changed)){
		get_bnh_page_hash(sri->l1_active_bnh->hash, sri->l0_bh->b_data);
		
		if(sri->l1_flags & sri_flag_bnh_on_bh)
			sri->l1_flags |= sri_flag_bh_changed;
		
		sri->l0_flags &= ~sri_flag_bh_changed;
		
		mark_buffer_dirty(sri->l0_bh);
		set_buffer_uptodate(sri->l0_bh);
		
		gwtw(sprintf(rep, "updating l1_active_bnh hash for block number: %u, l0_bh_num: %u, pl1: %u to: ", sri->l1_active_bnh->blocknumber, sri->l0_bh_num, sri->active_l1));
		gwtw(rep+=strlen(rep));
		gwtw(printhexstring(sri->l1_active_bnh->hash, rep, 12));
		gwtw(rep+=strlen(rep));
	}
	
	if(sri->l2_active_bnh && sri->l1_bh_num && (sri->l1_flags & sri_flag_bh_changed)){
		get_bnh_page_hash(sri->l2_active_bnh->hash, sri->l1_bh->b_data);
		
		if(sri->l2_flags & sri_flag_bnh_on_bh)
			sri->l2_flags |= sri_flag_bh_changed;
		
		sri->l1_flags &= ~sri_flag_bh_changed;
		
		mark_buffer_dirty(sri->l1_bh);
		set_buffer_uptodate(sri->l1_bh);
		
		gwtw(sprintf(rep, " * updating l2_active_bnh hash for block number: %u, l1_bh_num: %u, pl2: %u to: ", sri->l2_active_bnh->blocknumber, sri->l1_bh_num, sri->active_l2));
		gwtw(rep+=strlen(rep));
		gwtw(printhexstring(sri->l2_active_bnh->hash, rep, 12));
		gwtw(rep+=strlen(rep));
	}
	
	if(sri->l3_active_bnh && sri->l2_bh_num && (sri->l2_flags & sri_flag_bh_changed)){
		get_bnh_page_hash(sri->l3_active_bnh->hash, sri->l2_bh->b_data);
		
		sri->l2_flags &= ~sri_flag_bh_changed;
		
		mark_buffer_dirty(sri->l2_bh);
		set_buffer_uptodate(sri->l2_bh);
		
		gwtw(sprintf(rep, " * updating l3_active_bnh hash for block number: %u, l2_bh_num: %u, pl3: %u to: ", sri->l3_active_bnh->blocknumber, sri->l2_bh_num, sri->active_l3));
		gwtw(rep+=strlen(rep));
		gwtw(printhexstring(sri->l3_active_bnh->hash, rep, 12));
		gwtw(rep+=strlen(rep));
	}
	
	gwtw(printk("<0>" "%s\n",repp));
	
	return 0;
}

//you should get write  inf->inode_rwsem 
int update_dir_inode_from_events(struct inode* in){
	struct GSFS_inode * inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf *ind=&inf->disk_info;
	struct buffer_head	*gdhb_bh=0,
				*gd_bh=0;

	char			*gdca,
				*temp,
				**gdhashes;
	
	//struct	GSFS_dirent	*gda=0;
	
	unsigned short		*gdirent_changes,
				gdc_count,
				changes_num,
				i,
				j,
	//			gdpage,
	//			gdoffset,
	//			current_gd_page=-1,
				gdlen,
				gd_per_page;
	struct avl_tree_node	**res;
	
	#ifdef test_write_inode_to_disk
		char 	repp[1000],
			*rep=repp;
		memset(rep,0,1000);
	#endif

	gwtw(sprintf(rep,"update dir inode:%lu from children events * ",in->i_ino));
	gwtw(rep+=strlen(rep));
	
	if(!inf->children)
		return 0;
	
	gdlen=gsfs_dirent_len;
	gd_per_page=Block_Size/gdlen;
	
	gdhb_bh=__bread(in->i_sb->s_bdev, ind->dir_inode_security.gdirent_hash_block, Block_Size);
	gdca=gdhb_bh->b_data;
	
	changes_num=avl_tree_get_size(inf->children);
	gwtw(sprintf(rep,"changes_num:%d *",changes_num));
	gwtw(rep+=strlen(rep));
	
	gdirent_changes=kzalloc(changes_num*sizeof(unsigned short),GFP_KERNEL);
	gdhashes=kzalloc(changes_num*sizeof(char*),GFP_KERNEL);
	
	gdc_count=0;
	
	res=kzalloc(changes_num*sizeof(struct avl_tree_node*),GFP_KERNEL);
	
	avl_tree_get_all_nodes(inf->children, res, changes_num);
	
	for(i=0;i<changes_num;i++){
		struct child	*child=res[i]->data;
		//int gdi=0;
		
		gwtw(sprintf(rep, "events from child index:%d *" , child->index));
		gwtw(rep+=strlen(rep));
		
		for(j=0;j<Events_Num;j++){
			if( !(child->events[j].flags & event_flag_is_present) ||
			     (child->events[j].flags & event_flag_from_disk) )
				continue;
			gwtw(sprintf(rep, "type :%d *",j));
			gwtw(rep+=strlen(rep));
			switch(j){
				/*
				case Non_FL_Crust_State_Set_Event:
				case Non_FL_Owner_Key_Set_Event:
					if(inf->igflags & igflag_has_sec_child){
						char	temp[400];
						switch(j){
							case Non_FL_Crust_State_Set_Event:
								sprintf(temp,"Non_FL_Crust_State_Set_Event");
								break;
							
							case Non_FL_Owner_Key_Set_Event:
								sprintf(temp,"Non_FL_Owner_Key_Set_Event");
								break;
						}
						
						printk("<0>" "One FL sec inode with ino:%lu received %s\n",in->i_ino,temp);
						gwi(printk("<0>" "One FL sec inode with ino:%lu received %s\n",in->i_ino,temp));
						break;
					}
					gdpage=child->index/gd_per_page;
					gdoffset=child->index%gd_per_page;
					
					if(current_gd_page!=gdpage){
						current_gd_page=gdpage;
						if(gd_bh){
							set_buffer_uptodate(gd_bh);
							mark_buffer_dirty(gd_bh);
							//write_one_bh_dev(gd_bh);
							brelse(gd_bh);
						}
						gd_bh=__bread(in->i_sb->s_bdev,get_dp_bn_of_in(in,gdpage),Block_Size);
						gda=(struct GSFS_dirent*)gd_bh->b_data;
					}
					
					switch(j){
						case Non_FL_Crust_State_Set_Event:
							encrypt_crust_state(	&gda[gdoffset].gd_child_security_fields.gd_crust_state_link, 
										(struct crust_state*)(child->events[j].data),
										inf->inode_crust_state, 
										inf->crust_last_ver );
									
							gda[gdoffset].gd_child_security_fields.gd_dirent_crust_link_pver=inf->crust_last_ver;
							
							gda[gdoffset].gd_last_crust_ver=child->events[j].datalen;
														
							gwtw(sprintf(rep,"encrypt crust state from: "));
							gwtw(rep+=strlen(rep));
							gwtw(printhexstring(child->events[j].data, rep, 81));
							gwtw(rep+=strlen(rep));
							gwtw(sprintf(rep," to: "));
							gwtw(rep+=strlen(rep));
							gwtw(printhexstring((char*)&gda[gdoffset].gd_child_security_fields.gd_crust_state_link, rep, 81));
							gwtw(rep+=strlen(rep));
							
							break;
							
						case Non_FL_Owner_Key_Set_Event:
							if(!(inf->igflags & igflag_present_owner_key))
								if(read_owner_key_for_sec_inode(in))
									break;
								
							encrypt_owner_key(	gda[gdoffset].gd_child_security_fields.gd_owner_key_link,
										child->events[j].data,
										inf->owner_key	);
										
							gwtw(sprintf(rep,"encrypt owner key from: "));
							gwtw(rep+=strlen(rep));
							gwtw(printhexstring(child->events[j].data, rep, 16));
							gwtw(rep+=strlen(rep));
							gwtw(sprintf(rep," to: "));
							gwtw(rep+=strlen(rep));
							gwtw(printhexstring((char*)&gda[gdoffset].gd_child_security_fields.gd_owner_key_link, rep, 16));
							gwtw(rep+=strlen(rep));
							gwtw(sprintf(rep," with key: "));
							gwtw(rep+=strlen(rep));
							gwtw(printhexstring(inf->owner_key, rep, 16));
							gwtw(rep+=strlen(rep));	
							
							break;
					}
				*/	
				case GDirent_Hash_Changed_Event:
					/*
					if(j!=GDirent_Hash_Changed_Event){
						if(gdi==0)
							temp=kmalloc(gsfs_hashlen,GFP_KERNEL);
						else
							temp=gdhashes[gdc_count-1];
						
						get_gdirent_hash(temp, &gda[gdoffset]);
					}
					else
					*/
						temp=child->events[j].data;
				
					//always GDirent_Hash_Changed_Event is added to parent
					//after other gdirent changes (owner_key and crust_state)
					//threrefore it is not needed to calculate new gdhash
				
					memcpy(gdca+gsfs_hashlen*child->index, temp, gsfs_hashlen);
					
					//if(gdi==0){
						gdhashes[gdc_count]=temp;
						gdirent_changes[gdc_count]=child->index;
						gdc_count++;
					//}
					
					//gdi++;
					
					break;
			}
			child->events[j].flags|=event_flag_from_disk;
		}
	}
	
	if(gd_bh){
		set_buffer_uptodate(gd_bh);
		mark_buffer_dirty(gd_bh);
		brelse(gd_bh);
	}
	
	if(gdc_count){
		int i;
		
		gwtw(sprintf(rep,"gdc_count:%d* ",gdc_count));
		gwtw(rep+=strlen(rep));
		
		for(i=0; i<gdc_count; i++)
			add_event_to_inode(in, gdirent_changes[i], GDirent_Hash_Changed_Event, gdhashes[i], gsfs_hashlen, event_flag_from_disk);

		update_hash_block_to_root(gdca,gdirent_changes,gdc_count);
		
		memcpy(ind->dir_inode_security.inode_gdirent_hash, gdca+hash_root_offset, gsfs_hashlen);
		
		inf->igflags|=igflag_inode_metadata_changed;
		
		mark_buffer_dirty(gdhb_bh);
		set_buffer_uptodate(gdhb_bh);
		
		gwtw(sprintf(rep,"new gdirent_hash for inode: "));
		gwtw(rep+=strlen(rep));
		gwtw(printhexstring(gdca+hash_root_offset, rep, 16));
		gwtw(rep+=strlen(rep));
	}
	brelse(gdhb_bh);
	
	kfree(gdirent_changes);
	kfree(gdhashes);
	
	kfree(res);
	
	gwtw(printk("<0>" "%s\n",repp));
	
	return 0;
}

int write_inode_to_disk(struct inode *in, int do_sync){
	struct GSFS_inode		*in_info=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind;
	struct GSFS_sb 			*gsb;
	int		ret=0,
			wod=0;		//write_on_disk
	unsigned long 	time;
	
	#ifdef test_write_inode_to_disk
		char 	repp[1000],
			*rep=repp;
		memset(rep,0,1000);
	#endif
	
	//printk("<0>" "write inode1 for in:%ld\n",in->i_ino);
	gwi(printk("<0>" "write inode %lu to disk started with do_sync:%d \n",in->i_ino, do_sync));
	gwtw(sprintf(rep,"write inode to disk for inode:%lu with do_sync:%d * ",in->i_ino, do_sync));
	gwtw(rep+=strlen(rep));
	
	if(!in->i_sb || !in->i_sb->s_fs_info)
		return -1;
	
	if(!in_info)
		return -1;
	
	gsb=(struct GSFS_sb*)in->i_sb->s_fs_info;
	
	down_write(&in_info->inode_rwsem);
	
	gwtw(printsemkeys(rep, gsb));
	gwtw(rep+=strlen(rep));  
	
	ind=&in_info->disk_info;
	time=current_fs_time(in->i_sb).tv_sec;
	
	//updating inode ondisk info
	if(in_info->igflags & igflag_inode_metadata_changed){
		ind->ino=in->i_ino;
		ind->inlink=in->i_nlink;
		ind->iblocks=in->i_blocks;
		ind->ibytes=in->i_bytes;
		ind->imode=in->i_mode;
		ind->isize=in->i_size;		
		ind->imtime=in->i_mtime.tv_sec;
		ind->ictime=time;
		ind->igflags=igflag_ondisk(in_info->igflags);
	}
	
	wod=0;
	if(in_info->igflags & (igflag_secure | igflag_has_sec_child) ){
		//updating changes occured in children 
		gwtw(sprintf(rep,"updating inode_from_events: * "));
		gwtw(rep+=strlen(rep));
		
		if(in_info->igflags & igflag_dir)
			update_dir_inode_from_events(in);
		else
			update_reg_inode_from_events(in);
		
		//calculating new metadata-hash and send it to parent
		//be careful we don't save inode hash in it
		//and inode_metadata_hash is root of its children mdhash_root
		//from mdhash block
		if(in_info->igflags & igflag_inode_metadata_changed){
			int ret;
			
			char aa[gsfs_hashlen];
			
			//temp=kzalloc(gsfs_hashlen,GFP_KERNEL);
			
			ret=get_inode_metadata_hash_for_parent(in,aa);
			
			set_IHP(gsb, ind->SAT_index, aa);
			
			in_info->igflags&=~igflag_inode_metadata_changed;
			wod=1;
			
			gwtw(sprintf(rep,"changing metadata_hash (ret=%d) to: ",ret));
			gwtw(rep+=strlen(rep));
			gwtw(printhexstring(aa, rep, 16));
			gwtw(rep+=strlen(rep));
			gwtw(memset(aa, 0, 16));
		}
	}
	else{
		//for non-sec inodes with no sec child
		if(in_info->igflags & igflag_inode_metadata_changed){
			in_info->igflags&=~igflag_inode_metadata_changed;
			wod=1;
			
			gwtw(sprintf(rep,"non-sec inode *"));
			gwtw(rep+=strlen(rep));
		}
	}
	
	//writing inode info to disk 
	if(wod){
		in_info->inode_bh=__bread(in->i_sb->s_bdev,in_info->inode_bnr,Block_Size);
		
		if(!in_info->inode_bh){
			gwtw(sprintf(rep,"bad inode_bh * "));
			gwtw(rep+=strlen(rep));
			
			ret=-1;
			goto back;
		}
		
		lock_buffer(in_info->inode_bh);
		
		memcpy(in_info->inode_bh->b_data,in_info,min(sizeof(struct GSFS_inode_disk_inf),(unsigned long)Block_Size));

		set_buffer_uptodate(in_info->inode_bh);
		unlock_buffer(in_info->inode_bh);
		
		if(do_sync){
			write_one_bh_dev(in_info->inode_bh);
			ret=0;
		}
		else{
			mark_buffer_dirty(in_info->inode_bh);
			ret=-EIO;
		}
		brelse(in_info->inode_bh);
		
		in_info->igflags=igflag_ondisk(in_info->igflags);
	}

back:	
	up_write(&in_info->inode_rwsem);
	
	gwi(printk("<0>" "write inode %lu to disk ended with do_sync:%d ",in->i_ino,do_sync));
	gwtw(printk("<0>" "%s\n",repp));
	
	in->i_state &= ~(I_DIRTY_SYNC | I_DIRTY_DATASYNC);
	
	return ret;
}

int GSFS_write_inode(struct inode *in, struct writeback_control *wbc){
	//printk("<0>" "write inode for inode: %ld, process id: %d\n",in->i_ino, current->pid);
	
	return write_inode_to_disk(in, wbc->sync_mode == WB_SYNC_ALL);
	
	//return 0;
}

#ifdef gsfs_test
	#define test_destroy_inode
#endif
#ifdef test_destroy_inode
	#define gwdi(p)	p
#else
	#define gwdi(p)
#endif

void GSFS_destroy_inode(struct inode* in){
	struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_sb* gsb=(struct GSFS_sb*)in->i_sb->s_fs_info;
	struct inode* parent_inode=0;
	#ifdef test_destroy_inode
	char 	repp[1000],
		*rep=repp;
	#endif
	
	gwdi(sprintf(rep,"destroy inode:%ld with count:%d * ",in->i_ino,atomic_read(&in->i_count)));
	gwdi(rep+=strlen(rep));
	//printk("<0>" "destroy inode:%ld\n",in->i_ino);
	
	if(!inf){
		gwdi(sprintf(rep, "no inf * "));
		gwdi(rep+=strlen(rep));
		goto back;
	}
	
	gwdi(sprintf(rep, "inf: %lx * ",(unsigned long)inf));
	gwdi(rep+=strlen(rep));
	
	if(inf->igflags & igflag_incomplete_inode)
		delete_inode_from_incom_inodes(gsb, in);
	
	down_write(&inf->inode_rwsem);
	
	gwdi(sprintf(rep, "igflags: %x * ",inf->igflags));
	gwdi(rep+=strlen(rep));
	
	if(inf->igflags & igflag_secure){
		gwdi(sprintf(rep, "inode is secure and users: %lx * ",(unsigned long)inf->users));
		gwdi(rep+=strlen(rep));
		
		if(inf->users)
			put_users(inf->users);
		else
			printk("<1>" "no users for inode to destroy.\n");
		
		gwdi(sprintf(rep, "inode crust state: %lx * ",(unsigned long)inf->inode_crust_struct));
		gwdi(rep+=strlen(rep));
				
		if(inf->inode_crust_struct){
			put_crust_struct(inf->inode_crust_struct);
			//memset(inf->inode_crust_state, 0, sizeof(struct crust_state));
			//kfree(inf->inode_crust_state);
		}
		else
			printk("<1>" "no crust state for inode to destroy\n");
		/*
		gwdi(sprintf(rep, "inode owner key : %lx * ",(unsigned long)inf->owner_key));
		gwdi(rep+=strlen(rep));
		
		if(inf->igflags & igflag_present_owner_key){
			memset(inf->owner_key, 0, gsfs_aes_keylen);
			kfree(inf->owner_key);
		}
		*/
		
		if(!(inf->igflags & igflag_dir)){
			if(!inf->sri){
				printk("<1>" "sec-reg-inode with no sri\n");
				
				gwdi(sprintf(rep, "sec-reg-inode with no sri\n"));
				gwdi(rep+=strlen(rep));	
			}
			else{
				if(inf->sri->l0_bh)
					brelse(inf->sri->l0_bh);
				if(inf->sri->l1_bh)
					brelse(inf->sri->l1_bh);
				if(inf->sri->l2_bh)
					brelse(inf->sri->l2_bh);
				
				memset(inf->sri, 0, sizeof(sri_struct));
				
				kfree(inf->sri);
				inf->sri=0;
			}
		}
	}
	
	gwdi(sprintf(rep, "inf-> children: %lx * ",(unsigned long)inf->children));
	gwdi(rep+=strlen(rep));
		
	if(inf->children)
		avl_tree_free(inf->children);
	
	if(likely(in->i_ino!=1))		
		if(inf->igflags & (igflag_secure | igflag_has_sec_child)){
			struct inode* pin;
			struct GSFS_inode* pinf;
			
			pin=ilookup(in->i_sb, inf->disk_info.parent_ino);
			if(!pin){
				//printk("<1>" "No pin for destroy_inode of inode:%lu\n",in->i_ino);
				gwdi(sprintf(rep, "pin: %lx * ",(unsigned long)pin));
				gwdi(rep+=strlen(rep));
			
				goto ret;
			}
			
			pinf=(struct GSFS_inode*)pin->i_private;
			
			if(!pinf){
				//printk("<1>" "No pinf for destroy_inode of inode:%lu\n",in->i_ino);
				gwdi(sprintf(rep, "pinf: %lx * ",(unsigned long)pinf));
				gwdi(rep+=strlen(rep));
				
				goto ret;
			}
			
			down_write(&pinf->inode_rwsem);
			
			remove_child_from_parent(pin, in);
			
			up_write(&pinf->inode_rwsem);
			
			parent_inode=pin;
			gwdi(sprintf(rep, "remove child * "));
			gwdi(rep+=strlen(rep));
		}
		
ret:	
	up_write(&inf->inode_rwsem);
	
	memset(inf, 0, sizeof(struct GSFS_inode));
	kfree(inf);
		
	in->i_private=0;
	
back:	
	gwdi(printk("<0>" "%s * parent_ino count:%u * \n", repp, (!parent_inode)?0:atomic_read(&parent_inode->i_count)));

	if(parent_inode)// && atomic_read(&parent_inode->i_count))
		iput(parent_inode);
	
	return;
}

void GSFS_delete_inode(struct inode* in){
	struct GSFS_sb *gsb=(struct GSFS_sb*)in->i_sb->s_fs_info;
	struct GSFS_inode *inf=(struct GSFS_inode*)in->i_private;
	unsigned int i=0,
		     off=0,
		     len,
		     j=0;
	
	gwi(printk("<0>" "delete inode %ld\n",in->i_ino));
	
	in->i_state=I_CLEAR;
	
	clear_IAT(gsb,in->i_ino);
	BAT_clear_one_block(gsb,inf->inode_bnr);
	
	if(inf->igflags & (igflag_secure | igflag_has_sec_child)){
		SAT_clear_one_index(gsb, inf->disk_info.SAT_index);
		
		if(inf->igflags & igflag_secure){
			if(inf->igflags & igflag_dir){
				BAT_clear_one_block(gsb, inf->disk_info.dir_inode_security.gdirent_hash_block);
				
				if(inf->igflags & igflag_active_user_block)
					BAT_clear_one_block(gsb, inf->disk_info.dir_inode_security.user_block);
			}
			else
				delete_sec_reg_inode_hash_blocks(in);
		}
	}
	
	while(j<inf->disk_info.grps_num){
		len=inf->disk_info.grps[i]-off+1;
		clear_BAT(gsb,inf->disk_info.grps[i+1],inf->disk_info.grps[i+1]+len-1);
		off=inf->disk_info.grps[i]+1;
		i+=2;
		j++;
	}
	
	gt(printk("<0>" "gsb_free_sec_ind:%d\n",gsb->gsb_disk.free_sec_indecis));
	
	return;
}

//we are not currently using this function
void GSFS_clear_inode(struct inode* in){
	gwi(printk("<0>" "clear inode %ld %d %d",in->i_ino,atomic_read(&in->i_count),in->i_nlink));
	
	return;
}

sector_t get_dp_bn_of_in(struct inode* in, unsigned int pn){
	struct GSFS_inode* in_inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf *ind=&in_inf->disk_info;

	//printk("<0>" "in_inf->grps_num:%u,in_inf->current_grp:%u ,in_inf->current_start:%u ,in_inf->current_end:%u, in_inf->start_bn:%u\n"
	//       ,in_inf->grps_num,in_inf->current_grp,in_inf->current_start,in_inf->current_end,in_inf->start_bn);

test:	if(in_inf->current_grp>=0 && in_inf->current_start<=pn && in_inf->current_end>=pn)
		return in_inf->start_bn+pn-in_inf->current_start;
		
	if(in_inf->current_grp<0){
		if(ind->grps_num<0)
			return 0;
		in_inf->current_grp=0;
		in_inf->current_start=0;
		in_inf->current_end=ind->grps[0];
		in_inf->start_bn=ind->grps[1];
		//printk("<0>" "pdn: %d test",pn);
		goto test;
	}
	
	while(in_inf->current_grp<(ind->grps_num-1) && pn>in_inf->current_end){
		in_inf->current_grp++;
		in_inf->current_start=in_inf->current_end+1;
		in_inf->current_end=ind->grps[2*in_inf->current_grp];
		in_inf->start_bn=ind->grps[2*in_inf->current_grp+1];
		//printk("<0>" "pdn: %d inc %d %d",pn,in_inf->current_grp,in_inf->grps_num);
	}
	
	while(in_inf->current_grp>0 && pn<in_inf->current_start){
		in_inf->current_grp--;
		in_inf->current_end=in_inf->current_start-1;
		if(in_inf->current_grp==0)
			in_inf->current_start=0;
		else
			in_inf->current_start=ind->grps[2*in_inf->current_grp-2]+1;
		in_inf->start_bn=ind->grps[2*in_inf->current_grp+1];
		//printk("<0>" "pdn: %d dec",pn);
	}
	
	if(in_inf->current_start<=pn && in_inf->current_end>=pn)
		return in_inf->start_bn+pn-in_inf->current_start;
	//printk("<0>" "pdn: %d noresponse",pn);
	
	return 0;	
}

unsigned int get_block_number_of_inode(struct GSFS_sb* gsb, unsigned int ino){
	unsigned int	bn=iat_bn(ino),
			off=iat_offset(ino);
	struct buffer_head* bh=get_lru_bh(gsb,IAT_LRU,gsb->gsb_disk.iat_start+ bn);
	unsigned int* p;
	if(!bh)
		return 0;
	p=(int*)bh->b_data;
	//put_bh(bh);
	//printk("<0>" "gibn:%d %d",p[off],bh->b_count);
	return p[off];	
}

//you should get inode_rwsem write for in befor calling this function
int verify_and_get_integrity_for_child(unsigned char type,struct inode*in, unsigned short index, char* dest){
	struct GSFS_inode *inf= (struct GSFS_inode*)in->i_private;
	unsigned char	event_type,
			*temp;
	int 		ret;
	unsigned int 	len,
			block_num;
	char		*integ_arr,
			*verifid_root;
	struct buffer_head* bh;
	
	switch(type){
		
		case GDirent_Integrity:
			event_type=GDirent_Hash_Changed_Event;
			block_num=inf->disk_info.dir_inode_security.gdirent_hash_block;
			integ_arr=inf->children_gdirent_hash_integrity;
			verifid_root=inf->disk_info.dir_inode_security.inode_gdirent_hash;
			break;
		
		default:
			return -1;		
	}
	
	ret=get_event(inf, index, event_type, dest, &len);
	//printk("<0>" "verify integrity: get_event_ret:%d, len:%d, data:\n",ret,len);
	//printkey(dest);
	
	if(!ret)
		return 0;
		
	bh=__bread(in->i_sb->s_bdev, block_num, Block_Size);
	
	ret=verify_hash_integrity(bh->b_data, integ_arr, index, dest, verifid_root);
	
	brelse(bh);
	
	if(!ret){
		
		temp=kmalloc(gsfs_hashlen, GFP_KERNEL);
		memcpy(temp, dest, gsfs_hashlen);
		
		add_event_to_inode(in, index, event_type, temp, gsfs_hashlen, event_flag_from_disk);
	}
	
	return ret;
}

//returns 1 for success and -1 for fail
inline int get_inode_hash(struct inode* in, char* mdhash){
	struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_sb* gsb=(struct GSFS_sb*)in->i_sb->s_fs_info;
	int ret;
	
	//getting mdhash from parents events or mdhash disk page
	if(inf->disk_info.SAT_index == -1)
		return -1;
	
	if( !test_one_SAT_index(gsb, inf->disk_info.SAT_index) )
		return -1;
	
	ret=get_IHP(gsb, inf->disk_info.SAT_index, mdhash);
	if(ret<0)
		return -1;
	
	return 1;
}

//test_integrity_of_(non_)?root_inode returns 
//	-1	 for not integrated
//	0 	for no integrity is required
//	1 	for dhash and mdhash is integrated

//you should get write sem for pinf->inode_rwsem 

inline int test_integrity_of_inode(struct inode* in, struct inode* pin, char* mdhash){
	struct GSFS_inode	* inf= (struct GSFS_inode*)in->i_private,
				* pinf=(struct GSFS_inode*)pin->i_private;
				
	//printk("<0>" "test_integrity_of_inode for in: %ld, SAT_index:%u\n",in->i_ino, inf->disk_info.SAT_index);
	
	if(!(pinf->igflags & (igflag_secure | igflag_has_sec_child)) )
		return 0;
	
	if(!is_set_one_index(pinf->disk_info.dir_inode_security.dir_inode_child_index_dedication_array, 
			inf->disk_info.index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS))
		return -1;
	
	if(!is_set_one_index(pinf->disk_info.dir_inode_security.dir_inode_sec_has_sec_child_array, 
		inf->disk_info.index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS)){
		if(pinf->igflags & igflag_secure )
			return -1;
		//if(pinf->igflags & igflag_has_sec_child)
		return 0;
	}
	
	//when index is set, the child is secure or has secure child
	if(!(inf->igflags & (igflag_secure | igflag_has_sec_child) ))
		return -1;
	
	return get_inode_hash(in, mdhash);
}

#ifdef gsfs_test
	#define test_get_inode_users_crust
#endif
#ifdef test_get_inode_users_crust
	#define  gwuc(p)  p
#else
	#define gwuc(p)	
#endif

//gets users, crust_state for a secure inode 
//you should get write of inode_rwsem  for pin before calling this function
//returns 0 if crust and users is set succefully
//returns -2 if user_block is not integrated
//returns -1 else
int get_inode_users_and_or_crust_state_from_parent(struct inode	* pin,		unsigned short childindex, 
						crust_struct	** dest_crust,	struct users** dest_users, 
						unsigned int user_block_num,	char* user_block_hash, 
						unsigned char flags, 		int input_ret1,
						int crust_can_get_event, int users_can_get_event){
	struct GSFS_inode	*pinf=(struct GSFS_inode*)pin->i_private;
	int 	len1=0,
		len2=0,
		rete=-1,
		retval=-1,
		ret1=input_ret1,
		ret2=-1;
	struct users	*users,
			*u1=0,
			*u2=0;
			
	#ifdef test_get_inode_users_crust
	char	*repp,
		*rep;
		
		repp=kmalloc(3000, GFP_KERNEL);
		rep=repp;
	#endif
		
	gwuc(sprintf(rep, "get_inode_users_and_crust_state_from_parent for inode:%lu, child:%d * ", pin->i_ino, childindex));
	gwuc(rep+=strlen(rep));
	gwuc(sprintf(rep, "with *dest_users: %lx * ",(unsigned long)*dest_users));
	gwuc(rep+=strlen(rep));
	
	if(ret1 && crust_can_get_event){
		ret1=get_event(pinf, childindex, Crust_Struct_Set_VEvent, dest_crust, &len1);
		
		if(!ret1)
			get_crust_struct(*dest_crust);
	}
	
	if(*dest_users)
		ret2=0;
	
	if(ret2 && users_can_get_event){
		ret2=get_event(pinf, childindex, Users_Set_VEvent, &users, &len2);
		
		if(!ret2)
			*dest_users=get_users(users);
	}
	
	gwuc(sprintf(rep, "crust struct: ret1:%d, len1:%d, users: ret2:%d, len2:%d * ",ret1, len1, ret2, len2));
	gwuc(rep+=strlen(rep));
	
	if(!ret1 && !ret2){
		retval=0;
		goto retend;
	}
	
	if(ret1)
		ret1=-1;
	if(ret2)
		ret2=-1;
	
	//if this childindex hasnt user_block therefore it use its parent users and crust_state
	if(!(flags & igflag_active_user_block)){
		if(ret1 && !(pinf->igflags & igflag_incomplete_inode)){
			*dest_crust=get_crust_struct(pinf->inode_crust_struct);
			ret1=1;
		}
		
		if(ret2){
			//*dest_users=get_users(pinf->users);
			u1=get_users(pinf->users);
			ret2=1;
		}
	}
	
	if((ret1<0 || ret2<0) && (flags & igflag_active_user_block)){
		struct buffer_head* bh=0;
		struct inode_user_page* iup;
		char ubhash[gsfs_hashlen];
		int br=0;
		
		gwuc(sprintf(rep, "openning user_block:%u * ",user_block_num));
		gwuc(rep+=strlen(rep));
				
		bh=__bread(pin->i_sb->s_bdev, user_block_num, Block_Size);
		if(!bh){
			br=1; 
			goto ret;
		}
		
		lock_buffer(bh);
		
		br=get_user_block_hash(ubhash, bh->b_data);
		if(br || strncmp(ubhash, user_block_hash, gsfs_hashlen)){
			br=2;
			
			gwuc(sprintf(rep, "sent user block hash : "));
			gwuc(rep+=strlen(rep));
			gwuc(printhexstring((char*)user_block_hash, rep, 16));
			gwuc(rep+=strlen(rep));
			
			gwuc(sprintf(rep, "new user block hash : "));
			gwuc(rep+=strlen(rep));
			gwuc(printhexstring(ubhash, rep, 16));
			gwuc(rep+=strlen(rep));
			
			rete=-2;
			
			printk("<1>" "Bad user_block hash for child_index: %d of inode: %ld", childindex, pin->i_ino);
			
			goto ret;
		}
			
		iup=(struct inode_user_page*)bh->b_data;
		
		if(flags & igflag_active_parent_link){
			if(ret1<0 && !(pinf->igflags & igflag_incomplete_inode)){
				int dret;
				
				*dest_crust=kzalloc(sizeof(crust_struct), GFP_KERNEL);
				spin_lock_init(&((*dest_crust)->lock));
				
				spin_lock(&pinf->inode_crust_struct->lock);
				
				dret=decrypt_crust_state( &(*dest_crust)->crust_state, 
						&iup->crust_state_link,
						&pinf->inode_crust_struct->crust_state, 
						iup->parent_cs_ver_for_cs_link
						);
				
				spin_unlock(&pinf->inode_crust_struct->lock);
				
				if(!dret){
					(*dest_crust)->max_ver=iup->max_ver;
					(*dest_crust)->count=1;
					(*dest_crust)->user_block=user_block_num;
					ret1=1;
					
					gwuc(sprintf(rep, "decrypting crust state from its link on user block and new crust_state with max_ver:%d is :",(*dest_crust)->max_ver));
					gwuc(rep+=strlen(rep));
					gwuc(printhexstring((char*)(&(*dest_crust)->crust_state), rep, 81));
					gwuc(rep+=strlen(rep));
				}
				else{
					kfree(*dest_crust);
					*dest_crust=0;
					gwuc(sprintf(rep, "unable to decrypting crust state form its link on user block "));
					gwuc(rep+=strlen(rep));
				}				
			}
			
			if(ret2<0){
				u1=get_users(pinf->users);
				
				gwuc(sprintf(rep, "get u1 from its parent * "));
				gwuc(rep+=strlen(rep));
			}
		}	
				
		//now we complete users
		if(ret2<0){
			int 	i,
				j;
			
			gwuc(sprintf(rep, "get u2 from user_block * "));
			gwuc(rep+=strlen(rep));	
			
			u2=kzalloc(sizeof(struct users), GFP_KERNEL);
			
			spin_lock_init(&u2->lock);
			
			u2->users_num=iup->num;
			if(u1)
				u2->users_num+=(u1->users_num-1);
			
			gwuc(sprintf(rep, " u2 -> usres_num : %u * ",u2->users_num));
			gwuc(rep+=strlen(rep));	
			
			u2->users=kzalloc(sizeof(unsigned int)*u2->users_num, GFP_KERNEL);
			u2->writability=kzalloc(sizeof(char)*u2->users_num, GFP_KERNEL);
			
			i=0;
			if(u1){
				//j=0 is for owner that will be added in next loop
				for(j=1; j<u1->users_num; j++, i++){
					u2->users[i]=u1->users[j];
					u2->writability[i]=u1->writability[j];
					
					gwuc(sprintf(rep, "parent user: uid:%d , write:%d # ",u2->users[i],u2->writability[i]));
					gwuc(rep+=strlen(rep));
				}
			}
			
			for(j=0; j<iup->num; i++, j++){
				u2->users[i]=iup->users_key[j].uid;
				u2->writability[i]=iup->users_key[j].writability;
				
				gwuc(sprintf(rep, "user block: uid:%d , write:%d # ",u2->users[i],u2->writability[i]));
				gwuc(rep+=strlen(rep));		
			}
			
			get_users(u2);
			if(u1)
				put_users(u1);
			
			ret2=1;
		}
				
		if(ret1<0){
			int	i,
				j=-1;
			struct rsa_key *key;
			char crust_hash[gsfs_hashlen];
			
			gwuc(sprintf(rep, "userblock num:%d, users in user_block:",iup->num));
			gwuc(rep+=strlen(rep));
						
			for(i=0; i<iup->num; i++){
				gwuc(sprintf(rep, "%d)uid:%u, ",i,iup->users_key[i].uid));
				gwuc(rep+=strlen(rep));
			
				if(iup->users_key[i].uid==current->loginuid){
					j=i;
					break;
				}
			}
			
			if(j==-1){
				br=3;
				
ret:				if(bh){
					unlock_buffer(bh);
					brelse(bh);
				}
				
				#ifdef test_get_inode_users_crust
					sprintf(rep,"(br=%d)No access because of ",br);
					rep+=strlen(rep);
					
					switch(br){
						case 1:
							sprintf(rep,"bad user block");
							break;
						case 2:
							sprintf(rep,"bad hash of user block");
							break;
						case 3:
							sprintf(rep,"no access for this user in user block");
							break;
						case 4:
							sprintf(rep,"no rsa_key");
							break;
						case 5:
							sprintf(rep,"error in decrypting rsa_crust");
							break;
						case 6:
							sprintf(rep,"bad hash of crust state in user_block");
							break;
					};
					rep+=strlen(rep);
					sprintf(rep, " * ");
					rep+=strlen(rep);
				#endif
				
				retval=rete;
				goto back;
			}
			
			key=get_rsa_key((struct GSFS_sb*)pin->i_sb->s_fs_info, current->loginuid, 1);
			if(!key || !key->key) {
				br=4;
				goto ret;
			}
			
			*dest_crust=kzalloc(sizeof(crust_struct), GFP_KERNEL);
			spin_lock_init(&((*dest_crust)->lock));
			
			(*dest_crust)->max_ver=iup->max_ver;
			(*dest_crust)->count=1;
			(*dest_crust)->user_block=user_block_num;
						
			spin_lock(&key->lock);
				
			i=rsa_decrypt_crust_state_from_user_block(&(*dest_crust)->crust_state,
								  iup->users_key[j].rsa_encrypted_key,
								  key->key);
			
			spin_unlock(&key->lock);
			
			if(i){
				kfree(*dest_crust);
				*dest_crust=0;
				br=5;
				goto ret;
			}
			
			get_crust_hash(crust_hash, &(*dest_crust)->crust_state);
			
			gwuc(sprintf(rep, "decrypting crust state from its user_block with max_ver:%d is:",(*dest_crust)->max_ver));
			gwuc(rep+=strlen(rep));
			gwuc(printhexstring((char*)(&(*dest_crust)->crust_state), rep, 81));
			gwuc(rep+=strlen(rep));
			
			gwuc(sprintf(rep, "crust hash from iup : "));
			gwuc(rep+=strlen(rep));
			gwuc(printhexstring(iup->crust_hash, rep, 16));
			gwuc(rep+=strlen(rep));
			
			gwuc(sprintf(rep, "crust hash from decrypted crust : "));
			gwuc(rep+=strlen(rep));
			gwuc(printhexstring(crust_hash, rep, 16));
			gwuc(rep+=strlen(rep));
			
			if(strncmp(crust_hash, iup->crust_hash, gsfs_hashlen)){
				kfree(*dest_crust);
				*dest_crust=0;
				br=6;
				goto ret;
			}			
			
			ret1=1;
		}
		
		unlock_buffer(bh);
		brelse(bh);
	}
	
back:	
	if(ret1==1){
		crust_struct ** cs;
		
		cs=kmalloc(sizeof(crust_struct*),GFP_KERNEL);
		*cs=*dest_crust;
		
		get_crust_struct(*cs);
		
		add_event_to_inode(pin, childindex, Crust_Struct_Set_VEvent, cs,
				   sizeof(crust_struct*), event_flag_from_disk );
					
		gwuc(sprintf(rep, "add crust struct to pin * "));
		gwuc(rep+=strlen(rep));
		
		ret1=0;
	}
	
	if(ret2==1)
		if(u1 || u2){
			struct users** uu;
			uu=kmalloc(sizeof(struct users*),GFP_KERNEL);
			
			if(u2)
				*dest_users=u2;
			else
				*dest_users=u1;
			
			*uu=*dest_users;
			
			add_event_to_inode(pin,childindex, Users_Set_VEvent, uu, 
					sizeof(struct users*), event_flag_from_disk );
			get_users(*uu);
			
			gwuc(sprintf(rep, "add users to pin * "));
			gwuc(rep+=strlen(rep));
			
			ret2=0;
		}
	
	if(!ret1 && !ret2)
		retval=0;
	
retend:	
	gwuc(sprintf(rep, "return with ret1:%d ret2:%d retval:%d", ret1, ret2, retval));
	gwuc(rep+=strlen(rep));
	gwuc(sprintf(rep, " and *dest_users: %lx *",(unsigned long)*dest_users));
	gwuc(rep+=strlen(rep));

	#ifdef test_get_inode_users_crust
		if(pin->i_ino<test_indecis_num && !test_indecis[pin->i_ino][1]){
			printk("<0>" "%s\n",repp);
			test_indecis[pin->i_ino][1]=1;
		}
		kfree(repp);
	#endif
	
	return retval;
}

#ifdef gsfs_test
	#define test_get_inode
#endif
#ifdef test_get_inode
	#define gwgi(p)	p
#else
	#define gwgi(p)
#endif

struct inode* GSFS_get_inode(struct GSFS_sb* gsb, unsigned int ino){
	struct inode			*in,
					*pin=0;
	struct GSFS_inode 		*inf,
					*pinf=0;
	struct GSFS_inode_disk_inf	*ind=0;
	int				int_test_req=1,			//integrity testing is required
					incom=0;
	char				metadata_hash_from_parent[gsfs_hashlen];
					
	#ifdef test_get_inode
	char	repp[1500],
		*rep=repp;
	#endif
	
	//printk("<0>" "0get_inode with ino: %u \n", ino);
	if(ino==0)
		return 0;
	
	in=iget_locked(gsb->sb,ino);
	
	if(!in)
		return in;
	
	if(!(in->i_state & I_NEW) && in->i_private){
		inf=in->i_private;
		
		if(inf->igflags & igflag_incomplete_inode){
			incom=1;
			ind=&inf->disk_info;
			pin=GSFS_get_inode(gsb, ind->parent_ino);
			
			gwgi(sprintf(rep,"get_inode for an incomplete inode and ino: %u * ", ino));
			gwgi(rep+=strlen(rep));
			
			down_write(&inf->inode_rwsem);
			
			goto incom1;
		}
		
		gwgi(sprintf(rep,"Not null i_private with no igflag_incomplete_inode * "));
		gwgi(rep+=strlen(rep));
	}
	
	gwgi(sprintf(rep,"get_inode with new inode and ino: %u * ", ino));
	gwgi(rep+=strlen(rep));
		
	if(!(in->i_state&I_NEW))
		return in;
	
	inf=kzalloc(sizeof(struct GSFS_inode),GFP_KERNEL);
	ind=&inf->disk_info;
	in->i_private=inf;
	
	inf->inode_bnr=get_block_number_of_inode(gsb,ino);
	
	inf->inode_bh=__bread(gsb->sb->s_bdev, inf->inode_bnr, Block_Size);
	
	if(!inf->inode_bh)
		goto fault;
	
	memcpy(ind,inf->inode_bh->b_data, min(sizeof(struct GSFS_inode_disk_inf), (unsigned long)Block_Size));
	brelse(inf->inode_bh);
	
	in->i_nlink=ind->inlink;
	in->i_blocks=ind->iblocks;
	in->i_bytes=ind->ibytes;
	in->i_uid=ind->iuid;
	inf->igflags=ind->igflags;
	in->i_mode=ind->imode;
	in->i_size=ind->isize;
	in->i_ctime.tv_sec=ind->ictime;
	in->i_mtime.tv_sec=ind->imtime;
	in->i_atime.tv_sec=ind->ictime;
	inf->current_grp=-1;
	
	GSFS_inode_init(in);
	
	gwgi(sprintf(rep,"reading ind from disk with inode block number: %u * ", inf->inode_bnr));
	gwgi(rep+=strlen(rep));
	
	//testing integrity of inode if its parent is secure or has secure child	
	if(unlikely(ino==1)){
		int_test_req=gsb->gsb_disk.root_inode_has_secure_child;
		
		if(int_test_req)
			int_test_req=get_inode_hash(in, metadata_hash_from_parent);
	}
	else{
		pin=GSFS_get_inode(gsb, ind->parent_ino);
		if(!pin)
			goto fault;
		
		//inf->parent=pin;
incom1:	
		pinf=(struct GSFS_inode*)pin->i_private;

		down_write(&pinf->inode_rwsem);
		
		if(incom)
			goto incom2;
		
		int_test_req=test_integrity_of_inode(in, pin, metadata_hash_from_parent);
	}
	
	gwgi(sprintf(rep,"response of int_test_req = %d * ", int_test_req));
	gwgi(rep+=strlen(rep));
	
	if(int_test_req<0)
		goto fault;
	
	if(int_test_req>0){
		char thash[gsfs_hashlen];
		
		if(likely(ino!=1))
			if(inf->igflags & (igflag_secure | igflag_has_sec_child))
				add_child_to_parent(pin, in);
		
		get_inode_metadata_hash_for_parent(in, thash);
		
		gwgi(sprintf(rep,"metadata hash from SAT : "));
		gwgi(rep+=strlen(rep));
		gwgi(printhexstring(metadata_hash_from_parent, rep, 16));
		gwgi(rep+=strlen(rep));
		gwgi(sprintf(rep,"metadata hash from inode : "));
		gwgi(rep+=strlen(rep));
		gwgi(printhexstring(thash, rep, 16));
		gwgi(rep+=strlen(rep));
		
		if(strncmp( metadata_hash_from_parent, thash, gsfs_hashlen))
			goto fault;
		
		//identifying users and crust_state of new inode		
		if(inf->igflags & igflag_secure){			
			int kr;
			
			gwgi(sprintf(rep,"inode is secure * "));
			gwgi(rep+=strlen(rep));
			
			//inf->inode_crust_struct=kmalloc(sizeof(struct crust_state),GFP_KERNEL);
			
			//setting crust last ver for call of get_inode_users and crust
			//if(inf->igflags & igflag_active_user_block)
			//	inf->crust_last_ver=ind->dir_inode_security.last_crust_ver;
incom2:			
			//for this function we need down_write of inode
			kr=get_inode_users_and_or_crust_state_from_parent(
				pin,					ind->index_in_parent,
				&inf->inode_crust_struct,		&inf->users,
				ind->dir_inode_security.user_block,	ind->dir_inode_security.inode_user_block_hash,
				ind->igflags,	-1,	1,	1);
			
			gwgi(sprintf(rep,"kr: %d * ",kr));
			gwgi(rep+=strlen(rep));
							
			if(kr==-1){	
					//can't set users and crust 
					
					//the following lines could be done by destroy inode 
					//and in this new igflag we save it for next user (if user logedin)
					
					//memset(inf->inode_crust_state, 0, sizeof(struct crust_state));
					//kfree(inf->inode_crust_state);
					//inf->inode_crust_state=0;
					
					gwgi(sprintf(rep,"fault from get_inode_users_and_crust_state_from_parent with kr:%d and users: %lx * ", kr, (unsigned long)inf->users ));
					gwgi(rep+=strlen(rep));
					
					if(!incom){
						inf->igflags|=igflag_incomplete_inode;
						
						add_inode_to_incom_inodes(gsb, in);
					}
					
					goto incomplete_inode_ret;
			}
			
			if(kr==-2)
				//user block isn't integrated
				goto fault;
						
			gwgi(sprintf(rep,"kr: %d, crust with last_ver %d : ", kr, inf->inode_crust_struct->max_ver));
			gwgi(rep+=strlen(rep));
			gwgi(printhexstring((char*)(&inf->inode_crust_struct->crust_state), rep, 81));
			gwgi(rep+=strlen(rep));	
			
			if(incom){				
				inf->igflags&=~igflag_incomplete_inode;
				
				//after user login get_inode_for_incom_inodes_of_uid that 
				//calls this function itself previously removes inode from incom_inodes
				
				//delete_inode_from_incom_inodes(gsb,in);
				
				goto incomplete_inode_ret;
			}
		}
	}

	//if(int_test_req==0 && pin){
	//	inf->parent=0;
	//	iput(pin);
	//}
	
incomplete_inode_ret:

	if(in->i_state & I_NEW)
		unlock_new_inode(in);

	if(pinf)
		up_write(&pinf->inode_rwsem);
	
	if(pin)
		iput(pin);
	
	if(incom)
		up_write(&inf->inode_rwsem);
	
	gwgi(printk("<0>" "%s\n",repp));
	
	return in;
	
fault:
	gwgi(printk("<0>" "%s\n",repp));
	
	if(pinf)
		up_write(&pinf->inode_rwsem);
	
	if(pin)
		iput(pin);
	
	iget_failed(in);		//make_bad_inode, unlock_new_inode, iput
	
	gwgi(printk("<0>" "Bad inode %lu, because of integrity.\n", in->i_ino));
	printk("<1>" "Bad inode %lu, because of integrity.\n", in->i_ino);
		
	return 0;
}

//get inl_key for a secure gdirent with encrypted inl
int get_users_and_decrypt_inl_of_gdirent(struct inode* dir,struct GSFS_dirent* gd,unsigned short index,
					struct users** users, struct gdirent_inl *inl, 
					char crust_can_get_event, char users_can_get_event){
	int ret;
	crust_struct* cs=0;
	char key[gsfs_aes_keylen];
	//crust_ver_type crust_ver;
	//struct GSFS_inode* dif=(struct GSFS_inode* )dir->i_private;
	
	//down_write(&dif->inode_rwsem);
	
	//if(gd->gd_flags & igflag_active_user_block)
	//	crust_ver=gd->gd_last_crust_ver;
	
	ret=get_inode_users_and_or_crust_state_from_parent(dir, index, &cs, users, 
							gd->gd_first_dir_security_fields.gd_user_block,
							gd->gd_first_dir_security_fields.gd_user_block_hash,
							gd->gd_flags,	-1,	crust_can_get_event,
							users_can_get_event);
	//up_write(&dif->inode_rwsem);
	//we dont need cust last ver that is in gdirent and maybe in crust_ver						
	
	//gt(printk("<0>" "get_users_and_decrypt_inl_of_gdirent for dir: %ld, index: %d, ret: %d, cs: %lx inl_ver:%d\n", dir->i_ino, index, ret,(unsigned long)cs, gd->gd_dirent_inl_ver));
	
	if(!ret && cs){
		spin_lock(&cs->lock);
		
		ret=crust_get_key_of_state(&cs->crust_state, gd->gd_dirent_inl_ver, key);
		
		spin_unlock(&cs->lock);
		
		if(!ret)
			ret=decrypt_inl(inl, &gd->gd_inl, key);
				
		put_crust_struct(cs);
	}
	
	cs=0;
	
	return ret;
}

#ifdef gsfs_test
	#define test_traverse_gds
#endif
#ifdef test_traverse_gds
	#define gwtt(p)	p
	#define gwttcount	rep+=strlen(rep); if(rep-repp>replen-300){ printk("<0>" "%s\n",repp);rep=repp;}
#else
	#define gwtt(p)
	#define gwttcount
#endif

//gibn=get_inode_by_name with these args: char* name, int len
//rd=readdir with these args:  loff_t* fpos, void* dirent, filldir_t filldir
//you should do down_write for in before calling
int traverse_all_gdirents_for_gibn_or_rd(struct inode* in, 
					 loff_t* fpos, void* dirent, filldir_t filldir, //rd inputs
					 char* name, int len){				//gibn inputs
	
	struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf *ind=&inf->disk_info;
	int 	res[GSFS_DEDICATION_ARRAY_LEN_BITS],
		res_num,
		res_index,
		dss,			//dir security state
		ret=0,
		current_gd_page=-1;
	struct GSFS_dirent *gda=0;
	struct buffer_head* gd_bh=0;
	unsigned short 	gdlen=gsfs_dirent_len,
			gd_per_page=Block_Size/gdlen;
	
	#ifdef test_traverse_gds
	int	replen=2000;
	char	*rep,
		*repp;
	#endif
	
	if(in->i_size==0)
		return 0;
	
	if(fpos)
		if(*fpos >= GSFS_MAX_INODES_PER_DIRECTORY)
			return 0;
	
	gwtt(rep=kmalloc(replen, GFP_KERNEL));
	gwtt(repp=rep);
	gwtt(sprintf(rep,"Traverse_all_gdirents for dir: %lu* ",in->i_ino));
	gwttcount;
	
	dss=0;	
	if(inf->igflags & igflag_has_sec_child)
		dss=1;
	if(inf->igflags & igflag_secure)
		dss=2;
	
	gwtt(sprintf(rep,"dss=%d * ",dss));
	gwttcount;
	
	res_num=get_all_set_indecis(ind->dir_inode_security.dir_inode_child_index_dedication_array, 
				    res, GSFS_DEDICATION_ARRAY_LEN_BITS);
	
	res_index=0;
	if(fpos){
		while(res_index<res_num && res[res_index]<*fpos)
			res_index++;
	
		if(res_index>=res_num)
			return 0;
	}
	
	gwtt(sprintf(rep,"res_index= %d * ",res_index));
	gwttcount;
	
	for( ; res_index<res_num && !ret ; res_index++){
		unsigned short 	index=res[res_index],
				gdpage=index/gd_per_page,
				gdoffset=index%gd_per_page;
		unsigned int	gdlen=0,
				gdino=0,
				k;
		char	clear_inl=0,
			//clear_key=0,
			*gdname=0,
			gdtype,
			gdhash_parent[gsfs_hashlen],
			gdhash_disk[gsfs_hashlen],
			gdbad_string[100];
			//key[gsfs_aes_keylen];
		struct GSFS_dirent	*gd=0;
		struct gdirent_inl inl;
		struct users*	users=0;
		
		memset(gdbad_string,0,100);
		
		if(fpos)
			*fpos=index;
		
		gwtt(sprintf(rep,"$$$ dirent with index:%d * ",index));
		gwttcount;
		
		if(current_gd_page!=gdpage){
			current_gd_page=gdpage;
			if(gd_bh)
				brelse(gd_bh);
			gd_bh=__bread(in->i_sb->s_bdev,get_dp_bn_of_in(in,gdpage),Block_Size);
			gda=(struct GSFS_dirent*)gd_bh->b_data;
		}
		
		gd=&gda[gdoffset];
		
		gwtt(sprintf(rep,"reading gd_page:%d and gdoffset: %d, gdflags:%x * ",gdpage ,gdoffset,gd->gd_flags));
		gwttcount;
		
		switch(dss){
			case 0:
			non_sec_gd_inl:
				//non_sec dir with no sec child
				gdname=gd->gd_name;
				gdlen=gd->gd_len;
				gdino=gd->gd_ino;
				break;
				
			case 1:
				//non_sec dir with some sec children
				k=is_set_one_index(ind->dir_inode_security.dir_inode_sec_has_sec_child_array, 
						   index, GSFS_DEDICATION_ARRAY_LEN_BITS);
					
				if(k && !(gd->gd_flags & (igflag_secure | igflag_has_sec_child))){
					gwtt(sprintf(rep,"parent secure array index hasn't been set* "));
					gwttcount;
					
					goto bad_gd;
				}
				
				if(!k && (gd->gd_flags & (igflag_secure | igflag_has_sec_child)) ){
					gwtt(sprintf(rep,"parent says that this child isn't secure, but itself say it is sec or has sec child* "));
					gwttcount;
					
					goto bad_gd;
				}
				
				if(!k)
					goto non_sec_gd_inl;
				
				goto verify_integ;
				
			case 2:
				//secdir with sec children
				if(!is_set_one_index(ind->dir_inode_security.dir_inode_sec_has_sec_child_array, 
					index, GSFS_DEDICATION_ARRAY_LEN_BITS)){
					gwtt(sprintf(rep,"parent sec index is not set * "));
					gwttcount;
					
					goto bad_gd;					
				}
				
				if(gd->gd_flags & igflag_encrypted_inl)
					if(inf->igflags & igflag_incomplete_inode)
						if(!(gd->gd_flags & igflag_active_user_block)){
						gwtt(sprintf(rep, "inf is incomplete and gd has no user block * "));
						goto bad_gd;
					}
				
			verify_integ:
				
				k=verify_and_get_integrity_for_child(GDirent_Integrity, in, index, gdhash_parent);
				
				gwtt(sprintf(rep,"verify integrity for gdirent with k: %d * ",k));
				gwttcount;
				
				if(k){
					printk("<1>" "Verifing integrity for gdirent with index: %d in inode %lu is failed.\n", index, in->i_ino);
					gwi(sprintf(gdbad_string, "parent secure array index hasn't been set"));
					goto cont;
				}
				
				get_gdirent_hash(gdhash_disk, gd);
				
				gwtt(sprintf(rep,"gdhash_disk:  * "));
				gwttcount;
				gwtt(printhexstring(gdhash_disk, rep, 16));
				gwttcount;
				
				gwtt(sprintf(rep,"gdhash_parent:  * "));
				gwttcount;
				gwtt(printhexstring(gdhash_parent, rep, 16));
				gwttcount;
				
				if(strncmp(gdhash_disk, gdhash_parent, gsfs_hashlen)){
					gwtt(sprintf(rep,"gdhash from disk and parent differs* "));
					gwttcount;
					gwi(sprintf(gdbad_string, "parent gdirent hash and disk gdirent hash differs"));
					goto bad_gd;
				}
				
				if(gd->gd_flags & igflag_has_sec_child){
					gwtt(sprintf(rep,"current gd has sec child and it is integrated. * "));
					gwttcount;
				
					goto non_sec_gd_inl;
				}
				
				if(!(gd->gd_flags & igflag_encrypted_inl)){
					gwtt(sprintf(rep,"current gd is sec and it is integrated but inl is not encrypted. * "));
					gwttcount;
				
					goto non_sec_gd_inl;
				}
				//secure gdirent with encrypted inl
				
				gwtt(sprintf(rep,"current gd is sec and it is integrated and is inl_enc. * "));
				gwttcount;				
				
				k=get_users_and_decrypt_inl_of_gdirent(in, gd, index, &users, &inl, 1, 1);
				
				gwtt(sprintf(rep,"ret of get_users_and_inl:%d and users: %lx * ",k,(unsigned long)users));
				gwttcount;
				
				if(!k){
					//clear_key=1;
					
					k=user_check_access(users, current->loginuid, MAY_READ);
					put_users(users);
					
					if(!k){
						
						gwtt(sprintf(rep,"you have access * "));
						gwttcount;
				
						//if(!decrypt_inl(&inl, &gd->gd_inl, key)){
							clear_inl=1;
							
							gdino=inl.ino;
							gdlen=inl.len;
							gdname=inl.name;
							
							gwtt(sprintf(rep,"decrypted inl: name:%s len:%d ino:%d * ",gdname, gdlen, gdino));
							gwttcount;
						//}
						//else{
						//	gwtt(sprintf(rep,"unable to decrypt * "));
						//	gwttcount;
							
						//	gwi(sprintf(gdbad_string, "unable to decrypt"));
						//	goto bad_gd;
						//}
					}
					else{
						gwtt(sprintf(rep,"you have not access * "));
						gwttcount;
				
						goto cont;
					}
				}	
				else{
					gwtt(sprintf(gdbad_string, "cant get key for this user * "));
					goto bad_gd;
				}
				
		}//end of switch
		
		if(fpos){
			//for readdir
			gdtype=DT_REG;
			if(gd->gd_flags & igflag_dir)
				gdtype=DT_DIR;
			
			ret=filldir(dirent, gdname, gdlen, *fpos, gdino, gdtype);
		}
		else{
			//for get_inode_by_name
			if(len == gdlen && !strncmp(name, gdname,len))
				 ret=gdino;
		}
		
	cont:
		//if(clear_key)
		//	memset(key, 0, gsfs_aes_keylen);
			
		if(clear_inl)
			memset(&inl, 0, gsfs_inl_len);
		
		#ifdef test_traverse_gds
			if(in->i_ino<test_indecis_num && !test_indecis[in->i_ino][0])
				printk("<0>" "%s\n",repp);
			rep=repp;
		#endif
		continue;
		
	bad_gd:
		gwi(printk("<0>" "Bad gdirent in reading gdirent from disk for dir %lu and index %d because %s.\n",in->i_ino, index, gdbad_string)); 
		goto cont;
	
	}
	
	if(gd_bh)
		brelse(gd_bh);
	
	if(fpos){
		if(res_index == res_num)
			*fpos = GSFS_MAX_INODES_PER_DIRECTORY;
		ret=0;
	}
	
	#ifdef test_traverse_gds
		if(!test_indecis[in->i_ino][0])
			test_indecis[in->i_ino][0]=1;
	#endif
	
	return ret;
}

//we assume that the access to dir inode is checked before this function calls
unsigned int get_inode_by_name(struct inode* dir, char* name, int len){
	int ret;
	struct GSFS_inode* inf=(struct GSFS_inode*)dir->i_private;
	
	down_write(&inf->inode_rwsem);
	
	ret=traverse_all_gdirents_for_gibn_or_rd(dir, 0, 0, 0, name, len);
	
	up_write(&inf->inode_rwsem);
	
	return ret;
}

struct dentry *GSFS_lookup(struct inode * dir, struct dentry *dentry, struct nameidata *nd){
	unsigned int ino;
	struct inode* ind;
	
	//gwi(printk("<0>" "lookup path:\"%s\", dir_ino: %ld, name:\"%s\"", nd->path.dentry->d_name.name, dir->i_ino ,(char*)dentry->d_name.name));
	
	ino=get_inode_by_name(dir, (char*)dentry->d_name.name,dentry->d_name.len);
	
	if(!ino)
		ind=NULL;
	else{
		ind= GSFS_get_inode((struct GSFS_sb*)dir->i_sb->s_fs_info,ino);
		
		if(ind==0)
			return ERR_PTR(-EIO);
	}
	
	return  d_splice_alias(ind, dentry);
}

//doesn't change the i_size but adds blocks to grps
int add_some_blocks_to_inode(struct inode* in,int some){
	int 	i,
		j,
		count,
		lastindex,
		startsec,
		startj;
	struct GSFS_sb* gsb=(struct GSFS_sb*)in->i_sb->s_fs_info;
	unsigned int* res;
	struct GSFS_inode *inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf *dinf=&inf->disk_info;
	
	if( some<=0 || dinf->grps_num>=GSFS_MAX_GROUP )
		return -1;
	
	res=kzalloc((1+some+1)*sizeof(unsigned int),GFP_KERNEL);
	
	i=BAT_get_some_blocks(gsb,some,res+1);
	
	if(i<some){
		for(j=1;j<=i;j++)
			clear_BAT(gsb,res[j],res[j]);
	
		kfree(res);
		return -1;
	}	
		
	if(dinf->grps_num>0){
		if(dinf->grps_num>1){
			lastindex=dinf->grps[2*dinf->grps_num-4]+1;
			count=dinf->grps[2*dinf->grps_num-2]-dinf->grps[2*dinf->grps_num-4];
		}
		else{
			lastindex=0;
			count=dinf->grps[2*dinf->grps_num-2]+1;
		}
		startj=1;		
		res[0]=dinf->grps[2*dinf->grps_num-1]+count-1;
		startsec=dinf->grps[2*dinf->grps_num-1];
		dinf->grps_num--;
	}
	else{
		lastindex=0;
		count=1;
		startsec=res[1];
		startj=2;
	}
	
	res[some+1]=0;	
		
	for(j=startj;j<=(some+1);j++){
		if(res[j]==(res[j-1]+1) && res[j]!=0){
			count++;
			continue;
		}
		dinf->grps[2*dinf->grps_num]=count+lastindex-1;
		dinf->grps[2*dinf->grps_num+1]=startsec;
	
		dinf->grps_num++;
		if(dinf->grps_num==GSFS_MAX_GROUP)
			break;
		lastindex+=count;
		count=1;
		startsec=res[j];
	}
	
	kfree(res);
	
	mark_inode_dirty(in);
	inf->igflags|=igflag_inode_metadata_changed;
	
	return j-2;
}

struct inode* GSFS_get_new_locked_inode_and_add_its_link(struct GSFS_sb* gsb, struct inode* parent,
							 struct dentry* dent, unsigned char flags){
	struct inode 			*inode=0;
	struct GSFS_inode	 	*inode_info=0,
					*parent_info=0;
	struct GSFS_inode_disk_inf	*inode_disk=0,
					*parent_disk=0;
	unsigned int	parent_ino=0,
			index_in_parent=0,
			ino,
			ibn,
			res[2],
			res_num;
	int 		i,
			j;
	
	//char	*new_owner_key;
	//struct 	crust_state* new_crust_state;
	
	if(parent)
		parent_ino=parent->i_ino;
	
	if(parent_ino){
		int 	gd_per_page = Block_Size/gsfs_dirent_len,
			current_gd_page;
			
		parent_info=(struct GSFS_inode*)parent->i_private;
		parent_disk=&parent_info->disk_info;
		
		down_write(&parent_info->inode_rwsem);
		
		if( parent_disk->dir_inode_security.child_num >= GSFS_MAX_INODES_PER_DIRECTORY )
			goto fail;
		
		if(parent_info->igflags & igflag_secure)
			flags|=igflag_secure;
		
		index_in_parent=set_and_get_one_index( parent_disk->dir_inode_security.dir_inode_child_index_dedication_array,
							GSFS_DEDICATION_ARRAY_LEN_BITS);
		
		//adding link initialization
		current_gd_page = index_in_parent/gd_per_page;
		if(current_gd_page >= parent->i_blocks){
			int	n,
				k;
			
			n=current_gd_page-parent->i_blocks+1;
			
			k=add_some_blocks_to_inode(parent,n);
			
			if(k!=n){
				printk("<0>" "Unable to add blocks, k=%d, n=%d, index_in_parent:%d\n",k,n, index_in_parent);
				//clear_one_index(parent_disk->dir_inode_security.dir_inode_child_index_dedication_array,
				//			index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
				index_in_parent=-1;
				goto fail;
			}
			
			parent->i_blocks+=n;
			
			parent->i_bytes=Block_Size;
			parent->i_size=parent->i_blocks<<Block_Size_Bits;
			//set_inode_ibytes_iblocks(parent);
		}
		
		/*
		if( parent->i_size==0 || (Block_Size-parent->i_bytes<gsfs_dirent_len) ){	
			if(Block_Size-parent->i_bytes<gsfs_dirent_len)
				parent->i_size+=Block_Size-parent->i_bytes;
			
			if(parent->i_blocks==parent_disk->grps_num)
				if(add_some_blocks_to_inode(parent,1)!=1)
					goto fail;
			
			parent->i_blocks++;
		}
		*/
	}
	
	if((flags & igflag_secure) && (flags & igflag_dir))
		res_num=2;
	else
		res_num=1;
	
	i=BAT_get_some_blocks(gsb,res_num,res);
	
	if(i<res_num){
bat_fail:	
		for(j=0;j<i;j++)
			BAT_clear_one_block(gsb,res[j]);
		
		goto fail;
	}
	
	ibn=res[0];
	ino=IAT_get_one_inode(gsb,ibn);
	if(!ino)
		goto bat_fail;
	
	inode=iget_locked(gsb->sb,ino);
	if(!inode || !(inode->i_state&I_NEW)){
		clear_IAT(gsb,ino);
		printk("<1>" "New inode is null or had been allocated !!!!!?????\n");
		goto bat_fail;
	}
	
	inode_info=kzalloc(sizeof(struct GSFS_inode),GFP_KERNEL);
	inode->i_private=inode_info;
	inode_disk=&inode_info->disk_info;
	inode_info->inode_bnr=ibn;
	
	inode_disk->parent_ino=parent_ino;
	inode_disk->ino=ino;
	
	if(parent_ino){
		parent_disk->dir_inode_security.child_num++;
		inode_disk->index_in_parent=index_in_parent;
		
		parent_info->igflags|=igflag_inode_metadata_changed;
	}
	
	inode_info->disk_info.grps_num=0;
	inode_info->current_grp=-1;
	
	inode->i_mode=0777;
	inode->i_nlink=0;
	inode->i_blkbits=Block_Size_Bits;
	inode->i_blocks=0;
	inode->i_size=0;
	inode->i_uid=current->loginuid;
	inode_disk->iuid=inode->i_uid;
	inode->i_bytes=0;
	inode->i_ctime.tv_sec=current_fs_time(inode->i_sb).tv_sec;
	inode->i_atime.tv_sec=inode->i_ctime.tv_sec;
	inode->i_mtime.tv_sec=inode->i_ctime.tv_sec;
	
	if( flags & igflag_dir){
		inode->i_mode|=S_IFDIR;
		inode_info->igflags|=igflag_dir;
	}
	else
		inode->i_mode|=S_IFREG;
	
	if(flags & igflag_secure){
		if(flags & igflag_dir)
			inode_disk->dir_inode_security.gdirent_hash_block=res[1];
				
		inode_disk->SAT_index=SAT_get_one_index(gsb);
		//printk("<0>" "sat_index:%u\n",inode_disk->SAT_index);
		if(inode_disk->SAT_index==-1)
			goto bat_fail;
	}
	else
		inode_disk->SAT_index=-1;
	
	if(likely(parent_ino)){
		if(parent_info->igflags & igflag_secure ){
			crust_struct **cs;
			struct users **uu;
			//parent inode is a secure parent 
			
			parent_disk->dir_inode_security.sec_has_sec_child_num++;
			set_one_index(parent_disk->dir_inode_security.dir_inode_sec_has_sec_child_array, 
				      inode_disk->index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
			      
			add_child_to_parent(parent, inode);
				
			//inode_info->parent=parent;
			
			//atomic_inc(&parent->i_count);
			
			inode_info->igflags |= ( igflag_secure  | 
						 igflag_encrypted_inl );
			
			inode_info->users = get_users(parent_info->users);
			
			uu=kmalloc(sizeof(struct users*), GFP_KERNEL);
			
			*uu=get_users(parent_info->users);
			
			add_event_to_inode(parent, inode_disk->index_in_parent, Users_Set_VEvent, 
					   uu, sizeof(struct users*), event_flag_from_disk );
			
			//if(!(parent_info->igflags & igflag_present_owner_key))
			//	read_owner_key_for_sec_inode(parent);
			
			/*
			new_owner_key=kzalloc(gsfs_aes_keylen, GFP_KERNEL);
			get_random_bytes(new_owner_key, gsfs_aes_keylen);
			inode_info->owner_key=kmalloc(gsfs_aes_keylen,GFP_KERNEL);
			memcpy(inode_info->owner_key, new_owner_key, gsfs_aes_keylen);
			inode_info->igflags|=igflag_present_owner_key;
			inode_info->add_event_to_parent( inode, Non_FL_Owner_Key_Set_Event, new_owner_key, gsfs_aes_keylen,0);
			
			new_crust_state=kzalloc(sizeof(struct crust_state),GFP_KERNEL);
			crust_get_next_state(new_crust_state, 0, new_owner_key);
			inode_info->inode_crust_state=kzalloc(sizeof(struct crust_state),GFP_KERNEL);
			memcpy(inode_info->inode_crust_state, new_crust_state, sizeof(struct crust_state));
			inode_info->crust_last_ver=0;
			inode_info->add_event_to_parent( inode, Non_FL_Crust_State_Set_Event, new_crust_state, 0,0);
			*/
			
			inode_info->inode_crust_struct=get_crust_struct(parent_info->inode_crust_struct);
			
			cs=kmalloc(sizeof(crust_struct*), GFP_KERNEL);
			
			*cs=get_crust_struct(parent_info->inode_crust_struct);
			
			add_event_to_inode(parent, inode_disk->index_in_parent, Crust_Struct_Set_VEvent, 
					   cs, sizeof(crust_struct*), event_flag_from_disk );
		}
		else{
			//parent inode is not a secure parent 
			clear_one_index(parent_disk->dir_inode_security.dir_inode_sec_has_sec_child_array, 
					inode_disk->index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
		}
		
		//clear_one_index(parent_disk->dir_inode_security.dir_inode_first_level_child_array, 
		//		inode_disk->index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
	}
	
	GSFS_inode_init(inode);
	
	//adding link
	if(likely(parent_ino)){
		struct GSFS_dirent* gd;
		struct buffer_head* bh;
		char*	gdhash=0;
		int 	gd_per_page=Block_Size/gsfs_dirent_len,
			current_gd_page=inode_disk->index_in_parent/gd_per_page;
		
		bh=__bread(parent->i_sb->s_bdev, get_dp_bn_of_in(parent, current_gd_page), Block_Size);
		if(!bh){
			printk("<1>" "Buffer head reading error while adding link.\n");
			goto fail;
		}
			
		gd=(struct GSFS_dirent*)(bh->b_data);
		gd+=(inode_disk->index_in_parent%gd_per_page);
		
		gd->gd_ino=ino;
		gd->gd_len=min((char)strlen(dent->d_name.name),(char)GSFS_MAX_NAME_LEN);
		memcpy(gd->gd_name,dent->d_name.name,gd->gd_len);
		gd->gd_name[(short)gd->gd_len]=0;
		
		if(inode_info->igflags & igflag_secure){
			struct gdirent_inl  	inl;
			char	key[gsfs_aes_keylen];
			
			spin_lock(&inode_info->inode_crust_struct->lock);
			
			crust_get_key_of_state(&inode_info->inode_crust_struct->crust_state, 
					       inode_info->inode_crust_struct->max_ver, key);
			
			gd->gd_dirent_inl_ver=inode_info->inode_crust_struct->max_ver;
					       
			spin_unlock(&inode_info->inode_crust_struct->lock);
					       
			encrypt_inl(&inl, &gd->gd_inl, key);
			
			memcpy(&gd->gd_inl, &inl, sizeof(struct gdirent_inl));
			
			gdhash=kzalloc(gsfs_hashlen, GFP_KERNEL);
			get_gdirent_hash(gdhash, gd);
		}
		
		gd->gd_flags=igflag_ondisk(inode_info->igflags);
			
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		if(gdhash)
			inode_info->add_event_to_parent(inode, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
			
		inode_inc_link_count(parent);
		parent->i_ctime.tv_sec=current_fs_time(parent->i_sb).tv_sec;
		
		//parent->i_size+=gsfs_dirent_len;
		//set_inode_ibytes_iblocks(parent);

		inode_inc_link_count(inode);
	}	
		
	mark_inode_dirty(inode);
	inode_info->igflags|=igflag_inode_metadata_changed;
	
	if(parent_ino){
		mark_inode_dirty(parent);
		parent_info->igflags|=igflag_inode_metadata_changed;
		
		up_write(&parent_info->inode_rwsem);
		
		//printk("<0>" "ino:%ld parent_count:%u\n",inode->i_ino, atomic_read(&parent->i_count));
	}
	
	return inode;
	
fail:	
	if(inode && parent_ino){
		remove_child_from_parent(parent,inode);
		//iput(inode_info->parent);
	}
	
	if(parent_info)
		up_write(&parent_info->inode_rwsem);
		
	if(inode)
		iget_failed(inode);
	
	return 0;
}

int GSFS_mkdir(struct inode * dir, struct dentry * dentry, int mode){
	struct inode* nino;
	int ret;
	
	//gwi(printk("<0>" "mkdir: %lu %s %d\n ",dir->i_ino,dentry->d_name.name,S_ISDIR(dir->i_mode)));
	
	nino=GSFS_get_new_locked_inode_and_add_its_link((struct GSFS_sb*)dir->i_sb->s_fs_info,
							dir, dentry, igflag_dir);
		
	if(nino){
		unlock_new_inode(nino);
		
		d_instantiate(dentry,nino);
		
		ret=0;
	}
	else
		ret=-1;
		
	return ret;
}

int GSFS_create(struct inode * dir, struct dentry * dentry, int mode, struct nameidata *nd){
	struct inode* nino;
	int ret;
	
	gwi(printk("<0>" "mkdir: %lu %s %d\n ",dir->i_ino,dentry->d_name.name,S_ISDIR(dir->i_mode)));
	
	nino=GSFS_get_new_locked_inode_and_add_its_link((struct GSFS_sb*)dir->i_sb->s_fs_info,
							dir, dentry, 0);
		
	if(nino){
		unlock_new_inode(nino);
		
		d_instantiate(dentry,nino);
		
		ret=0;
	}
	else
		ret=-1;
		
	return ret;
}

int GSFS_unlink(struct inode * dir, struct dentry *dentry){
	struct inode	*in=dentry->d_inode;
	
	struct GSFS_inode	*dirf=(struct GSFS_inode*)dir->i_private,
				*inf=(struct GSFS_inode*)in->i_private;
	int k;
	
	gwi(printk("<0>" "unlink %s from inode:%lu,%d %d\n",dentry->d_name.name,dir->i_ino,dir->i_nlink,in->i_nlink));
	
	if(inf->disk_info.dir_inode_security.child_num)
		return -ENOENT;
	
	down_write(&inf->inode_rwsem);
	
	down_write(&dirf->inode_rwsem);
		
	k=clear_one_index(dirf->disk_info.dir_inode_security.dir_inode_child_index_dedication_array, 
			inf->disk_info.index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
	if(likely(k>=1)){
		dirf->disk_info.dir_inode_security.child_num--;
		
		if(inf->igflags & (igflag_secure | igflag_has_sec_child)){

			k=clear_one_index(dirf->disk_info.dir_inode_security.dir_inode_sec_has_sec_child_array, 
					inf->disk_info.index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
			if(likely(k>=1))
				dirf->disk_info.dir_inode_security.sec_has_sec_child_num--;
			else
				gt(printk("<0>" "unlink for parent: %ld, child: %ld with no sec child index is set.\n", dir->i_ino, in->i_ino));
		}
	}
	else
		gt(printk("<0>" "unlink for parent: %ld, child: %ld with no child index is set.\n", dir->i_ino, in->i_ino));

	
	//in->i_ctime.tv_sec=current_fs_time(dir->i_sb).tv_sec;
	dir->i_mtime.tv_sec=current_fs_time(dir->i_sb).tv_sec;
	dir->i_ctime.tv_sec=dir->i_mtime.tv_sec;
	
	inode_dec_link_count(in);
	inode_dec_link_count(dir);
	
	dirf->igflags|=igflag_inode_metadata_changed;
	mark_inode_dirty(dir);
	
	up_write(&dirf->inode_rwsem);
	
	up_write(&inf->inode_rwsem);
	
	gt(printk("<0>" "unlink for parent: %ld, child: %ld, index in parent: %d, parent child_num: %d, parent: sec_child_num: %d, par_ar:%lx\n",
		dir->i_ino, in->i_ino, inf->disk_info.index_in_parent, dirf->disk_info.dir_inode_security.child_num,
		dirf->disk_info.dir_inode_security.sec_has_sec_child_num,
		(unsigned long)dirf->disk_info.dir_inode_security.dir_inode_child_index_dedication_array[0]));
	
	return 0;
}

int GSFS_rmdir(struct inode * dir, struct dentry *dentry){
	//struct inode* in=dentry->d_inode;
		
	//printk("<0>" "rmdir %s from inode:%lu,%d %d\n",dentry->d_name.name,dir->i_ino,dir->i_nlink,in->i_nlink);
	
	return GSFS_unlink(dir,dentry);
}

/*
int GSFS_link(struct dentry* odent, struct inode* ndir, struct dentry* ndent){
	int ret;
	struct inode* in=odent->d_inode;
	
	ret=GSFS_add_link(ndent,ndir,in->i_ino,DT_REG);
	if(!ret){
		atomic_inc(&in->i_count);
		d_instantiate(ndent,in);
		inode_inc_link_count(in);
	}			
	return ret;
}
*/

int GSFS_rename(struct inode* odir,struct dentry* odent, struct inode* ndir, struct dentry* ndent){
	int 	ret=-1;
	struct inode* dir=odir;
	struct GSFS_inode *dif=(struct GSFS_inode*)dir->i_private,
			  *inf=(struct GSFS_inode*)odent->d_inode->i_private;
	struct GSFS_inode_disk_inf *ind=&(inf->disk_info);
	
	struct GSFS_dirent* gd;
	struct buffer_head* bh;
	char*	gdhash=0;
	int 	gd_per_page=Block_Size/gsfs_dirent_len,
		current_gd_page;
	
	if(odir->i_ino != ndir->i_ino)
		return -ENOENT;
	
	if(inf->igflags & igflag_secure)
		if(ind->iuid != current->loginuid)
			return -1;
	
	down_write(&dif->inode_rwsem);
	current_gd_page=ind->index_in_parent/gd_per_page;
	
	bh=__bread(dir->i_sb->s_bdev, get_dp_bn_of_in(dir, current_gd_page), Block_Size);
	if(!bh){
		printk("<1>" "Buffer head reading error while renaming.\n");
		goto fail;
	}
		
	gd=(struct GSFS_dirent*)(bh->b_data);
	gd+=(ind->index_in_parent%gd_per_page);
	
	gd->gd_len=min((char)strlen(ndent->d_name.name),(char)GSFS_MAX_NAME_LEN);
	gd->gd_ino=ind->ino;
	memcpy(gd->gd_name,ndent->d_name.name,gd->gd_len);
	gd->gd_name[(short)gd->gd_len]=0;
		
	if((inf->igflags & igflag_secure) && (inf->igflags & igflag_encrypted_inl)){
		struct gdirent_inl  	inl;
		char	key[gsfs_aes_keylen];
		
		spin_lock(&inf->inode_crust_struct->lock);
		
		ret=crust_get_key_of_state(&inf->inode_crust_struct->crust_state, 
					inf->inode_crust_struct->max_ver, key);
		
		gd->gd_dirent_inl_ver=inf->inode_crust_struct->max_ver;
		
		//printk("<0>" "%s %d %d\n",gd->gd_name,gd->gd_ino,gd->gd_dirent_inl_ver);
		
		spin_unlock(&inf->inode_crust_struct->lock);
		
		if(ret)
			goto fail;
		
		encrypt_inl(&inl, &gd->gd_inl, key);
		
		memcpy(&gd->gd_inl, &inl, sizeof(struct gdirent_inl));
	}
	
	if(inf->igflags & (igflag_secure | igflag_has_sec_child)){
		gdhash=kzalloc(gsfs_hashlen, GFP_KERNEL);
		get_gdirent_hash(gdhash, gd);
	}
	
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	brelse(bh);
	
	if(gdhash)
		add_event_to_inode(dir, ind->index_in_parent, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
	
	dir->i_ctime.tv_sec=current_fs_time(dir->i_sb).tv_sec;
	
	dif->igflags|=igflag_inode_metadata_changed;
	mark_inode_dirty(dir);

	ret=0;
fail:
	up_write(&dif->inode_rwsem);
	
	return ret;
}

void GSFS_truncate(struct inode* in){
	struct GSFS_inode *inf=(struct GSFS_inode *)in->i_private;
	struct GSFS_inode_disk_inf *ind=&inf->disk_info;
	unsigned int nbn,
		     obn=in->i_blocks,
		     i=0,
		     len,
		     get=0;
		     
	//gwi(printk("<0>" "truncate inode # %lu to size %llu from %lu:%u\n",in->i_ino,in->i_size,in->i_blocks,in->i_bytes));
			
	if((in->i_size > in->i_sb->s_maxbytes) || (inf->igflags & igflag_dir)){
ret_no_change:	
		in->i_size=in->i_bytes;
		if(in->i_blocks!=0)
			in->i_size+=((in->i_blocks-1)<<Block_Size_Bits);
		return;
	}
	
	nbn=in->i_size>>Block_Size_Bits;
	
	if(in->i_size>(nbn<<Block_Size_Bits))
		nbn++;
	
	if(nbn==obn)
		goto ret;
	
	if(nbn>obn){
		if(ind->grps_num==0 || ind->grps[2*ind->grps_num-2]<(nbn-1)){
			if(ind->grps_num!=0)
				len=nbn-ind->grps[2*ind->grps_num-2]-1;
			else
				len=nbn;
			
			if(len){
				if(inf->igflags & igflag_secure)
					if(update_sec_reg_inode_hash_blocks(in, nbn-len, len))
						goto ret_no_change;
					
				if(add_some_blocks_to_inode(in,len)==-1)
					goto ret_no_change;
			}
		}
		i=0;
		while(i<ind->grps_num){
			if(ind->grps[2*i]>=(nbn-1)){
				in->i_blocks=nbn;//1+ind->grps[2*i];
				get=1;
				break;				
			}
			i++;
		}
		if(!get)
			in->i_size=in->i_blocks<<Block_Size_Bits;	
	}
	
	inf->current_grp=-1;
ret:	
	in->i_ctime.tv_sec=current_fs_time(in->i_sb).tv_sec;
	inf->igflags|=igflag_inode_metadata_changed;
	mark_inode_dirty(in);
	
	set_inode_ibytes_iblocks(in);
	
	return;
}

//exec permission for dirs is needed for cd into them
//or getting ino and other infos for dir's children
//mask=	MAY_READ, MAY_WRITE, MAY_EXEC
int GSFS_permission(struct inode *in , int mask){
	struct GSFS_inode *inf=(struct GSFS_inode *)in->i_private;
	int ret;
	int 	k1=0,
		k2=0;

	down_read(&inf->inode_rwsem);
	
	if(inf->igflags & igflag_incomplete_inode){
		if(mask & MAY_WRITE)
			ret=-1;
		else
			ret=0;
		goto back;
	}
	
	if(inf->igflags & igflag_secure)
		k1=1;
	
	if(inf->users)
		k2=1;
	
	if((k1 && !k2) || (!k1 && k2)){
		ret=-1;
		goto back;
	}
	
	if(!k1 && !k2){
		ret=0;
		goto back;
	}
	
	if(!(mask & MAY_WRITE))
		if(!(inf->igflags & igflag_encrypted_inl)){
			ret=0;
			goto back;
		}
		
	if(mask & MAY_WRITE)
		if(inf->igflags & igflag_dir)
			if(inf->igflags & igflag_secure){
				
				if(inf->disk_info.iuid != current->loginuid){
					ret=-1;
					goto back;
				}
				
				ret=0;
				goto back;
			}
			
	ret=user_check_access(inf->users, current->loginuid, mask);

back:	
	up_read(&inf->inode_rwsem);

	//printk("<0>" "read:%d write:%d exec:%d for inode:%ld and ret:%d and inf->users:%lx\n",mask&MAY_READ, mask&MAY_WRITE, mask&MAY_EXEC,in->i_ino,ret, (unsigned long)inf->users);

	return ret;
	
}

struct inode_operations GSFS_inode_operations={	
	.lookup	=	GSFS_lookup,
	.mkdir	=	GSFS_mkdir,
	.create	=	GSFS_create,
	.unlink	=	GSFS_unlink,
	.rmdir	=	GSFS_rmdir,
	//.link	=	GSFS_link,
	.truncate=	GSFS_truncate,
	.rename	=	GSFS_rename,
	.permission=	GSFS_permission,
};

#ifdef gsfs_test
	#define incom_test
#endif
#ifdef incom_test
	#define gwit(p)		p
#else
	#define gwit(p)	
#endif
void add_inode_to_incom_inodes(struct GSFS_sb* gsb, struct inode* in){
	struct incom_inodes	*incom,
				*prev=0;
	int j;
	
	gwit(printk("<0>" "add_inode_to_incom_inodes for in:%ld \n",in->i_ino));
	
	down_write(&gsb->incom_inodes_rwsem);
				
	incom=gsb->incom_inodes;

	while(incom){
		if(incom->count==MAX_INCOM_INODES && incom->next){
			prev=incom;
			incom=incom->next;
		}
		else
			break;
	}
	
	if(incom->count==MAX_INCOM_INODES)
		prev=incom;
	
	if(incom==0 || incom->count==MAX_INCOM_INODES){
		incom=kzalloc(sizeof(struct incom_inodes),GFP_KERNEL);
		if(prev)
			prev->next=incom;
		else
			gsb->incom_inodes=incom;		
	}
	
	for(j=0;j<MAX_INCOM_INODES;j++)
		if(!incom->inodes[j]){
			incom->inodes[j]=in;
			incom->count++;			
			gwit(printk("<0>" "add in j:%d\n",j));
			break;
		}
	
	up_write(&gsb->incom_inodes_rwsem);
	
	return;
}

void delete_inode_from_incom_inodes(struct GSFS_sb* gsb, struct inode* in){
	struct incom_inodes	*incom;
	
	gwit(printk("<0>" "delete_inode_from_incom_inodes for inode: %ld \n",in->i_ino));
	
	down_write(&gsb->incom_inodes_rwsem);
				
	incom=gsb->incom_inodes;

	while(incom){
		int j;
		
		for(j=0; j<MAX_INCOM_INODES; j++)
			if(incom->inodes[j] && incom->inodes[j]->i_ino == in->i_ino){
				incom->count--;
				incom->inodes[j]=0;
				gwit(printk("<0>" "found for j: %d.\n",j));
				goto ret;
			}
		
		incom=incom->next;
	}
	
	gwit(printk("<0>" "Incom inode: %ld is not in list \n",in->i_ino));
	
ret:
	up_write(&gsb->incom_inodes_rwsem);
	
	return;
}

void free_incom_inodes(struct GSFS_sb* gsb){
	struct incom_inodes	* incom,
				* next;
				
	down_write(&gsb->incom_inodes_rwsem);
	
	incom=gsb->incom_inodes;
	
	while(incom){
		next=incom->next;
		incom->next=0;
	
		kfree(incom);
		
		incom=next;
	}
	
	gsb->incom_inodes=0;
	
	up_write(&gsb->incom_inodes_rwsem);
	
	return;
}

void get_inode_for_incom_inodes_of_uid(struct GSFS_sb* gsb, uid_t uid){
	struct incom_inodes	* incom;
	unsigned int 	res[MAX_INCOM_INODES],
			resc=0,
			k;

	gwit(printk("<0>" "get_inode_for_incom_inodes_of_uid for uid:%u \n",uid));
	
	down_write(&gsb->incom_inodes_rwsem);
	
	incom=gsb->incom_inodes;
	
	while(incom){
		int j;
		
		for(j=0; j<MAX_INCOM_INODES; j++)
			if(incom->inodes[j]){
				struct inode* in=incom->inodes[j];
				struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
				
				if(inf->users){
					if(!user_check_access(inf->users, uid, MAY_READ)){
						res[resc++]= in->i_ino;
						
						incom->count--;
						incom->inodes[j]=0;
						
						gwit(printk("<0>" "inode :%ld \n",in->i_ino));
						
					}
				}
				else{
					gwit(printk("<0>" "incom inode %ld with no users\n",in->i_ino));
				}	
			}
		
		incom=incom->next;
	}
	
	up_write(&gsb->incom_inodes_rwsem);

	for(k=0; k<resc; k++){
		struct inode* in2;
		struct dentry *dent;
		
		in2=GSFS_get_inode(gsb, res[k]);
		
		dent=d_find_alias(in2);
		
		if(dent){
			shrink_dcache_parent(dent);
		
			dput(dent);
		}
		
		iput(in2);
	}
	
	return;
}
