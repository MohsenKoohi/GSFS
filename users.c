#include "gsfs.h"

int GSFS_add_new_user(struct super_block* sb,rsa_context*rsa,uid_t uid){
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	
	if(add_new_public_key(gsb,rsa,uid))
		return -1;
	
	printk("<1>" "GSFS: Adding new user public key with uid: %u\n",uid);
	
	return 0;
}

int GSFS_user_login(struct super_block* sb,rsa_context*rsa,uid_t uid, char* pt, int ptlen, char* ct, int ctlen ){
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	
	if(GSFS_add_new_private_key(sb, rsa, uid, pt, ptlen, ct, ctlen))
		return -1;
	
	printk("<1>" "GSFS: user with uid: %u logged in\n",uid);
	
	#ifdef gsfs_test
		clear_test_indecis();
	#endif
	
	get_inode_for_incom_inodes_of_uid(gsb, uid);
	
	return 0;
}

int GSFS_user_logout(struct super_block* sb,uid_t uid){
	int ret=0;
	
	//ret=GSFS_remove_private_key(sb,uid)
	
	return ret;
}

#ifdef gsfs_test
	#define test_add_user_block
#endif
#ifdef test_add_user_block
	#define gwt(p)	p
	#define gwtc	if(rep-repp>replen-300){printk("<0>" "%s\n",repp);rep=repp;}
#else
	#define gwt(p)
	#define gwtc
#endif

int add_user_block_to_inode(struct inode* in){
	struct GSFS_inode * inf=(struct GSFS_inode*)in->i_private,
			*pinf;
	struct GSFS_inode_disk_inf *ind=&inf->disk_info;
	struct super_block* sb=in->i_sb;
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	unsigned int 	res[2],
			i,
			err=0,
			res_num;
	struct buffer_head* bh;
	struct inode_user_page* iup;
	struct rsa_key* rkey;
	struct inode* pin=0;
	//struct crust_state* cs;
	unsigned short  gdlen,
			gd_per_page,
			gdoffset,
			gdpage,
			inf_index_in_parent;
	struct GSFS_dirent 	*gd;
	struct users** uev;
	crust_struct**	csev;
	char		*gdh;
	char *new_owner_key;
	
	#ifdef test_add_user_block
		int 	replen=1000;
		char 	*repp,
			*rep;
			
		rep=kzalloc(replen, GFP_KERNEL);
		repp=rep;
		sprintf(rep, "add_user_block_to_inode for inode with ino:%lu* ",in->i_ino);
		rep+=strlen(rep);
	#endif
	
	if(inf->igflags & igflag_first_level_sec_inode)
		return -1;
	
	//it is part of above condition
	//because in this version of GSFS you cant remove parent link  if exists
	//if(inf->igflags & igflag_active_parent_link)	
	//	return -1;
	
	if(ind->dir_inode_security.child_num!=0)
		return -1;
	
	if(inf->igflags & igflag_secure)
		if(ind->iuid != current->loginuid)
			return -1;
	
	pin=GSFS_get_inode(gsb, inf->disk_info.parent_ino);
	if(!pin)
		return -1;
	
	pinf=(struct GSFS_inode*)pin->i_private;
	if(!pinf)
		return -1;
	
	down_write(&inf->inode_rwsem);
	
	//it is impossible for inode to have owner_key because it hasnt user_block
	/*
	if(inf->igflags & igflag_secure)
		if(!inf->inode_crust_struct->owner_key){
			int ret;
			
			spin_lock(&inf->inode_crust_struct->lock);
			
			ret=read_owner_key_for_crust_struct(inf->inode_crust_struct, in->i_sb, 
							    ind->dir_inode_security.inode_user_block_hash);
			
			spin_unlock(&inf->inode_crust_struct->lock);
			
			if(ret){
				
				gwt(sprintf(rep,"Unable to read owner key * "));
				gwt(rep+=strlen(rep));	
				
				up_write(&inf->inode_rwsem);
				
				goto bad_ret;
			}
		}
	*/
	down_write(&pinf->inode_rwsem);
	
	gwt(sprintf(rep,"parent inode: %ld * ",pin->i_ino));
	gwt(rep+=strlen(rep));
		
	//allocating 1 block for user_block of inode or 2 block if it isnot secure
	if(inf->igflags & igflag_secure)
		res_num=1;
	else
		res_num=2;
	
	i=BAT_get_some_blocks(gsb, res_num, res);
	if(i!= res_num){
		res_num=i;
		goto bat_ret;
	}
		
	ind->dir_inode_security.user_block=res[0];
	if(!(inf->igflags & igflag_secure)){
		ind->dir_inode_security.gdirent_hash_block=res[1];
		
		ind->SAT_index=SAT_get_one_index(gsb);
		if(ind->SAT_index==-1)
			goto bat_ret;
		
		gwt(sprintf(rep,"new SAT_index: %d * ",ind->SAT_index));
		gwt(rep+=strlen(rep));
	}
	gwt(sprintf(rep,"allocated blocks num:%d and blocks: %d %d* ",res_num,res[0],res[1]));
	gwt(rep+=strlen(rep));
	
	//preparing owner key
	new_owner_key=kzalloc(gsfs_aes_keylen, GFP_KERNEL);
	if(inf->igflags & igflag_secure){
		//its parent has its owner key
		/*			
		ind->dir_inode_security.last_crust_ver++;
		
		memset(inf->inode_crust_state, 0, sizeof(struct crust_state));
		*/
		get_random_bytes(new_owner_key, gsfs_aes_keylen);
		
		put_crust_struct(inf->inode_crust_struct);
		
		gwt(sprintf(rep,"inode is secure, new owner_key: * "));
		gwt(rep+=strlen(rep));	
		gwt(printhexstring(new_owner_key, rep, 16));
		gwt(rep+=strlen(rep));	
		gwtc;
		
	}
	else{
		//parent is not secure and we should allocate new owner key
	
		add_child_to_parent(pin, in);
		//inf->parent=pin;
				
		//ind->dir_inode_security.last_crust_ver=0;
		
		//new_owner_key=kzalloc(gsfs_aes_keylen, GFP_KERNEL);
		get_random_bytes(new_owner_key, gsfs_aes_keylen);
		
		//inf->owner_key=kzalloc(gsfs_aes_keylen,GFP_KERNEL);
		//memcpy(inf->owner_key,new_owner_key,gsfs_aes_keylen);
		
		//inf->igflags|=igflag_present_owner_key;
		
		//add_event_to_inode(pin, ind->index_in_parent, Non_FL_Owner_Key_Set_Event, 
		//			   new_owner_key, gsfs_aes_keylen, event_flag_from_disk);

		//inf->inode_crust_state=kzalloc(sizeof(struct crust_state),GFP_KERNEL);
		
		gwt(sprintf(rep,"inode isnt secure, allocating owner_key: "));
		gwt(rep+=strlen(rep));
		gwt(printhexstring(new_owner_key, rep, 16));
		gwt(rep+=strlen(rep));	
		gwtc;
	}
	
	//inf->crust_last_ver=ind->dir_inode_security.last_crust_ver;
	
	//now the owner_key is available
	//we should create new crust struct and complete iup field 
	//with encrypted owner key and crust state 
	inf->inode_crust_struct=kzalloc(sizeof(crust_struct), GFP_KERNEL);
	
	spin_lock_init(&inf->inode_crust_struct->lock);
	
	inf->inode_crust_struct->owner_key=new_owner_key;
	inf->inode_crust_struct->count=1;
	inf->inode_crust_struct->max_ver=0;
	inf->inode_crust_struct->user_block=ind->dir_inode_security.user_block;
	
	crust_get_next_state(&inf->inode_crust_struct->crust_state, inf->inode_crust_struct->max_ver, 
			     inf->inode_crust_struct->owner_key);
	
	//add_event for new crust_struct
	csev=kzalloc(sizeof(crust_struct*), GFP_KERNEL);
	
	*csev=get_crust_struct(inf->inode_crust_struct);
	
	add_event_to_inode(pin, ind->index_in_parent, Crust_Struct_Set_VEvent, csev, 
			   sizeof(crust_struct*), event_flag_from_disk);
	
	rkey=get_rsa_key(gsb, current->loginuid, 0);
	if(!rkey || !rkey->key)
		goto cs_bat_ret;
	
	gwt(sprintf(rep,"new crust_state: "));
	gwt(rep+=strlen(rep));
	gwt(printhexstring((char*)(&inf->inode_crust_struct->crust_state), rep, 81));
	gwt(rep+=strlen(rep));
	gwtc;
	
	bh=__bread(in->i_sb->s_bdev, ind->dir_inode_security.user_block, Block_Size);
	iup=(struct inode_user_page*)bh->b_data;
	lock_buffer(bh);
	
	iup->num=1;
	iup->max_ver=0;
		
	spin_lock(&rkey->lock);
	
	iup->owner_key.uid=current->loginuid;
	iup->owner_key.writability=1;	
	i=rsa_encrypt_owner_key_for_user_block(iup->owner_key.rsa_encrypted_key, new_owner_key, rkey->key);

	iup->users_key[0].uid=current->loginuid;
	iup->users_key[0].writability=1;	
	i=rsa_encrypt_crust_state_for_user_block(iup->users_key[0].rsa_encrypted_key, 
						 &inf->inode_crust_struct->crust_state, rkey->key);
	
	spin_unlock(&rkey->lock);
	
	get_crust_hash(iup->crust_hash, &inf->inode_crust_struct->crust_state);
	
	gwt(sprintf(rep,"crust hash for user_block: "));
	gwt(rep+=strlen(rep));
	gwt(printhexstring(iup->crust_hash, rep, 16));
	gwt(rep+=strlen(rep));		
	
	get_user_block_hash(ind->dir_inode_security.inode_user_block_hash, bh->b_data);
	
	gwt(sprintf(rep,"user block hash: "));
	gwt(rep+=strlen(rep));
	gwt(printhexstring(ind->dir_inode_security.inode_user_block_hash, rep, 16));
	gwt(rep+=strlen(rep));		
		
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	brelse(bh);
	
	//changing flags of inode
	inf->igflags |=(igflag_secure | igflag_first_level_sec_inode );
	//inf->igflags &= ~(igflag_active_parent_link);		//it is impossible because above filter :)
	
	gwt(sprintf(rep,"new flags of inf: %x * ",inf->igflags));
	gwt(rep+=strlen(rep));
		
	//now we should set users field of our new be secured inode
	if(inf->users)		//only if it was secure previously
		put_users(inf->users);
	
	inf->users=kzalloc(sizeof(struct users), GFP_KERNEL);
	
	spin_lock_init(&inf->users->lock);
	
	inf->users->users_num=1;
	inf->users->users=kzalloc(sizeof(unsigned int), GFP_KERNEL);
	inf->users->writability=kzalloc(sizeof(char), GFP_KERNEL);
	inf->users->users[0]=current->loginuid;
	inf->users->writability[0]=1;
	get_users(inf->users);
	
	uev=kzalloc(sizeof(struct users*),GFP_KERNEL);
	*uev=inf->users;
	add_event_to_inode(pin, ind->index_in_parent, Users_Set_VEvent, uev, 
			   sizeof(struct users*), event_flag_from_disk);
	get_users(inf->users);
	
	gwt(sprintf(rep,"new users struct count:%d* ",inf->users->count));
	gwt(rep+=strlen(rep));
	gwtc;
	
	//now we should update gdirent fields
	gdlen=gsfs_dirent_len;
	gd_per_page=Block_Size/gdlen;
	gdpage=ind->index_in_parent/gd_per_page;
	gdoffset=ind->index_in_parent%gd_per_page;
	
	gwt(sprintf(rep,"reading gdpage:%d gdoffset:%d* ", gdpage, gdoffset));
	gwt(rep+=strlen(rep));
	
	bh=__bread(pin->i_sb->s_bdev,get_dp_bn_of_in(pin,gdpage),Block_Size);
	gd=(struct GSFS_dirent*)bh->b_data;
	gd+=gdoffset;
	
	//encrypting inl if its parent is not sec and it is sec
	if(!(pinf->igflags & igflag_secure)){
		//if(!(inf->igflags & igflag_encrypted_inl)){
			struct gdirent_inl inl;
			char key[gsfs_aes_keylen];
			
			spin_lock(&inf->inode_crust_struct->lock);
			
			crust_get_key_of_state(&inf->inode_crust_struct->crust_state, 
					       inf->inode_crust_struct->max_ver, key);
			
			spin_unlock(&inf->inode_crust_struct->lock);
			
			encrypt_inl(&inl, &gd->gd_inl, key);		
			
			memcpy(&gd->gd_inl, &inl, gsfs_inl_len);
			
			inf->igflags |=igflag_encrypted_inl;
			
			gd->gd_dirent_inl_ver=inf->inode_crust_struct->max_ver;
					
			memset(key, 0, gsfs_aes_keylen);
			memset(&inl, 0, gsfs_inl_len);
			
			gwt(sprintf(rep,"encrypt_inl to: "));
			gwt(rep+=strlen(rep));
			gwt(printhexstring((char*)&gd->gd_inl, rep, 64));
			gwt(rep+=strlen(rep));
			gwtc;
		//}
	}
	else{
		//we should change inl key to be compatible with new allocated crust
		//if(!(inf->igflags & igflag_first_level_sec_inode)){
			struct gdirent_inl inl;
			char key[gsfs_aes_keylen];
			
			spin_lock(&pinf->inode_crust_struct->lock);
			crust_get_key_of_state(&pinf->inode_crust_struct->crust_state, 
					       gd->gd_dirent_inl_ver, key);
			spin_unlock(&pinf->inode_crust_struct->lock);
			
			decrypt_inl(&inl, &gd->gd_inl, key);
			
			gwt(sprintf(rep,"decrypt previous inl from: "));
			gwt(rep+=strlen(rep));
			gwt(printhexstring((char*)&gd->gd_inl, rep, 64));
			gwt(rep+=strlen(rep));
			gwtc;
			
			gwt(sprintf(rep,"to: "));
			gwt(rep+=strlen(rep));
			gwt(printhexstring((char*)&inl, rep, 64));
			gwt(rep+=strlen(rep));
			gwtc;
			
			spin_lock(&inf->inode_crust_struct->lock);
			crust_get_key_of_state(&inf->inode_crust_struct->crust_state, 
					       inf->inode_crust_struct->max_ver, key);
			gd->gd_dirent_inl_ver=inf->inode_crust_struct->max_ver;
			spin_unlock(&inf->inode_crust_struct->lock);
			
			encrypt_inl(&gd->gd_inl, &inl, key);
			
			gwt(sprintf(rep,"and decrypt_inl to: "));
			gwt(rep+=strlen(rep));
			gwt(printhexstring((char*)&gd->gd_inl, rep, 64));
			gwt(rep+=strlen(rep));
			gwtc;
			
			memset(key, 0, gsfs_aes_keylen);
		//}
	}
	////setting gdflags
	gd->gd_flags=igflag_ondisk(inf->igflags);
	
	//cleaning previous gd_child_security_fields
	//gd->gd_child_security_fields.gd_dirent_crust_link_pver=0;
	//memset(&gd->gd_child_security_fields.gd_crust_state_link, 0, sizeof(struct crust_state));
	
	////we remain owner_key_link if it exists even we write it in user block for owner
	//if(!(pinf->igflags & igflag_secure ))
	//	memset(gd->gd_child_security_fields.gd_owner_key_link, 0, gsfs_aes_keylen);
	
	////setting user_block and crust_ver and copying user_block_hash 
	gd->gd_first_dir_security_fields.gd_user_block=ind->dir_inode_security.user_block;

	memcpy(gd->gd_first_dir_security_fields.gd_user_block_hash, 
	       ind->dir_inode_security.inode_user_block_hash, gsfs_hashlen);
	
	gdh=kzalloc(gsfs_hashlen, GFP_KERNEL);
	get_gdirent_hash(gdh, gd);
	
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	brelse(bh);
	
	gwt(sprintf(rep,"new gdirent hash: "));
	gwt(rep+=strlen(rep));
	gwt(printhexstring(gdh, rep, 16));
	gwt(rep+=strlen(rep));
	gwtc;
	
	inf->add_event_to_parent(in, GDirent_Hash_Changed_Event, gdh, gsfs_hashlen, 0);
	
	//making inode dirty
	mark_inode_dirty(in);
	inf->igflags|=igflag_inode_metadata_changed;
	
	inf_index_in_parent=ind->index_in_parent;
	
	up_write(&pinf->inode_rwsem);
	up_write(&inf->inode_rwsem);
	
	down_write(&pinf->inode_rwsem);
	
	//set_one_index(pinf->disk_info.dir_inode_security.dir_inode_first_level_child_array, 
	//			inf_index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
		
	//now we should update parents igflag for integrity	
	do{
		unsigned int pino;
		unsigned short inf_igflags;
		
		gwt(sprintf(rep,"updating parent with ino: %lu* ",pin->i_ino));
		gwt(rep+=strlen(rep));
		gwtc;
		
		mark_inode_dirty(pin);
		pinf->igflags |= igflag_inode_metadata_changed;
				
		if(pinf->igflags & igflag_secure)
			break;
			
		set_one_index(pinf->disk_info.dir_inode_security.dir_inode_sec_has_sec_child_array, 
				inf_index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
		pinf->disk_info.dir_inode_security.sec_has_sec_child_num++;
		
		if(pinf->igflags & igflag_has_sec_child)
			break;
		
		pinf->igflags|=igflag_has_sec_child;
		
		//allocating blocks for data,mdata,gdirent hash of pin
		res_num=BAT_get_some_blocks(gsb, 1, res);
		if(res_num!=1)
			goto bat_ret2;
		
		pinf->disk_info.dir_inode_security.gdirent_hash_block=res[0];
		pinf->disk_info.SAT_index=SAT_get_one_index(gsb);
		
		gwt(sprintf(rep,"allocated blocks for parents: %u and new sat_index: %u * ",res[0],pinf->disk_info.SAT_index));
		gwt(rep+=strlen(rep));
		gwtc;
		
		if(pin->i_ino==1){
			gsb->gsb_disk.root_inode_has_secure_child=0x37;
			gsb->sgflags|=sgflag_sb_ondisk;
			sb->s_dirt=1;
			
			break;
		}
		
		//else we should update gdirent of pin in its pin:
		pino=pinf->disk_info.parent_ino;
			
		in=pin;
		inf=pinf;
		pin=GSFS_get_inode(gsb, pino);
		pinf=(struct GSFS_inode*)pin->i_private;
		
		inf_index_in_parent=inf->disk_info.index_in_parent;
		inf_igflags=inf->igflags;
		
		down_write(&pinf->inode_rwsem);
		
		add_child_to_parent(pin, in);
		//inf->parent=pin;
		
		up_write(&pinf->inode_rwsem);
		
		up_write(&inf->inode_rwsem);
		
		iput(in);
		
		down_write(&pinf->inode_rwsem);
		
		gdpage=inf_index_in_parent/gd_per_page;
		gdoffset=inf_index_in_parent%gd_per_page;
		
		bh=__bread(pin->i_sb->s_bdev,get_dp_bn_of_in(pin,gdpage),Block_Size);
		gd=(struct GSFS_dirent*)bh->b_data;
		gd+=gdoffset;
		
		gd->gd_flags=igflag_ondisk(inf_igflags);
		
		gdh=kzalloc(gsfs_hashlen, GFP_KERNEL);
		get_gdirent_hash(gdh, gd);
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		gwt(sprintf(rep,"gdhash sent to parent of inode: %lx is:* ",in->i_ino));
		gwt(rep+=strlen(rep));
		gwt(printhexstring(gdh, rep, 16));
		gwt(rep+=strlen(rep));
		gwtc;
		
		add_event_to_inode(pin, inf_index_in_parent, GDirent_Hash_Changed_Event, gdh, gsfs_hashlen, 0);
		
	}while(pin);
	
	if(pin){
		up_write(&pinf->inode_rwsem);
		
		//if(pinf->igflags & igflag_secure)
		iput(pin);
	}
	
	gwt(printk("<0>" "%s * ret= 0 *\n",repp));
	gwt(memset(repp,0,replen));
	gwt(kfree(repp));
	
	return 0;

cs_bat_ret:
	put_crust_struct(inf->inode_crust_struct);
	
bat_ret:
	err=-1;
			
bat_ret2:

	for(i=0;i<res_num;i++)
		BAT_clear_one_block(gsb, res[i]);
	
	if(pin){
		up_write(&pinf->inode_rwsem);
		
		iput(pin);
	}
	
	if(err==-1)
		up_write(&inf->inode_rwsem);

	gwt(printk("<0>" "%s * ret= -1 *\n",repp));
	gwt(memset(repp,0,replen));
	gwt(kfree(repp));
	
	return -1;
}

int GSFS_make_sec(struct super_block*sb, char* dest){
	struct nameidata nd;
	int ret;
	struct inode* in;
	
	ret=path_lookup(dest, LOOKUP_DIRECTORY, &nd);
	if(ret)
		return ret;
	
	in=nd.path.dentry->d_inode;
		
	path_put(&nd.path);
	
	if(in->i_sb->s_magic != GSFS_MAGIC)
		return -1;
	
	if(in->i_ino == 1)
		return -1;
	
	atomic_inc(&in->i_count);
	
	ret=add_user_block_to_inode(in);
	
	iput(in);
	
	return ret;
}

#ifdef gsfs_test
	#define test_add_users_to_inode
#endif
#ifdef test_add_users_to_inode
	#define gwti(p)	p
#else
	#define gwti(p)
#endif

void make_inl_unencrypted(struct inode* in){
	struct GSFS_inode		*inf=(struct GSFS_inode*)in->i_private,
					*pinf;
	struct inode			*pin;
	struct GSFS_inode_disk_inf	*ind=&inf->disk_info;
	struct super_block		*sb=in->i_sb;
	struct buffer_head		*bh;
	
	#ifdef test_add_users_to_inode
	char 	repp[1000],
		*rep=repp;
	#endif
	
	if(!(inf->igflags & igflag_secure))
		return;
	
	if(!(inf->igflags & igflag_encrypted_inl))
		return;
	
	gwti(sprintf(rep,"make_inl_unencrypted for inode %lu with flags %x * ",in->i_ino, inf->igflags));
	gwti(rep+=strlen(rep));
		
	down_write(&inf->inode_rwsem);
	
	inf->igflags &= ~igflag_encrypted_inl;
	
	pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);	
	//inf->parent;
	
	if(pin){
		short	gdlen=gsfs_dirent_len;
		short	gd_per_page=Block_Size/gdlen;
		int	gdpage,
			gdoffset;
		struct GSFS_dirent* gd;
		char 	*gdhash;
		struct gdirent_inl inl;
		char key[gsfs_aes_keylen];
		
		pinf=(struct GSFS_inode*)pin->i_private;
		down_write(&pinf->inode_rwsem);
		
		gdpage=ind->index_in_parent/gd_per_page;
		gdoffset=ind->index_in_parent%gd_per_page;
		
		bh=__bread(sb->s_bdev, get_dp_bn_of_in(pin,gdpage), Block_Size);
		gd=(struct GSFS_dirent*)bh->b_data;
		gd+=gdoffset;
		
		//changing flags
		gd->gd_flags &= ~igflag_encrypted_inl;
		
		gwti(sprintf(rep,"new gdflags : %x * ",gd->gd_flags));
		gwti(rep+=strlen(rep));
		
		//decrypting inl
		spin_lock(&inf->inode_crust_struct->lock);
		
		crust_get_key_of_state(&inf->inode_crust_struct->crust_state, gd->gd_dirent_inl_ver, key);
		
		spin_unlock(&inf->inode_crust_struct->lock);
		
		decrypt_inl(&inl, &gd->gd_inl, key);		
		
		gwti(sprintf(rep,"decrypting inl to name:%s, len:%d, in:%u * ", inl.name, inl.len, inl.ino));
		gwti(rep+=strlen(rep));		
		
		memcpy(&gd->gd_inl, &inl, gsfs_inl_len);
				
		//new hash of gdirent
		//adding new hash of gdirent to parent
		gdhash=kzalloc(gsfs_hashlen, GFP_KERNEL);
		get_gdirent_hash(gdhash, gd);
		
		gwti(sprintf(rep,"new gdhash :  "));
		gwti(rep+=strlen(rep));
		gwti(printhexstring(gdhash, rep, 16));
		gwti(rep+=strlen(rep));
		
		inf->add_event_to_parent(in, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
				
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		mark_inode_dirty(pin);
		
		up_write(&pinf->inode_rwsem);
	}	
	
	inf->igflags |=igflag_inode_metadata_changed;
	mark_inode_dirty(in);
	
	up_write(&inf->inode_rwsem);
	
	gwti(printk("<0>" "%s\n",repp));
	
	if(pin){
		make_inl_unencrypted(pin);
		iput(pin);
	}
	return;
}

//you should not get inf->inode_rwsem 
//you should not get pinf->inode_rwsem before calling
void update_inode_users(struct inode* in, int update_users){
	struct GSFS_inode		*inf=(struct GSFS_inode*)in->i_private,
					*pinf=0;
	struct inode			*pin=0;
	struct GSFS_inode_disk_inf	*ind=&inf->disk_info;
	int k;
	
	#ifdef test_add_users_to_inode
	char 	repp[1000],
		*rep=repp;
	#endif
		
	gwti(sprintf(rep, "update_inode_users for inode: %ld * ", in->i_ino));
	gwti(rep+=strlen(rep));
	
	down_write(&inf->inode_rwsem);
	
	if(update_users){
		if(inf->igflags & igflag_active_user_block)
			if(!(inf->igflags & igflag_active_parent_link)){
				gwti(sprintf(rep, " This inode hasn't parent link and therefore its users will not change * "));
				gwti(rep+=strlen(rep));
				
				goto ret_sem;
			}

		pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);
		//inf->parent;
		if(!pin){
			gwti(sprintf(rep, "No parent * "));
			gwti(rep+=strlen(rep));
			
			goto ret_sem;
		}
		
		pinf=(struct GSFS_inode*)pin->i_private;
		if(!pinf){
			gwti(sprintf(rep, "No parent inf* "));
			gwti(rep+=strlen(rep));
			
			goto ret_sem;
		}
		
		//we havent got it from parent
		down_write(&pinf->inode_rwsem);
		
		put_users(inf->users);
		//put_crust_struct(inf->inode_crust_struct);
		
		inf->users=0;
		//inf->inode_crust_struct=0;
		
		k=get_inode_users_and_or_crust_state_from_parent(pin, ind->index_in_parent, 0, &inf->users, 
							ind->dir_inode_security.user_block,
							ind->dir_inode_security.inode_user_block_hash, 
							inf->igflags, 0, 1, 0);
		
		//we havent got it from parent
		up_write(&pinf->inode_rwsem);
		
		gwti(sprintf(rep, "k= %d * ",k));
		gwti(rep+=strlen(rep));
	}
	
	if(ind->dir_inode_security.child_num){
		int res_num;
		
		res_num=avl_tree_get_size(inf->children);
		
		if(res_num){	
			atn** res;
			int i;
			
			res=kzalloc(sizeof(atn *)*res_num,GFP_KERNEL);
			avl_tree_get_all_nodes(inf->children, res, res_num);
			
			up_write(&inf->inode_rwsem);
			
			for(i=0;i<res_num;i++){
				struct inode* in=res[i]->data->inode;
				
				if(in)
					update_inode_users(in, 1);
				
				#ifdef test_add_users_to_inode
					if(in)
						sprintf(rep, "update_inode_users for inode: %ld * ",res[i]->data->inode->i_ino);
					else
						sprintf(rep, "update_inode_users for children with no inode ???? * ");
					rep+=strlen(rep);
				#endif
			}
			
			kfree(res);
			
			down_write(&inf->inode_rwsem);
		}
	}

ret_sem:
	up_write(&inf->inode_rwsem);
	
	if(pin)
		iput(pin);
	
	gwti(printk("<0>" "%s\n",repp));
	
	return ;
}

int add_users_to_inode(struct inode* in, unsigned int* uids, unsigned int* writes, int num){
	struct GSFS_inode		*inf=(struct GSFS_inode*)in->i_private,
					*pinf=0;
	struct inode			*pin=0;
	struct GSFS_inode_disk_inf	*ind=&inf->disk_info;
	struct super_block		*sb=in->i_sb;
	struct GSFS_sb			*gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct buffer_head		*bh;
	int ret=-1;
	struct inode_user_page* iup;
	struct rsa_key *rkey;
	int i;
	struct users 	*pusers,
			**uev;
	crust_struct 	**csev=0;
	
	#ifdef test_add_users_to_inode
	char	*repp,
		*rep;
		
	repp=kzalloc(3000,GFP_KERNEL);
	rep=repp;
	#endif
	
	if(!(inf->igflags & igflag_secure))
		return -1;
	
	if(inf->igflags & igflag_incomplete_inode)
		return -1;
	
	if(ind->iuid != current->loginuid)
		return -1;
	
	//in this version we cant add user block to inode with children
	//because this inode can access other children of parent if we get it the crust_state of parent
	//we should find some solutions for this problem
	if(!(inf->igflags & igflag_first_level_sec_inode))
		if(ind->dir_inode_security.child_num)
			return -1;
	
	gwti(sprintf(rep,"add_users_to_inode for inode: %lu and flags: %x * ",in->i_ino, inf->igflags));
	gwti(rep+=strlen(rep));
	
	down_write(&inf->inode_rwsem);
		
	//if in is not first_level_sec we should make it fls
	if(!(inf->igflags & igflag_first_level_sec_inode)){
		unsigned int res;
		
		//block dedication for user_block
		if( BAT_get_some_blocks(gsb, 1, &res)!=1)
			goto retsem;
		
		ind->dir_inode_security.user_block=res;
		//ind->dir_inode_security.last_crust_ver=inf->crust_last_ver;
		
		gwti(sprintf(rep,"adding user block to inode with number: %u * ",res));
		gwti(rep+=strlen(rep));	
	}
	
	//adding users to user block user block
	bh=__bread(sb->s_bdev, ind->dir_inode_security.user_block, Block_Size);
	
	lock_buffer(bh);
	iup=(struct inode_user_page*)bh->b_data;
	
	gwti(sprintf(rep,"openning user block with number %u * ",ind->dir_inode_security.user_block));
	gwti(rep+=strlen(rep));
	
	//adding owner_rsa_key if user block is new === always
	if(inf->igflags & igflag_first_level_sec_inode){
		//comparing user_block hash
		char ubhash[gsfs_hashlen];
		int hr;
				
		hr=get_user_block_hash(ubhash, bh->b_data);
		if(hr || strncmp(ubhash, ind->dir_inode_security.inode_user_block_hash, gsfs_hashlen)){
			printk("<1>" "Bad user block hash for inode:%ld\n", in->i_ino);
			
			gwti(sprintf(rep,"Bad user block hash for inode * "));
			gwti(rep+=strlen(rep));
			
			unlock_buffer(bh);
			brelse(bh);
			goto retsem;
		}
		/*
		if(!inf->inode_crust_struct->owner_key)
			if(read_owner_key_for_crust_struct(inf->inode_crust_struct, in->i_sb, 
							ind->dir_inode_security.user_block_hash)){
				
				gwti(sprintf(rep,"unable to read owner_key of crsut_struct * "));
				gwti(rep+=strlen(rep));
			
				goto retsem;
			}	
		*/
	}
	else{	
		/*
		if(!(inf->igflags & igflag_present_owner_key))
			if(read_owner_key_for_sec_inode(in)){
				gwti(sprintf(rep,"Cannot read owner key of inode * "));
				gwti(rep+=strlen(rep));
	
				brelse(bh);
				goto  retsem;
			}
		*/
		crust_struct *pcs;
		
		//we should allocate new crust_struct and write first fields of iup
		put_crust_struct(inf->inode_crust_struct);
		
		inf->inode_crust_struct=kzalloc(sizeof(crust_struct), GFP_KERNEL);
	
		spin_lock_init(&inf->inode_crust_struct->lock);
		
		inf->inode_crust_struct->owner_key=kmalloc(gsfs_aes_keylen, GFP_KERNEL);
		get_random_bytes(inf->inode_crust_struct->owner_key, gsfs_aes_keylen);
		
		gwti(sprintf(rep,"user_block is new and new owner_key  : "));
		gwti(rep+=strlen(rep));
		gwti(printhexstring(inf->inode_crust_struct->owner_key, rep, 16));
		gwti(rep+=strlen(rep));
				
		inf->inode_crust_struct->count=1;
		inf->inode_crust_struct->max_ver=0;
		inf->inode_crust_struct->user_block=ind->dir_inode_security.user_block;
		
		crust_get_next_state(&inf->inode_crust_struct->crust_state, inf->inode_crust_struct->max_ver, 
				inf->inode_crust_struct->owner_key);
		
		gwti(sprintf(rep,"* inode_crust_state: "));
		gwti(rep+=strlen(rep));
		gwti(printhexstring((char*)&inf->inode_crust_struct->crust_state, rep, 81));
		gwti(rep+=strlen(rep));		
				
		//add_event for new crust_struct
		csev=kmalloc(sizeof(crust_struct*), GFP_KERNEL);
		
		*csev=get_crust_struct(inf->inode_crust_struct);
		//adding later when we will have pinf;
		
		iup->num=1;
		iup->max_ver=0;
		
		rkey=get_rsa_key(gsb, current->loginuid, 0);
	
		spin_lock(&rkey->lock);
		
		iup->owner_key.uid=current->loginuid;
		iup->owner_key.writability=1;	
		i=rsa_encrypt_owner_key_for_user_block(iup->owner_key.rsa_encrypted_key,
						inf->inode_crust_struct->owner_key, rkey->key);
		
		gwti(sprintf(rep,"adding rsa encrypted owner key: %d * ",i));
		gwti(rep+=strlen(rep));
		
		iup->users_key[0].uid=current->loginuid;
		iup->users_key[0].writability=1;	
		i=rsa_encrypt_crust_state_for_user_block(iup->users_key[0].rsa_encrypted_key, 
							&inf->inode_crust_struct->crust_state, rkey->key);
		
		gwti(sprintf(rep,"adding rsa encrypted crust: %d * ",i));
		gwti(rep+=strlen(rep));
		
		spin_unlock(&rkey->lock);
		
		get_crust_hash(iup->crust_hash, &inf->inode_crust_struct->crust_state);
		
		gwti(sprintf(rep,"adding crust hash : "));
		gwti(rep+=strlen(rep));
		gwti(printhexstring(iup->crust_hash, rep, 16));
		gwti(rep+=strlen(rep));
		
		iup->max_ver=inf->inode_crust_struct->max_ver;
	//}	
	
		//encrypting crust_state for parent access using iup
		pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);
	//if(pin && !(inf->igflags & igflag_first_level_sec_inode)){
		pinf=(struct GSFS_inode*)pin->i_private;
				
		down_read(&pinf->inode_rwsem);
		
		pcs=get_crust_struct(pinf->inode_crust_struct);
		
		up_read(&pinf->inode_rwsem);
		
		spin_lock(&pcs->lock);
		
		iup->parent_cs_ver_for_cs_link=pcs->max_ver;
		
		i=encrypt_crust_state(&iup->crust_state_link, &inf->inode_crust_struct->crust_state, 
				    &pcs->crust_state, pcs->max_ver);
		
		spin_unlock(&pcs->lock);
		
		put_crust_struct(pcs);
		
		inf->igflags |= igflag_active_parent_link;
		
		gwti(sprintf(rep,"encrypt_crust_state to iup_link with ver: %d and i:%d and link: ",iup->parent_cs_ver_for_cs_link, i));
		gwti(rep+=strlen(rep));
		gwti(printhexstring((char*)&iup->crust_state_link, rep, 81));
		gwti(rep+=strlen(rep));
	}
	
	for(i=0; i<num; i++){
		int j;
		int l;
		
		gwti(sprintf(rep,"adding rsa encrypted crust state for user: %u  ",uids[i]));
		gwti(rep+=strlen(rep));
		
		rkey=get_rsa_key(gsb, uids[i], 0);
		if(!rkey || !rkey->key){
			gwti(sprintf(rep,"failed because of no rsa key * "));
			gwti(rep+=strlen(rep));
		
			writes[i]=1;
			continue;
		}
		
		for(j=0;j<iup->num;j++)
			if(iup->users_key[j].uid==uids[i]){
				if(iup->users_key[j].writability || !writes[i]){
					gwti(sprintf(rep,"failed because of added previously * "));
					gwti(rep+=strlen(rep));
			
					writes[i]=2;
					j=-1;
				}
				else
					goto change_write;
				break;
			}			
		if(j==-1)
			continue;
		
		if(iup->num >= GSFS_MAX_USERS_PER_INODE){
			gwti(sprintf(rep,"failed because of no place for this and other uids * "));
			gwti(rep+=strlen(rep));
		
			while(i<num)
				writes[i++]=3;
			break;
		}
		
		j=iup->num;
		iup->num++;
change_write:
		iup->users_key[j].uid=uids[i];
		iup->users_key[j].writability=writes[i];
		
		spin_lock(&rkey->lock);
		
		l=rsa_encrypt_crust_state_for_user_block(iup->users_key[j].rsa_encrypted_key, 
							 &inf->inode_crust_struct->crust_state, rkey->key);
		
		spin_unlock(&rkey->lock);
		
		writes[i]=0;
		
		gwti(sprintf(rep,"succeeded with l:%d * ",l));
		gwti(rep+=strlen(rep));
	}
	
	gwti(sprintf(rep,"new iup->num = %d ",iup->num));
	gwti(rep+=strlen(rep));
		
	get_user_block_hash(ind->dir_inode_security.inode_user_block_hash, bh->b_data);
	
	gwti(sprintf(rep,"new user block hash: "));
	gwti(rep+=strlen(rep));
	gwti(printhexstring(ind->dir_inode_security.inode_user_block_hash, rep, 16));
	gwti(rep+=strlen(rep));
	
	//updating users field of inf
	put_users(inf->users);
	
	inf->users=kzalloc(sizeof(struct users), GFP_KERNEL);
	
	gwti(sprintf(rep,"making new users for inf: %lx * ",(unsigned long)inf->users));
	gwti(rep+=strlen(rep));
		
	spin_lock_init(&inf->users->lock);
	
	inf->users->users_num=iup->num;
	pusers=0;
	
	if(!pin)
		pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);
	
	if(pin && (inf->igflags & igflag_active_parent_link)){
		pinf=(struct GSFS_inode*)pin->i_private;
		
		down_read(&pinf->inode_rwsem);
		
		if(pinf->igflags & igflag_secure)
			if(inf->igflags & igflag_active_parent_link)
				pusers=get_users(pinf->users);
		
		up_read(&pinf->inode_rwsem);
	}
	
	if(pusers)
		inf->users->users_num += pusers->users_num-1;
	
	inf->users->users=kzalloc(sizeof(unsigned int)*inf->users->users_num, GFP_KERNEL);
	inf->users->writability=kzalloc(sizeof(unsigned char)*inf->users->users_num, GFP_KERNEL);
	inf->users->count=2;
	
	i=0;
	if(pusers){
		int j;
		
		gwti(sprintf(rep,"Adding parent users : * "));
		gwti(rep+=strlen(rep));
		
		//j=0 is for owner that will be added in next loop of iup users :)
		for( j=1; j<pusers->users_num; i++, j++){
			inf->users->users[i]=pusers->users[j];
			inf->users->writability[i]=pusers->writability[j];
			
			gwti(sprintf(rep,"uid :%u, write:%d # ",inf->users->users[i],inf->users->writability[i]));
			gwti(rep+=strlen(rep));	
		}
		
		put_users(pusers);
		pusers=0;
	}
	
	if(iup->num){
		int j;
		
		gwti(sprintf(rep," * users of iup: "));
		gwti(rep+=strlen(rep));	
		
		for( j=0; j<iup->num; i++, j++){
			inf->users->users[i]=iup->users_key[j].uid;
			inf->users->writability[i]=iup->users_key[j].writability;
			
			gwti(sprintf(rep,"uid :%u, write:%d # ",inf->users->users[i],inf->users->writability[i]));
			gwti(rep+=strlen(rep));	
		}
	}
	
	//setting users_event(uev) to be added later 
	uev=kmalloc(sizeof(struct users*),GFP_KERNEL);
	*uev=inf->users;
	
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	brelse(bh);	
	
	//updating gdirent of in to be compatible with ind for user-block and user-block-hash 
	if(likely(pin)){
		short	gdlen=gsfs_dirent_len;
		short	gd_per_page=Block_Size/gdlen;
		int	gdpage,
			gdoffset;
		struct GSFS_dirent* gd;
		char 	*gdhash;
		int k;
		
		pinf=(struct GSFS_inode*)pin->i_private;
		down_write(&pinf->inode_rwsem);
		
		gdpage=ind->index_in_parent/gd_per_page;
		gdoffset=ind->index_in_parent%gd_per_page;
		
		bh=__bread(sb->s_bdev, get_dp_bn_of_in(pin,gdpage), Block_Size);
		gd=(struct GSFS_dirent*)bh->b_data;
		gd+=gdoffset;
		
		//decrypting gd_inl and encrypting it with new crust_state for this inode if 
		//it is newly first_level_sec
		if(!(inf->igflags & igflag_first_level_sec_inode)){
			struct gdirent_inl inl;
			char key[gsfs_aes_keylen];
			
			spin_lock(&pinf->inode_crust_struct->lock);
			crust_get_key_of_state(&pinf->inode_crust_struct->crust_state, 
					       gd->gd_dirent_inl_ver, key);
			spin_unlock(&pinf->inode_crust_struct->lock);
			
			decrypt_inl(&inl, &gd->gd_inl, key);
			
			spin_lock(&inf->inode_crust_struct->lock);
			crust_get_key_of_state(&inf->inode_crust_struct->crust_state, 
					       inf->inode_crust_struct->max_ver, key);
			gd->gd_dirent_inl_ver=inf->inode_crust_struct->max_ver;
			spin_unlock(&inf->inode_crust_struct->lock);
			
			encrypt_inl(&gd->gd_inl, &inl, key);
			
			memset(key, 0, gsfs_aes_keylen);
		}
				
		gd->gd_first_dir_security_fields.gd_user_block=ind->dir_inode_security.user_block;
		gd->gd_flags |= igflag_first_level_sec_inode;
		if(inf->igflags & igflag_active_parent_link)
			gd->gd_flags|=igflag_active_parent_link;
		
		memcpy(gd->gd_first_dir_security_fields.gd_user_block_hash, 
		       ind->dir_inode_security.inode_user_block_hash, gsfs_hashlen);
		
		//adding new hash of gdirent to parent
		gdhash=kzalloc(gsfs_hashlen, GFP_KERNEL);
		get_gdirent_hash(gdhash, gd);
		k=inf->add_event_to_parent(in, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
		
		gwti(sprintf(rep," * updating gdirent with new flag %x, new hash of user block with response: %d * ",gd->gd_flags, k));
		gwti(rep+=strlen(rep));
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		//adding uev to pin
		add_event_to_inode( pin, ind->index_in_parent, Users_Set_VEvent, uev, 
				    sizeof(struct users*), event_flag_from_disk);
		
		//adding csev to pin
		if(csev)
			add_event_to_inode(pin, ind->index_in_parent, Crust_Struct_Set_VEvent, csev, 
				sizeof(crust_struct*), event_flag_from_disk);
		
		mark_inode_dirty(pin);
		
		up_write(&pinf->inode_rwsem);
	}
	else
		goto retsem;

	//updating parents gdirent to be not inl_encrypted if user block is newly added
	if(!(inf->igflags & igflag_first_level_sec_inode)){
		
		inf->igflags |= igflag_first_level_sec_inode;
		
		//set_one_index(pinf->disk_info.dir_inode_security.dir_inode_first_level_child_array, 
		//		ind->index_in_parent, GSFS_DEDICATION_ARRAY_LEN_BITS);
	
		make_inl_unencrypted(pin);
	}
	
	gwti(sprintf(rep,"new inf->igflags: %x * ",inf->igflags));
	gwti(rep+=strlen(rep));
	
	//updating children users to accept new users added above
	//becareful that if crust_struct is changed there is no child for inode in this version :) :)
	if(ind->dir_inode_security.child_num){
		int res_num;
		
		res_num=avl_tree_get_size(inf->children);
	
		if(res_num){	
			
			up_write(&inf->inode_rwsem);
			
			update_inode_users(in, 0);
			
			down_write(&inf->inode_rwsem);
		}
	}	
	
	ret=0;
	
	mark_inode_dirty(in);
	inf->igflags|=igflag_inode_metadata_changed;
	
retsem:
	up_write(&inf->inode_rwsem);
	
	if(pin)
		iput(pin);
	
	gwti(printk("<0>" "%s\n",repp));
	gwti(kfree(repp));
	
	return ret;
}

int GSFS_add_users_to_inode(struct super_block* sb, char* dest, unsigned int* uids, 
				unsigned int* writes, int num){
	struct nameidata nd;
	int ret;
	struct inode* in;
	
	ret=path_lookup(dest, LOOKUP_DIRECTORY, &nd);
	if(ret)
		return ret;
	
	in=nd.path.dentry->d_inode;
		
	path_put(&nd.path);
	
	if(in->i_sb->s_magic != GSFS_MAGIC)
		return -1;
	
	if(in->i_ino == 1)
		return -1;

	atomic_inc(&in->i_count);
	
	ret=add_users_to_inode(in, uids, writes, num);
	
	iput(in);
	
	return ret;
}

#ifdef gsfs_test
	#define test_user_rovocation
#endif
#ifdef test_user_rovocation
	#define gwur(p)	p
#else
	#define gwur(p)
#endif

char*	add_crust_max_ver_and_update_parent_link(struct inode* child, crust_struct* pcs){
	struct super_block 	*sb=child->i_sb;
	struct GSFS_sb		*gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct GSFS_inode	*cinf=(struct GSFS_inode*)child->i_private;
	struct GSFS_inode_disk_inf *cind=&cinf->disk_info;
	struct buffer_head	*bh;
	struct inode_user_page 	*iup;
	char 			*ubhash=0;
	struct rsa_key 		*rkey;
	int 			ret,
				i;
	
	#ifdef test_user_rovocation
		char	*rep,
			*repp;
			
		rep=kmalloc(3000, GFP_KERNEL);
		repp=rep;
		
		sprintf(rep, "add_crust_max_ver_and_update_parent_link for inode: %ld * ",child->i_ino);
		rep+=strlen(rep);
	#endif
	
	down_write(&cinf->inode_rwsem);
	
	//preparing owner_key
	if(read_owner_key_for_crust_struct(cinf->inode_crust_struct, child->i_sb, 
					   cind->dir_inode_security.inode_user_block_hash)){
		gwur(sprintf(rep, "unable to read owner key * "));
		gwur(rep+=strlen(rep));
		
		goto ret_sem;
	}
	
	spin_lock(&cinf->inode_crust_struct->lock);
	
	//calculating new crust state of crust struct
	if(cinf->inode_crust_struct->max_ver== crust_maxver)
		goto brs_lock;
	
	cinf->inode_crust_struct->max_ver++;
	
	ret=crust_get_next_state(&cinf->inode_crust_struct->crust_state,  cinf->inode_crust_struct->max_ver, 
				 cinf->inode_crust_struct->owner_key);
	if(ret){
		gwur(sprintf(rep, "unable to crust_get_next_state * "));
		gwur(rep+=strlen(rep));
		
brs_lock:
		spin_unlock(&cinf->inode_crust_struct->lock);
		goto ret_sem;
	}
	
	gwur(sprintf(rep, "nex crust state with ver: %d is:  ",cinf->inode_crust_struct->max_ver));
	gwur(rep+=strlen(rep));
	gwur(printhexstring((char*)&cinf->inode_crust_struct->crust_state, rep, 81));
	gwur(rep+=strlen(rep));
		
	//updating user-block
	rkey=get_rsa_key(gsb, current->loginuid, 0);
	if(!rkey || !rkey->key)
		goto brs_lock;
	
	spin_lock(&rkey->lock);
				
	bh=__bread(sb->s_bdev, cind->dir_inode_security.user_block, Block_Size);

	lock_buffer(bh);

	iup=(struct inode_user_page*)bh->b_data;
	
	iup->max_ver=cinf->inode_crust_struct->max_ver;
	
	/*
	iup->owner_key.uid=current->loginuid;
	iup->owner_key.writability=1;	
	ret=rsa_encrypt_owner_key_for_user_block(iup->owner_key.rsa_encrypted_key, inf->owner_key, rkey->key);
	*/
	
	iup->users_key[0].uid=current->loginuid;
	iup->users_key[0].writability=1;	
	ret=rsa_encrypt_crust_state_for_user_block(iup->users_key[0].rsa_encrypted_key, 
						   &cinf->inode_crust_struct->crust_state, rkey->key);
	
	spin_unlock(&rkey->lock);
	
	get_crust_hash(iup->crust_hash, &cinf->inode_crust_struct->crust_state);
	
	gwur(sprintf(rep, "rsa_encrypt: %d  ",ret));
	gwur(rep+=strlen(rep));
	
	for(i=1; i<iup->num; i++){
		rkey=get_rsa_key(gsb, iup->users_key[i].uid, 0);
		if(!rkey || !rkey->key)
			continue;
		
		spin_lock(&rkey->lock);
		
		//iup->users_key[new_iup_count].uid=iup->users_key[i].uid;
		//iup->users_key[new_iup_count].writability=iup->users_key[i].writability;
		ret=rsa_encrypt_crust_state_for_user_block(iup->users_key[i].rsa_encrypted_key, 
							   &cinf->inode_crust_struct->crust_state, rkey->key);
		
		spin_unlock(&rkey->lock);
		
		gwur(sprintf(rep, "rsa_encrypt for user:%u ret:%d  ",iup->users_key[i].uid,ret));
		gwur(rep+=strlen(rep));
	}
	
	//updating parent_link in user_block if it is existed
	if(cinf->igflags & igflag_active_parent_link){	//always one
		
		spin_lock(&pcs->lock);
		
		iup->parent_cs_ver_for_cs_link=pcs->max_ver;
		
		encrypt_crust_state(&iup->crust_state_link, &cinf->inode_crust_struct->crust_state, 
				    &pcs->crust_state, pcs->max_ver);
		
		spin_unlock(&pcs->lock);
		
		gwur(sprintf(rep, "encrypt crust state link: "));
		gwur(rep+=strlen(rep));
		gwur(printhexstring((char*)&iup->crust_state_link, rep, 81));
		gwur(rep+=strlen(rep));
	}
	
	get_user_block_hash(cind->dir_inode_security.inode_user_block_hash, bh->b_data);
		
	spin_unlock(&cinf->inode_crust_struct->lock);
	
	gwur(sprintf(rep, "new user_block_hash:"));
	gwur(rep+=strlen(rep));
	gwur(printhexstring(cind->dir_inode_security.inode_user_block_hash, rep, 16));
	gwur(rep+=strlen(rep));
	
	ubhash=kmalloc(gsfs_hashlen, GFP_KERNEL);
	memcpy(ubhash, cind->dir_inode_security.inode_user_block_hash, gsfs_hashlen);
	
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	
ret_sem:
	mark_inode_dirty(child);
	cinf->igflags |=igflag_inode_metadata_changed;
	
	up_write(&cinf->inode_rwsem);
	
	gwur(printk("<0>" "%s * ubhash: %lx *\n", repp, (unsigned long)ubhash));
	gwur(kfree(repp));
	
	return ubhash;
}

int traverse_all_gdirents_for_updating_links_and_users(struct inode* in, int update_users){
	struct GSFS_inode* 		inf=(struct GSFS_inode*)in->i_private;
	struct GSFS_inode_disk_inf 	*ind=&inf->disk_info;
	unsigned int 			*children_inos,
					child_count=0,
					i;
	int 				*res,
					res_num,
					res_index,
					cin_count=0,
					first_round,	
					dss,			//dir security state
					//ret=0,
					current_gd_page=-1;
					
	struct GSFS_dirent	*gda=0;
	struct inode		**children_inodes;
	struct buffer_head	*gd_bh=0;
	
	unsigned short 		gdlen=gsfs_dirent_len,
				gd_per_page=Block_Size/gdlen;
	crust_struct		*pcs;
	char			**children_ubhashes;
	
	#ifdef test_user_rovocation
	char	*rep,
		*repp;
		
		repp=kmalloc(3000, GFP_KERNEL);
		rep=repp;
	#endif
	
	if(in->i_size==0)
		return 0;
		
	res=kzalloc(sizeof(int)*GSFS_DEDICATION_ARRAY_LEN_BITS, GFP_KERNEL);
	children_inos=kzalloc(sizeof(unsigned int)*GSFS_DEDICATION_ARRAY_LEN_BITS, GFP_KERNEL);
	children_inodes=kzalloc(sizeof(struct inode*)*GSFS_DEDICATION_ARRAY_LEN_BITS, GFP_KERNEL);
	children_ubhashes=kzalloc(sizeof(char*)*GSFS_DEDICATION_ARRAY_LEN_BITS, GFP_KERNEL);
		
	gwur(sprintf(rep,"Traverse_all_gdirents for updating users and links for dir: %lu* ",in->i_ino));
	gwur(rep+=strlen(rep));
	
	down_write(&inf->inode_rwsem);
	
	if(update_users){
		struct inode* pin;
		struct GSFS_inode* pinf;
		int k;
		
		gwur(sprintf(rep, " updating users : "));
		gwur(rep+=strlen(rep));
				
		if(inf->igflags & igflag_active_user_block)
			if(!(inf->igflags & igflag_active_parent_link)){
				gwur(sprintf(rep, " This inode hasn't parent link and therefore its users will not change * "));
				gwur(rep+=strlen(rep));
				
				goto end_user_update;
			}

		pin=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);
		//inf->parent;
		if(!pin){
			gwur(sprintf(rep, "No parent * "));
			gwur(rep+=strlen(rep));
			
			goto end_user_update;
		}
		
		pinf=(struct GSFS_inode*)pin->i_private;
		if(!pinf){
			gwur(sprintf(rep, "No parent inf* "));
			gwur(rep+=strlen(rep));
			
			goto end_user_update;
		}
		
		//we havent got it from parent
		down_write(&pinf->inode_rwsem);
		
		put_users(inf->users);
		inf->users=0;
		
		k=get_inode_users_and_or_crust_state_from_parent(pin, ind->index_in_parent, 0, &inf->users, 
							ind->dir_inode_security.user_block,
							ind->dir_inode_security.inode_user_block_hash, 
							inf->igflags, 0, 1, 0);
		
		//we havent got it from parent
		up_write(&pinf->inode_rwsem);
		
		gwur(sprintf(rep, "k= %d * ",k));
		gwur(rep+=strlen(rep));
	}
	
end_user_update:

	dss=2;
	gwur(sprintf(rep,"dss=%d * ",dss));
	gwur(rep+=strlen(rep));
	
	res_num=get_all_set_indecis(ind->dir_inode_security.dir_inode_child_index_dedication_array, 
				    res, GSFS_DEDICATION_ARRAY_LEN_BITS);
	
	first_round=1;
round:	
	res_index=0;	
	
	gwur(sprintf(rep,"res_index= %d and first_round: %d * ",res_index , first_round));
	gwur(rep+=strlen(rep));
	
	for(res_index=0 ; res_index<res_num  ; res_index++){
		unsigned short 	index=res[res_index],
				gdpage=index/gd_per_page,
				gdoffset=index%gd_per_page;
		unsigned int	gdlen=0,
				gdino=0,
				k;
		char	clear_inl=0,
			//clear_key=0,
			*gdname=0,
			//gdtype,
			gdhash_parent[gsfs_hashlen],
			gdhash_disk[gsfs_hashlen],
			gdbad_string[100];
			//key[gsfs_aes_keylen];
		struct GSFS_dirent	*gd=0;
		struct gdirent_inl inl;
		struct users*	users=0;
		
		memset(gdbad_string,0,100);
		
		gwur(sprintf(rep,"$$$ traverse_for_updating_links_and_users inode: %ld dirent with index:%d * ",in->i_ino,index));
		gwur(rep+=strlen(rep));
		
		if(current_gd_page!=gdpage){
			current_gd_page=gdpage;
			if(gd_bh)
				brelse(gd_bh);
			gd_bh=__bread(in->i_sb->s_bdev,get_dp_bn_of_in(in,gdpage),Block_Size);
			gda=(struct GSFS_dirent*)gd_bh->b_data;
		}
		
		gd=&gda[gdoffset];
		
		gwur(sprintf(rep,"reading gd_page:%d and gdoffset: %d, gdflags:%x * ",gdpage ,gdoffset,gd->gd_flags));
		gwur(rep+=strlen(rep));
		
		if(first_round){
			if(!is_set_one_index(ind->dir_inode_security.dir_inode_sec_has_sec_child_array, 
				index, GSFS_DEDICATION_ARRAY_LEN_BITS)){
				gwur(sprintf(rep,"parent sec index is not set * "));
				gwur(rep+=strlen(rep));
				
				goto bad_gd;					
			}
					
			k=verify_and_get_integrity_for_child(GDirent_Integrity, in, index, gdhash_parent);
			gwur(sprintf(rep,"verify integrity for gdirent with k: %d * ",k));
			gwur(rep+=strlen(rep));
			
			if(k){
				printk("<1>" "Verifing integrity for gdirent with index: %d in inode %lu is failed.\n", index, in->i_ino);
				goto cont;
			}
			
			get_gdirent_hash(gdhash_disk, gd);
			
			gwur(sprintf(rep,"gdhash_disk:  * "));
			gwur(rep+=strlen(rep));
			gwur(printhexstring(gdhash_disk, rep, 16));
			gwur(rep+=strlen(rep));
			
			gwur(sprintf(rep,"gdhash_parent:  * "));
			gwur(rep+=strlen(rep));
			gwur(printhexstring(gdhash_parent, rep, 16));
			gwur(rep+=strlen(rep));
			
			if(strncmp(gdhash_disk, gdhash_parent, gsfs_hashlen)){
				gwur(sprintf(rep,"gdhash from disk and parent differs* "));
				gwur(rep+=strlen(rep));
				gwi(sprintf(gdbad_string, "parent gdirent hash and disk gdirent hash differs"));
				goto bad_gd;
			}
			
			//integrated gdirent
			if((gd->gd_flags & igflag_active_user_block) && !(gd->gd_flags & igflag_active_parent_link)){
				gwur(sprintf(rep,"current gd is sec and it is integrated and has user block but hasnt link from parent. * "));
				gwur(rep+=strlen(rep));
				goto cont;
			}
			
			if(!(gd->gd_flags & igflag_encrypted_inl)){
				gwur(sprintf(rep,"current gd is sec and it is integrated but inl is not encrypted. * "));
				gwur(rep+=strlen(rep));
			
				gdname=gd->gd_name;
				gdlen=gd->gd_len;
				gdino=gd->gd_ino;
				goto instr;
			}
			//secure gdirent with encrypted inl
			gwur(sprintf(rep,"current gd is sec and it is integrated and is inl_enc. * "));
			gwur(rep+=strlen(rep));				
			
			k=get_users_and_decrypt_inl_of_gdirent(in, gd, index, &users, &inl, 1, 0);
			gwur(sprintf(rep,"ret of get_users_and_inl:%d and users: %lx * ",k,(unsigned long)users));
			gwur(rep+=strlen(rep));
			
			if(!k){
				//k=user_check_access(users, current->loginuid, MAY_READ);
				put_users(users);
				
				if(!k){
					
					gwur(sprintf(rep,"you have access * "));
					gwur(rep+=strlen(rep));
			
					clear_inl=1;
					
					gdino=inl.ino;
					gdlen=inl.len;
					gdname=inl.name;
					gwur(sprintf(rep,"decrypted inl: name:%s len:%d ino:%d * ",gdname, gdlen, gdino));
					gwur(rep+=strlen(rep));
				}
				else{
					gwur(sprintf(rep,"you have not access * "));
					gwur(rep+=strlen(rep));
					goto cont;
				}
			}	
			else{
				gwur(sprintf(gdbad_string, "cant get key for this user * "));
				goto bad_gd;
			}
			
		instr:
			children_inos[child_count]=gdino;
			child_count++;
			
			gwur(sprintf(rep,"adding gdino:%u, gdname:%s, child_count:%d *",gdino, gdname, child_count));
			gwur(rep+=strlen(rep));
			
		cont:
			if(clear_inl)
				memset(&inl, 0, gsfs_inl_len);
			
			#ifdef test_user_rovocation
				printk("<0>" "traverse for revoke: %s\n",repp);
				rep=repp;
			#endif
			
			gdname=0;
			gdlen=0;
			gdino=0;
			
			continue;
			
		bad_gd:
			gwi(printk("<0>" "Bad gdirent in reading gdirent from disk for dir %lu and index %d because %s.\n",in->i_ino, index, gdbad_string)); 
			goto cont;
		}
		else{
			//in second round we should update only gdirent user block hash field
			//because it is changed and we should get new gdirent hash 
			char* gdhash;
			
			memcpy(gd->gd_first_dir_security_fields.gd_user_block_hash, children_ubhashes[res_index],
			       gsfs_hashlen);
			
			gdhash=children_ubhashes[res_index];//kzalloc(gsfs_hashlen, GFP_KERNEL);
			get_gdirent_hash(gdhash, gd);
			add_event_to_inode(in, index, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
			
			mark_buffer_dirty(gd_bh);
			set_buffer_uptodate(gd_bh);
			
			gwur(sprintf(rep,"updating user_block_hash to: "));
			gwur(rep+=strlen(rep));
			gwur(printhexstring(gd->gd_first_dir_security_fields.gd_user_block_hash, rep, 16));
			gwur(rep+=strlen(rep));
		}
	}
	
	if(gd_bh)
		brelse(gd_bh);
	
	if(first_round)
		pcs=get_crust_struct(inf->inode_crust_struct);
	else{
		mark_inode_dirty(in);
		inf->igflags |=igflag_inode_metadata_changed;
	}
	
	up_write(&inf->inode_rwsem);
	
	if(first_round){
		first_round=0;
		res_num=0;
		cin_count=0;
		
		gwur(sprintf(rep,"end of first_round for in:%ld* get_inode and add_max_ver for children * ",in->i_ino));
		gwur(rep+=strlen(rep));
			
		for(i=0; i<child_count; i++){
			children_inodes[cin_count]=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, children_inos[i]);
			
			if(children_inodes[cin_count]){
				struct GSFS_inode *cinf=(struct GSFS_inode*)children_inodes[cin_count]->i_private;
				
				res[res_num]=cinf->disk_info.index_in_parent;
				
				//we should change its user block if it is active parent link and threrefore active user block
				if(cinf->igflags & igflag_active_parent_link){
					children_ubhashes[res_num]=add_crust_max_ver_and_update_parent_link(children_inodes[cin_count], pcs);
				
					if(children_ubhashes[res_num]){
						res_num++;
						
						gwur(sprintf(rep,"add_max_ver for gd_ino: %u ",cinf->disk_info.ino));
						gwur(rep+=strlen(rep));	
					}
				}				
				cin_count++;
				
				gwur(sprintf(rep," get_inode for child index: %d and gd_ino:%u #",cinf->disk_info.index_in_parent, cinf->disk_info.ino));
				gwur(rep+=strlen(rep));	
			}
		}
		put_crust_struct(pcs);
		
		if(res_num){
			down_write(&inf->inode_rwsem);
			
			gwur(sprintf(rep,"goto second round for res_num:%d * ",res_num));
			gwur(rep+=strlen(rep));
			
			goto round;
		}
	}
	
	gwur(sprintf(rep,"# traverse_all_gdirents_for_updating_links_and_users for children : "));
	gwur(rep+=strlen(rep));
			
	for(i=0;i<cin_count;i++){
		gwur(sprintf(rep,"%d) ino: %ld ",i,children_inodes[i]->i_ino));
		gwur(rep+=strlen(rep));
			
		traverse_all_gdirents_for_updating_links_and_users(children_inodes[i], 1);
		
		iput(children_inodes[i]);
	}
		
	kfree(res);
	kfree(children_inos);
	kfree(children_inodes);
	kfree(children_ubhashes);

	gwur(printk("<0>" "%s\n",repp));
	gwur(kfree(repp));
	
	return 0;
}

int revoke_inode_users(struct inode* in,unsigned int* uids, int* rets,int num){
	struct inode* pin=0;
	struct GSFS_inode	*inf,
				*pinf;
	struct GSFS_inode_disk_inf	*ind;
	struct buffer_head	*bh;
	struct inode_user_page	*iup;
	int 	ret,
		i,
		new_iup_count;
	struct users	*pusers,
			**uev;
	struct rsa_key	*rkey;
	struct GSFS_sb	*gsb;
	struct super_block* sb;
	
	#ifdef test_user_rovocation
		char	*rep,
			*repp;
			
		rep=kmalloc(3000, GFP_KERNEL);
		repp=rep;
		
		sprintf(rep, "revoke_inode_users for in: %ld * ", in->i_ino);
		rep+=strlen(rep);
	#endif
	
	inf=(struct GSFS_inode*)in->i_private;
	ind=&inf->disk_info;
	
	sb=in->i_sb;
	gsb=(struct GSFS_sb*)sb->s_fs_info;
	
	memset(rets, -1, num*sizeof(int));
	
	down_write(&inf->inode_rwsem);
	
	gwur(sprintf(rep, "inf->igflags: %x * ",inf->igflags));
	gwur(rep+=strlen(rep));	
	
	if(	!(inf->igflags & igflag_secure) 		||
		!(inf->igflags & igflag_active_user_block) 	||
		(inf->igflags & igflag_incomplete_inode) 	||
		(ind->iuid != current->loginuid)
	   )
		goto bad_ret_sem;
	
	//preparing owner_key
	if(read_owner_key_for_crust_struct(inf->inode_crust_struct, in->i_sb, 
					   ind->dir_inode_security.inode_user_block_hash)){
		gwur(sprintf(rep, "unable to read owner key for crust struct * "));
		gwur(rep+=strlen(rep));
	
		goto bad_ret_sem;
	}
	
	gwur(sprintf(rep, "read owner key: "));
	gwur(rep+=strlen(rep));
	gwur(printhexstring(inf->inode_crust_struct->owner_key, rep, 16));
	gwur(rep+=strlen(rep));
	
	//calculating new crust state of crust struct
	spin_lock(&inf->inode_crust_struct->lock);
	
	if(inf->inode_crust_struct->max_ver== crust_maxver)
		goto brs_lock;
	
	inf->inode_crust_struct->max_ver++;
	
	ret=crust_get_next_state(&inf->inode_crust_struct->crust_state,  inf->inode_crust_struct->max_ver, 
				 inf->inode_crust_struct->owner_key);
	if(ret){
		gwur(sprintf(rep, "unable to get new crust state for ver: %u * ", inf->inode_crust_struct->max_ver));
		gwur(rep+=strlen(rep));	
brs_lock:
		spin_unlock(&inf->inode_crust_struct->lock);
		
		goto bad_ret_sem;
	}
	
	gwur(sprintf(rep, "new crust struct with ver: %d, ret: %d : ", inf->inode_crust_struct->max_ver, ret));
	gwur(rep+=strlen(rep));
	gwur(printhexstring((char*)&inf->inode_crust_struct->crust_state, rep, 81));
	gwur(rep+=strlen(rep));

	//calculating user_block 
	rkey=get_rsa_key(gsb, current->loginuid, 0);
	if(!rkey || !rkey->key){
		gwur(sprintf(rep, "unable to get rsa key for uid: %u * ",current->loginuid));
		gwur(rep+=strlen(rep));
		
		goto brs_lock;
	}
	bh=__bread(sb->s_bdev, ind->dir_inode_security.user_block, Block_Size);
	
	lock_buffer(bh);
	
	iup=(struct inode_user_page*)bh->b_data;
	new_iup_count=1;
	spin_lock(&rkey->lock);
	
	gwur(sprintf(rep, "previous iup->max_ver: %u *", iup->max_ver));
	gwur(rep+=strlen(rep));
	
	iup->max_ver=inf->inode_crust_struct->max_ver;
	
	/*
	iup->owner_key.uid=current->loginuid;
	iup->owner_key.writability=1;	
	ret=rsa_encrypt_owner_key_for_user_block(iup->owner_key.rsa_encrypted_key, inf->owner_key, rkey->key);
	*/
	
	iup->users_key[0].uid=current->loginuid;
	iup->users_key[0].writability=1;	
	ret=rsa_encrypt_crust_state_for_user_block(iup->users_key[0].rsa_encrypted_key, 
						   &inf->inode_crust_struct->crust_state, rkey->key);
	
	spin_unlock(&rkey->lock);
	
	get_crust_hash(iup->crust_hash, &inf->inode_crust_struct->crust_state);
	
	gwur(sprintf(rep, "new crust_hash: "));
	gwur(rep+=strlen(rep));
	gwur(printhexstring(iup->crust_hash, rep, 16));
	gwur(rep+=strlen(rep));
	
	for(i=1; i<iup->num; i++){
		int 	j,
			found=0;
		
		for(j=0; j<num; j++){
			if(uids[j]==current->loginuid)
				continue;
			
			if(uids[j]==iup->users_key[i].uid){
				
				gwur(sprintf(rep, "finding uids[j]: %u and revoking it :) * ",uids[j]));
				gwur(rep+=strlen(rep));
	
				rets[j]=0;
				found=1;
				break;
			}
		}
		
		if(found)
			continue;
		
		gwur(sprintf(rep, "adding user: %u to user_block: ", iup->users_key[i].uid));
		gwur(rep+=strlen(rep));
			
		rkey=get_rsa_key(gsb, iup->users_key[i].uid, 0);
		if(!rkey || !rkey->key)
			continue;
		
		spin_lock(&rkey->lock);
		
		iup->users_key[new_iup_count].uid=iup->users_key[i].uid;
		iup->users_key[new_iup_count].writability=iup->users_key[i].writability;
		ret=rsa_encrypt_crust_state_for_user_block(iup->users_key[new_iup_count].rsa_encrypted_key, 
							   &inf->inode_crust_struct->crust_state, rkey->key);
		
		spin_unlock(&rkey->lock);
		
		new_iup_count++;
		
		gwur(sprintf(rep, "and ret of rsa_encrpyt: %d * ", ret));
		gwur(rep+=strlen(rep));
	}
	
	iup->num=new_iup_count;
	
	gwur(sprintf(rep, "new iup->num: %u *",iup->num));
	gwur(rep+=strlen(rep));
		
	//updating parent_link in user_block if it is existed
	if(inf->igflags & igflag_active_parent_link){
		crust_struct *pcs;
		pin=GSFS_get_inode(gsb, inf->disk_info.parent_ino);
		pinf=(struct GSFS_inode*)pin->i_private;
		
		
		down_read(&pinf->inode_rwsem);
		
		pcs=get_crust_struct(pinf->inode_crust_struct);
		
		up_read(&pinf->inode_rwsem);
		
		spin_lock(&pcs->lock);
		
		iup->parent_cs_ver_for_cs_link=pcs->max_ver;
		
		ret=encrypt_crust_state(&iup->crust_state_link, &inf->inode_crust_struct->crust_state, 
				    &pcs->crust_state, pcs->max_ver);
		
		spin_unlock(&pcs->lock);
		
		put_crust_struct(pcs);
		
		gwur(sprintf(rep, "updating parent_link in user block wiht pver: %d, encrypt crust state: %d to ",iup->parent_cs_ver_for_cs_link,ret));
		gwur(rep+=strlen(rep));
		gwur(printhexstring((char*)&iup->crust_state_link, rep, 81));
		gwur(rep+=strlen(rep));
	}
	
	spin_unlock(&inf->inode_crust_struct->lock);
	
	//calculating new user block hash
	get_user_block_hash(ind->dir_inode_security.inode_user_block_hash, bh->b_data);
	
	gwur(sprintf(rep, "new user_block hash : "));
	gwur(rep+=strlen(rep));
	gwur(printhexstring(ind->dir_inode_security.inode_user_block_hash, rep, 16));
	gwur(rep+=strlen(rep));
	
	gwur(printk("<0>" "%s\n",repp));
	gwur(rep=repp);
	
	gwur(sprintf(rep, "continue of revoke_inode_users for in: %ld * ", in->i_ino));
	gwur(rep+=strlen(rep));
		
	//now we should update the users struct of inode
	gwur(sprintf(rep, "update users * "));
	gwur(rep+=strlen(rep));
	
	put_users(inf->users);
	
	inf->users=kzalloc(sizeof(struct users), GFP_KERNEL);
	
	spin_lock_init(&inf->users->lock);
	
	inf->users->users_num=iup->num;
	pusers=0;
	
	if(!pin)
		pin=GSFS_get_inode(gsb, inf->disk_info.parent_ino);
	if(pin && (inf->igflags & igflag_active_parent_link)){
		pinf=(struct GSFS_inode*)pin->i_private;
		
		down_read(&pinf->inode_rwsem);
		
		if(pinf->igflags & igflag_secure)
			if(inf->igflags & igflag_active_parent_link)
				pusers=get_users(pinf->users);
		
		up_read(&pinf->inode_rwsem);
	}
	
	if(pusers)
		inf->users->users_num += pusers->users_num-1;
	
	inf->users->users=kzalloc(sizeof(unsigned int)*inf->users->users_num, GFP_KERNEL);
	inf->users->writability=kzalloc(sizeof(unsigned char)*inf->users->users_num, GFP_KERNEL);
	inf->users->count=2;
	
	i=0;
	if(pusers){
		int j;
		
		gwur(sprintf(rep,"Adding parent users : * "));
		gwur(rep+=strlen(rep));
		
		//j=0 is for owner that will be added in next loop of iup users :)
		for( j=1; j<pusers->users_num; i++, j++){
			inf->users->users[i]=pusers->users[j];
			inf->users->writability[i]=pusers->writability[j];
			
			gwur(sprintf(rep,"uid :%u, write:%d # ",inf->users->users[i],inf->users->writability[i]));
			gwur(rep+=strlen(rep));	
		}
		
		put_users(pusers);
		pusers=0;
	}
	
	if(iup->num){
		int j;
		
		gwur(sprintf(rep," * users of iup: "));
		gwur(rep+=strlen(rep));	
		
		for( j=0; j<iup->num; i++, j++){
			inf->users->users[i]=iup->users_key[j].uid;
			inf->users->writability[i]=iup->users_key[j].writability;
			
			gwur(sprintf(rep,"uid :%u, write:%d # ",inf->users->users[i],inf->users->writability[i]));
			gwur(rep+=strlen(rep));	
		}
	}
	
	//setting users_event(uev) to be added later 
	uev=kmalloc(sizeof(struct users*),GFP_KERNEL);
	*uev=inf->users;
	
	mark_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	brelse(bh);	
	
	//updating gdirent of in to be compatible with ind for user-block and user-block-hash 
	if(likely(pin)){
		short	gdlen=gsfs_dirent_len;
		short	gd_per_page=Block_Size/gdlen;
		int	gdpage,
			gdoffset;
		struct GSFS_dirent* gd;
		char 	*gdhash;
		int k;
		
		pinf=(struct GSFS_inode*)pin->i_private;
		down_write(&pinf->inode_rwsem);
		
		gdpage=ind->index_in_parent/gd_per_page;
		gdoffset=ind->index_in_parent%gd_per_page;
		
		bh=__bread(sb->s_bdev, get_dp_bn_of_in(pin,gdpage), Block_Size);
		gd=(struct GSFS_dirent*)bh->b_data;
		gd+=gdoffset;
		
		//gd->gd_first_dir_security_fields.gd_user_block=ind->dir_inode_security.user_block;
		memcpy(gd->gd_first_dir_security_fields.gd_user_block_hash, 
		       ind->dir_inode_security.inode_user_block_hash, gsfs_hashlen);
		
		//adding new hash of gdirent to parent
		gdhash=kzalloc(gsfs_hashlen, GFP_KERNEL);
		get_gdirent_hash(gdhash, gd);
		k=inf->add_event_to_parent(in, GDirent_Hash_Changed_Event, gdhash, gsfs_hashlen, 0);
		
		gwur(sprintf(rep," * updating gdirent with new user block hash * "));
		gwur(rep+=strlen(rep));
		
		mark_buffer_dirty(bh);
		set_buffer_uptodate(bh);
		brelse(bh);
		
		//adding uev to pin
		add_event_to_inode( pin, ind->index_in_parent, Users_Set_VEvent, uev, 
				    sizeof(struct users*), event_flag_from_disk);
		
		mark_inode_dirty(pin);
		
		up_write(&pinf->inode_rwsem);
	}

	mark_inode_dirty(in);
	inf->igflags |=igflag_inode_metadata_changed;
		
	up_write(&inf->inode_rwsem);
	
	//updating all children links and users
	traverse_all_gdirents_for_updating_links_and_users(in, 0);
	
	if(pin)
		iput(pin);
	
	gwur(printk("<0>" "%s\n",repp));
	gwur(kfree(repp));
	
	return 0;
	
bad_ret_sem:

	up_write(&inf->inode_rwsem);
	
	gwur(printk("<0>" "%s\n",repp));
	gwur(kfree(repp));
	
	return -1;
}

int GSFS_revoke_users(struct super_block* sb, char* dest, unsigned int* uids, 
				int* rets, int num){
	struct nameidata nd;
	int ret;
	struct inode* in;
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	
	if(gsb->gsb_disk.revocation_num >= crust_maxver)
		return -1;
	gsb->gsb_disk.revocation_num++;
	
	ret=path_lookup(dest, LOOKUP_DIRECTORY, &nd);
	if(ret)
		return ret;
	
	in=nd.path.dentry->d_inode;
		
	path_put(&nd.path);
	
	if(in->i_sb->s_magic != GSFS_MAGIC)
		return -1;
	
	if(in->i_ino == 1)
		return -1;

	atomic_inc(&in->i_count);
	
	ret=revoke_inode_users(in, uids, rets, num);
	
	iput(in);
	
	return ret;
}
