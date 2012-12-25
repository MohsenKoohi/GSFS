#include "gsfs.h"

#ifdef gsfs_test
	#define development_file
#endif
#ifdef development_file
	#define gwf(mesg) mesg
#else
	#define	gwf(mesf)
#endif

int GSFS_readdir (struct file * filp, void * dirent, filldir_t filldir){
	struct inode* 	in=filp->f_dentry->d_inode;
	struct GSFS_inode* inf=(struct GSFS_inode*)in->i_private;
	int ret;
	
	//gw(printk("<0>" "readdir: from inode %ld  and position: %ld and file size: %ld\n",in->i_ino,(unsigned long)filp->f_pos,(unsigned long)in->i_size));
	
	down_write(&inf->inode_rwsem);
	
	ret=traverse_all_gdirents_for_gibn_or_rd(in, &filp->f_pos, dirent, filldir, 0, 0);
	
	up_write(&inf->inode_rwsem);
	
	return ret;
}

//int m=0;
int GSFS_file_open(struct inode * inode, struct file * filp){
	gwf(printk("<0>" "File open with inode :%lu pid:%u",inode->i_ino,current->pid));
	/*
	if(inode->i_ino == 3 && m++==0){
		int 	num=10,
			i=0,
			blocks[100],
			res[100];
		struct ver_IV_AT* vias[100];
		for(i=0;i<num;i++){
			blocks[i]=i;
			vias[i]=kzalloc(via_len, GFP_KERNEL);
			//sprintf(vias[i]->AT,"hello %d %d",i,i);
			
		}
		update_sec_reg_inode_hash_blocks(inode, 0, 1);
		get_blocks_via(inode, blocks, vias, res, 1);
		
		for(i=0;i<num;i++){
			blocks[i]=640+i;
			//vias[i]=kzalloc(via_len, GFP_KERNEL);
			sprintf(vias[i]->AT,"hello 640 %d %d",i,i);
			
		}
		
		update_sec_reg_inode_hash_blocks(inode, 0, 1000);
		set_blocks_via(inode, blocks, vias, res, num);
		for(i=0;i<num;i++){
			printk("<0>" "res[%d]=%d\n",i,res[i]);
			printkey(vias[i]->AT);
			kfree(vias[i]);
		}
		
		return -1;
	}
	*/
	return 0;
}

ssize_t GSFS_file_read (struct file *filp, char __user *charp, size_t len, loff_t *off){
	struct inode		*inode=filp->f_mapping->host;
	struct GSFS_inode	*inf=(struct GSFS_inode*)inode->i_private;
	sector_t sec_start,
		 sec_end,
		 sec_len;
	struct page	**res,
			**res2,
			**restemp;
	unsigned int *pn,
		     *pn2,
		     *pntemp;
	int 	i,
		odirect=filp->f_flags&O_SYNC,
		j,
		lock,
		pncount,
		pn2count;
	unsigned long 	pagestartbyte=0,
			pageendbyte,
			bufstart,
			bytes_in_first_buf_page;
	unsigned long	pagelen;		
	size_t  rlen;
	//char	*dest,
	//	*src;
	
	gwf(printk("<0>" "File read with inode :%lu size:%lu offset:%llu , off:%llu, charp:%lx, pid:%u\n",inode->i_ino,len,*off,filp->f_pos,(unsigned long)charp,current->pid));
	if((*off>=inode->i_size)){
		gwf(printk("<0>" "File read ended for *pos<size with inode :%lu size:%lu offset:%llu , off:%llu, charp:%lx, pid:%u\n",inode->i_ino,len,*off,filp->f_pos,(unsigned long)charp,current->pid));
		return 0;
	}
	if(!access_ok(VERIFY_WRITE,charp,len)){
		gwf(printk("<0>" "File read ended for access_nok with inode :%lu size:%lu offset:%llu , off:%llu, charp:%lx, pid:%u\n",inode->i_ino,len,*off,filp->f_pos,(unsigned long)charp,current->pid));
		return -EIO;
	}
	sec_start=(*off)>>Block_Size_Bits;
	if((*off+len)>inode->i_size)
		len=inode->i_size-*off;
	sec_end=(*off+len-1)>>Block_Size_Bits;
	sec_len=sec_end-sec_start+1;
	bytes_in_first_buf_page=((1+~((*off)&((unsigned long)Block_Size-1)))&(Block_Size-1));
	if(!bytes_in_first_buf_page)
		bytes_in_first_buf_page=Block_Size;
	pn=kzalloc(sec_len*sizeof(unsigned int),GFP_KERNEL);
	pn2=kzalloc(sec_len*sizeof(unsigned int),GFP_KERNEL);
	res=kzalloc(sec_len*sizeof(struct page*),GFP_KERNEL);
	res2=kzalloc(sec_len*sizeof(struct page*),GFP_KERNEL);
	for(i=sec_start,j=0;i<=sec_end;i++,j++)
		pn[j]=i;
	
	gwf(printk("<0>" "GSFS_file_read: sec_start:%lu, sec_end:%lu, sec_len:%lu, bytes_in_first_buf_page: %lu\n",
			sec_start,sec_end,sec_len,bytes_in_first_buf_page));
			
	pncount=GSFS_get_data_pages_of_inode(inode, pn, sec_len ,res,odirect);	
	//printk("<0>" "res[%u]=%d \n",j,res[j]);
	rlen=0;
	pn2count=0;
	lock=0;
	do{
		for(j=0;j<pncount;j++){
			//printk("<0>" "res[%u]=%lx \n",j,res[j]);
			
			if(unlikely(!res[j]))
				continue;
			
			if(lock && PageLocked(res[j])){
				//printk("<0>" "Locking for j:%u\n",j);
				wait_on_page_locked(res[j]);
				lock=0;
			}
			else 
				if(PageLocked(res[j])){
					pn2[pn2count]=pn[j];
					res2[pn2count]=res[j];
					pn2count++;
					continue;
				}
				
			//the page is available for writing to buffer
			 
			if(pn[j]==sec_start){
				pagestartbyte=((*off)&(Block_Size-1));
				bufstart=(unsigned long)charp;
			}
			else{
				pagestartbyte=0;
				bufstart=(unsigned long)(charp)+bytes_in_first_buf_page+((pn[j]-sec_start-1)<<Block_Size_Bits);
			}
			if(pn[j]==sec_end)
				pageendbyte=((*off+len-1)&(Block_Size-1));
			else
				pageendbyte=Block_Size-1;
			pagelen=(unsigned long)(pageendbyte-pagestartbyte+1);
			
			if(inf->igflags & igflag_secure){
				struct GSFS_page	*gp=(struct GSFS_page*)page_private(res[j]);
				
				if(unlikely(!gp || !gp->sip) || unlikely(!(gp->sip->spflags & spflag_page_is_ready_for_read)) ){
					//printk("<0>" "page is not ready for inode:%lu, index: %lu\n", inode->i_ino, res[j]->index);
					//if(gp && gp->sip)
					//	printk("<0>" "and flags:%d\n",gp->sip->spflags);
					goto add_cont;
				}
			}
			
			i=__copy_to_user_inatomic((void*)bufstart,page_address(res[j])+pagestartbyte,pagelen);
add_cont:			
			rlen+=(pagelen-i);
			mark_page_accessed(res[j]);
			/*
			dest=(char*)bufstart;
			src=(char*)pagestartbyte;
			for(i=0;i<pagelen;i++)
				dest[i]=src[i];
			*/
			//printk("<0>" "asdfasd%s",dest);
			//rlen+=i;
			GSFS_put_data_page_of_inode(inode,res[j]);
			//gwf(printk("<0>" "file read for inode:%lu, j:%u pn[j]:%u pagestartbyte:%lx bufstart:%lx pagelen:%lu i:%u sec_start:%lu\n",
			//		inode->i_ino, j, pn[j],(unsigned long)pagestartbyte,(unsigned long)bufstart,pagelen,i,sec_start));
		}
		lock=1;
		pncount=pn2count;
		pn2count=0;
		
		pntemp=pn2;
		pn2=pn;
		pn=pntemp;
		
		restemp=res2;
		res2=res;
		res=restemp;
		
		gwf(printk("<0>" "file read for inode:%lu pncount:%u\n",inode->i_ino,pncount));
	}while(pncount);
	
	kfree(pn);
	kfree(pn2);
	kfree(res);
	kfree(res2);
	
	(*off)+=rlen;
	gwf(printk("<0>" "file read ends rlen=%lu len:%lu\n",rlen,len));
	return rlen;
}

ssize_t GSFS_file_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	struct inode		*inode=filp->f_mapping->host;
	struct address_space	*mapping=filp->f_mapping;
	sector_t 		sec_start,
				sec_end,
				sec_len;
	unsigned long 		bufstart,
				bytes_in_first_buf_page,
				bytes_in_last_buf_page,
				pagestartbyte,
				pageendbyte,
				pagelen;
	size_t  		rlen;
	struct page		*res[2],
				*page,
				**pages;
	unsigned int 		i,
				j,
				pages_count,
				start_read,
				end_read;
		
	gwf(printk("<0>" "File write with inode :%lu len:%lu offset:%llu , filepos:%llu, buf:%lx inode_size:%llu, pid:%u\n",inode->i_ino,len,*off,filp->f_pos,(unsigned long)buf,inode->i_size,current->pid));
	if(unlikely(!access_ok(VERIFY_READ,buf,len)))
		return -1;
	
	mutex_lock(&inode->i_mutex);	
	
	if((*off+len)>inode->i_size){
		if((*off+len)>inode->i_sb->s_maxbytes)
			return -1;
		inode->i_size=(*off+len);
		GSFS_truncate(inode);		
	}
	
	if(filp->f_flags & O_APPEND)
		*off=inode->i_size;
	
	current->backing_dev_info=mapping->backing_dev_info;
	file_remove_suid(filp);
	file_update_time(filp);
	//inode_inc_iversion(inode);
	
	sec_start=(*off)>>Block_Size_Bits;
	sec_end=(*off+len-1)>>Block_Size_Bits;
	sec_len=sec_end-sec_start+1;
	
	pages=kzalloc(sizeof(struct page*) * sec_len, GFP_KERNEL);
	
	bytes_in_first_buf_page=Block_Size-((*off)&((unsigned long)Block_Size-1));
	bytes_in_last_buf_page=((*off+len)&((unsigned long)Block_Size-1));
	if(bytes_in_last_buf_page==0)
		bytes_in_last_buf_page=Block_Size;	
	
	start_read=(bytes_in_first_buf_page!=Block_Size)?1:0;
	end_read=(bytes_in_last_buf_page!=Block_Size && inode->i_size>(*off+len))?1:0;

	gwf(printk("<0>" "GSFS write bytes_in_first_buf_page:%lu, bytes_in_last_buf_page:%lu\n",bytes_in_first_buf_page,bytes_in_last_buf_page));
	gwf(printk("<0>" "GSFS write start_read:%u, end_read:%u, sec_start:%lu, sec_end:%lu\n",start_read,end_read,sec_start,sec_end));
	
	if(sec_start==sec_end){
		if(start_read || end_read){
			res[0]=GSFS_get_data_page_of_inode_with_read(inode, sec_start);
			gwf(printk("<0>" "sec_start==sec_end, start_read || end_read , res[0]=%lx",(unsigned long)res[0]));
		}
		else{
			res[0]=GSFS_get_locked_data_page_of_inode_without_read(inode,sec_start);
			if(likely(res[0]))
				unlock_page(res[0]);
			gwf(printk("<0>" "sec_start==sec_end, !(start_read || end_read) , res[0]=%lx",(unsigned long)res[0]));
		}
		res[1]=0;
		if(unlikely(!res[0])){
			gwf(printk("<0>" "GSFS write len:-1\n"));
			mutex_unlock(&inode->i_mutex);
			printk("<1>" "GSFS write len:-1\n");
			
			kfree(pages);
			
			return len;
		}
	}
	else{
		if(start_read){
			res[0]=GSFS_get_data_page_of_inode_with_read(inode, sec_start);
			gwf(printk("<0>" "sec_start!=sec_end, start_read, res[0]=%lx",(unsigned long)res[0]));
		}
		else{
			res[0]=GSFS_get_locked_data_page_of_inode_without_read(inode,sec_start);
			if(likely(res[0]))
				unlock_page(res[0]);
			gwf(printk("<0>" "sec_start!=sec_end, !start_read, res[0]=%lx",(unsigned long)res[0]));
		}
	}
	
	pages_count=0;
	if(sec_len>1)
		for(i=sec_start+1;i<=sec_end-1;i++)
			pages[pages_count++]=GSFS_get_locked_data_page_of_inode_without_read(inode,i);
	
	if(sec_start != sec_end){
		if(end_read){
			res[1]=GSFS_get_data_page_of_inode_with_read(inode,sec_end);
			gwf(printk("<0>" "sec_start!=sec_end, end_read, res[1]=%lx",(unsigned long)res[1]));
		}
		else{
			res[1]=GSFS_get_locked_data_page_of_inode_without_read(inode,sec_end);
			if(likely(res[1]))
				unlock_page(res[1]);
			gwf(printk("<0>" "sec_start!=sec_end, !end_read, res[1]=%lx",(unsigned long)res[1]));
		}
		
		if(unlikely(!res[0] || !res[1])){
			gwf(printk("<0>" "GSFS write len:-1\n"));
			printk("<1>" "GSFS write len:-1\n");
			mutex_unlock(&inode->i_mutex);
			
			kfree(pages);
			
			return len;
		}
	}
	
	rlen=0;
	bufstart=(unsigned long)buf+bytes_in_first_buf_page;
	pagelen=Block_Size;
	
	//100% expected complete pages that should be copied
	pages_count=0;
	if(sec_len>1)
		for(i=sec_start+1;i<=sec_end-1;i++){
			gwf(printk("<0>" "write page complete pages, i:%u, bufstart:%lx, rlen=%lu\n",i,bufstart,rlen));
			
			page=pages[pages_count++];
			if(unlikely(!page))
				goto buf_cont;
			
			j=__copy_from_user_inatomic(page_address(page),(void*)bufstart,pagelen);
			
			rlen+=(Block_Size-j);
			
			mark_page_accessed(page);
			set_page_dirty(page);
			put_page(page);
			
			unlock_page(page);
buf_cont:			
			bufstart+=pagelen;		
		}
	
	//first and last page that are not surely complete
	for(i=0;i<2 && res[i];i++){		
		page=res[i];
		wait_on_page_locked(page);
		lock_page(page);
		if(page->index==sec_start){
			bufstart=(unsigned long)buf;
			pagestartbyte=Block_Size-bytes_in_first_buf_page;
			if(sec_start==sec_end)
				pageendbyte=pagestartbyte+len-1;
			else
				pageendbyte=Block_Size-1;
		}
		else{
			bufstart=(unsigned long)buf+bytes_in_first_buf_page+((sec_len-2)<<Block_Size_Bits);
			pageendbyte=bytes_in_last_buf_page-1;
			pagestartbyte=0;
		}
		gwf(printk("<0>" "gsfs_write for first and last page, i=%u, page:%lx, bufstart:%lx, pagestartbyte:%lu, pageendbyte:%lu\n",
				i,(unsigned long)page,bufstart,pagestartbyte,pageendbyte));
		pagelen=pageendbyte-pagestartbyte+1;
		j=__copy_from_user_inatomic(page_address(page)+pagestartbyte,(void*)bufstart,pagelen);
		rlen+=(pagelen-j);		
		mark_page_accessed(page);
		set_page_dirty(page);
		put_page(page);
		unlock_page(page);		
	}
	
	mutex_unlock(&inode->i_mutex);
	(*off)+=rlen;
	
	gwf(printk("<0>" "GSFS write rlen:%lu\n",rlen));
	
	kfree(pages);
	
	if(filp->f_flags & O_SYNC){
		write_inode_now(inode,1);
	}
	
	return rlen;
}

struct file_operations GSFS_file_fops={
	.open		=	GSFS_file_open,
	.read 		= 	GSFS_file_read,
	.write		=	GSFS_file_write,
	.mmap		=	generic_file_mmap,
};

struct file_operations GSFS_dir_fops={
	.readdir	=	GSFS_readdir,
};