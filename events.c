#include "gsfs.h"

unsigned char EventsLen[]={	gsfs_hashlen,			//for GDirent_Hash_Changed_Event
				sizeof(crust_struct*),		//for Crust_Struct_Set_VEvent
				sizeof(struct users*)		//for Users_Set_VEvent
};

inline void free_one_present_event(struct event* ev, unsigned char type){
	struct users **cc;
	crust_struct  **cs;
	
	//printk("<0>" "events type:%d data:%lx datalen:%d\n",type,ev->data,ev->datalen);
	switch(type){
			
		case Users_Set_VEvent:
			cc=ev->data;
			put_users(*cc);
			break;
		
		case Crust_Struct_Set_VEvent:
			cs=ev->data;
			put_crust_struct(*cs);
			break;
	}
	
	memset(ev->data, 0, EventsLen[type]);
	kfree(ev->data);
		
	return;
}

void free_child(struct child* child){
	int i;
	
	//printk("<0>" "free_child_events index:%d\n",ev->index);
	for(i=0;i<Events_Num;i++)
		if(child->events[i].flags & event_flag_is_present)
			free_one_present_event(&child->events[i], i);
	
	//child->inode can be set only with GSFS_get_inode and with no traverse_all_gdirents
	//therefore for non-integrated children it can't be set
	//if(child->inode)
	//	((struct GSFS_inode*)child->inode->i_private)->parent=0;
		
	memset(child, 0, sizeof(struct child));
	
	kfree(child);
	
	return;
}

void print_all_events(struct child* child){
	char 	repp[1000],
		*rep=repp;
	int 	i;
	
	if(child->inode)
		sprintf(rep, "print_all_events for child with inode:%lx and ino:%lu *",(unsigned long)child->inode, child->inode->i_ino);
	else
		sprintf(rep, "print_all_events for child with no inode *");	
	rep+=strlen(rep);
	
	for(i=0;i<Events_Num;i++){
		struct event* ev=&child->events[i];
		sprintf(rep,"type: %d, flags: %d, datalen: %d, data:%lx ##",i,ev->flags, ev->datalen,(unsigned long) ev->data);
		rep+=strlen(rep);
	}
	
	printk("<0>" "%s\n",repp);
	return;
}

//you should get down_write for inode_inf before running this function
int add_event_to_inode( struct inode* inode, unsigned short child_index, unsigned char type, void* data, unsigned int datalen, unsigned char flags){
	struct avl_tree_node* atn;
	struct GSFS_inode* inode_info=(struct GSFS_inode*)inode->i_private;
	struct child* child;
	int ret;
	
	if(unlikely(type>=Events_Num))
		return -1;
	
	ret=0;
	
	atn=avl_tree_search( inode_info->children, child_index);
	if(atn){
		child=atn->data;
		//printk("<0>" "add event to inode: pino:%lu index:%d type%d len:%d\n",inode->i_ino, child_index, type,datalen);
	}
	else{
		//be care full that it is possible to go here for example
		//for traverse all gdirents for a has_sec_child dir 
		//therefore we should make this part active to add child
		
		gt(printk("<0>" "add event to inode with no child_index: pino:%lu index:%d type:%d len:%d\n",inode->i_ino, child_index, type,datalen));
		
		child=kzalloc(sizeof(struct child),GFP_KERNEL);
		child->index=child_index;
		inode_info->children=avl_tree_insert(inode_info->children, child);
		
		if(!inode_info->children){
			printk("<1>" "Some errors in inserting new child_events.\n");
			ret=-1;
		}
		
		if(ret==-1)
			goto back;
	}
	
	if(child->events[type].flags & event_flag_is_present)
		if(child->events[type].data)
			if(child->events[type].data != data)
				free_one_present_event(&child->events[type],type);
	
	child->events[type].data=data;
	child->events[type].datalen=datalen;
	child->events[type].flags=flags|event_flag_is_present;	

back:	
	//print_all_events(child);
	return ret;
}

int general_add_event_to_parent(struct inode* in, unsigned char type, void* data, unsigned int len, char get_sem){
	struct GSFS_inode	*inf=(struct GSFS_inode*)in->i_private,
				*pinf;
	struct inode* parent;
	int ret;

	parent=GSFS_get_inode((struct GSFS_sb*)in->i_sb->s_fs_info, inf->disk_info.parent_ino);	
	//inf->parent;
	if(!parent){
		//printk("<0>" "add_event with no parent for inode:%lu child_index:%d type:%d\n",in->i_ino,inf->disk_info.index_in_parent, type);
		return -1;
	}
	pinf=(struct GSFS_inode*)parent->i_private;
	
	if(get_sem)
		down_write(&pinf->inode_rwsem);
	
	ret=add_event_to_inode(parent, inf->disk_info.index_in_parent, type, data, len, 0);
	
	if(get_sem)
		up_write(&pinf->inode_rwsem);
	
	mark_inode_dirty(parent);
	
	iput(parent);
		
	return ret;
}

//you should get down_read for inf->inode_rwsem before running this function
int get_event(struct GSFS_inode* inf, unsigned short child_index,unsigned char type, void* data, unsigned int* len){
	atn* res;
	struct child* child;
	int ret=-1;
	
	//printk("<0>" "get_event for inode:%u child_index:%d type:%d\n",inf->disk_info.ino, child_index, type);
	
	res=avl_tree_search(inf->children, child_index);
	
	if(res){
		child=res->data;
		if(child->events[type].flags & event_flag_is_present){
			ret=0;
			memcpy(data, child->events[type].data, EventsLen[type]);
			*len=child->events[type].datalen;
			//printk("<0>" "get_event: len:%d, data:\n",*len);
			//printkey(data);
			
		}
		//print_all_events(res->data);
	}	
	
	return ret;
}
