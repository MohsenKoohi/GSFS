#include "gsfs.h"
#include "message.h"

struct super_block* sb=0;

int GSFS_cdev_file_open(struct inode * inode, struct file * filp){
	//printk("<0>" "GSFS_cdev_File open with inode :%lu pid:%u",inode->i_ino,current->pid);
	return 0;
}

void GSFS_gum_exit(void){
	struct GSFS_sb *gsb;
	struct ocl_message* mes;
	
	if(unlikely(!sb))
		return;
	gsb=(struct GSFS_sb*)sb->s_fs_info;	
	if(unlikely(!gsb || !gsb->gum_struct.gum_is_initialized))
		return;
	
	down(&gsb->gum_struct.gum_struct_sem);
	
	mes=(struct ocl_message*)gsb->gum_struct.gum_ocl_mes;
	
	if(gsb->gum_struct.gum_pid && mes){
		gt(printk("<0>" "Writing OCL_Exit to gum\n"));
		
		mes->type=OCL_Exit;
		up(&gsb->gum_struct.gum_is_ready_sem);
		
		gsb->gum_struct.gum_pid=0;
	}
	
	//this line can be done only here because there are no other one to down this sem
	//in all other situations up for gum_struct_sem can be done only by gsfs_user_module process
	up(&gsb->gum_struct.gum_struct_sem);
	
	return;	
}

#define keys_per_page	(Block_Size/gsfs_aes_keylen)
#define IVs_per_page	(Block_Size/gsfs_aes_keylen)

//for  anonymous mapping
OCL_kernel_struct* gum_get_gctr_pages(struct super_block* sb, struct page ** IVs, 
				     struct page ** keys, struct page ** res, unsigned int count){
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	OCL_kernel_struct *oks=0;
	
	down(&gsb->gum_struct.gum_struct_sem);
	
	if(likely(gsb->gum_struct.gum_pid)){
		struct vm_area_struct *vma=gsb->gum_struct.gum_vma;
		unsigned long	address=gsb->gum_struct.gum_start_address;
		int 	i;
		unsigned int 	count_pages;
		struct ocl_message *mes=gsb->gum_struct.gum_ocl_mes;
		
		gt(printk("<0>" "gum_get_gctr_pages ** vma: %lx, address: %lx\n",(unsigned long)vma, (unsigned long) address));
		
		count_pages=count/keys_per_page;
		if(count%IVs_per_page)
			count_pages++;
		vma->vm_flags |= VM_INSERTPAGE;
		mes->IVs_start_address=address;
		for(i=0; i<count_pages; i++){
			if(likely(IVs[i])){
				//int k=
				//get_page(IVs[i]);
				//printk("<0>" "IVs k: %d, address: %lx, page: %lx, %d\n",0,address, IVs[i], atomic_read(&IVs[i]->_count));
				vm_insert_page(vma, address, IVs[i]);
				address+=Block_Size;
				//printk("<0>" "IVs k: %d, address: %lx, page: %lx, %d\n",0,address, IVs[i], atomic_read(&IVs[i]->_count));
			}
			else
				goto up_ret;
		}
		
		mes->keys_start_address=address;
		for(i=0; i<count_pages; i++){
			if(likely(keys[i])){
				//int k=
				//get_page(keys[i]);
				vm_insert_page(vma, address, keys[i]);
				address+=Block_Size;
				//printk("<0>" "keys k: %d, address: %lx, page: %lx\n",k,address, keys[i]);
			}
			else
				goto up_ret;
		}
		
		mes->results_start_address=address;
		for(i=0; i<count; i++){
			if(likely(res[i])){
				//int k=
				//get_page(res[i]);
				vm_insert_page(vma, address, res[i]);
				address+=Block_Size;
				//printk("<0>" "res k: %d, address: %lx, page: %lx\n",k,address, res[i]);
			}
			else
				goto up_ret;
		}
		
		mes->pages_count=count;
		
		oks=kzalloc(sizeof(OCL_kernel_struct), GFP_KERNEL);
		atomic_set(&oks->waiters_returned, 0);
		
		oks->results=res;
		oks->IVs=IVs;
		oks->keys=keys;
		oks->results_count=count;
		oks->IVs_pages_count=count_pages;
		
		sema_init(&oks->sem, 0);
		sema_init(&oks->wanu_sem, 0);
		
		mes->kernel_struct=oks;
		mes->type=OCL_Get_Response;
		
		up(&gsb->gum_struct.gum_is_ready_sem);
	}
	else 
		goto up_ret;
	
	return oks;
	
up_ret:
	up(&gsb->gum_struct.gum_struct_sem);
	return 0;
}


OCL_kernel_struct* gum_get_gctr_pages_for_gsfs_dev_mmap(struct super_block* sb, struct page ** IVs, 
				     struct page ** keys, struct page ** res, unsigned int count){
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	OCL_kernel_struct *oks=0;
	
	down(&gsb->gum_struct.gum_struct_sem);
	
	if(likely(gsb->gum_struct.gum_pid)){
		struct vm_area_struct *vma=gsb->gum_struct.gum_vma;
		unsigned long	address=gsb->gum_struct.gum_start_address;
		int 	i;
		unsigned int 	count_pages,
				vpc=0;
		
		unsigned long	*vp;
		struct ocl_message *mes=gsb->gum_struct.gum_ocl_mes;
		
		gt(printk("<0>" "gum_get_gctr_pages ** vma: %lx, address: %lx\n",(unsigned long)vma, (unsigned long) address));
		
		if(unlikely(!vma))
			goto up_ret;
		
		count_pages=count/keys_per_page;
		if(count%IVs_per_page)
			count_pages++;
		
		vp=kzalloc(sizeof(unsigned long)*(1+2*count_pages+count), GFP_KERNEL);
		vma->vm_private_data=vp;
		vp[vpc++]=1+2*count_pages+count;
		
		mes->IVs_start_address=address;
		
		for(i=0; i<count_pages; i++){
			if(likely(IVs[i])){
				vp[vpc]=page_to_pfn(IVs[i]);
				address+=Block_Size;
				//printk("<0>" "IVs i: %d, address: %lx, page: %lx, vp[vpc]:%lx\n",vpc, address, (unsigned long)IVs[i], vp[vpc]);
				vpc++;
			}
			else
				goto up_ret;
		}
		
		mes->keys_start_address=address;
		for(i=0; i<count_pages; i++){
			if(likely(keys[i])){
				vp[vpc]=page_to_pfn(keys[i]);
				address+=Block_Size;
				//printk("<0>" "keys i: %d, address: %lx, page: %lx vp[vpc]:%lx\n",vpc, address, (unsigned long)keys[i], vp[vpc]);
				vpc++;
			}
			else
				goto up_ret;
		}
		
		mes->results_start_address=address;
		for(i=0; i<count; i++){
			if(likely(res[i])){
				vp[vpc]=page_to_pfn(res[i]);
				address+=Block_Size;
				//printk("<0>" "res i: %d, address: %lx, page: %lx vp[vpc]:%lx\n", vpc, address, (unsigned long)res[i], vp[vpc]);
				vpc++;
			}
			else
				goto up_ret;
		}
		
		mes->pages_count=count;
		
		oks=kzalloc(sizeof(OCL_kernel_struct), GFP_KERNEL);
		atomic_set(&oks->waiters_returned, 0);
		
		oks->results=res;
		oks->IVs=IVs;
		oks->keys=keys;
		oks->results_count=count;
		oks->IVs_pages_count=count_pages;
		
		sema_init(&oks->sem, 0);
		sema_init(&oks->wanu_sem, 0);
		
		mes->kernel_struct=oks;
		mes->type=OCL_Get_Response;
		
		up(&gsb->gum_struct.gum_is_ready_sem);
	}
	else 
		goto up_ret;
	
	return oks;
	
up_ret:
	up(&gsb->gum_struct.gum_struct_sem);
	return 0;
}

int m=0;
struct vm_area_struct* last_vma=0;

ssize_t GSFS_cdev_file_read (struct file *filp, char __user *charp, size_t len, loff_t *off){
	struct GSFS_sb* gsb=(struct GSFS_sb*)sb->s_fs_info;
	struct ocl_message *mes=(struct ocl_message*)charp;
	
	if(unlikely(current->loginuid!=0)){
ret_exit:		
		mes->type=OCL_Exit;
		return len;
	}
	
	if(unlikely(mes->type==110)){
		char *rep,repp[2000];
		struct page* p1, *p2,*p3;
		OCL_kernel_struct* oks;
		rep=repp;
		sprintf(rep,"110 ** ");
		rep+=strlen(rep);
		
		p1=alloc_page(GFP_KERNEL);
		p2=alloc_page(GFP_KERNEL);
		p3=alloc_page(GFP_KERNEL);
		memset(page_address(p1), 0, 16);
		memset(page_address(p2), 0, 16);
		memset(page_address(p3), 0, 4096);
		
		oks=gum_get_gctr_pages(sb, &p2, &p1, &p3, 1);
		sprintf(rep, "OCL_kernel_struct: %lx **", (unsigned long)oks);
		rep+=strlen(rep);
		if(oks){
			int i;
			unsigned char *pp=page_address(p3);
			
			down(&oks->sem);
			sprintf(rep, "ret of oks: %d **", oks->ret);
			if(!oks->ret)		
			for(i=0;i<256;i++){
				if(i%8==7)
					sprintf(rep, "%02x ** ", pp[i]);
				else
					sprintf(rep, "%02x", pp[i]);
				rep+=strlen(rep);
			}
			kfree(oks);
		}
		__free_page(p1);
		__free_page(p2);
		__free_page(p3);
		
		printk("<0>" "%s\n", repp);
		return 0;
	}
	
	if(mes->type==OCL_Is_Ready){
		if(!gsb->gum_struct.gum_pid){
			int ret_dtl;
			
			down(&gsb->gum_struct.gum_struct_sem);
			
			//if there was a gum process and it was damaged, the gum_is_ready_sem is up
			//therefore we should down it and if there was no gum process it is down 
			//and we don't need to down it
			ret_dtl=down_trylock(&gsb->gum_struct.gum_is_ready_sem);
			
			gsb->gum_struct.gum_pid=current->pid;
			
			gt(printk("<0>" "Setting gum_pid to %u  with ret of down_trylock: %d\n", current->pid, ret_dtl));
		}
		
		if(gsb->gum_struct.gum_pid != current->pid)
			goto ret_exit;
		
		gsb->gum_struct.gum_vma=find_vma(current->mm, mes->pages_start_address);//last_vma;
		gt(printk("<0>" "gum_vma: %lx \n", (unsigned long)gsb->gum_struct.gum_vma));
		
		gsb->gum_struct.gum_ocl_mes=kzalloc(sizeof(struct ocl_message), GFP_KERNEL);
		memcpy(gsb->gum_struct.gum_ocl_mes, mes, sizeof(struct ocl_message));
		
		gsb->gum_struct.gum_start_address=mes->pages_start_address;
		
		up(&gsb->gum_struct.gum_struct_sem);
		
		down(&gsb->gum_struct.gum_is_ready_sem);
		
		memcpy(mes, gsb->gum_struct.gum_ocl_mes, sizeof(struct ocl_message));
		memset(gsb->gum_struct.gum_ocl_mes, 0, sizeof(struct ocl_message));
		kfree(gsb->gum_struct.gum_ocl_mes);
		
		return len;
	}
	
	if(unlikely(mes->type==OCL_Is_Damaged)){
		OCL_kernel_struct* oks;
		
		if(unlikely(mes->kernel_struct==0) && likely(gsb->gum_struct.gum_pid)){
			//all of people are waiting here for we. Should we wait for another things?
			//down(&gsb->gum_struct.gum_struct_sem);
			
			gsb->gum_struct.gum_pid=0;
			
			up(&gsb->gum_struct.gum_struct_sem);
			
			return len;
		}
		
		oks=(OCL_kernel_struct*)mes->kernel_struct;
		gt(printk("<0>" "OCL_Response_Is_Damged: for oks: %lx \n", (unsigned long)oks));
		
		if(likely(oks)){
			int i;
			
			down(&oks->wanu_sem);
			
			oks->ret=-1;	//failed
			
			for(i=0;i<oks->waiters_num; i++)
				up(&oks->sem);
			
		}
	}
	
	if(mes->type==OCL_Response_Is_Ready){
		OCL_kernel_struct* oks=(OCL_kernel_struct*)mes->kernel_struct;
		gt(printk("<0>" "OCL_Response_Is_Ready: for oks: %lx \n", (unsigned long)oks));
		
		if(likely(oks)){
			int i;
			
			down(&oks->wanu_sem);
			
			oks->ret=0;	//successfull
			
			for(i=0;i<oks->waiters_num; i++)
				up(&oks->sem);
		}
		
		return len;
	}
	
	return len;
}

ssize_t GSFS_cdev_file_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	struct message *mes=(struct message*)buf;
	int ret=0;
		
	switch(mes->type){
		case NEWUSER:
			//New User
			ret=GSFS_add_new_user(sb,(rsa_context*)mes->data,current->loginuid);
			break;
			
		case LOGIN:
			//login
			ret=GSFS_user_login(sb,(rsa_context*)mes->data,current->loginuid, 
					    (char*)mes->data2, mes->data2len,
					    (char*)mes->data3, mes->data3len    );
			break;
			
		case LOGOUT:
			//logout
			ret=GSFS_user_logout(sb,current->loginuid);
			break;
			
		case MAKESEC:
			//make sec
			ret=GSFS_make_sec(sb, (char*)mes->data);
			break;
		
		case ADDUSERS:
			//add users
			ret=GSFS_add_users_to_inode(sb, (char*)mes->data, (unsigned int*)mes->data2, 
						   (unsigned int*)mes->data3, mes->data2len);
			break;
			
		case REVOKEUSERS:
			//add users
			ret=GSFS_revoke_users(sb, (char*)mes->data, (unsigned int*)mes->data2, 
						   (int*)mes->data3, mes->data2len);
			break;
			
		default:
			//printk("<0>" "non-known message with type%d\n",mes->type);
			return -1;
	}
	
	return ret;
}

static int vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf){
	unsigned long * vp=(unsigned long*)vma->vm_private_data;
	if(likely(vp)){
		unsigned int offset=1+(((unsigned long)vmf->virtual_address-vma->vm_start)>>PAGE_SHIFT);
		if(likely(offset<vp[0]))
			//vm_insert_page(vma,(unsigned long)vmf->virtual_address,(struct page*) vp[offset]);
			vm_insert_pfn(vma, (unsigned long)vmf->virtual_address, vp[offset]);
		
		//gt(printk("<0>" "vma_fault for va: %lx, vma_start: %lx, offset: %u, len: %lx, vp[offset]:%lx\n", (unsigned long)vmf->virtual_address, vma->vm_start, offset, vp[0], vp[offset]));
	}
	return  VM_FAULT_NOPAGE;
}

static void vma_close(struct vm_area_struct *vma){
	unsigned long len=0;
	
	if(likely(vma->vm_private_data)){
		len=((unsigned long*)vma->vm_private_data)[0];
		
		if(likely(len)){
			memset(vma->vm_private_data, 0, len*sizeof(unsigned long));
		}
		
		kfree(vma->vm_private_data);
	}
	
	gt(printk("<0>" "Closing vma: %lx , with pd: %lx, len: %lx\n", (unsigned long)vma, (unsigned long)vma->vm_private_data, len));
		
	return;
}

static void vma_open(struct vm_area_struct *vma){
	gt(printk("<0>" "Opening vma: %lx , with pd: %lx, len\n", (unsigned long)vma, (unsigned long)vma->vm_private_data));
	return ;
}

static const struct vm_operations_struct GSFS_cdev_vma_ops = {
	.open = vma_open,
	.close = vma_close,
	.fault = vma_fault,
};

int GSFS_cdev_file_mmap (struct file *filp, struct vm_area_struct *vma){
	if(unlikely(current->loginuid!=0)){
		return -1;
	}
	
	vma->vm_ops=&GSFS_cdev_vma_ops;
	//vma->vm_flags |= VM_RESERVED;
	vma->vm_flags |= VM_RESERVED | VM_IO | VM_PFNMAP | VM_DONTEXPAND;
	last_vma=vma;
	
	gt(printk("<0>" "New vma: %lx, start: %lx, end: %lx\n", (unsigned long)vma, vma->vm_start, vma->vm_end));
	
	return 0;
}

dev_t dev;
int dev_alloc=0;
struct cdev* GSFS_cdev=0;

struct file_operations GSFS_cdev_fops={
	.open	=	GSFS_cdev_file_open,
	.read	=	GSFS_cdev_file_read,
	.write	=	GSFS_cdev_file_write,
	//.mmap	=	GSFS_cdev_file_mmap,
};

int GSFS_chardev_init(struct super_block* super_block){
	int i;
	
	i=alloc_chrdev_region(&dev,0,1,"GSFS_chardev");
	if(i){
		printk("<0>" "Cann't allocate chardev region\n");
		return -1;
	}
	dev_alloc=1;
	GSFS_cdev=cdev_alloc();
	GSFS_cdev->owner=THIS_MODULE;
	GSFS_cdev->ops=&GSFS_cdev_fops;
	i=cdev_add(GSFS_cdev,dev,1);
	if(i || sb){
		GSFS_chardev_exit();
		return -1;
	}
	sb=super_block;
	//printk("<0>" "%d \n",i);
	
	return 0;
}

void GSFS_chardev_exit(void){
	sb=0;
	if(GSFS_cdev)
		cdev_del(GSFS_cdev);
	if(dev_alloc)
		unregister_chrdev_region(dev,1);
	return;
}